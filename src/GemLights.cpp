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
// Soul Gems' copies of them too), Rustic Soul Gems (either download, once its author allows it - GemPermissions.h), or the
// game's own gems by their size; a gem mod waiting on its author's permission stays unlit. Never a static or
// a museum display that only wears a gem's model (his rule: displays never get lit) - with RE::Light loaded, its light on one
// is put out.
//
// HOW (once a frame, after the player's update, on the main thread): every quarter second the gems around the player are
// looked for; the nearest 24 (kMaxLights) each get one light of ours hung on its model, made and registered the way ReLight
// does it (the credit below), in its color, at its strength and reach for the lighting found (Lighting.cpp - Community Shaders' inverse-square words, or
// the same light drawn plain on ENB and Vanilla), breathing softly when the pulse is on. A gem picked up, unloaded or turned off
// loses its light; a game loaded drops them all.
//
// NEVER TWO LIGHTS ON ONE GEM, OURS ON EVERY GEM: another light on a gem we light is put out while ours is on it (OtherLights,
// looked at every scan) - a Light Placer file's light (Emissive Soul Gems', Subtle's, CS Light's ...), RE::Light's, or a light
// inside the gem's own model (the ENB particle lights of Praedy's, Subtle's, Skyking's ... meshes) wherever it shines: on ENB,
// and on Community Shaders when a particle-light config (Data\ParticleLights\<texture>.ini) names it. The menu's Their Own
// Lights gives a kind back (TheirsKept); this mod's own RE::Light files light their gems themselves (Gems::OwnerOf).
//
// CREDIT: a light is made and registered the way ReLight by Truman does it (github.com/TrumanGIT/ReLight, GPL-3.0-or-later,
// used with his permission, as Illuminated uses it): a master NiPointLight cloned for every use and handed to the shadow scene
// node.

#include "Plugin.h"

#include <DirectXTex.h>
#include <nlohmann/json.hpp>

namespace Plugin
{
	namespace
	{
		constexpr const char* kName = "SoulGemLight";
		constexpr float       kAmbient = 0.1f;               // a plain light's ambient: a tenth of its color (RE::Light's rule)
		constexpr float       kScanEvery = 0.25f;            // seconds between looks around the player
		constexpr std::size_t kMaxLights = 24;               // the most gem lights at once - the nearest gems
		constexpr std::size_t kMostLooked = 4 * kMaxLights;  // the nearest gems a scan looks at closely (on show? another light on it?)
		constexpr auto        kRetryAfter = std::chrono::seconds(15);
		constexpr float       kRange = 4096.0f;                               // how far from the player a gem is lit (its light reaches about 100-160 units)
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
			Gems::Look  look;
			bool        needsSoul{ false };              // an empty gem record: lit only when the placed gem holds a soul
			Gems::Owner owner{ Gems::Owner::kThisMod };  // the light mod that would light it (the menu may leave it to that one)
		};
		std::unordered_map<const RE::TESBoundObject*, Gem> gGems;             // the gems this plugin lights; read on the main thread after data load
		std::vector<std::pair<std::string, std::size_t>>   gFamilies;         // the lit gem records by look, for the menu
		std::array<std::size_t, 4>                         gOtherLighters{};  // gem records another light mod lights too, by Gems::Owner
		std::vector<std::pair<std::string, std::size_t>>   gBlocked;          // gem records left unlit by their mod, until its author allows it
		std::string                                        gRustic;           // which Rustic Soul Gems download is loaded
		bool                                               gRusticLit{ false };

		// What Light Placer's configs claim (models by their whole path as NormalPath writes it, formIDs tokens in lower case), read
		// once when Light Placer is loaded: whose light a gem would wear (put out while ours is on it, unless the menu keeps that
		// kind). Emissive Soul Gems' own file's models apart, so its switch is offered only when it is installed
		struct Claims
		{
			std::unordered_set<std::string> models, forms, emissive;
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
							into->insert(std::move(name));
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
		[[nodiscard]] Gems::Claim Claimed(const Claims& a_claims, const RE::TESForm* a_form, std::string_view a_model)
		{
			const auto path = Gems::NormalPath(a_model);
			if (a_claims.emissive.contains(path)) {
				return Gems::Claim::kEmissive;
			}
			if (a_claims.models.contains(path)) {
				return Gems::Claim::kOther;
			}
			if (const char* id = EditorIdOf(a_form); id && *id && a_claims.forms.contains(Lower(id))) {
				return Gems::Claim::kOther;
			}
			const auto* file = a_form->GetFile(0);
			if (!file) {
				return Gems::Claim::kNone;
			}
			const auto plugin = Lower(file->GetFilename());
			const auto local = a_form->GetLocalFormID();
			const bool byId = std::ranges::any_of(a_claims.forms, [&](const std::string& a_token) { return Gems::TokenNamesForm(a_token, plugin, local); });
			return byId ? Gems::Claim::kOther : Gems::Claim::kNone;
		}

		// ---------------------------------------------------------- a gem model's own color, read once per model at data load
		// a file the game can open (loose or in an archive) under a_folder ("textures", "meshes"), handed to a_take piece by
		// piece; false when it cannot be opened
		template <class F>
		bool ReadGameFile(std::string_view a_path, std::string_view a_folder, F&& a_take)
		{
			// the same normal form the permission lookups use, so "Data\Textures\x.dds" or "\Meshes\x.nif" open too
			const auto                   path = std::string(a_folder) + "\\" + (a_folder == "meshes" ? Gems::NormalPath(a_path) : Gems::TexturePath(a_path));
			RE::BSResourceNiBinaryStream stream(path);
			const auto*                  source = stream.good() ? stream.stream.get() : nullptr;
			if (!source) {
				return false;
			}
			std::vector<std::uint8_t> chunk(65536);
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
			ReadGameFile(a_path, "textures", [&](std::span<const std::uint8_t> a_piece) { bytes.insert(bytes.end(), a_piece.begin(), a_piece.end()); });
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

		// the color a gem model shows: the diffuse textures of its lit shapes that glow (the gem itself - an inner shape that
		// does not glow, or an effect-shader glow shell, is left out; every lit shape when none glows), a normal map in a color
		// slot never counted. Read through the game's own model loader, once per model; none on VR (the loader's address is SE /
		// AE only) or when nothing in it carries a color - the caller keeps the table's color then
		std::optional<Gems::Rgb> ModelColor(std::string_view a_model)
		{
			static std::unordered_map<std::string, std::optional<Gems::Rgb>> read;
			if (REL::Module::IsVR() || a_model.empty()) {
				return std::nullopt;
			}
			const auto [it, added] = read.try_emplace(Gems::NormalPath(a_model));
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
			RE::BSVisit::TraverseScenegraphGeometries(root.get(), [&](RE::BSGeometry* a_geometry) {
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
				return RE::BSVisit::BSVisitControl::kContinue;
			});
			Gems::ColorSum glowing, all;
			for (const auto& [name, tex] : textures) {
				if (tex.read) {
					all.Add(tex.sum);
					if (tex.glows) {
						glowing.Add(tex.sum);
					}
				}
			}
			const auto glow = glowing.Result();
			it->second = glow ? glow : all.Result();
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

		// the fingerprints of a model's color textures (the winning files, in the order it names them), then of the model itself;
		// once per model
		const std::vector<std::uint64_t>& PrintsOf(std::string_view a_model)
		{
			static std::unordered_map<std::string, std::vector<std::uint64_t>> read;
			const auto [it, added] = read.try_emplace(Gems::NormalPath(a_model));
			if (!added) {
				return it->second;
			}
			std::vector<std::uint8_t> nif;
			if (!ReadGameFile(a_model, "meshes", [&](std::span<const std::uint8_t> a_piece) { nif.insert(nif.end(), a_piece.begin(), a_piece.end()); }) || nif.empty()) {
				return it->second;
			}
			for (const auto& texture : Gems::TexturesNamed(nif)) {
				if (const auto print = Gems::ColorTexture(texture) ? TexturePrint(texture) : std::nullopt) {
					it->second.push_back(*print);
				}
			}
			it->second.push_back(Gems::Fingerprint(nif));
			return it->second;
		}

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

		// a gem this plugin lights is kept. One a Light Placer file lights is counted and lit by us all the same, so our sliders rule
		// it (the other light is put out at the scan); one this mod's own RE::Light files light is left to RE::Light (Gems::OwnerOf)
		bool Take(const RE::TESBoundObject* a_form, std::string_view a_model, const Gem& a_gem, const Others& a_others)
		{
			if (const auto mod = BlockedBy(a_model)) {
				CountName(gBlocked, *mod);
				return false;
			}
			const auto claim = a_others.lightPlacer ? Claimed(a_others.claims, a_form, a_model) : Gems::Claim::kNone;
			const auto owner = Gems::OwnerOf(a_model, a_gem.look.family, claim, a_others.reLight);
			if (owner != Gems::Owner::kThisMod) {
				++gOtherLighters[static_cast<std::size_t>(owner)];
			}
			if (owner == Gems::Owner::kReLight) {
				return false;
			}
			auto gem = a_gem;
			gem.owner = owner;
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
				if (const auto seen = SeenColor(path)) {  // the color its lit-room picture shows
					look->rgb = *seen;
				}
				SetGlow(*look, GlowOf(path));
				added += Take(bound, path, Gem{ *look, false, Gems::Owner::kThisMod }, a_others) ? 1 : 0;
			}
			return added;
		}

		// displays (statics wearing a gem's or a Soul Cairn piece's model, or a copy RE::Light's keys reach) that RE::Light lights:
		// with RE::Light loaded their light is put out, since displays never get lit
		std::unordered_set<const RE::TESBoundObject*> gDisplays;
		std::atomic<std::size_t>                      gDisplaysDark{ 0 };
		constexpr const char*                         kReLightName = "RL";  // the name RE::Light gives every light it hangs
		constexpr float                               kDisplayEvery = 1.0f;
		float                                         gSinceDisplays = kDisplayEvery;

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

		// every loaded display, whatever the lights switch says or how far she is (RE::Light lights every loaded reference). Each
		// model is walked every time: a remembered model pointer can be a new model at a freed one's address after a cell reload
		void DarkenDisplays()
		{
			auto* tes = RE::TES::GetSingleton();
			if (gDisplays.empty() || !tes) {
				return;
			}
			std::size_t dark = 0;
			tes->ForEachReference([&](RE::TESObjectREFR* a_ref) {
				const auto* base = a_ref ? a_ref->GetBaseObject() : nullptr;
				auto*       shown = base && gDisplays.contains(base) && a_ref->Is3DLoaded() ? a_ref->Get3D() : nullptr;
				if (shown && DarkenDisplay(shown)) {
					++dark;
				}
				return RE::BSContainer::ForEachResult::kContinue;
			});
			gDisplaysDark = dark;
		}

		// ---------------------------------------------------------- the lights, main thread only
		struct Kept
		{
			RE::NiPointer<RE::NiNode>       root;  // the gem's model
			RE::NiPointer<RE::NiPointLight> light;
			RE::NiPointer<RE::BSLight>      bs;
			const Gem*                      gem{ nullptr };
			float                           phase{ 0.0f };         // where its pulse starts, so gems side by side do not breathe as one
			float                           wroteRadius{ -1.0f };  // Community Shaders rewrites the light's radius every frame
			bool                            seen{ false };
			// the other lights on this gem, put out while ours is on it and given back as they were when ours goes (his "Ours on
			// every gem"); each with whether it was already out
			std::vector<std::pair<RE::NiPointer<RE::NiAVObject>, bool>> putOut;
		};
		std::unordered_map<RE::FormID, Kept> gKept;  // by the placed gem's form id
		// a gem whose light could not be made or registered is tried again only after kRetryAfter, never four times a second -
		// by the placed gem AND its model, so a new gem that gets the same form id or a model at the same address (both are
		// reused) is never held back by another gem's failure
		struct Retry
		{
			const RE::NiNode*                     root{ nullptr };
			std::chrono::steady_clock::time_point at;
		};
		std::unordered_map<RE::FormID, Retry> gRetryAt;
		std::unordered_set<std::string>       gParticleTextures;  // the textures Data\ParticleLights\*.ini name (lower case, no extension)
		std::atomic<std::size_t>              gCount{ 0 }, gMade{ 0 }, gPutOutNow{ 0 }, gWithGlow{ 0 }, gWithGlowKept{ 0 };
		float                                 gClock = 0.0f, gSinceScan = kScanEvery;

		void Drop(Kept& a_kept, RE::ShadowSceneNode* a_scene)
		{
			for (const auto& [other, wasOut] : a_kept.putOut) {
				other->SetAppCulled(wasOut);
			}
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

		// this frame's numbers onto the light
		void Follow(Kept& a_kept, const Gems::Settings& a_s)
		{
			const auto& look = a_kept.gem->look;
			const float pulse = a_s.pulse ? Gems::PulseAt(gClock + a_kept.phase) : 1.0f;
			const auto  shown = Gems::ShownAt(look.kind, InverseSquare(), LightingFound() == Lighting::kVanilla, a_s, pulse);
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
			} else {
				data.ambient = { data.diffuse.red * kAmbient, data.diffuse.green * kAmbient, data.diffuse.blue * kAmbient };
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
				a_root->DetachChild(light);
				a_kept.light.reset();
				a_kept.root.reset();
				return false;
			}
			a_kept.bs.reset(bs);
			++gMade;
			return true;
		}

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

		// every other light on this placed gem: one another mod hung under it (Light Placer's, RE::Light's) and a shape whose texture
		// makes a light in this game - on ENB an ENB light texture (fxglowENB, glowsoft01_enbl ...; a shape only NAMED
		// "EnbParticleLight..." over a plain texture lights nothing there), on Community Shaders a texture a particle-light config
		// names. Looked at every scan, so a light another mod adds later is found the same quarter second
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

		// a gem on show is never lit (displays never get lit): a placed gem wearing a display script - Legacy of the Dragonborn's
		// museum shows the real Azura's Star and Black Star through dbm_dynamicdisplayscript, enabled once donated. A gem left
		// lying about carries no such script (the College's carry defaultDisableHavokOnLoad). The script table's place is SE / AE's
		std::atomic<std::size_t> gOnShowNow{ 0 };

		[[nodiscard]] bool OnShow(const RE::TESObjectREFR* a_ref)
		{
			auto* vm = REL::Module::IsVR() ? nullptr : RE::BSScript::Internal::VirtualMachine::GetSingleton();
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

		// ours on the gem (made once, again when its model was rebuilt), then its other lights put out; false when ours could not
		// be made - the gem is let go below, its other lights left as they were
		[[nodiscard]] bool Keep(RE::FormID a_id, RE::NiNode* a_root, const Gem& a_gem, RE::ShadowSceneNode* a_scene, const Gems::Settings& a_s,
			std::vector<RE::NiPointer<RE::NiAVObject>>&& a_others)
		{
			auto& kept = gKept[a_id];
			if (kept.light && (kept.root.get() != a_root || kept.light->parent != a_root)) {
				Drop(kept, a_scene);  // the gem's model was rebuilt: a new one
			}
			kept.gem = &a_gem;
			if (!kept.light) {
				const auto now = std::chrono::steady_clock::now();
				if (const auto retry = gRetryAt.find(a_id); retry != gRetryAt.end() && retry->second.root == a_root && now < retry->second.at) {
					return false;
				}
				kept.phase = static_cast<float>(a_id % 997u) / 997.0f * Gems::kPulseSeconds;
				if (!Make(kept, a_root, a_scene, a_s)) {
					if (gRetryAt.size() > 256) {
						gRetryAt.clear();  // gems long gone
					}
					gRetryAt[a_id] = { a_root, now + kRetryAfter };
					static bool warned = false;
					if (!std::exchange(warned, true)) {
						SKSE::log::warn("a soul gem light could not be made or registered; such a gem is tried again every {} s", kRetryAfter.count());
					}
					return false;
				}
				gRetryAt.erase(a_id);
			}
			// each other light is recorded before it is put out (room made first), so none is ever left out unrecorded
			std::vector<std::pair<RE::NiPointer<RE::NiAVObject>, bool>> putOut;
			putOut.reserve(a_others.size());
			for (auto& other : a_others) {
				const auto had = std::ranges::find(kept.putOut, other, &std::pair<RE::NiPointer<RE::NiAVObject>, bool>::first);
				const bool wasOut = had != kept.putOut.end() ? had->second : other->GetAppCulled();
				putOut.emplace_back(std::move(other), wasOut);
				putOut.back().first->SetAppCulled(true);
			}
			for (const auto& [old, wasOut] : kept.putOut) {
				if (std::ranges::find(putOut, old, &std::pair<RE::NiPointer<RE::NiAVObject>, bool>::first) == putOut.end()) {
					old->SetAppCulled(wasOut);  // gone from the gem, or another mod's light that left it
				}
			}
			kept.putOut = std::move(putOut);
			kept.seen = true;
			return true;
		}

		void Scan(RE::ShadowSceneNode* a_scene, const Gems::Settings& a_s)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* tes = RE::TES::GetSingleton();
			if (!player || !tes || !player->GetParentCell()) {
				return;
			}
			for (auto& [id, kept] : gKept) {
				kept.seen = false;
			}
			const auto lighting = LightingFound();
			// a gem's own model light shines on an ENB (found running), or on Community Shaders when a particle-light config names it
			const bool  ownLights = lighting == Lighting::kEnb || (lighting == Lighting::kShaders && !gParticleTextures.empty());
			std::size_t onShow = 0;
			struct Candidate
			{
				RE::TESObjectREFR* ref{ nullptr };  // read this frame only, on the main thread
				RE::FormID         id{ 0 };
				RE::NiNode*        root{ nullptr };
				const Gem*         gem{ nullptr };
				float              distance{ 0.0f };  // squared, to the player
			};
			std::vector<Candidate> candidates;
			const auto             here = player->GetPosition();
			tes->ForEachReferenceInRange(player, kRange, [&](RE::TESObjectREFR* a_ref) {
				const auto* base = a_ref ? a_ref->GetBaseObject() : nullptr;
				const auto  it = base ? gGems.find(base) : gGems.end();
				if (it == gGems.end() || a_ref->IsDisabled() || a_ref->IsDeleted() || !a_ref->Is3DLoaded()) {
					return RE::BSContainer::ForEachResult::kContinue;
				}
				if (it->second.needsSoul && a_ref->extraList.GetSoulLevel() == RE::SOUL_LEVEL::kNone) {
					return RE::BSContainer::ForEachResult::kContinue;
				}
				auto*      model = a_ref->Get3D();
				auto*      root = model ? model->AsNode() : nullptr;
				const auto id = a_ref->GetFormID();
				if (!root) {
					return RE::BSContainer::ForEachResult::kContinue;
				}
				// a gem that holds our light already counts as a fifth nearer, so two gems at the kMaxLights edge do not swap lights
				// four times a second as she walks
				const auto held = gKept.find(id);
				const bool lit = held != gKept.end() && held->second.light && held->second.root.get() == root;
				candidates.push_back({ a_ref, id, root, &it->second, a_ref->GetPosition().GetSquaredDistance(here) * (lit ? 0.64f : 1.0f) });
				return RE::BSContainer::ForEachResult::kContinue;
			});
			// the nearest kMaxLights gems keep a light: a shop, a stash or the Soul Cairn would otherwise hang one on every gem in reach;
			// a farther gem is let go below and takes a light again when it is among the nearest
			std::ranges::sort(candidates, {}, &Candidate::distance);
			// a gem whose other light the menu keeps ("Use their light") gets none of ours: not seen, so a light of ours on it is let
			// go below and the lights it put out come back
			struct Lit
			{
				const Candidate*                           c{ nullptr };
				std::vector<RE::NiPointer<RE::NiAVObject>> others;
			};
			std::vector<Lit> lit;
			std::size_t      looked = 0;
			for (const auto& c : candidates) {
				if (lit.size() == kMaxLights || ++looked > kMostLooked) {
					break;
				}
				// asked only of the nearest kMostLooked gems, outside the reference walk: it takes the Papyrus VM's lock
				if (OnShow(c.ref)) {
					++onShow;
					continue;
				}
				auto       others = OtherLights(c.root, lighting, ownLights);
				const bool placedLight = std::ranges::any_of(others, [](const auto& a_other) { return netimmerse_cast<RE::NiLight*>(a_other.get()) != nullptr; });
				const bool meshLight = std::ranges::any_of(others, [](const auto& a_other) { return !netimmerse_cast<RE::NiLight*>(a_other.get()); });
				if (!Gems::TheirsKept(c.gem->owner, placedLight, meshLight, a_s)) {
					lit.push_back({ &c, std::move(others) });
				}
			}
			gOnShowNow = onShow;
			std::size_t putOut = 0;
			for (auto& l : lit) {
				const bool others = !l.others.empty();
				putOut += Keep(l.c->id, l.c->root, *l.c->gem, a_scene, a_s, std::move(l.others)) && others ? 1 : 0;
			}
			gPutOutNow = putOut;
			std::erase_if(gKept, [&](auto& a_entry) {
				if (!a_entry.second.seen) {
					Drop(a_entry.second, a_scene);
					return true;
				}
				return false;
			});
		}

		void Frame(float a_delta)
		{
			auto*      scene = RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0];
			const auto s = SettingsNow();
			if (!std::isfinite(a_delta) || a_delta < 0.0f) {
				a_delta = 0.0f;  // one NaN would stop the scan and the pulse until the next load
			}
			gSinceDisplays += a_delta;
			if (gSinceDisplays >= kDisplayEvery) {
				gSinceDisplays = 0.0f;
				DarkenDisplays();
			}
			if (!scene) {
				return;  // no shadow scene to take our lights out of: they stay as they are until there is one
			}
			if (!s.on) {
				DropAll(scene);
				gPutOutNow = 0;
				return;
			}
			gClock = std::fmod(gClock + a_delta, 3600.0f);
			gSinceScan += a_delta;
			if (gSinceScan >= kScanEvery) {
				gSinceScan = 0.0f;
				Scan(scene, s);
			}
			std::erase_if(gKept, [&](auto& a_entry) {
				auto& kept = a_entry.second;
				if (!kept.light || !kept.root || !kept.root->parent) {  // picked up or unloaded since the last look
					Drop(kept, scene);
					return true;
				}
				for (const auto& [other, wasOut] : kept.putOut) {
					if (netimmerse_cast<RE::NiLight*>(other.get())) {
						other->SetAppCulled(true);  // a Light Placer light's own conditions may show it again
					}
				}
				Follow(kept, s);
				return false;
			});
			gCount = gKept.size();
		}

		// Actor::Update on the player: our work once a frame, on the main thread, after the game's own (Illuminated's hook)
		struct PlayerUpdate
		{
			static void thunk(RE::PlayerCharacter* a_this, float a_delta)
			{
				func(a_this, a_delta);
				Guarded("player update", [a_delta]() { Frame(a_delta); });
			}
			static inline REL::Relocation<decltype(thunk)> func;
		};

		// Quit to Main Menu: the world our lights hang in is gone and the player's update stops until a game loads, so they are
		// taken out when the Main Menu opens (on the main thread; a game loading drops them too)
		class MainMenuWatch final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
			{
				if (a_event && a_event->opening && std::string_view(a_event->menuName.c_str()) == RE::MainMenu::MENU_NAME) {
					static_cast<void>(Later("main menu", []() { DropGemLights(); }));
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
			if (!a_star && (!a_named || a_look.family == Gems::Family::kNamed)) {
				SetGlow(a_look, GlowOf(a_model));
			}
			if (const auto seen = a_star ? std::nullopt : SeenColor(a_model, a_named)) {
				a_look.rgb = *seen;
				++fromPicture;
			} else if (!Gems::ColorFromModel(a_look.family)) {
				return;
			} else if (const auto own = ModelColor(a_model)) {
				a_look.rgb = Gems::ShownColor(*own, a_look.kind);
				++fromModel;
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
			} else if (!look || look->family == Gems::Family::kSoulCairn) {
				look = Gems::GameLook(KindOfCapacity(gem->GetMaximumCapacity()), gem->CanHoldNPCSoul());
			}
			// a star's picture rows were keyed by the other star's model, so Azura's Star reads its own textures; the Black Star
			// keeps its purple
			if (!black) {
				readColor(*look, model, star, namedId(gem));
			}
			look->ownColor = namedId(gem) != 0;  // on a plain model too (a kGame look): it keeps its own color under the pick
			// a gem in textures of its own (its record swaps them onto the model): their color, never the model's
			if (const auto swapped = star ? std::vector<std::string>{} : SwappedDiffuses(gem); !swapped.empty()) {
				if (const auto mod = BlockedSwap(swapped)) {
					CountName(gBlocked, *mod);
					continue;
				}
				if (const auto own = SwapColor(swapped, look->kind)) {
					look->rgb = *own;
					look->ownColor = true;
					SetGlow(*look, SwapGlow(swapped));
					++fromSwap;
				}
			}
			const bool always = star || holds || look->family == Gems::Family::kNamed;
			const bool taken = Take(gem, model, Gem{ *look, !always, Gems::Owner::kThisMod }, others);
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
			look.ownColor = true;
			pieces += Take(bound, path, Gem{ look, false, Gems::Owner::kThisMod }, others) ? 1 : 0;
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
			"model; Rustic Soul Gems {}; taken over from Emissive Soul Gems {}, from other Light Placer files {}, left to "
			"RE::Light {} (Light Placer {}, RE::Light {}); {} particle-light config(s) - a gem's own model light is put out under ours",
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
		gWithGlowKept = static_cast<std::size_t>(std::ranges::count_if(gGems, [&](const auto& a_entry) {
			return ownGlow(a_entry.second) && !Gems::PickRecolors(a_entry.second.look);
		}));
		gRusticLit = std::ranges::any_of(gGems, [](const auto& a_entry) { return a_entry.second.look.family == Gems::Family::kRustic; });
		for (const auto& [mod, n] : gBlocked) {
			SKSE::log::info("gems: {} record(s) wear {}'s files - not lit until its author allows it", n, mod);
		}
		for (const auto& [form, gem] : gGems) {
			if (const auto* soulGem = form->As<RE::TESModel>(); soulGem && form->Is(RE::FormType::SoulGem) && Gems::ColorFromModel(gem.look.family)) {
				const char* model = soulGem->GetModel();
				SKSE::log::info("  {:08X} {} | {} | {},{},{}", form->GetFormID(), form->GetName(), model ? model : "", gem.look.rgb[0], gem.look.rgb[1], gem.look.rgb[2]);
			}
		}
	}

	void InstallFrame()
	{
		REL::Relocation<std::uintptr_t> vtbl{ RE::PlayerCharacter::VTABLE[0] };
		PlayerUpdate::func = vtbl.write_vfunc(REL::Relocate(0xAD, 0xAD, 0xAF), PlayerUpdate::thunk);  // Actor::Update
		static MainMenuWatch mainMenu;
		if (auto* ui = RE::UI::GetSingleton()) {
			ui->AddEventSink<RE::MenuOpenCloseEvent>(&mainMenu);
		}
		SKSE::log::info("lights follow the player's update (Actor::Update)");
	}

	void DropGemLights()
	{
		DropAll(RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0]);
		gRetryAt.clear();
		gSinceScan = kScanEvery;  // look around at once in the next game
		gSinceDisplays = kDisplayEvery;
	}

	void ApplyNow()
	{
		// one look around now, not at the next frame: while the menu pauses the game no frame comes, and a light switched off
		// would stay lit until it runs again (the frame also brings every light kept up to the settings)
		gSinceScan = kScanEvery;
		Frame(0.0f);
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
	std::size_t                                      GemParticleConfigs() { return gParticleTextures.size(); }
	bool                                             LightColorMatters(int a_gemMod) { return Gems::LightColorShown(a_gemMod, gWithGlow.load(), gWithGlowKept.load()); }
	std::string                                      RusticFound() { return gRusticLit ? gRustic : std::string(); }

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
			const auto  rgb = Gems::ColorAt(kept.gem->look, s);  // the color it wears now (the menu's "Gem colors" pick)
			lights.push_back({ { "ref", std::format("0x{:08X}", id) }, { "family", std::string(Gems::FamilyName(kept.gem->look.family)) },
				{ "kind", static_cast<int>(kept.gem->look.kind) }, { "rgb", { rgb[0], rgb[1], rgb[2] } }, { "fade", d.fade }, { "radius", d.radius.x },
				{ "at", { std::round(p.x), std::round(p.y), std::round(p.z) } } });
		}
		nlohmann::json families = nlohmann::json::object(), blocked = nlohmann::json::object();
		for (const auto& [name, n] : gFamilies) {
			families[name] = n;
		}
		for (const auto& [name, n] : gBlocked) {
			blocked[std::string(name)] = n;
		}
		const nlohmann::json out{ { "gemForms", gGems.size() }, { "litByLook", families }, { "waitingOnPermission", blocked }, { "rustic", gRustic },
			{ "takenOver", { { "emissiveSoulGems", gOtherLighters[1] }, { "lightPlacer", gOtherLighters[2] } } },
			{ "leftTo", { { "reLight", gOtherLighters[3] } } }, { "otherLightsPutOutNearYou", gPutOutNow.load() }, { "onShowNearYou", gOnShowNow.load() },
			{ "particleLightConfigs", gParticleTextures.size() }, { "displays", gDisplays.size() }, { "displaysDark", gDisplaysDark.load() },
			{ "lightsNow", gKept.size() }, { "lightsMade", gMade.load() }, { "lights", lights } };
		return out.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
	}
}
