// Soul Gem Hub - SKSE plugin
// Copyright (C) 2026 izzydoingit
// GPL-3.0-or-later; see LICENSE.txt and the notice at the top of main.cpp.
//
// The gems and their lights.
//
// WHAT IS LIT (FindGems, once at data load): every soul gem record - lit when it holds a soul, or when the gem placed in the
// world does (a soul trapped into an empty one), and always for Azura's Star and the Black Star - and the items and pieces known
// by their own model (the Soul Essence Gem, the Soul Cairn's gem bowl and chunks). A gem's look - its color and strength -
// comes from the model its record wears in this load order, so it follows whatever mesh mod wins: Praedy's Soul Gems (Subtle
// Soul Gems' copies of them too), Rustic Soul Gems (either download), or the game's own gems by their size; a gem mod whose
// author asked not to be supported stays unlit (GemPermissions.h). Never a static or
// a museum display that only wears a gem's model (his rule: displays never get lit) - with RE::Light loaded, its light on one
// is put out.
//
// HOW (from what the game announces, on the main thread; nothing runs on a clock): a look runs when a gem's 3D loads or is released,
// a cell loads, her cell changes, a game loads or a setting changes. The nearest 24 (kMaxLights) gems each get one
// light of ours hung on its model, made and registered the way ReLight does it (the credit below), in its color, at its strength and
// reach for the lighting found (Lighting.cpp - Community Shaders' inverse-square words, or the same light drawn plain on ENB and
// Vanilla). The soft pulse is a dimmer controller the game itself runs on the light. A gem picked up, unloaded or turned off loses
// its light; a game loaded drops them all.
//
// WHOSE LIGHT (the menu's one choice, Gems::WhoLights): what else lights a gem is read off its model when it loads and once more -
// a Light Placer file's light, RE::Light's, or a light inside the gem's own model (the ENB particle lights of Praedy's, Subtle's,
// Skyking's ... meshes) wherever it shines: on ENB, and on Community Shaders when a particle-light config
// (Data\ParticleLights\<texture>.ini) names it. By default a gem with such a light keeps it and ours stays off; on our lights ours goes
// on and the other is put out; on the game's own lights none of ours and the other is put out. This mod's own RE::Light files light
// their gems themselves (Gems::OwnerOf).
//
// CREDIT: a light is made and registered the way ReLight by Truman does it (github.com/TrumanGIT/ReLight, GPL-3.0-or-later,
// used with his permission, as Illuminated uses it): a master NiPointLight cloned for every use and handed to the shadow scene
// node.

#include "Plugin.h"

#include "FrameCost.h"

#include <DirectXTex.h>
#include <nlohmann/json.hpp>
#include <utility>

namespace Plugin
{
	namespace
	{
		constexpr const char* kName = "SoulGemLight";
		constexpr float       kAmbient = 0.1f;                                // a plain light's ambient: a tenth of its color (RE::Light's rule)
		constexpr std::size_t kMaxLights = 24;                                // the most gem lights at once - the nearest gems
		constexpr std::size_t kMostLooked = 4 * kMaxLights;                   // the nearest gems a look looks at closely (on show? another light on it?)
		constexpr RE::FormID  kAzurasStar = 0x063B27, kBlackStar = 0x063B29;  // in Skyrim.esm

		// ---------------------------------------------------------- the light, as Illuminated's LightKit.h makes it
		// Community Shaders' inverse-square lighting reads a flag and the cutoff from the first eight bytes of a light's runtime data
		// (as the game lays it out, its ambient color's red and green); read and written as bytes, always after SetLightAttenuation
		namespace Isl
		{
			constexpr std::uint32_t kFlag = 1u << 10;        // InverseSquare
			constexpr std::uint32_t kInitialised = 1u << 8;  // Light Limit Fix marks every light it makes so

			void Write(RE::NiLight* a_light, const std::array<std::uint32_t, 2>& a_words) noexcept
			{
				std::memcpy(reinterpret_cast<std::byte*>(&a_light->GetLightRuntimeData()), a_words.data(), sizeof(a_words));
			}
			// a light of our own: the flag word written whole, the cutoff beside it, and the third word (Community Shaders' light
			// record id - a clone carries 1.0f there) cleared
			void SetOwn(RE::NiLight* a_light, float a_cutoff) noexcept
			{
				Write(a_light, { kFlag | kInitialised, std::bit_cast<std::uint32_t>(std::clamp(a_cutoff, Gems::kLowestCutoff, Gems::kHighestCutoff)) });
				a_light->GetLightRuntimeData().ambient.blue = 0.0f;
			}
			// the same where Community Shaders draws linear: Initialised alone, no cutoff - an ambient color there reads as flags
			void SetOwnLinear(RE::NiLight* a_light) noexcept
			{
				Write(a_light, { kInitialised, 0u });
				a_light->GetLightRuntimeData().ambient.blue = 0.0f;
			}
		}

		// a new light for the scene, cloned from one master made once: ReLight found that a freshly made light, attached straight
		// away, crashes. nullptr when the game could not make one
		[[nodiscard]] RE::NiPointLight* CloneLight()
		{
			static RE::NiPointer<RE::NiPointLight> master;
			if (!master) {
				const RE::NiPointer<RE::NiPointLight> fresh(RE::NiPointLight::Create());
				auto*                                 clone = fresh ? netimmerse_cast<RE::NiPointLight*>(fresh->Clone()) : nullptr;
				if (!clone) {
					return nullptr;
				}
				master.reset(clone);
			}
			return netimmerse_cast<RE::NiPointLight*>(master->Clone());
		}

		// a light already attached and updated once, registered as ReLight registers its own: dynamic, kept inside its cell (portal
		// strict), lighting land and water too, never a shadow caster (shadows from lights are the shader mod's job)
		[[nodiscard]] RE::BSLight* AddToScene(RE::ShadowSceneNode* a_scene, RE::NiPointLight* a_light)
		{
			RE::ShadowSceneNode::LIGHT_CREATE_PARAMS params{};
			params.dynamic = true;
			params.shadowLight = false;
			params.portalStrict = true;
			params.affectLand = true;
			params.affectWater = true;
			params.neverFades = true;
			params.fov = 90.0f;
			params.falloff = 1.0f;
			params.nearDistance = 5.0f;
			params.depthBias = 1.0f;
			params.sceneGraphIndex = 0;
			params.restrictedNode = nullptr;
			params.lensFlareData = nullptr;
			return a_scene && a_light ? a_scene->AddLight(a_light, params) : nullptr;
		}

		// ---------------------------------------------------------- the gems, found once at data load
		struct Gem
		{
			Gems::Look         look;
			bool               needsSoul{ false };              // an empty gem record: lit only when the placed gem holds a soul
			Gems::Owner        owner{ Gems::Owner::kThisMod };  // the light mod that would light it (the menu may leave it to that one)
			const std::string* ownerName{ nullptr };            // that mod's name for the menu: the Light Placer folder its file sits in
		};
		std::unordered_map<const RE::TESBoundObject*, Gem> gGems;             // the gems this plugin lights; read on the main thread after data load
		std::vector<std::pair<std::string, std::size_t>>   gFamilies;         // the lit gem records by look, for the menu
		std::array<std::size_t, 4>                         gOtherLighters{};  // gem records another light mod lights too, by Gems::Owner
		std::vector<std::pair<std::string, std::size_t>>   gBlocked;          // gem records left unlit by their mod, at its author's request
		std::string                                        gRustic;           // which Rustic Soul Gems download is loaded
		bool                                               gRusticLit{ false };
		// the mods whose Light Placer files light our gems (each name's address stays put for the gems that point at it) and the gem
		// records each lights, for the menu - written at data load only
		std::unordered_set<std::string>                  gOwnerNames;
		std::vector<std::pair<std::string, std::size_t>> gLitBy;

		// What Light Placer's configs claim (models by their whole path as NormalPath writes it, formIDs tokens in lower case), read
		// once when Light Placer is loaded: whose light a gem would wear, for the menu's names. Emissive Soul Gems' own file's models
		// apart
		struct Claims
		{
			std::unordered_map<std::string, std::string> models, forms;  // -> the mod whose file claims it
			std::unordered_set<std::string>              emissive;
		};

		// a file's extension in lower case, as wide text: path::string() throws on a name the ANSI code page cannot show
		[[nodiscard]] std::wstring LowerExtension(const std::filesystem::path& a_path)
		{
			auto ext = a_path.extension().wstring();
			std::ranges::transform(ext, ext.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
			return ext;
		}

		// a path as UTF-8 for the log (path::string() throws on a name the local code page cannot hold)
		[[nodiscard]] std::string Utf8(const std::filesystem::path& a_path)
		{
			const auto  u8 = a_path.u8string();
			std::string out(u8.size(), '\0');
			std::ranges::transform(u8, out.begin(), [](char8_t c) { return static_cast<char>(c); });
			return out;
		}

		[[nodiscard]] Claims ReadLightPlacerClaims()
		{
			Claims          out;
			std::error_code ec;
			const auto      root = std::filesystem::current_path() / "Data" / "LightPlacer";
			std::size_t     files = 0;
			using enum std::filesystem::directory_options;
			for (std::filesystem::recursive_directory_iterator it(root, skip_permission_denied, ec), end; !ec && it != end; it.increment(ec)) {
				std::error_code fileEc;  // one file that cannot be looked at must not end the walk
				if (!it->is_regular_file(fileEc) || LowerExtension(it->path()) != L".json") {
					continue;
				}
				std::ifstream in(it->path(), std::ios::binary);
				const auto    doc = nlohmann::json::parse(in, nullptr, false, true);  // no exceptions; a comment does not drop the file
				const bool    emissive = Lower(Utf8(it->path().stem())) == "emissive soul gems";
				// Light Placer's own layout is LightPlacer\<mod>\<file>.json: the folder names the mod; a file straight in it, itself
				const auto rel = it->path().lexically_relative(root);
				auto       owner = emissive ? std::string("Emissive Soul Gems") : Utf8(rel.has_parent_path() ? *rel.begin() : rel.stem());
				if (!emissive && Lower(owner) == "emissive soul gems") {
					owner += " (" + Utf8(it->path().stem()) + ")";  // its menu line and switch are Light Placer's, never Emissive's own
				}
				++files;
				if (!doc.is_array()) {
					if (doc.is_discarded()) {
						SKSE::log::warn("Light Placer: {} did not read as JSON - the gems it claims are not known", Utf8(it->path().lexically_relative(root)));
					}
					continue;
				}
				for (const auto& entry : doc) {
					if (!entry.is_object()) {
						continue;
					}
					for (const auto& [key, into] : { std::pair{ "models", &out.models }, std::pair{ "formIDs", &out.forms } }) {
						const auto found = entry.find(key);
						if (found == entry.end() || !found->is_array()) {
							continue;
						}
						const bool models = key == std::string_view("models");
						for (const auto& v : *found) {
							if (!v.is_string()) {
								continue;
							}
							auto name = models ? Gems::NormalPath(v.get<std::string>()) : Lower(v.get<std::string>());
							if (models && emissive) {
								out.emissive.insert(name);
							}
							into->try_emplace(std::move(name), owner);  // the first file read keeps it
						}
					}
				}
			}
			if (ec && ec != std::errc::no_such_file_or_directory) {
				SKSE::log::warn("Light Placer: its folder could not be read to the end ({}) - configs after that point are not known", ec.message());
			}
			SKSE::log::info("Light Placer is loaded: {} config file(s) read, {} model(s) and {} form(s) claimed, {} model(s) by Emissive Soul Gems' own file",
				files, out.models.size(), out.forms.size(), out.emissive.size());
			return out;
		}

		using TweaksEditorId = const char* (*)(std::uint32_t);

		[[nodiscard]] TweaksEditorId FindTweaksEditorId()
		{
			const auto tweaks = REX::W32::GetModuleHandleW(L"po3_Tweaks");
			if (!tweaks) {
				return nullptr;
			}
			return reinterpret_cast<TweaksEditorId>(REX::W32::GetProcAddress(tweaks, "GetFormEditorID"));
		}

		// the game keeps no editor ID for a soul gem: powerofthree's Tweaks keeps them and exports the lookup, so ask it first
		[[nodiscard]] const char* EditorIdOf(const RE::TESForm* a_form)
		{
			static const TweaksEditorId tweaks = FindTweaksEditorId();
			if (tweaks) {
				if (const char* id = tweaks(a_form->GetFormID()); id && *id) {
					return id;
				}
			}
			return a_form->GetFormEditorID();
		}

		// which file claims this form: by its model's whole path, or a formIDs token ("SoulGemGrandFilled", "0x2E4FF~Skyrim.esm" ...)
		[[nodiscard]] Gems::Claim Claimed(const Claims& a_claims, const RE::TESForm* a_form, std::string_view a_model, std::string& a_mod)
		{
			const auto path = Gems::NormalPath(a_model);
			if (a_claims.emissive.contains(path)) {
				a_mod = "Emissive Soul Gems";
				return Gems::Claim::kEmissive;
			}
			if (const auto m = a_claims.models.find(path); m != a_claims.models.end()) {
				a_mod = m->second;
				return Gems::Claim::kOther;
			}
			if (const char* id = EditorIdOf(a_form); id && *id) {
				if (const auto f = a_claims.forms.find(Lower(id)); f != a_claims.forms.end()) {
					a_mod = f->second;
					return Gems::Claim::kOther;
				}
			}
			const auto* file = a_form->GetFile(0);
			if (!file) {
				return Gems::Claim::kNone;
			}
			const auto plugin = Lower(file->GetFilename());
			const auto local = a_form->GetLocalFormID();
			const auto byId = std::ranges::find_if(a_claims.forms, [&](const auto& a_token) { return Gems::TokenNamesForm(a_token.first, plugin, local); });
			if (byId == a_claims.forms.end()) {
				return Gems::Claim::kNone;
			}
			a_mod = byId->second;
			return Gems::Claim::kOther;
		}

		// ---------------------------------------------------------- a gem model's own color, read once per model at data load
		std::unordered_set<std::string> gParticleTextures;  // the textures Data\ParticleLights\*.ini name (lower case, no extension)

		// the textures an ENB particle light draws (ENB Light's, Rudy's): the names Light Limit Fix and Bottled Shaders ship particle
		// light configs for
		constexpr std::array<std::string_view, 3> kEnbLightTextures{ "fxglowenb", "glowsoft01_enbl", "enblightglow" };

		// the file name of the texture an effect-shader shape draws, lower case without folder or extension ("" for any other shape)
		[[nodiscard]] std::string EffectTexture(RE::NiAVObject* a_object)
		{
			auto* geometry = a_object ? a_object->AsGeometry() : nullptr;
			auto* shader = geometry ? geometry->GetGeometryRuntimeData().shaderProperty.get() : nullptr;
			if (!shader || !shader->material || shader->GetMaterialType() != RE::BSShaderMaterial::Type::kEffect) {
				return {};
			}
			const char* path = static_cast<RE::BSEffectShaderMaterial*>(shader->material)->sourceTexturePath.c_str();
			auto        name = Gems::BareName(path ? path : "");
			if (const auto dot = name.rfind('.'); dot != std::string::npos) {
				name.erase(dot);
			}
			return name;
		}

		// the material of a shape that is a light of the gem's own: an effect shape drawing an ENB light texture or one a
		// particle-light config names (Animated Filled Soul Gems' blue halo), else nullptr
		[[nodiscard]] const RE::BSEffectShaderMaterial* OwnLightShape(RE::NiAVObject* a_object)
		{
			const auto texture = EffectTexture(a_object);
			if (texture.empty() || (std::ranges::find(kEnbLightTextures, texture) == kEnbLightTextures.end() && !gParticleTextures.contains(texture))) {
				return nullptr;
			}
			return static_cast<const RE::BSEffectShaderMaterial*>(a_object->AsGeometry()->GetGeometryRuntimeData().shaderProperty->material);
		}

		// a file the game can open under a_folder ("textures", "meshes"), handed to a_take piece by piece; false when it cannot be
		// opened. A loose file is read off the disk (through the mod manager's virtual folder): the game takes a loose file before an
		// archive, and a runtime mesh patcher (PGLite) hands the game's loader bytes of its own for the path, so the file's own
		// bytes - its fingerprint - are only on the disk. An archived file goes through the game's loader
		template <class F>
		bool ReadGameFile(std::string_view a_path, std::string_view a_folder, F&& a_take)
		{
			// the same normal form the permission lookups use, so "Data\Textures\x.dds" or "\Meshes\x.nif" open too
			const auto                path = std::string(a_folder) + "\\" + (a_folder == "meshes" ? Gems::NormalPath(a_path) : Gems::TexturePath(a_path));
			std::vector<std::uint8_t> chunk(65536);
			// a path the ANSI code page might not hold goes through the game, which reads it as it is
			std::error_code ec;
			const auto      root = std::filesystem::current_path(ec);
			if (!ec && std::ranges::none_of(path, [](char a_c) { return static_cast<unsigned char>(a_c) >= 0x80; })) {
				if (std::ifstream loose(root / "Data" / path, std::ios::binary); loose) {
					while (loose.read(reinterpret_cast<char*>(chunk.data()), static_cast<std::streamsize>(chunk.size())) || loose.gcount() > 0) {
						a_take(std::span<const std::uint8_t>(chunk.data(), static_cast<std::size_t>(loose.gcount())));
					}
					// a read that broke off part way gave only some of the file: no fingerprint, no color from it
					return !loose.bad();
				}
			}
			RE::BSResourceNiBinaryStream stream(path);
			const auto*                  source = stream.good() ? stream.stream.get() : nullptr;
			if (!source) {
				return false;
			}
			for (std::uint64_t got = 0; source->DoRead(chunk.data(), chunk.size(), got) == RE::BSResource::ErrorCode::kNone && got > 0;) {
				a_take(std::span<const std::uint8_t>(chunk.data(), static_cast<std::size_t>(got)));
			}
			return true;
		}

		// a texture's fingerprint, once per file: the permission block and the picture rows each ask for it
		std::optional<std::uint64_t> TexturePrint(std::string_view a_path)
		{
			static std::unordered_map<std::string, std::optional<std::uint64_t>> read;
			const auto [it, added] = read.try_emplace(Gems::TexturePath(a_path));
			if (added) {
				Gems::FingerprintSum sum;
				if (ReadGameFile(a_path, "textures", [&](std::span<const std::uint8_t> a_piece) { sum.Add(a_piece); }) && sum.Size()) {
					it->second = sum.Result();
				}
			}
			return it->second;
		}

		// the gem mod a file of the game's belongs to, while its author has not allowed this mod to light it: by its name, else
		// by its bytes (streamed - only the first 64 KB are hashed, a 4K texture is never held whole)
		std::optional<std::string_view> BlockedTexture(std::string_view a_path)
		{
			if (const auto byName = Gems::BlockedModOfPath(Gems::TexturePath(a_path))) {
				return byName;
			}
			const auto print = TexturePrint(a_path);
			return print ? Gems::BlockedModOf(*print) : std::nullopt;
		}

		[[nodiscard]] const char* DiffuseOf(RE::BSGeometry* a_geometry)
		{
			auto* shader = netimmerse_cast<RE::BSLightingShaderProperty*>(a_geometry ? a_geometry->GetGeometryRuntimeData().shaderProperty.get() : nullptr);
			auto* material = shader ? static_cast<RE::BSLightingShaderMaterialBase*>(shader->material) : nullptr;
			return material && material->textureSet ? material->textureSet->GetTexturePath(RE::BSTextureSet::Textures::kDiffuse) : nullptr;
		}

		// The pixels of a texture into a_sum: a mip level near 64 pixels wide is enough for a mean color, and decoding it costs
		// little. False when the file cannot be read or decoded.
		bool AddTexturePixels(const std::string& a_path, Gems::ColorSum& a_sum)
		{
			std::vector<std::uint8_t> bytes;
			if (!ReadGameFile(a_path, "textures", [&](std::span<const std::uint8_t> a_piece) { bytes.insert(bytes.end(), a_piece.begin(), a_piece.end()); })) {
				return false;
			}
			DirectX::TexMetadata  meta{};
			DirectX::ScratchImage loaded;
			if (bytes.empty() || FAILED(DirectX::LoadFromDDSMemory(bytes.data(), bytes.size(), DirectX::DDS_FLAGS_NONE, &meta, loaded))) {
				return false;
			}
			std::size_t mip = 0;
			while (mip + 1 < meta.mipLevels && (meta.width >> mip) > 64) {
				++mip;
			}
			const auto* image = loaded.GetImage(mip, 0, 0);
			if (!image) {
				return false;
			}
			// 8-bit RGBA as stored: an sRGB texture stays sRGB (made plain, DirectXTex turns it linear - darker and stronger than the
			// gem shows); one already in that format is read as it is (DirectXTex refuses a conversion to the same format)
			const auto            rgbaFormat = DirectX::IsSRGB(image->format) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
			DirectX::ScratchImage rgba;
			const DirectX::Image* pixels = image;
			if (image->format != rgbaFormat) {
				const auto result = DirectX::IsCompressed(image->format) ?
				                        DirectX::Decompress(*image, rgbaFormat, rgba) :
				                        DirectX::Convert(*image, rgbaFormat, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, rgba);
				pixels = SUCCEEDED(result) ? rgba.GetImage(0, 0, 0) : nullptr;
			}
			if (!pixels) {
				return false;
			}
			for (std::size_t y = 0; y < pixels->height; ++y) {
				const auto* row = pixels->pixels + y * pixels->rowPitch;
				for (std::size_t x = 0; x < pixels->width; ++x) {
					a_sum.Add(row[x * 4], row[x * 4 + 1], row[x * 4 + 2], row[x * 4 + 3]);
				}
			}
			return true;
		}

		// the color a gem model shows: the diffuse textures of its lit shapes that glow (the gem itself - an inner shape that does not
		// glow, or an effect-shader glow shell, is left out; every lit shape when none glows), a normal map in a color slot never
		// counted, so a gem mod with no gem texture of its own follows the texture it wears. Only when no texture carries a color: the
		// color a grey texture's shape's emissive gives it (a cloud tile), then a light shape of its own. Read through the game's own
		// model loader, once per model; none on VR (the loader's address is SE / AE only) or when nothing in it carries a color -
		// the caller keeps the table's color then. A star (a_star: by its record or its model) never takes a light shape's color - it
		// wears its body's color (ColorSum::Body), whatever light a replacer hangs in it; a_gem: a star's gem instead, the colored
		// part of its textures (ColorSum::Result)
		std::optional<Gems::Rgb> ModelColor(std::string_view a_model, bool a_star, bool a_gem = false)
		{
			static std::map<std::tuple<std::string, bool, bool>, std::optional<Gems::Rgb>> read;
			if (REL::Module::IsVR() || a_model.empty()) {
				return std::nullopt;
			}
			const auto [it, added] = read.try_emplace({ Gems::NormalPath(a_model), a_star, a_gem });
			if (!added) {
				return it->second;
			}
			RE::NiPointer<RE::NiNode> root;
			const std::string         path(a_model);
			if (RE::BSModelDB::Demand(path.c_str(), root, RE::BSModelDB::DBTraits::ArgsType{}) != RE::BSResource::ErrorCode::kNone || !root) {
				return std::nullopt;
			}
			// each texture decoded once; it counts as glowing when ANY shape wearing it glows, whichever shape came first
			struct Texture
			{
				Gems::ColorSum sum;
				bool           read{ false }, glows{ false };
			};
			std::unordered_map<std::string, Texture> textures;
			Gems::ColorSum                           ownLight, greyGlow;
			RE::BSVisit::TraverseScenegraphGeometries(root.get(), [&](RE::BSGeometry* a_geometry) {
				if (const auto* light = OwnLightShape(a_geometry)) {
					ownLight.AddShaderColor(light->baseColor.red, light->baseColor.green, light->baseColor.blue);
					return RE::BSVisit::BSVisitControl::kContinue;
				}
				const char* diffuse = DiffuseOf(a_geometry);
				if (!diffuse || !*diffuse || Gems::IsNormalMap(diffuse)) {
					return RE::BSVisit::BSVisitControl::kContinue;
				}
				auto*       lit = netimmerse_cast<RE::BSLightingShaderProperty*>(a_geometry->GetGeometryRuntimeData().shaderProperty.get());
				const auto* e = lit ? lit->emissiveColor : nullptr;
				const bool  glows = e && lit->emissiveMult > 0.0f && (std::max)({ e->red, e->green, e->blue }) > 0.0f;
				auto [tex, fresh] = textures.try_emplace(Lower(diffuse));
				if (fresh && !(tex->second.read = AddTexturePixels(diffuse, tex->second.sum))) {
					SKSE::log::info("gems: {} - its texture {} could not be read", a_model, diffuse);
				}
				tex->second.glows = tex->second.glows || glows;
				if (glows && tex->second.read && tex->second.sum.Colorless()) {
					greyGlow.AddShaderColor(e->red, e->green, e->blue);
				}
				return RE::BSVisit::BSVisitControl::kContinue;
			});
			Gems::ColorSum glowing, all, body;
			for (const auto& [name, tex] : textures) {
				if (tex.read) {
					all.Add(tex.sum);
					(tex.glows ? glowing : body).Add(tex.sum);
				}
			}
			// a star wears its body's color as a whole - the textures that do not glow, else all of them - never its stripes'
			if (a_star || Gems::StarOf(a_model) != Gems::StarModel::kNone) {
				const auto whole = body.Body();
				const auto metal = whole ? whole : all.Body();
				const auto gem = all.Result();
				// both come from this one load, so the other reading is kept too
				read.insert_or_assign({ std::get<0>(it->first), a_star, !a_gem }, a_gem ? metal : gem);
				return it->second = a_gem ? gem : metal;
			}
			it->second = Gems::ModelColorOf(glowing, all, greyGlow, ownLight);
			return it->second;
		}

		// the gem mod whose author has not allowed this mod to light it yet, when a_model or one of its diffuse textures is that
		// mod's file - such a gem stays unlit until the permission comes; once per model
		std::optional<std::string_view> BlockedBy(std::string_view a_model)
		{
			static std::unordered_map<std::string, std::optional<std::string_view>> read;
			if (a_model.empty()) {
				return std::nullopt;
			}
			const auto [it, added] = read.try_emplace(Gems::NormalPath(a_model));
			if (!added) {
				return it->second;
			}
			auto& found = it->second;
			if ((found = Gems::BlockedModOfPath(it->first))) {
				return found;
			}
			std::vector<std::uint8_t> nif;
			if (!ReadGameFile(a_model, "meshes", [&](std::span<const std::uint8_t> a_piece) { nif.insert(nif.end(), a_piece.begin(), a_piece.end()); }) || nif.empty()) {
				return found;
			}
			if ((found = Gems::BlockedModOf(Gems::Fingerprint(nif)))) {
				return found;
			}
			// the color textures the model names (read off its bytes, so VR is covered too)
			for (const auto& texture : Gems::TexturesNamed(nif)) {
				if (Gems::ColorTexture(texture) && (found = BlockedTexture(texture))) {
					break;
				}
			}
			return found;
		}

		struct ModelFiles
		{
			std::vector<std::uint64_t> prints;    // its color textures' (the winning files, in the order it names them), then its own
			std::vector<std::string>   textures;  // every texture path it names (TexturePath form)
		};

		// once per model
		const ModelFiles& FilesOf(std::string_view a_model)
		{
			static std::unordered_map<std::string, ModelFiles> read;
			const auto [it, added] = read.try_emplace(Gems::NormalPath(a_model));
			if (!added) {
				return it->second;
			}
			std::vector<std::uint8_t> nif;
			if (!ReadGameFile(a_model, "meshes", [&](std::span<const std::uint8_t> a_piece) { nif.insert(nif.end(), a_piece.begin(), a_piece.end()); }) || nif.empty()) {
				return it->second;
			}
			for (const auto& texture : Gems::TexturesNamed(nif)) {
				it->second.textures.push_back(Gems::TexturePath(texture));
				if (const auto print = Gems::ColorTexture(texture) ? TexturePrint(texture) : std::nullopt) {
					it->second.prints.push_back(*print);
				}
			}
			it->second.prints.push_back(Gems::Fingerprint(nif));
			return it->second;
		}

		const std::vector<std::uint64_t>& PrintsOf(std::string_view a_model) { return FilesOf(a_model).prints; }

		// the color a gem's picture shows, when one of its model's files is one a close picture of that gem has read
		// (GemSeenColors.h) - for a named quest gem (a_named: its record's id) its own row first, then the plain gem's
		std::optional<Gems::Rgb> SeenColor(std::string_view a_model, std::uint32_t a_named = 0)
		{
			if (a_model.empty()) {
				return std::nullopt;
			}
			const auto& prints = PrintsOf(a_model);
			if (a_named) {
				for (const auto print : prints) {
					if (const auto seen = Gems::NamedSeenColorOf(a_named, print)) {
						return seen;
					}
				}
			}
			for (const auto print : prints) {
				if (const auto seen = Gems::SeenColorOf(print)) {
					return seen;
				}
			}
			return std::nullopt;
		}

		// the color textures a gem actually wears, off its loaded model: a runtime patcher or a pack can point it elsewhere than its
		// file names. Once per model; none on VR (the loader's address is SE / AE only)
		const std::vector<std::uint64_t>& WornPrintsOf(std::string_view a_model)
		{
			static std::unordered_map<std::string, std::vector<std::uint64_t>> read;
			const auto [it, added] = read.try_emplace(Gems::NormalPath(a_model));
			if (!added || a_model.empty() || REL::Module::IsVR()) {
				return it->second;
			}
			RE::NiPointer<RE::NiNode> root;
			const std::string         path(a_model);
			if (RE::BSModelDB::Demand(path.c_str(), root, RE::BSModelDB::DBTraits::ArgsType{}) != RE::BSResource::ErrorCode::kNone || !root) {
				return it->second;
			}
			auto& worn = it->second;
			RE::BSVisit::TraverseScenegraphGeometries(root.get(), [&worn](RE::BSGeometry* a_geometry) {
				const char* diffuse = OwnLightShape(a_geometry) ? nullptr : DiffuseOf(a_geometry);
				const auto  print = diffuse && *diffuse && Gems::ColorTexture(diffuse) ? TexturePrint(diffuse) : std::nullopt;
				if (print && std::ranges::find(worn, *print) == worn.end()) {
					worn.push_back(*print);
				}
				return RE::BSVisit::BSVisitControl::kContinue;
			});
			return worn;
		}

		// a pack or a patcher dresses this gem in something else than its own files: the rows of those files do not hold for it. The game's own
		// gems keep his set whatever dresses them
		bool PackOnTop(std::string_view a_model)
		{
			const auto& named = PrintsOf(a_model);
			return !a_model.empty() && !Gems::GameFilesIn(named) && !Gems::WearsNamed(named, WornPrintsOf(a_model));
		}

		// the color measured off the gem's own pictures, by what it wears (GemMath.h MeasuredColorWorn; the game's gems MeasuredPairColor)
		std::optional<Gems::Rgb> MeasuredColor(std::string_view a_model)
		{
			if (a_model.empty()) {
				return std::nullopt;
			}
			// the game's own gems keep his set, but for a pack measured on them (Vanilla PBR's set, his order): only this model's row in that
			// texture counts there, never a row another model's take left on the texture
			const auto& named = PrintsOf(a_model);
			const auto& worn = WornPrintsOf(a_model);
			if (!Gems::GameFilesIn(named)) {
				return Gems::MeasuredColorWorn(named, worn);
			}
			return Gems::WearsNamed(named, worn) ? Gems::MeasuredColorIn(named) : Gems::MeasuredPairColor(named, worn);
		}

		// a color set by eye (GemMath.h kCairnEyeColors / kStarEyeColors, through a_of), by the files SeenColor reads
		std::optional<Gems::Rgb> EyeColor(std::string_view a_model, std::optional<Gems::Rgb> (*a_of)(std::uint64_t) noexcept)
		{
			for (const auto print : PrintsOf(a_model)) {
				if (const auto eye = a_of(print)) {
					return eye;
				}
			}
			return std::nullopt;
		}

		// the gem's own glow, when its first model file the glow table lists (GemGlowColors.h) says it differs from its light
		const Gems::GlowColor* GlowOf(std::string_view a_model)
		{
			if (a_model.empty()) {
				return nullptr;
			}
			for (const auto print : PrintsOf(a_model)) {
				if (const auto* glow = Gems::GlowColorOf(print)) {
					return glow;
				}
			}
			return nullptr;
		}

		// a game gem model laid out as the game's: its own glow (GemMath.h kGameGlows)
		std::optional<Gems::Rgb> GameGlow(std::string_view a_model)
		{
			const auto& files = FilesOf(a_model);
			return files.prints.empty() ? std::nullopt : Gems::GameGlowOf(a_model, files.textures, files.prints);
		}

		// a look's own glow, from a glow table row (none: the look keeps only its color)
		void SetGlow(Gems::Look& a_look, const Gems::GlowColor* a_glow) noexcept
		{
			a_look.glow = a_glow ? std::optional(Gems::Rgb{ a_glow->r, a_glow->g, a_glow->b }) : std::nullopt;
			a_look.glowAlways = a_glow && a_glow->always;
		}

		// the diffuse textures a record's own texture swaps (MODS) put on its model: Andrealletius Soul Gem Overhaul's Solar and
		// Astral gems wear the game's models in textures of their own, so a color read off the model alone would be the game's
		[[nodiscard]] std::vector<std::string> SwappedDiffuses(const RE::TESForm* a_form)
		{
			std::vector<std::string> out;
			const auto*              swap = a_form ? a_form->As<RE::TESModelTextureSwap>() : nullptr;
			if (!swap || !swap->alternateTextures) {
				return out;
			}
			for (const auto& alt : std::span(swap->alternateTextures, swap->numAlternateTextures)) {
				const char* diffuse = alt.textureSet ? alt.textureSet->GetTexturePath(RE::BSTextureSet::Textures::kDiffuse) : nullptr;
				if (diffuse && *diffuse && !Gems::IsNormalMap(diffuse)) {
					out.emplace_back(diffuse);
				}
			}
			return out;
		}

		// the gem mod waiting on its author's permission whose texture a record swaps onto its model, if any
		// once per set of textures: a gem mod's many records share a few texture sets
		std::optional<std::string_view> BlockedSwap(const std::vector<std::string>& a_textures)
		{
			static std::map<std::vector<std::string>, std::optional<std::string_view>> read;
			const auto [it, added] = read.try_emplace(a_textures);
			if (added) {
				for (const auto& texture : a_textures) {
					if ((it->second = BlockedTexture(texture))) {
						break;
					}
				}
			}
			return it->second;
		}

		// the fingerprints of swapped textures, once per set (the picture row and the glow row are both looked up by them)
		const std::vector<std::uint64_t>& SwapPrints(const std::vector<std::string>& a_textures)
		{
			static std::map<std::vector<std::string>, std::vector<std::uint64_t>> read;
			const auto [it, added] = read.try_emplace(a_textures);
			if (added) {
				for (const auto& texture : a_textures) {
					if (const auto print = TexturePrint(texture)) {
						it->second.push_back(*print);
					}
				}
			}
			return it->second;
		}

		std::optional<Gems::Rgb> SwapColorOnce(const std::vector<std::string>& a_textures, Gems::Kind a_kind);

		// the color swapped textures show: a picture row of one of them first, else their pixels (ColorSum, the pale-blue rule);
		// once per set of textures and kind
		std::optional<Gems::Rgb> SwapColor(const std::vector<std::string>& a_textures, Gems::Kind a_kind)
		{
			static std::map<std::pair<std::vector<std::string>, Gems::Kind>, std::optional<Gems::Rgb>> read;
			const auto [it, added] = read.try_emplace({ a_textures, a_kind });
			if (added) {
				it->second = SwapColorOnce(a_textures, a_kind);
			}
			return it->second;
		}

		std::optional<Gems::Rgb> SwapColorOnce(const std::vector<std::string>& a_textures, Gems::Kind a_kind)
		{
			for (const auto print : SwapPrints(a_textures)) {
				if (const auto seen = Gems::SeenColorOf(print)) {
					return seen;
				}
			}
			Gems::ColorSum all;
			for (const auto& texture : a_textures) {
				Gems::ColorSum one;
				if (AddTexturePixels(texture, one)) {
					all.Add(one);
				}
			}
			const auto rgb = all.Result();
			return rgb ? std::optional(Gems::ShownColor(*rgb, a_kind)) : std::nullopt;
		}

		// the own glow swapped textures show, when the glow table lists one of them
		const Gems::GlowColor* SwapGlow(const std::vector<std::string>& a_textures)
		{
			for (const auto print : SwapPrints(a_textures)) {
				if (const auto* glow = Gems::GlowColorOf(print)) {
					return glow;
				}
			}
			return nullptr;
		}

		// one more of a_name in a name -> count list (kept in the order first seen)
		void CountName(std::vector<std::pair<std::string, std::size_t>>& a_list, std::string_view a_name)
		{
			const auto it = std::ranges::find(a_list, a_name, &std::pair<std::string, std::size_t>::first);
			if (it == a_list.end()) {
				a_list.emplace_back(std::string(a_name), 1);
			} else {
				++it->second;
			}
		}

		[[nodiscard]] Gems::Kind KindOfCapacity(RE::SOUL_LEVEL a_level) noexcept
		{
			switch (a_level) {
			case RE::SOUL_LEVEL::kPetty:
				return Gems::Kind::kPetty;
			case RE::SOUL_LEVEL::kLesser:
				return Gems::Kind::kLesser;
			case RE::SOUL_LEVEL::kCommon:
				return Gems::Kind::kCommon;
			case RE::SOUL_LEVEL::kGreater:
				return Gems::Kind::kGreater;
			default:
				return Gems::Kind::kGrand;
			}
		}

		[[nodiscard]] bool Loaded(RE::TESDataHandler* a_dh, std::string_view a_plugin)
		{
			return a_dh->LookupLoadedModByName(a_plugin) || a_dh->LookupLoadedLightModByName(a_plugin);
		}

		// the other light mods in this game, read once at data load
		struct Others
		{
			bool   lightPlacer{ false }, reLight{ false };
			Claims claims;
		};

		// a gem this plugin lights is kept. One a Light Placer file lights is counted and named: the menu's choice of whose light decides
		// at each look, off what really hangs on it; one this mod's own RE::Light files light is left to RE::Light (Gems::OwnerOf)
		bool Take(const RE::TESBoundObject* a_form, std::string_view a_model, const Gem& a_gem, const Others& a_others)
		{
			if (const auto mod = BlockedBy(a_model)) {
				CountName(gBlocked, *mod);
				return false;
			}
			std::string mod;
			const auto  claim = a_others.lightPlacer ? Claimed(a_others.claims, a_form, a_model, mod) : Gems::Claim::kNone;
			const auto  owner = Gems::OwnerOf(a_model, a_gem.look.family, claim, a_others.reLight);
			if (owner != Gems::Owner::kThisMod) {
				++gOtherLighters[static_cast<std::size_t>(owner)];
			}
			if (owner == Gems::Owner::kReLight) {
				return false;
			}
			auto gem = a_gem;
			gem.owner = owner;
			if (owner != Gems::Owner::kThisMod && !mod.empty()) {
				gem.ownerName = &*gOwnerNames.insert(mod).first;
				CountName(gLitBy, mod);
			}
			gGems.insert_or_assign(a_form, std::move(gem));
			return true;
		}

		// items and pieces that are not soul gem records, known by their own model: a_items lets an item take a named gem's model
		// (the Soul Essence Gem); the Soul Cairn's pieces are an activator, a misc item and a furniture, never a static - a static
		// wearing one of their models is decor (a display)
		template <class T>
		std::size_t AddByModel(bool a_items, const Others& a_others)
		{
			std::size_t added = 0;
			auto*       dh = RE::TESDataHandler::GetSingleton();
			// the plain form list, never GetFormArray<T>(): that one reinterprets the game's TESForm pointers as T*, which is wrong
			// for a class whose form part does not sit at its start - a movable static's sits 0x20 in (BGSMovableStatic.h), and its
			// typed list crashed the game at data load (a call through a vtable read 0x20 off)
			for (auto* form : dh->GetFormArray(T::FORMTYPE)) {
				auto*       bound = form ? form->template As<RE::TESBoundObject>() : nullptr;
				const auto* model = bound ? form->template As<RE::TESModel>() : nullptr;
				const char* path = model ? model->GetModel() : nullptr;
				auto        look = path && *path ? Gems::LookOfModel(path, false) : std::nullopt;
				if (!look || (look->family != Gems::Family::kSoulCairn && !(a_items && look->family == Gems::Family::kNamed))) {
					continue;
				}
				if (const auto eye = look->kind == Gems::Kind::kCairn ? EyeColor(path, Gems::CairnEyeColorOf) : std::nullopt) {
					look->rgb = *eye;
				} else if (const auto seen = SeenColor(path)) {  // the color its lit-room picture shows
					look->rgb = *seen;
				}
				SetGlow(*look, GlowOf(path));
				look->exact = look->rgb == Gems::kGameCairnRgb;
				added += Take(bound, path, Gem{ *look, false, Gems::Owner::kThisMod, nullptr }, a_others) ? 1 : 0;
			}
			return added;
		}

		// displays (statics wearing a gem's or a Soul Cairn piece's model, or a copy RE::Light's keys reach) that RE::Light lights:
		// with RE::Light loaded their light is put out, since displays never get lit
		std::unordered_set<const RE::TESBoundObject*> gDisplays;
		std::atomic<std::size_t>                      gDisplaysDark{ 0 };
		constexpr const char*                         kReLightName = "RL";  // the name RE::Light gives every light it hangs

		template <class T>
		std::size_t AddDisplays()
		{
			std::size_t added = 0;
			for (auto* form : RE::TESDataHandler::GetSingleton()->GetFormArray(T::FORMTYPE)) {
				auto*       bound = form ? form->template As<RE::TESBoundObject>() : nullptr;
				const auto* model = bound ? form->template As<RE::TESModel>() : nullptr;
				const char* path = model ? model->GetModel() : nullptr;
				const bool  gemModel = path && *path && (Gems::LookOfModel(path, false) || Gems::ReLightKeyed(path));
				if (gemModel && !gGems.contains(bound)) {
					added += gDisplays.insert(bound).second ? 1 : 0;
				}
			}
			return added;
		}

		// RE::Light's light on a display's model goes out (culled, so nothing it would light is lit) - only RE::Light's: a light
		// another mod put there is that mod's; true when one was
		[[nodiscard]] bool DarkenDisplay(RE::NiAVObject* a_root)
		{
			bool dark = false;
			RE::BSVisit::TraverseScenegraphObjects(a_root, [&](RE::NiAVObject* a_object) {
				if (netimmerse_cast<RE::NiLight*>(a_object) && a_object->name == kReLightName) {
					if (!a_object->GetAppCulled()) {
						a_object->SetAppCulled(true);
					}
					dark = true;
				}
				return RE::BSVisit::BSVisitControl::kContinue;
			});
			return dark;
		}

		// ---------------------------------------------------------- the lights, main thread only
		// the soft pulse, run by the game itself: a dimmer controller on a light writes its fade, so nothing of ours runs per frame.
		// The game's calls as CommonLib-NG names them (pull request #477 by ItzIvy05, MIT; our pinned CommonLib has not got them) and
		// the controller set up as Ivy's Baked Goods sets up its own (GPL-3.0). Their address ids are SE / AE only: no pulse on VR
		namespace Dimmer
		{
			constexpr std::uint32_t kKeys = 9;  // straight lines along Gems::PulseAt's curve

			template <class T>
			[[nodiscard]] T* Made(REL::RelocationID a_id)
			{
				using func_t = T*();
				REL::Relocation<func_t> func{ a_id };
				return func();
			}

			// a looping dimmer on a_light, its keys still to be written; nullptr when the game could not make one
			[[nodiscard]] RE::NiPointer<RE::NiInterpController> Hang(RE::NiLight* a_light, float a_phase)
			{
				RE::NiPointer<RE::NiInterpController>  dimmer(Made<RE::NiInterpController>(RELOCATION_ID(72049, 73695)));
				RE::NiPointer<RE::NiFloatInterpolator> interpolator(Made<RE::NiFloatInterpolator>(RELOCATION_ID(71088, 72686)));
				RE::NiPointer<RE::NiFloatData>         data(Made<RE::NiFloatData>(RELOCATION_ID(71069, 72667)));
				if (!dimmer || !interpolator || !data) {
					return nullptr;
				}
				using keys_t = RE::NiFloatKey*(std::uint32_t);
				using replace_t = void(RE::NiFloatData*, RE::NiFloatKey*, std::uint32_t, RE::NiFloatData::KeyType);
				REL::Relocation<keys_t>    newKeys{ RELOCATION_ID(72540, 74222) };
				REL::Relocation<replace_t> replaceAnim{ RELOCATION_ID(71071, 72669) };
				replaceAnim(data.get(), newKeys(kKeys), kKeys, RE::NiFloatData::KeyType::kLink);
				interpolator->floatData = data;
				dimmer->SetInterpolator(interpolator.get(), 0);
				using Flag = RE::NiTimeController::Flag;
				dimmer->flags.reset(Flag::kCycleType_Mask);  // kCycleType_Loop is 0
				dimmer->flags.set(Flag::kAnimType_AppInit, Flag::kComputeScaledTime);
				dimmer->frequency = 1.0f;
				dimmer->phase = a_phase;
				dimmer->loKeyTime = 0.0f;
				dimmer->hiKeyTime = Gems::kPulseSeconds;
				dimmer->SetTarget(a_light);
				return dimmer;
			}
		}

		struct Kept
		{
			RE::NiPointer<RE::NiNode>             root;  // the gem's model
			RE::NiPointer<RE::NiPointLight>       light;
			RE::NiPointer<RE::BSLight>            bs;
			RE::NiPointer<RE::NiInterpController> dimmer;  // the soft pulse
			const Gem*                            gem{ nullptr };
			float                                 phase{ 0.0f };         // where its pulse starts, so gems side by side do not breathe as one
			float                                 wroteRadius{ -1.0f };  // Community Shaders rewrites the light's radius every frame
			bool                                  seen{ false };
		};
		std::unordered_map<RE::FormID, Kept> gKept;  // by the placed gem's form id
		std::atomic<std::size_t>             gCount{ 0 }, gMade{ 0 }, gPutOutNow{ 0 }, gWithGlow{ 0 };
		std::atomic<std::uint32_t>           gChoices{ 0 };  // a bit per ColorChoice whose gem mod is loaded (the menu offers it)
		// the gems near her left to another mod's light at the last look, for the menu (drawn on the render thread): by the mod
		// whose Light Placer file lights them, or by the look whose model carries its own light
		std::mutex                                       gLeftLock;
		std::vector<std::pair<std::string, std::size_t>> gLeftByMod, gLeftByModel;  // gLeftLock

		// a light of another mod taken out, with what gives it back as it was. Its place in the scene is never kept: a removed place something
		// still holds keeps lighting every floor it touched (measured). Light Placer writes a light's culled flag back itself, never the place
		struct PutOutLight
		{
			RE::NiPointer<RE::NiAVObject> object;
			const RE::BSLight*            queued{ nullptr };  // only compared: the place this took out, while its removal may still be queued
			bool                          tookOut{ false };
			bool                          dynamic{ true }, shadow{ false }, portalStrict{ false }, affectLand{ true }, affectWater{ true };
			bool                          wasOut{ false };
		};

		// a gem reference loaded in the world, known from its load event (or a walk). What other lights hang on it is read when it
		// loads and once more (kLooksAtLoad), then never again until its model is rebuilt
		struct TrackedGem
		{
			RE::NiPointer<RE::NiNode>                  root;
			std::vector<RE::NiPointer<RE::NiAVObject>> others;
			std::vector<PutOutLight>                   putOut;
			int                                        looks{ 0 };
			bool                                       keepOut{ false };
			bool                                       relit{ false };  // placed in a cell again: its owner may have put a light back in the place we took it from
			bool                                       hadModel{ false };
		};
		std::unordered_map<RE::FormID, TrackedGem> gNearGems;  // the main thread's
		constexpr int                              kLooksAtLoad = 2;
		// a gem whose load event came before its model is looked for this many looks more, then left to the next walk
		constexpr int kWaitLooks = 300;
		int           gWaitLooks = 0;
		// a light to go back whose removal is still queued: looked at this many looks more, and when a menu closes (the queue may wait out a pause)
		int               gGiveBackLooks = 0;
		std::atomic<bool> gGiveBackWaits{ false };

		// what happened since the last look, from the event sinks (any thread): an id and nothing else is kept
		struct Noted
		{
			RE::FormID id{ 0 };
			// relit: placed in a cell again with its model in - Light Placer puts its light back in the scene there
			bool loaded{ false }, display{ false }, relit{ false };
		};
		std::mutex                     gNotedLock;
		std::vector<Noted>             gNoted;  // gNotedLock
		std::atomic<bool>              gLookQueued{ false }, gWalkWanted{ false }, gAgainQueued{ false };
		std::atomic<std::size_t>       gLooks{ 0 }, gWalks{ 0 }, gNoModelYet{ 0 }, gTrackedNow{ 0 };
		std::vector<RE::FormID>        gDisplaysAgain;  // the main thread's: RE::Light hangs its light in its own load hook, before or after our look
		std::unordered_set<RE::FormID> gDarkDisplays;   // the main thread's

		void Drop(Kept& a_kept, RE::ShadowSceneNode* a_scene)
		{
			a_kept.dimmer.reset();
			if (a_scene && a_kept.bs) {
				a_scene->RemoveLight(a_kept.bs);
			}
			if (a_kept.light && a_kept.light->parent) {
				a_kept.light->parent->DetachChild(a_kept.light.get());
			}
			a_kept = {};
		}

		void DropAll(RE::ShadowSceneNode* a_scene)
		{
			for (auto& [id, kept] : gKept) {
				Drop(kept, a_scene);
			}
			gKept.clear();
			gCount = 0;
		}

		// the settings onto the light: its color, strength and reach, and the keys the game's dimmer moves its fade along
		void Follow(Kept& a_kept, const Gems::Settings& a_s)
		{
			const auto& look = a_kept.gem->look;
			const auto  shownAt = [&](float a_pulse) { return Gems::ShownAt(look.kind, InverseSquare(), LightingFound() == Lighting::kVanilla, a_s, a_pulse); };
			const auto  shown = shownAt(1.0f);
			auto&       data = a_kept.light->GetLightRuntimeData();
			const auto  rgb = Gems::ColorAt(look, a_s);
			data.diffuse = { rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f };
			data.fade = shown.fade;
			if (a_kept.wroteRadius != shown.radius) {
				a_kept.wroteRadius = shown.radius;
				data.radius = { shown.radius, shown.radius, Gems::kGemSize };
				a_kept.light->SetLightAttenuation(shown.radius);
			}
			if (InverseSquare()) {
				Isl::SetOwn(a_kept.light.get(), shown.cutoff);  // after SetLightAttenuation, which writes the same words
			} else if (LightLimitFix()) {
				Isl::SetOwnLinear(a_kept.light.get());
			} else {
				data.ambient = { data.diffuse.red * kAmbient, data.diffuse.green * kAmbient, data.diffuse.blue * kAmbient };
			}
			if (!a_s.pulse || REL::Module::IsVR()) {
				if (a_kept.dimmer) {
					a_kept.dimmer->Stop();
				}
				return;
			}
			if (!a_kept.dimmer) {
				a_kept.dimmer = Dimmer::Hang(a_kept.light.get(), a_kept.phase);
			}
			auto* interpolator = a_kept.dimmer ? netimmerse_cast<RE::NiFloatInterpolator*>(a_kept.dimmer->GetInterpolator(0)) : nullptr;
			auto* keys = interpolator && interpolator->floatData ? interpolator->floatData->keys : nullptr;
			if (keys) {
				for (std::uint32_t i = 0; i < Dimmer::kKeys; ++i) {
					const float at = Gems::kPulseSeconds * static_cast<float>(i) / static_cast<float>(Dimmer::kKeys - 1);
					keys[i].SetTime(at);
					keys[i].SetValue(shownAt(Gems::PulseAt(at)).fade);
				}
				a_kept.dimmer->Start(0.0f);
			}
		}

		[[nodiscard]] bool Make(Kept& a_kept, RE::NiNode* a_root, RE::ShadowSceneNode* a_scene, const Gems::Settings& a_s)
		{
			auto* light = CloneLight();
			if (!light) {
				return false;
			}
			light->name = kName;
			a_root->AttachChild(light, true);
			a_kept.light.reset(light);
			a_kept.root.reset(a_root);
			Follow(a_kept, a_s);
			RE::NiUpdateData update{};
			light->Update(update);
			auto* bs = AddToScene(a_scene, light);
			if (!bs) {
				a_kept.dimmer.reset();
				a_root->DetachChild(light);
				a_kept.light.reset();
				a_kept.root.reset();
				return false;
			}
			a_kept.bs.reset(bs);
			++gMade;
			return true;
		}

		// every other light on this placed gem: one another mod hung under it (Light Placer's, RE::Light's) and a shape whose texture
		// makes a light in this game - on ENB an ENB light texture (fxglowENB, glowsoft01_enbl ...; a shape only NAMED
		// "EnbParticleLight..." over a plain texture lights nothing there), on Community Shaders a texture a particle-light config
		// names. A claim alone is no light: a file may claim a gem it does not light here, so the node says
		[[nodiscard]] std::vector<RE::NiPointer<RE::NiAVObject>> OtherLights(RE::NiAVObject* a_root, Lighting a_lighting, bool a_ownLights)
		{
			std::vector<RE::NiPointer<RE::NiAVObject>> out;
			RE::BSVisit::TraverseScenegraphObjects(a_root, [&](RE::NiAVObject* a_object) {
				if (netimmerse_cast<RE::NiLight*>(a_object)) {
					if (a_object->name != kName) {
						out.emplace_back(a_object);
					}
					return RE::BSVisit::BSVisitControl::kContinue;
				}
				if (a_ownLights) {
					const auto texture = EffectTexture(a_object);
					const bool own = a_lighting == Lighting::kEnb ? std::ranges::find(kEnbLightTextures, texture) != kEnbLightTextures.end() :
					                                                !texture.empty() && gParticleTextures.contains(texture);
					if (own) {
						out.emplace_back(a_object);
					}
				}
				return RE::BSVisit::BSVisitControl::kContinue;
			});
			return out;
		}

		// the other lights on a gem out. A light's place in the scene goes with it, so a mod that writes its light's culled flag back
		// every frame (Light Placer, from its conditions) cannot show it again, and nothing of ours has to keep culling it. A light
		// out already is looked at again: Light Placer puts the same light back in the scene when the gem is placed in a cell
		void PutOut(TrackedGem& a_gem, RE::ShadowSceneNode* a_scene)
		{
			for (const auto& other : a_gem.others) {
				auto out = std::ranges::find(a_gem.putOut, other, &PutOutLight::object);
				if (out == a_gem.putOut.end()) {
					a_gem.putOut.emplace_back().object = other;
					out = a_gem.putOut.end() - 1;
					out->wasOut = other->GetAppCulled();
				}
				other->SetAppCulled(true);
				auto* light = netimmerse_cast<RE::NiLight*>(other.get());
				auto* bs = light ? a_scene->GetLight(light) : nullptr;
				// a removal of ours still queued is the same place: never removed twice, unless its owner has just put it back
				if (!bs) {
					out->queued = nullptr;  // its removal has run: a new place at the same address is not this one
				}
				if (bs && (a_gem.relit || bs != out->queued)) {
					out->queued = bs;
					out->tookOut = true;
					out->dynamic = bs->dynamic;
					out->shadow = bs->IsShadowLight();
					out->portalStrict = bs->portalStrict;
					out->affectLand = bs->affectLand;
					out->affectWater = bs->affectWater;
					a_scene->RemoveLight(RE::NiPointer<RE::BSLight>(bs));
				}
			}
			a_gem.relit = false;
		}

		// each as it was - only while the gem still wears that model: a light of an old model would come back as a ghost
		// true when one could not go back yet: its removal is still queued, and a light added before that runs stays dark
		[[nodiscard]] bool GiveBack(TrackedGem& a_gem, RE::ShadowSceneNode* a_scene)
		{
			bool waiting = false;
			std::erase_if(a_gem.putOut, [&](const PutOutLight& out) {
				out.object->SetAppCulled(out.wasOut);
				auto*       light = netimmerse_cast<RE::NiLight*>(out.object.get());
				const auto* now = light ? a_scene->GetLight(light) : nullptr;
				if (now && now == out.queued) {
					waiting = true;
					return false;
				}
				// its owner may have put it back itself meanwhile (Light Placer, at the gem's placing): never a second place
				if (out.tookOut && light && !now) {
					RE::ShadowSceneNode::LIGHT_CREATE_PARAMS params{};
					params.dynamic = out.dynamic;
					params.shadowLight = out.shadow;
					params.portalStrict = out.portalStrict;
					params.affectLand = out.affectLand;
					params.affectWater = out.affectWater;
					params.neverFades = true;
					params.fov = 90.0f;
					params.falloff = 1.0f;
					params.nearDistance = 5.0f;
					params.depthBias = 1.0f;
					a_scene->AddLight(light, params);
				}
				return true;
			});
			return waiting;
		}

		// true when another look should try again what GiveBack could not give back yet
		[[nodiscard]] bool GiveBackAgain(bool a_waits)
		{
			if (!a_waits) {
				gGiveBackWaits.store(false, std::memory_order_relaxed);
				return false;
			}
			if (!gGiveBackWaits.exchange(true, std::memory_order_relaxed)) {
				gGiveBackLooks = kWaitLooks;
			}
			if (gGiveBackLooks <= 0) {
				return false;
			}
			--gGiveBackLooks;
			return true;
		}

		// a gem on show is never lit (displays never get lit): a placed gem wearing a display script - Legacy of the Dragonborn's
		// museum shows the real Azura's Star and Black Star through dbm_dynamicdisplayscript, enabled once donated. A gem left
		// lying about carries no such script (the College's carry defaultDisableHavokOnLoad). The same check runs on VR (Papyrus
		// Extender's VR build reads this table the same way); Legacy's own displays are also known by their place
		// (Gems::KnownDisplay), for the moment before their script is bound
		std::atomic<std::size_t> gOnShowNow{ 0 };

		[[nodiscard]] bool OnShow(const RE::TESObjectREFR* a_ref)
		{
			if (const auto* file = a_ref->GetFile(0); file && Gems::KnownDisplay(file->GetFilename(), a_ref->GetLocalFormID())) {
				return true;
			}
			auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			auto* policy = vm ? vm->GetObjectHandlePolicy() : nullptr;
			if (!policy) {
				return false;
			}
			const auto          handle = policy->GetHandleForObject(static_cast<RE::VMTypeID>(RE::FormType::Reference), a_ref);
			RE::BSSpinLockGuard locker(vm->attachedScriptsLock);
			const auto          found = vm->attachedScripts.find(handle);
			return found != vm->attachedScripts.end() && std::ranges::any_of(found->second, [](const auto& a_script) {
				const auto* type = a_script.get() ? a_script.get()->GetTypeInfo() : nullptr;
				return type && Gems::IsDisplayScript(type->GetName());
			});
		}

		// ours on the gem: made once, again when its model was rebuilt; false when it could not be made (the next look tries again)
		[[nodiscard]] bool Keep(RE::FormID a_id, RE::NiNode* a_root, const Gem& a_gem, RE::ShadowSceneNode* a_scene, const Gems::Settings& a_s)
		{
			auto& kept = gKept[a_id];
			if (kept.light && (kept.root.get() != a_root || kept.light->parent != a_root)) {
				Drop(kept, a_scene);  // the gem's model was rebuilt: a new one
			}
			kept.gem = &a_gem;
			if (!kept.light) {
				kept.phase = static_cast<float>(a_id % 997u) / 997.0f * Gems::kPulseSeconds;
				if (!Make(kept, a_root, a_scene, a_s)) {
					static bool warned = false;
					if (!std::exchange(warned, true)) {
						SKSE::log::warn("a soul gem light could not be made or registered; such a gem is tried again at the next look");
					}
					return false;
				}
			}
			kept.seen = true;
			return true;
		}

		// a display's RE::Light light out, once it is loaded (the set of dark ones is for the report)
		void DarkenOne(RE::FormID a_id)
		{
			auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_id);
			auto* shown = ref && ref->Is3DLoaded() ? ref->Get3D() : nullptr;
			if (shown && DarkenDisplay(shown)) {
				gDarkDisplays.insert(a_id);
			}
		}

		void LookAgainSoon();

		// What every gem wears now, from what the game has announced (Watch, below) - never on a clock. Which gems are the nearest
		// kMaxLights is judged here too, so it holds from one announcement to the next: a stash of more gems does not re-sort as she
		// walks inside one cell. a_walk: the references of the loaded cells are walked as well (a load, her cell, a setting)
		void Look(bool a_walk)
		{
			const FrameCost::Scope cost{ 0, "a look (a gem loaded or unloaded, her cell changed, a setting)" };
			++gLooks;
			std::vector<Noted> noted;
			{
				std::lock_guard lock(gNotedLock);
				noted.swap(gNoted);
			}
			for (const auto id : std::exchange(gDisplaysAgain, {})) {
				DarkenOne(id);
			}
			for (const auto& n : noted) {
				if (n.display) {
					if (n.loaded) {
						DarkenOne(n.id);
						gDisplaysAgain.push_back(n.id);
					} else {
						gDarkDisplays.erase(n.id);
					}
				} else if (n.loaded) {
					auto& tracked = gNearGems[n.id];
					if (n.relit) {
						tracked.looks = 0;  // its other lights are read and put out again
						tracked.relit = true;
					}
					gWaitLooks = kWaitLooks;
				}
			}
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* tes = RE::TES::GetSingleton();
			if (a_walk && tes) {
				const FrameCost::Scope walk{ 1, "a walk of the loaded references" };
				++gWalks;
				gDarkDisplays.clear();
				std::erase_if(gNearGems, [](const auto& a_entry) {
					const auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_entry.first);
					return !ref || (a_entry.second.hadModel && !ref->Is3DLoaded());
				});
				tes->ForEachReference([](RE::TESObjectREFR* a_ref) {
					const auto* base = a_ref ? a_ref->GetBaseObject() : nullptr;
					if (base && gGems.contains(base)) {
						gNearGems.try_emplace(a_ref->GetFormID());
					} else if (base && gDisplays.contains(base)) {
						DarkenOne(a_ref->GetFormID());
					}
					return RE::BSContainer::ForEachResult::kContinue;
				});
			}
			gDisplaysDark = gDarkDisplays.size();
			gTrackedNow = gNearGems.size();
			auto* scene = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
			if (!scene) {
				return;  // no shadow scene to take our lights out of: they stay as they are until there is one
			}
			const auto s = SettingsNow();
			if (!s.on || !player || !tes || !player->GetParentCell()) {
				if (!s.on) {
					DropAll(scene);
					bool waits = false;
					for (auto& [id, gem] : gNearGems) {
						waits = GiveBack(gem, scene) || waits;
					}
					if (GiveBackAgain(waits)) {
						LookAgainSoon();
					}
					gPutOutNow = 0;
					std::lock_guard lock(gLeftLock);
					gLeftByMod.clear();
					gLeftByModel.clear();
				}
				return;
			}
			for (auto& [id, kept] : gKept) {
				kept.seen = false;
			}
			const auto lighting = LightingFound();
			// a gem's own model light shines on an ENB (found running), or on Community Shaders when a particle-light config names it
			const bool ownLights = lighting == Lighting::kEnb || (lighting == Lighting::kShaders && !gParticleTextures.empty());
			struct Candidate
			{
				RE::TESObjectREFR* ref{ nullptr };  // read this look only, on the main thread
				RE::FormID         id{ 0 };
				RE::NiNode*        root{ nullptr };
				const Gem*         gem{ nullptr };
				TrackedGem*        tracked{ nullptr };
				float              distance{ 0.0f };  // squared, to the player
				bool               lit{ false };
			};
			std::vector<Candidate> candidates;
			candidates.reserve(gNearGems.size());
			const auto  here = player->GetPosition();
			const auto* space = player->GetWorldspace();
			std::size_t noModelYet = 0;
			for (auto& [id, gem] : gNearGems) {
				gem.keepOut = false;
				auto* ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(id);
				if (!ref || !ref->Is3DLoaded()) {
					const bool had = gem.hadModel;
					noModelYet += ref && !had && !ref->IsDisabled() && !ref->IsDeleted() ? 1 : 0;
					gem = {};  // unloaded: every light of the old model went with it
					gem.hadModel = had;
					continue;
				}
				const auto* base = ref->GetBaseObject();
				const auto  it = base ? gGems.find(base) : gGems.end();
				auto*       model = ref->Get3D();
				auto*       root = model ? model->AsNode() : nullptr;
				// a gem switched off in the menu takes no place among the nearest: a farther one gets the light. A door or a fast
				// travel leaves the old cell's references here for a moment
				if (it == gGems.end() || !root || !Gems::GemLit(it->second.look, s) || ref->IsDisabled() || ref->IsDeleted() ||
					(space ? ref->GetWorldspace() != space : ref->GetParentCell() != player->GetParentCell()) ||
					(it->second.needsSoul && ref->extraList.GetSoulLevel() == RE::SOUL_LEVEL::kNone)) {
					continue;
				}
				if (gem.root.get() != root) {
					gem = {};
					gem.root.reset(root);
					gem.hadModel = true;
				}
				// a gem that holds our light already counts as a fifth nearer, so two gems at the kMaxLights edge do not swap lights
				const auto held = gKept.find(id);
				const bool lit = held != gKept.end() && held->second.light && held->second.root.get() == root;
				candidates.push_back({ ref, id, root, &it->second, &gem, ref->GetPosition().GetSquaredDistance(here) * (lit ? 0.64f : 1.0f), lit });
			}
			// the nearest kMaxLights gems keep a light: a shop, a stash or the Soul Cairn would otherwise hang one on every gem loaded;
			// a farther gem is let go below and takes a light again when it is among the nearest
			std::ranges::sort(candidates, {}, &Candidate::distance);
			using Gems::WhoLights;
			std::size_t                                      ours = 0, looked = 0, onShow = 0;
			bool                                             again = !gDisplaysAgain.empty();
			std::vector<std::pair<std::string, std::size_t>> byMod, byModel;  // the gems left to another mod's light, for the menu
			for (const auto& c : candidates) {
				if (++looked > kMostLooked || (s.who != WhoLights::kGame && ours == kMaxLights)) {
					break;
				}
				// asked only of the nearest kMostLooked gems, outside any reference walk: it takes the Papyrus VM's lock
				if (OnShow(c.ref)) {
					++onShow;
					continue;
				}
				auto& gem = *c.tracked;
				if (gem.looks < kLooksAtLoad) {
					gem.others = OtherLights(c.root, lighting, ownLights);
					++gem.looks;
					again = again || gem.looks < kLooksAtLoad;
				}
				const bool theirs = !gem.others.empty();
				// a claimed gem's light may hang a moment after its load event: the second look decides before ours is hung on it
				if (s.who == WhoLights::kOthers && !theirs && !c.lit && c.gem->owner != Gems::Owner::kThisMod && gem.looks < kLooksAtLoad) {
					continue;
				}
				const bool hung = Gems::OursOn(s.who, theirs) && Keep(c.id, c.root, *c.gem, scene, s);
				ours += hung ? 1 : 0;
				if (theirs && s.who == WhoLights::kOthers) {
					const bool placed = std::ranges::any_of(gem.others, [](const auto& a_other) { return netimmerse_cast<RE::NiLight*>(a_other.get()) != nullptr; });
					if (placed) {
						CountName(byMod, c.gem->ownerName ? std::string_view(*c.gem->ownerName) : std::string_view("another mod"));
					} else {
						CountName(byModel, Gems::FamilyName(c.gem->look.family));
					}
				} else if (theirs && (hung || s.who == WhoLights::kGame)) {
					gem.keepOut = true;
					PutOut(gem, scene);
				}
			}
			std::size_t putOut = 0;
			bool        waits = false;
			for (auto& [id, gem] : gNearGems) {
				if (!gem.keepOut && !gem.putOut.empty()) {
					waits = GiveBack(gem, scene) || waits;
				}
				putOut += gem.putOut.empty() ? 0 : 1;
			}
			again = GiveBackAgain(waits) || again;
			std::erase_if(gKept, [&](auto& a_entry) {
				if (!a_entry.second.seen) {
					Drop(a_entry.second, scene);
					return true;
				}
				return false;
			});
			gCount = gKept.size();
			gOnShowNow = onShow;
			gPutOutNow = putOut;
			gNoModelYet = noModelYet;
			{
				std::lock_guard lock(gLeftLock);
				gLeftByMod = std::move(byMod);
				gLeftByModel = std::move(byModel);
			}
			if (noModelYet > 0 && gWaitLooks > 0) {
				--gWaitLooks;
				again = true;
			}
			if (again) {
				LookAgainSoon();
			}
		}

		// one look on the main thread for any number of announcements (one task, never a task that queues itself)
		void RequestLook(bool a_walk)
		{
			if (a_walk) {
				gWalkWanted = true;
			}
			if (!gLookQueued.exchange(true) && !Later("gem look", []() {
					gLookQueued = false;
					Look(gWalkWanted.exchange(false));
				})) {
				gLookQueued = false;
			}
		}

		// one more look a moment later. By way of the UI's queue: a main-thread task queued from a main-thread task can run in the
		// same pass over SKSE's queue, and one re-queuing itself held every save load for 90 s
		void LookAgainSoon()
		{
			if (gAgainQueued.exchange(true)) {
				return;
			}
			if (auto* tasks = SKSE::GetTaskInterface()) {
				tasks->AddUITask([]() {
					gAgainQueued = false;
					RequestLook(false);
				});
			} else {
				gAgainQueued = false;
			}
		}

		// any thread: only a gem or a display is noted (a cell's load is thousands of references; gGems and gDisplays are fixed after
		// data load)
		void Note(const RE::TESObjectREFR* a_ref, bool a_loaded, bool a_relit = false)
		{
			const auto* base = a_ref ? a_ref->GetBaseObject() : nullptr;
			const bool  gem = base && gGems.contains(base), display = !gem && base && gDisplays.contains(base);
			if (gem || display) {
				{
					std::lock_guard lock(gNotedLock);
					gNoted.push_back({ a_ref->GetFormID(), a_loaded, display, a_relit });
				}
				RequestLook(false);
			}
		}

		// the game's own calls for a reference's 3D, once per load and per release, on whichever thread (the way RE::Light by Truman and
		// Light Placer by powerof3 hear of it, both credited on the page): the game's load event names no gem placed by a script, and none
		// disabled or picked up. Installed after theirs, so a light they hang is on the model by the time this one notes the gem
		struct Load3D
		{
			static RE::NiAVObject* thunk(RE::TESObjectREFR* a_this, bool a_backgroundLoading)
			{
				auto* object = func(a_this, a_backgroundLoading);
				if (object) {
					Guarded("gem load", [a_this]() { Note(a_this, true); });
				}
				return object;
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		struct Release3D
		{
			static void thunk(RE::TESObjectREFR* a_this)
			{
				Guarded("gem release", [a_this]() { Note(a_this, false); });
				func(a_this);
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		// the game placing a reference in a cell (a gem dropped or moved): it sets the cell, then attaches the reference's lights, where
		// Light Placer puts its light back in the scene - after our looks of the gem's load. A gem with its model in is noted again
		struct SetParentCell
		{
			static void thunk(RE::TESObjectREFR* a_this, RE::TESObjectCELL* a_cell)
			{
				func(a_this, a_cell);
				Guarded("gem placed", [a_this, a_cell]() {
					if (a_cell && a_this->Is3DLoaded()) {
						Note(a_this, true, true);
					}
				});
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		// what the game announces, each only noted here - the main thread acts: a cell's load (a look) and her own cell change (a walk)
		class Watch final :
			public RE::BSTEventSink<RE::TESCellFullyLoadedEvent>,
			public RE::BSTEventSink<RE::BGSActorCellEvent>,
			public RE::BSTEventSink<RE::MenuOpenCloseEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::TESCellFullyLoadedEvent*, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override
			{
				RequestLook(false);  // the nearest again; the Load3D hook has noted every gem it brought
				return RE::BSEventNotifyControl::kContinue;
			}

			RE::BSEventNotifyControl ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*) override
			{
				if (a_event && a_event->flags.get() == RE::BGSActorCellEvent::CellFlag::kEnter) {
					RequestLook(true);
				}
				return RE::BSEventNotifyControl::kContinue;
			}

			// Quit to Main Menu: the world our lights hang in is gone, so they are taken out when the Main Menu opens (on the main
			// thread; a game loading drops them too). A loading screen takes every light out of the scene's lists and ours are not put back:
			// when it closes each is let go, and the look makes it again
			RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
			{
				const std::string_view menu = a_event ? a_event->menuName.c_str() : "";
				if (a_event && a_event->opening && menu == RE::MainMenu::MENU_NAME) {
					static_cast<void>(Later("main menu", []() { DropGemLights(); }));
				} else if (a_event && !a_event->opening && menu == RE::LoadingMenu::MENU_NAME) {
					static_cast<void>(Later("loading screen", []() {
						DropAll(RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0]);
						RequestLook(true);
					}));
				} else if (a_event && !a_event->opening && gGiveBackWaits.load(std::memory_order_relaxed)) {
					// the removal queue may have waited out the pause: a fresh set of looks, or one look at the close could miss it
					static_cast<void>(Later("menu closed", []() {
						gGiveBackLooks = kWaitLooks;
						RequestLook(false);
					}));
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};
	}

	void FindGems()
	{
		gGems.clear();
		gFamilies.clear();
		gOtherLighters = {};
		gBlocked.clear();
		auto* dh = RE::TESDataHandler::GetSingleton();
		if (!dh) {
			SKSE::log::error("gems: no data handler - nothing is lit");
			return;
		}
		const bool sorted = Loaded(dh, "RUSTIC SOULGEMS - Sorted.esp"), unsorted = Loaded(dh, "RUSTIC SOULGEMS - Unsorted.esp");
		gRustic = sorted && unsorted ? "both" : sorted ? "Sorted" :
		                                    unsorted   ? "Unsorted" :
		                                                 "";
		// the particle-light configs (Data\ParticleLights\<texture>.ini): a Community Shaders that draws particle lights (Bottled
		// Shaders, Community Shaders Expanded) lights every shape drawing such a texture itself
		gParticleTextures.clear();
		std::error_code ec;
		for (std::filesystem::directory_iterator it(std::filesystem::current_path() / "Data" / "ParticleLights", ec), end; !ec && it != end; it.increment(ec)) {
			if (LowerExtension(it->path()) == L".ini") {
				std::string stem;  // texture names are plain ASCII; path::string() throws on a name the ANSI code page cannot show
				for (const wchar_t c : it->path().stem().wstring()) {
					stem.push_back(c < 128 ? static_cast<char>(std::tolower(static_cast<unsigned char>(c))) : '?');
				}
				gParticleTextures.insert(std::move(stem));
			}
		}
		Others others;
		others.lightPlacer = REX::W32::GetModuleHandleA("po3_LightPlacer.dll") != nullptr;
		// RE::Light (ReLight) lights this mod's gems only through this mod's RE::Light files: without them we light those gems
		const bool reLightDll = REX::W32::GetModuleHandleA("Relight.dll") != nullptr;
		const auto ourConfigs = std::filesystem::current_path() / "Data" / "SKSE" / "Plugins" / "ReLight" / "Configs" / "Illuminated - Soul Gem Hub";
		others.reLight = reLightDll && std::filesystem::is_directory(ourConfigs, ec);
		if (others.lightPlacer) {
			others.claims = ReadLightPlacerClaims();
		}
		const auto*                                               azurasStar = dh->LookupForm(kAzurasStar, "Skyrim.esm");
		const auto*                                               blackStar = dh->LookupForm(kBlackStar, "Skyrim.esm");
		std::size_t                                               records = 0, lit = 0, fromModel = 0, fromPicture = 0, fromSwap = 0, withGlow = 0;
		std::array<bool, Gems::kColorChoices + 1>                 choicesFound{};  // ColorChoice order, kNone first
		std::vector<std::pair<const RE::TESForm*, std::uint32_t>> named;
		for (const auto& n : Gems::kNamedGems) {
			if (const auto* form = dh->LookupForm(n.id, n.plugin)) {
				named.emplace_back(form, n.id);
			}
		}
		const auto namedId = [&](const RE::TESForm* a_form) {
			const auto it = std::ranges::find(named, a_form, &std::pair<const RE::TESForm*, std::uint32_t>::first);
			return it == named.end() ? 0u : it->second;
		};
		// a gem wears the color its lit-room picture shows (stars keep their own; a named quest gem its own row), else a look its
		// model colors the one read off the model; a blocked mod's model is never read here
		const auto readColor = [&](Gems::Look& a_look, std::string_view a_model, bool a_star, std::uint32_t a_named) {
			if (BlockedBy(a_model)) {
				return;
			}
			const bool packed = !a_star && PackOnTop(a_model);
			if (!a_star && !packed && (!a_named || a_look.family == Gems::Family::kNamed)) {
				SetGlow(a_look, GlowOf(a_model));
				if (!a_look.glow && a_look.family == Gems::Family::kGame) {
					a_look.glow = GameGlow(a_model);
				}
			}
			if (const auto measured = a_star || a_named ? std::nullopt : MeasuredColor(a_model)) {
				// the color the game shows for this gem, measured off its own pictures: its light wears it, not another glow
				Gems::WearFixedColor(a_look, *measured);
				++fromPicture;
			} else if (const auto seen = a_star || packed ? std::nullopt : SeenColor(a_model, a_named)) {
				a_look.rgb = *seen;
				++fromPicture;
			} else if (const auto eye = a_star ? EyeColor(a_model, Gems::StarEyeColorOf) : std::nullopt) {
				a_look.rgb = *eye;
				++fromPicture;
			} else if (Gems::ColorFromModel(a_look.family) || packed) {
				if (const auto own = ModelColor(a_model, a_star)) {
					a_look.rgb = Gems::ShownColor(*own, a_look.kind);
					++fromModel;
				}
			}
			if (a_star) {
				a_look.gemRgb = Gems::ShownColor(Gems::StarGemColor(ModelColor(a_model, true, true), a_look.blackStar), a_look.kind);
				return;
			}
			// the game's own gems wear the set's colors, a named quest gem on a game model too; a gem mod measured or pictured keeps its own
			if (!packed && a_look.family == Gems::Family::kGame && !MeasuredColor(a_model) && GameGlow(a_model)) {
				Gems::WearGameSetColor(a_look, LightLimitFix());
			}
			// his color calls by the gem's own model (its fingerprint is the last of its files) stand over everything read above; a
			// named quest gem keeps its own color
			const auto& prints = PrintsOf(a_model);
			if (const auto* his = a_named || prints.empty() || packed ? nullptr : Gems::HisColorOf(prints.back())) {
				Gems::ApplyHisColor(a_look, *his);
				choicesFound[static_cast<std::size_t>(his->choice)] = true;
			}
		};
		for (auto* gem : dh->GetFormArray<RE::TESSoulGem>()) {
			if (!gem) {
				continue;
			}
			++records;
			const bool  holds = gem->GetContainedSoul() != RE::SOUL_LEVEL::kNone;
			const char* path = gem->GetModel();
			const auto  model = std::string_view(path ? path : "");
			// a star by its record, or by the star model it wears (a gem mod's own star records)
			const auto starModel = Gems::StarOf(model);
			const bool black = gem == blackStar || (gem != azurasStar && starModel == Gems::StarModel::kBlack);
			const bool star = black || gem == azurasStar || starModel == Gems::StarModel::kAzura;
			auto       look = model.empty() ? std::nullopt : Gems::LookOfModel(model, !gRustic.empty());
			if (star) {
				look = Gems::Look{ Gems::Family::kStar, Gems::Kind::kStar, black ? Gems::kBlackStarRgb : Gems::kAzurasStarRgb };
				look->blackStar = black;
			} else if (!look || look->family == Gems::Family::kSoulCairn) {
				look = Gems::GameLook(KindOfCapacity(gem->GetMaximumCapacity()), gem->CanHoldNPCSoul());
			}
			// a star's picture rows were keyed by the other star's model, so both stars read their own textures (readColor)
			readColor(*look, model, star, namedId(gem));
			look->named = namedId(gem) != 0;  // on a plain model too (a kGame look): its own light switch
			// a gem in textures of its own (its record swaps them onto the model): their color, never the model's
			if (const auto swapped = star ? std::vector<std::string>{} : SwappedDiffuses(gem); !swapped.empty()) {
				if (const auto mod = BlockedSwap(swapped)) {
					CountName(gBlocked, *mod);
					continue;
				}
				// its own textures, never the game model's glow
				SetGlow(*look, SwapGlow(swapped));
				look->exact = false;
				if (const auto own = SwapColor(swapped, look->kind)) {
					look->rgb = *own;
					++fromSwap;
				}
			}
			const bool always = star || holds || look->family == Gems::Family::kNamed;
			const bool taken = Take(gem, model, Gem{ *look, !always, Gems::Owner::kThisMod, nullptr }, others);
			withGlow += taken && look->glow ? 1 : 0;
			if (taken && always) {
				++lit;
				CountName(gFamilies, Gems::FamilyName(look->family));
			}
		}
		std::size_t pieces = AddByModel<RE::TESObjectMISC>(true, others) + AddByModel<RE::IngredientItem>(true, others) +
		                     AddByModel<RE::TESObjectACTI>(false, others) + AddByModel<RE::TESFurniture>(false, others);
		// the named quest gems are lit always, on any look: with no model of their own they wear a plain gem's, in their own
		// lit-room color where a picture read it (kNamedSeenColors), else the plain gem's
		for (const auto& n : Gems::kNamedGems) {
			auto*       form = dh->LookupForm(n.id, n.plugin);
			auto*       bound = form ? form->As<RE::TESBoundObject>() : nullptr;
			const auto* has = bound ? form->As<RE::TESModel>() : nullptr;
			const char* path = has ? has->GetModel() : nullptr;
			if (!path || !*path) {
				continue;
			}
			if (const auto it = gGems.find(bound); it != gGems.end()) {
				if (it->second.needsSoul) {
					it->second.needsSoul = false;
					++lit;
					CountName(gFamilies, Gems::FamilyName(it->second.look.family));
				}
				continue;
			}
			if (form->Is(RE::FormType::SoulGem)) {
				continue;  // left to RE::Light, or waiting on its author's permission, by the soul gem pass
			}
			auto look = Gems::LookOfModel(path, !gRustic.empty()).value_or(Gems::GameLook(n.kind, false));
			readColor(look, path, false, n.id);
			look.named = true;
			pieces += Take(bound, path, Gem{ look, false, Gems::Owner::kThisMod, nullptr }, others) ? 1 : 0;
		}
		gDisplays.clear();
		if (others.reLight) {
			const auto displays = AddDisplays<RE::TESObjectSTAT>() + AddDisplays<RE::BGSMovableStatic>();
			SKSE::log::info("gems: {} display(s) wearing a gem's model - an RE::Light light on one is put out (displays never get lit)", displays);
		}
		std::string list;
		for (const auto& [name, n] : gFamilies) {
			list += std::format("{}{} {}", list.empty() ? "" : ", ", n, name);
		}
		using enum Gems::Owner;
		SKSE::log::info(
			"gems: {} soul gem record(s), {} lit always ({}), the rest when a placed gem holds a soul; {} other piece(s) by their "
			"model; Rustic Soul Gems {}; claimed by Emissive Soul Gems {}, by other Light Placer files {} (the menu's Whose Light decides), left to "
			"RE::Light {} (Light Placer {}, RE::Light {}); {} particle-light config(s)",
			records, lit, list.empty() ? "none" : list, pieces, gRustic.empty() ? "not loaded" : gRustic,
			gOtherLighters[static_cast<std::size_t>(kEmissive)], gOtherLighters[static_cast<std::size_t>(kLightPlacer)],
			gOtherLighters[static_cast<std::size_t>(kReLight)], others.lightPlacer ? "loaded" : "not loaded",
			others.reLight ? "loaded with this mod's files" : reLightDll ? "loaded without this mod's files" :
																		   "not loaded",
			gParticleTextures.size());
		SKSE::log::info(
			"gems: {} soul gem record(s) wear the color their picture shows, {} the color read off their own model, {} the color of "
			"their own swapped textures; {} have an own glow of another color (the Light color default)",
			fromPicture, fromModel, fromSwap, withGlow);
		// a glow worn on both choices (ASGO's Solar gems) gives the menu's Light color nothing to switch
		const auto ownGlow = [](const Gem& a_gem) { return a_gem.look.glow.has_value() && !a_gem.look.glowAlways; };
		gWithGlow = static_cast<std::size_t>(std::ranges::count_if(gGems, [&](const auto& a_entry) { return ownGlow(a_entry.second); }));
		std::uint32_t choices = 0;
		for (std::size_t i = 1; i < choicesFound.size(); ++i) {
			choices |= choicesFound[i] ? 1u << i : 0u;
		}
		gChoices = choices;
		gRusticLit = std::ranges::any_of(gGems, [](const auto& a_entry) { return a_entry.second.look.family == Gems::Family::kRustic; });
		for (const auto& [mod, n] : gBlocked) {
			SKSE::log::info("gems: {} record(s) wear {}'s files - not lit, at its author's request", n, mod);
		}
		for (const auto& [form, gem] : gGems) {
			if (const auto* soulGem = form->As<RE::TESModel>(); soulGem && form->Is(RE::FormType::SoulGem) && Gems::ColorFromModel(gem.look.family)) {
				const char* model = soulGem->GetModel();
				// the color its light wears on the default settings, then the color found for it
				const auto worn = Gems::ColorAt(gem.look, Gems::Settings{});
				SKSE::log::info("  {:08X} {} | {} | {},{},{} (on the default settings)", form->GetFormID(), form->GetName(), model ? model : "", worn[0], worn[1], worn[2]);
				SKSE::log::info("    found: {},{},{}", gem.look.rgb[0], gem.look.rgb[1], gem.look.rgb[2]);
				if (const auto& g = gem.look.gemRgb) {
					SKSE::log::info("    its gem (Star light on Gem): {},{},{}", (*g)[0], (*g)[1], (*g)[2]);
				}
			}
		}
	}

	void WatchGems()
	{
		static Watch watch;
		if (auto* ui = RE::UI::GetSingleton()) {
			ui->AddEventSink<RE::MenuOpenCloseEvent>(&watch);
		}
		if (auto* events = RE::ScriptEventSourceHolder::GetSingleton()) {
			events->AddEventSink<RE::TESCellFullyLoadedEvent>(&watch);
		}
		if (auto* player = RE::PlayerCharacter::GetSingleton()) {
			player->AsBGSActorCellEventSource()->AddEventSink(&watch);
		}
		REL::Relocation<std::uintptr_t> vtbl{ RE::TESObjectREFR::VTABLE[0] };
		Load3D::func = vtbl.write_vfunc(0x6A, Load3D::thunk);
		Release3D::func = vtbl.write_vfunc(0x6B, Release3D::thunk);
		SetParentCell::func = vtbl.write_vfunc(REL::Relocate(0x98, 0x98, 0x99), SetParentCell::thunk);
		SKSE::log::info("the lights follow the game's own calls and events (a gem placed in a cell too); the soft pulse is run by the game");
	}

	void LookAfterLoad()
	{
		RequestLook(true);
	}

	void DropGemLights()
	{
		DropAll(RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0]);
		gNearGems.clear();  // the old game's: nothing to give back, every light on them is gone with them
		gDisplaysAgain.clear();
		gDarkDisplays.clear();
		gTrackedNow = 0;
		gNoModelYet = 0;
		gPutOutNow = 0;
		{
			std::lock_guard lock(gNotedLock);
			gNoted.clear();
		}
		{
			std::lock_guard lock(gLeftLock);
			gLeftByMod.clear();
			gLeftByModel.clear();
		}
	}

	void ApplyNow()
	{
		// a look now, walking the loaded references (a gem moved by a script sends no load event), and the settings onto every light
		Look(true);
		RefreshLights();
	}

	void RefreshLights()
	{
		const auto s = SettingsNow();
		for (auto& [id, kept] : gKept) {
			if (kept.light && kept.gem) {
				Follow(kept, s);
			}
		}
	}

	std::size_t GemLightCount() { return gCount.load(); }

	std::vector<std::pair<std::string, std::size_t>> GemFamilies() { return gFamilies; }
	std::vector<std::pair<std::string, std::size_t>> GemsBlocked() { return gBlocked; }
	std::array<std::size_t, 4>                       GemsOtherLighters() { return gOtherLighters; }
	std::size_t                                      GemsOtherLightsPutOut() { return gPutOutNow.load(); }
	std::vector<std::pair<std::string, std::size_t>> GemsLitBy() { return gLitBy; }
	GemsLeft                                         GemsLeftNear()
	{
		std::lock_guard lock(gLeftLock);
		return { gLeftByMod, gLeftByModel };
	}
	bool        LightColorMatters() { return gWithGlow.load() > 0; }
	bool        ColorChoiceFound(Gems::ColorChoice a_choice) { return (gChoices.load() >> static_cast<std::uint32_t>(a_choice)) & 1u; }
	std::string RusticFound() { return gRusticLit ? gRustic : std::string(); }

	std::string GlowNow(RE::FormID a_ref)
	{
		auto*          ref = RE::TESForm::LookupByID<RE::TESObjectREFR>(a_ref);
		auto*          root = ref ? ref->Get3D() : nullptr;
		nlohmann::json shapes = nlohmann::json::array();
		if (root) {
			RE::BSVisit::TraverseScenegraphGeometries(root, [&](RE::BSGeometry* a_geometry) {
				auto* lit = netimmerse_cast<RE::BSLightingShaderProperty*>(a_geometry->GetGeometryRuntimeData().shaderProperty.get());
				if (lit && lit->emissiveColor) {
					const auto& e = *lit->emissiveColor;
					shapes.push_back({ { "shape", a_geometry->name.c_str() }, { "emissive", { e.red, e.green, e.blue } }, { "mult", lit->emissiveMult },
						{ "ownEmit", lit->flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kOwnEmit) },
						{ "glowMap", lit->flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kGlowMap) } });
				}
				return RE::BSVisit::BSVisitControl::kContinue;
			});
		}
		// a shape name that is not valid UTF-8 is written with replacement characters, never thrown on
		const nlohmann::json out{ { "ref", std::format("0x{:08X}", a_ref) }, { "loaded", root != nullptr }, { "shapes", shapes } };
		return out.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
	}

	std::string GemsReport()
	{
		// built as JSON, not by text: a mod or family name holding a quote or a non-UTF-8 byte stays a valid reply
		const auto     s = SettingsNow();
		nlohmann::json lights = nlohmann::json::array();
		for (const auto& [id, kept] : gKept) {
			if (!kept.light || !kept.gem) {
				continue;
			}
			const auto& d = kept.light->GetLightRuntimeData();
			const auto& p = kept.light->world.translate;
			const auto  rgb = Gems::ColorAt(kept.gem->look, s);  // the color it wears now (the menu's color choices)
			lights.push_back({ { "ref", std::format("0x{:08X}", id) }, { "family", std::string(Gems::FamilyName(kept.gem->look.family)) },
				{ "kind", static_cast<int>(kept.gem->look.kind) }, { "rgb", { rgb[0], rgb[1], rgb[2] } }, { "fade", d.fade }, { "radius", d.radius.x }, { "pulseRun", kept.dimmer != nullptr },
				{ "at", { std::round(p.x), std::round(p.y), std::round(p.z) } } });
		}
		nlohmann::json families = nlohmann::json::object(), blocked = nlohmann::json::object();
		for (const auto& [name, n] : gFamilies) {
			families[name] = n;
		}
		for (const auto& [name, n] : gBlocked) {
			blocked[std::string(name)] = n;
		}
		nlohmann::json litBy = nlohmann::json::object(), nearByMod = nlohmann::json::object(), nearByModel = nlohmann::json::object();
		for (const auto& [name, n] : gLitBy) {
			litBy[name] = n;
		}
		const auto left = GemsLeftNear();
		for (const auto& [name, n] : left.byMod) {
			nearByMod[name] = n;
		}
		for (const auto& [name, n] : left.byModel) {
			nearByModel[name] = n;
		}
		const nlohmann::json out{ { "gemForms", gGems.size() }, { "litByLook", families }, { "waitingOnPermission", blocked }, { "rustic", gRustic },
			{ "claimedBy", { { "emissiveSoulGems", gOtherLighters[1] }, { "lightPlacer", gOtherLighters[2] } } },
			{ "leftTo", { { "reLight", gOtherLighters[3] } } }, { "litBy", litBy }, { "leftNearYou", { { "byMod", nearByMod }, { "byModel", nearByModel } } }, { "otherLightsPutOutNearYou", gPutOutNow.load() }, { "onShowNearYou", gOnShowNow.load() },
			{ "particleLightConfigs", gParticleTextures.size() }, { "displays", gDisplays.size() }, { "displaysDark", gDisplaysDark.load() },
			{ "lightsNow", gKept.size() }, { "lightsMade", gMade.load() }, { "looks", gLooks.load() }, { "walks", gWalks.load() },
			{ "tracked", gTrackedNow.load() }, { "noModelYet", gNoModelYet.load() }, { "lights", lights } };
		return out.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
	}
}
