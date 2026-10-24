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
// Soul Gems' copies of them too), Rustic Soul Gems (either download), or the game's own gems by their size. Never a static or
// a museum display that only wears a gem's model (his rule: displays never get lit) - with RE::Light loaded, its light on one
// is put out.
//
// HOW (once a frame, after the player's update, on the main thread): every quarter second the gems around the player are
// looked for; each gets one light of ours hung on its model, made and registered the way ReLight does it (the credit below),
// in its color, at its strength and reach for the lighting found (Lighting.cpp - Community Shaders' inverse-square words, or
// the same light drawn plain on ENB and Vanilla), breathing softly when the pulse is on. A gem picked up, unloaded or turned off
// loses its light; a game loaded drops them all.
//
// NOTHING ANOTHER MOD ALREADY DOES: a gem another light mod lights is left to it - a Light Placer file's
// gem (Emissive Soul Gems', CS Light's, the CS Lights add-ons'), a gem whose model already holds a light that is not ours (Light
// Placer's, RE::Light's, one inside the mesh - looked at every scan), and a gem whose own model carries a light (the ENB particle
// lights of Praedy's, Subtle's, Skyking's, Kanjs' ... meshes) wherever that light shines: always on ENB, and on Community
// Shaders when a particle-light config (Data\ParticleLights\<texture>.ini - Bottled Shaders ships one for fxglowENB) names it.
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
		constexpr float       kAmbient = 0.1f;     // a plain light's ambient: a tenth of its color (RE::Light's rule)
		constexpr float       kScanEvery = 0.25f;  // seconds between looks around the player
		constexpr std::size_t kMaxLights = 24;     // the most gem lights at once - the nearest gems
		constexpr auto        kRetryAfter = std::chrono::seconds(15);
		constexpr float       kRange = 4096.0f;                               // how far from the player a gem is lit (its light reaches under 100 units)
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

		// a light already attached and updated once, registered as ReLight registers its own: dynamic, no shadow, kept inside its
		// cell (portal strict), lighting land and water too
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
			Gems::Look look;
			bool       needsSoul{ false };  // an empty gem record: lit only when the placed gem holds a soul
		};
		std::unordered_map<const RE::TESBoundObject*, Gem> gGems;      // the gems this plugin lights; read on the main thread after data load
		std::vector<std::pair<std::string, std::size_t>>   gFamilies;  // the lit gem records by look, for the menu
		std::array<std::size_t, 4>                         gLeftTo{};  // gem records left to another light mod, by Gems::Owner
		std::vector<std::pair<std::string, std::size_t>>   gBlocked;   // gem records left unlit by their mod, until its author allows it
		std::string                                        gRustic;    // which Rustic Soul Gems download is loaded

		// What Light Placer's configs claim (models by their file name, formIDs tokens in lower case), read once when Light Placer
		// is loaded: a gem another config already lights is left to it, so no gem ever wears two lights (Emissive Soul Gems', CS
		// Light's, Subtle Soul Gems' ...)
		struct Claims
		{
			std::unordered_set<std::string> models, forms;
		};

		// a file's extension in lower case, as wide text: path::string() throws on a name the ANSI code page cannot show
		[[nodiscard]] std::wstring LowerExtension(const std::filesystem::path& a_path)
		{
			auto ext = a_path.extension().wstring();
			std::ranges::transform(ext, ext.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
			return ext;
		}

		[[nodiscard]] Claims ReadLightPlacerClaims()
		{
			Claims          out;
			std::error_code ec;
			const auto      root = std::filesystem::current_path() / "Data" / "LightPlacer";
			std::size_t     files = 0;
			for (std::filesystem::recursive_directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
				if (!it->is_regular_file(ec) || LowerExtension(it->path()) != L".json") {
					continue;
				}
				std::ifstream in(it->path(), std::ios::binary);
				const auto    doc = nlohmann::json::parse(in, nullptr, false, true);  // no exceptions; a comment does not drop the file
				++files;
				if (!doc.is_array()) {
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
						for (const auto& v : *found) {
							if (v.is_string()) {
								into->insert(key == std::string_view("models") ? Gems::NormalPath(v.get<std::string>()) : Lower(v.get<std::string>()));
							}
						}
					}
				}
			}
			SKSE::log::info("Light Placer is loaded: {} config file(s) read, {} model(s) and {} form(s) claimed", files, out.models.size(), out.forms.size());
			return out;
		}

		// does a formIDs token name this form ("SoulGemGrandFilled", "0x2E4FF~Skyrim.esm", "0002E4FF~skyrim.esm" ...)
		[[nodiscard]] bool Claimed(const Claims& a_claims, const RE::TESForm* a_form, std::string_view a_model)
		{
			if (a_claims.models.contains(Gems::NormalPath(a_model))) {
				return true;
			}
			if (const char* id = a_form->GetFormEditorID(); id && *id && a_claims.forms.contains(Lower(id))) {
				return true;
			}
			const auto* file = a_form->GetFile(0);
			if (!file) {
				return false;
			}
			const auto plugin = Lower(file->GetFilename());
			const auto local = a_form->GetLocalFormID();
			return std::ranges::any_of(a_claims.forms, [&](const std::string& a_token) { return Gems::TokenNamesForm(a_token, plugin, local); });
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

		// the gem mod a file of the game's belongs to, while its author has not allowed this mod to light it: by its name, else
		// by its bytes (streamed - only the first 64 KB are hashed, a 4K texture is never held whole)
		std::optional<std::string_view> BlockedTexture(std::string_view a_path)
		{
			if (const auto byName = Gems::BlockedModOfPath(Gems::TexturePath(a_path))) {
				return byName;
			}
			Gems::FingerprintSum sum;
			if (!ReadGameFile(a_path, "textures", [&](std::span<const std::uint8_t> a_piece) { sum.Add(a_piece); }) || !sum.Size()) {
				return std::nullopt;
			}
			return Gems::BlockedModOf(sum.Result());
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
			// already 8-bit RGBA: read as it is (DirectXTex refuses a conversion to the same format)
			DirectX::ScratchImage rgba;
			const DirectX::Image* pixels = image;
			if (image->format != DXGI_FORMAT_R8G8B8A8_UNORM) {
				const auto result = DirectX::IsCompressed(image->format) ?
				                        DirectX::Decompress(*image, DXGI_FORMAT_R8G8B8A8_UNORM, rgba) :
				                        DirectX::Convert(*image, DXGI_FORMAT_R8G8B8A8_UNORM, DirectX::TEX_FILTER_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, rgba);
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
			it->second = glowing.Result() ? glowing.Result() : all.Result();
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

		// the color a gem's picture shows, when one of its color textures (the winning files) or its model is a file a close
		// picture of that gem has read (GemSeenColors.h); once per model
		std::optional<Gems::Rgb> SeenColor(std::string_view a_model)
		{
			static std::unordered_map<std::string, std::optional<Gems::Rgb>> read;
			if (a_model.empty()) {
				return std::nullopt;
			}
			const auto [it, added] = read.try_emplace(Gems::NormalPath(a_model));
			if (!added) {
				return it->second;
			}
			std::vector<std::uint8_t> nif;
			if (!ReadGameFile(a_model, "meshes", [&](std::span<const std::uint8_t> a_piece) { nif.insert(nif.end(), a_piece.begin(), a_piece.end()); }) || nif.empty()) {
				return it->second;
			}
			for (const auto& texture : Gems::TexturesNamed(nif)) {
				Gems::FingerprintSum sum;
				if (Gems::ColorTexture(texture) && ReadGameFile(texture, "textures", [&](std::span<const std::uint8_t> a_piece) { sum.Add(a_piece); }) && sum.Size()) {
					if ((it->second = Gems::SeenColorOf(sum.Result()))) {
						return it->second;
					}
				}
			}
			it->second = Gems::SeenColorOf(Gems::Fingerprint(nif));
			return it->second;
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

		// a gem this plugin lights is kept; one another light mod lights is counted and left to it (Gems::OwnerOf)
		bool Take(const RE::TESBoundObject* a_form, std::string_view a_model, const Gem& a_gem, const Others& a_others)
		{
			if (const auto mod = BlockedBy(a_model)) {
				CountName(gBlocked, *mod);
				return false;
			}
			const bool claimed = a_others.lightPlacer && Claimed(a_others.claims, a_form, a_model);
			const auto owner = Gems::OwnerOf(a_model, a_gem.look.family, a_others.lightPlacer, claimed, a_others.reLight);
			if (owner != Gems::Owner::kThisMod) {
				++gLeftTo[static_cast<std::size_t>(owner)];
				return false;
			}
			gGems.insert_or_assign(a_form, a_gem);
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
				const auto  look = path && *path ? Gems::LookOfModel(path, false) : std::nullopt;
				if (!look || (look->family != Gems::Family::kSoulCairn && !(a_items && look->family == Gems::Family::kNamed))) {
					continue;
				}
				added += Take(bound, path, Gem{ *look, false }, a_others) ? 1 : 0;
			}
			return added;
		}

		// displays (statics wearing a gem's or a Soul Cairn piece's model, or a copy RE::Light's keys reach) that RE::Light lights:
		// his answer to checklist 138, "make our mod turn off displays" - with RE::Light loaded their light is put out (displays never
		// get lit)
		std::unordered_set<const RE::TESBoundObject*> gDisplays;
		std::atomic<std::size_t>                      gDisplaysDark{ 0 };
		constexpr const char*                         kReLightName = "RL";  // the name RE::Light gives every light it hangs
		constexpr float                               kDisplayEvery = 1.0f;
		// the displays whose RE::Light light is out, by the model it was put out on - a model rebuilt is looked at again
		std::unordered_map<RE::FormID, const RE::NiAVObject*> gDarkened;
		float                                                 gSinceDisplays = kDisplayEvery;

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

		// every loaded display, whatever the lights switch says or how far she is (RE::Light lights every loaded reference)
		void DarkenDisplays()
		{
			auto* tes = RE::TES::GetSingleton();
			if (gDisplays.empty() || !tes) {
				return;
			}
			if (gDarkened.size() > 4096) {
				gDarkened.clear();  // references long unloaded
			}
			std::size_t dark = 0;
			tes->ForEachReference([&](RE::TESObjectREFR* a_ref) {
				const auto* base = a_ref ? a_ref->GetBaseObject() : nullptr;
				auto*       shown = base && gDisplays.contains(base) && a_ref->Is3DLoaded() ? a_ref->Get3D() : nullptr;
				if (!shown) {
					return RE::BSContainer::ForEachResult::kContinue;
				}
				auto& done = gDarkened[a_ref->GetFormID()];
				if (done == shown || DarkenDisplay(shown)) {
					done = shown;
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
			float                           phase{ 0.0f };  // where its pulse starts, so gems side by side do not breathe as one
			bool                            seen{ false };
		};
		std::unordered_map<RE::FormID, Kept> gKept;  // by the placed gem's form id
		// a gem's model carries a light that shines here - looked at once per gem RECORD (its model is the record's): never by a placed
		// gem's form id, which the game hands to the next gem placed after one is deleted (a gem would take the verdict of the one
		// placed before it under the same id)
		std::unordered_map<const RE::TESBoundObject*, bool> gOwnLight;
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
		std::atomic<std::size_t>              gCount{ 0 }, gMade{ 0 }, gOwnLightNow{ 0 };
		float                                 gClock = 0.0f, gSinceScan = kScanEvery;

		void Drop(Kept& a_kept, RE::ShadowSceneNode* a_scene)
		{
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

		// the glow each lit shape of a placed gem shows this frame (the game animates it); main thread
		[[nodiscard]] std::span<const Gems::Glow> GlowsOf(RE::NiAVObject* a_root)
		{
			static std::vector<Gems::Glow> glows;
			glows.clear();
			RE::BSVisit::TraverseScenegraphGeometries(a_root, [](RE::BSGeometry* a_geometry) {
				auto* lit = netimmerse_cast<RE::BSLightingShaderProperty*>(a_geometry ? a_geometry->GetGeometryRuntimeData().shaderProperty.get() : nullptr);
				if (lit && lit->emissiveColor) {
					const auto& e = *lit->emissiveColor;
					glows.push_back({ e.red, e.green, e.blue, lit->emissiveMult });
				}
				return RE::BSVisit::BSVisitControl::kContinue;
			});
			return glows;
		}

		// this frame's numbers onto the light
		void Follow(Kept& a_kept, const Gems::Settings& a_s)
		{
			const auto& look = a_kept.gem->look;
			const float pulse = a_s.pulse ? Gems::PulseAt(gClock + a_kept.phase) : 1.0f;
			const auto  shown = Gems::ShownFor(Gems::kStrength[static_cast<std::size_t>(look.kind)], InverseSquare(),
				a_s.brightness / 100.0f, a_s.reach / 100.0f, pulse);
			auto&       data = a_kept.light->GetLightRuntimeData();
			// a gem whose color is its model's texture follows the glow it shows now; the colors settled by eye stay as they are
			const auto rgb = Gems::ColorFromModel(look.family) && !look.seen && a_kept.root ? Gems::LiveColor(look.rgb, GlowsOf(a_kept.root.get())) : look.rgb;
			data.diffuse = { rgb[0] / 255.0f, rgb[1] / 255.0f, rgb[2] / 255.0f };
			data.fade = shown.fade;
			if (data.radius.x != shown.radius) {
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
				a_kept = {};
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
			if (!shader || shader->GetMaterialType() != RE::BSShaderMaterial::Type::kEffect || !shader->material) {
				return {};
			}
			const char* path = static_cast<RE::BSEffectShaderMaterial*>(shader->material)->sourceTexturePath.c_str();
			auto        name = Gems::BareName(path ? path : "");
			if (const auto dot = name.rfind('.'); dot != std::string::npos) {
				name.erase(dot);
			}
			return name;
		}

		// does this placed gem's model already hold a light that is not ours (one Light Placer or RE::Light hung on it, or one inside
		// the mesh): looked at every scan, so a light another mod adds later still wins - ours is let go the same quarter second
		[[nodiscard]] bool HoldsOtherLight(RE::NiAVObject* a_root)
		{
			bool found = false;
			RE::BSVisit::TraverseScenegraphObjects(a_root, [&](RE::NiAVObject* a_object) {
				if (netimmerse_cast<RE::NiLight*>(a_object) && a_object->name != kName) {
					found = true;
					return RE::BSVisit::BSVisitControl::kStop;
				}
				return RE::BSVisit::BSVisitControl::kContinue;
			});
			return found;
		}

		// does this placed gem's model carry a light of its own that shines in this game: on ENB a shape drawing an ENB light texture
		// (fxglowENB, glowsoft01_enbl ...) - a shape only NAMED "EnbParticleLight..." over a plain texture lights nothing there; on
		// Community Shaders a shape whose texture a particle-light config names. The game's own lighting draws neither
		[[nodiscard]] bool CarriesOwnLight(const RE::TESBoundObject* a_base, RE::NiAVObject* a_root, Lighting a_lighting)
		{
			const auto [it, added] = gOwnLight.try_emplace(a_base, false);
			if (added) {
				RE::BSVisit::TraverseScenegraphObjects(a_root, [&](RE::NiAVObject* a_object) {
					const auto texture = EffectTexture(a_object);
					const bool particle = !texture.empty() && gParticleTextures.contains(texture);
					const bool enb = a_lighting == Lighting::kEnb && (particle || std::ranges::find(kEnbLightTextures, texture) != kEnbLightTextures.end());
					if (enb || (a_lighting == Lighting::kShaders && particle)) {
						it->second = true;
						return RE::BSVisit::BSVisitControl::kStop;
					}
					return RE::BSVisit::BSVisitControl::kContinue;
				});
			}
			return it->second;
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

		void Keep(RE::FormID a_id, RE::NiNode* a_root, const Gem& a_gem, RE::ShadowSceneNode* a_scene, const Gems::Settings& a_s)
		{
			auto& kept = gKept[a_id];
			if (kept.light && (kept.root.get() != a_root || kept.light->parent != a_root)) {
				Drop(kept, a_scene);  // the gem's model was rebuilt: a new light
			}
			kept.gem = &a_gem;
			if (!kept.light) {
				const auto now = std::chrono::steady_clock::now();
				if (const auto retry = gRetryAt.find(a_id); retry != gRetryAt.end() && retry->second.root == a_root && now < retry->second.at) {
					return;  // not seen: let go below
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
					return;  // not seen: let go below
				}
				gRetryAt.erase(a_id);
			}
			kept.seen = true;
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
			// an ENB's own particle lights shine only while an ENB runs - settings files left behind light nothing
			const bool  ownLights = (lighting == Lighting::kEnb && EnbRunningFound()) || (lighting == Lighting::kShaders && !gParticleTextures.empty());
			std::size_t leftToOwn = 0, onShow = 0;
			struct Candidate
			{
				RE::FormID  id;
				RE::NiNode* root;
				const Gem*  gem;
				float       distance;  // squared, to the player
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
				if (OnShow(a_ref)) {
					++onShow;
					return RE::BSContainer::ForEachResult::kContinue;
				}
				auto*      model = a_ref->Get3D();
				auto*      root = model ? model->AsNode() : nullptr;
				const auto id = a_ref->GetFormID();
				if (!root) {
					return RE::BSContainer::ForEachResult::kContinue;
				}
				if ((ownLights && CarriesOwnLight(base, root, lighting)) || HoldsOtherLight(root)) {
					++leftToOwn;  // another light shines on it here: left to that light, never a second one
					return RE::BSContainer::ForEachResult::kContinue;
				}
				// a gem that holds our light already counts as a fifth nearer, so two gems at the kMaxLights edge do not swap lights
				// four times a second as she walks
				const auto held = gKept.find(id);
				const bool lit = held != gKept.end() && held->second.light && held->second.root.get() == root;
				candidates.push_back({ id, root, &it->second, a_ref->GetPosition().GetSquaredDistance(here) * (lit ? 0.64f : 1.0f) });
				return RE::BSContainer::ForEachResult::kContinue;
			});
			gOwnLightNow = leftToOwn;
			gOnShowNow = onShow;
			// the nearest kMaxLights gems keep a light: a shop, a stash or the Soul Cairn would otherwise hang one on every gem in reach;
			// a farther gem is let go below and takes a light again when it is among the nearest
			if (candidates.size() > kMaxLights) {
				std::ranges::nth_element(candidates, candidates.begin() + kMaxLights, {}, &Candidate::distance);
				candidates.resize(kMaxLights);
			}
			for (const auto& c : candidates) {
				Keep(c.id, c.root, *c.gem, a_scene, a_s);
			}
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
			gSinceDisplays += (std::max)(a_delta, 0.0f);
			if (gSinceDisplays >= kDisplayEvery) {
				gSinceDisplays = 0.0f;
				DarkenDisplays();
			}
			if (!scene || !s.on) {
				DropAll(scene);
				gOwnLightNow = 0;
				return;
			}
			gClock = std::fmod(gClock + (std::max)(a_delta, 0.0f), 3600.0f);
			gSinceScan += (std::max)(a_delta, 0.0f);
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
	}

	void FindGems()
	{
		gGems.clear();
		gFamilies.clear();
		gLeftTo = {};
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
		others.reLight = REX::W32::GetModuleHandleA("Relight.dll") != nullptr;  // RE::Light (ReLight)
		if (others.lightPlacer) {
			others.claims = ReadLightPlacerClaims();
		}
		const auto* azurasStar = dh->LookupForm(kAzurasStar, "Skyrim.esm");
		const auto* blackStar = dh->LookupForm(kBlackStar, "Skyrim.esm");
		std::size_t records = 0, lit = 0, fromModel = 0, fromPicture = 0;
		for (auto* gem : dh->GetFormArray<RE::TESSoulGem>()) {
			if (!gem) {
				continue;
			}
			++records;
			const bool  star = gem == azurasStar || gem == blackStar;
			const bool  holds = gem->GetContainedSoul() != RE::SOUL_LEVEL::kNone;
			const char* path = gem->GetModel();
			const auto  model = std::string_view(path ? path : "");
			auto        look = model.empty() ? std::nullopt : Gems::LookOfModel(model, !gRustic.empty());
			if (star) {
				look = Gems::Look{ Gems::Family::kStar, Gems::Kind::kStar, gem == azurasStar ? Gems::kAzurasStarRgb : Gems::kBlackStarRgb };
			} else if (!look || look->family == Gems::Family::kSoulCairn) {
				look = Gems::GameLook(KindOfCapacity(gem->GetMaximumCapacity()), gem->CanHoldNPCSoul());
			}
			// a gem left unlit for want of its author's permission never needs its color read
			if (Gems::ColorFromModel(look->family) && !BlockedBy(model)) {
				if (const auto seen = SeenColor(model)) {
					look->rgb = *seen;
					look->seen = true;
					++fromPicture;
				} else if (const auto own = ModelColor(model)) {
					look->rgb = *own;
					++fromModel;
				}
			}
			const bool always = star || holds || look->family == Gems::Family::kNamed;
			if (Take(gem, model, Gem{ *look, !always }, others) && always) {
				++lit;
				CountName(gFamilies, Gems::FamilyName(look->family));
			}
		}
		const std::size_t pieces = AddByModel<RE::TESObjectMISC>(true, others) + AddByModel<RE::IngredientItem>(true, others) +
		                           AddByModel<RE::TESObjectACTI>(false, others) + AddByModel<RE::TESFurniture>(false, others);
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
			"model; Rustic Soul Gems {}; left to Emissive Soul Gems' models {}, to other Light Placer configs {}, to RE::Light {} "
			"(Light Placer {}, RE::Light {}); {} particle-light config(s) - a gem whose own model's light shines here is left to it",
			records, lit, list.empty() ? "none" : list, pieces, gRustic.empty() ? "not loaded" : gRustic,
			gLeftTo[static_cast<std::size_t>(kEmissive)], gLeftTo[static_cast<std::size_t>(kLightPlacer)],
			gLeftTo[static_cast<std::size_t>(kReLight)], others.lightPlacer ? "loaded" : "not loaded", others.reLight ? "loaded" : "not loaded",
			gParticleTextures.size());
		SKSE::log::info("gems: {} soul gem record(s) wear the color their picture shows, {} the color read off their own model", fromPicture, fromModel);
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
		SKSE::log::info("lights follow the player's update (Actor::Update)");
	}

	void DropGemLights()
	{
		DropAll(RE::BSShaderManager::State::GetSingleton().shadowSceneNode[0]);
		gOwnLight.clear();  // looked at again in the next game
		gRetryAt.clear();
		gDarkened.clear();
		gSinceScan = kScanEvery;  // look around at once in the next game
		gSinceDisplays = kDisplayEvery;
	}

	void ApplyNow()
	{
		gSinceScan = kScanEvery;  // the next frame looks around again and draws every light with the settings as they are now
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
	std::size_t GemLightsMade() { return gMade.load(); }

	std::vector<std::pair<std::string, std::size_t>> GemFamilies() { return gFamilies; }
	std::vector<std::pair<std::string, std::size_t>> GemsBlocked() { return gBlocked; }
	std::array<std::size_t, 4>                       GemsLeftTo() { return gLeftTo; }
	std::size_t                                      GemsLeftToOwnLight() { return gOwnLightNow.load(); }
	std::string                                      RusticFound() { return gRustic; }

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
		std::string lights;
		for (const auto& [id, kept] : gKept) {
			if (!kept.light || !kept.gem) {
				continue;
			}
			const auto& d = kept.light->GetLightRuntimeData();
			const auto& p = kept.light->world.translate;
			lights += std::format(R"({}{{"ref":"0x{:08X}","family":"{}","kind":{},"rgb":[{},{},{}],"fade":{:.4f},"radius":{:.1f},"at":[{:.0f},{:.0f},{:.0f}]}})",
				lights.empty() ? "" : ",", id, Gems::FamilyName(kept.gem->look.family), static_cast<int>(kept.gem->look.kind),
				kept.gem->look.rgb[0], kept.gem->look.rgb[1], kept.gem->look.rgb[2], d.fade, d.radius.x, p.x, p.y, p.z);
		}
		std::string families, blocked;
		for (const auto& [name, n] : gFamilies) {
			families += std::format(R"({}"{}":{})", families.empty() ? "" : ",", name, n);
		}
		for (const auto& [name, n] : gBlocked) {
			blocked += std::format(R"({}"{}":{})", blocked.empty() ? "" : ",", name, n);
		}
		return std::format(
			R"({{"gemForms":{},"litByLook":{{{}}},"waitingOnPermission":{{{}}},"rustic":"{}","leftTo":{{"emissiveSoulGems":{},"lightPlacer":{},"reLight":{},"ownLightNearYou":{}}},"onShowNearYou":{},"particleLightConfigs":{},"displays":{},"displaysDark":{},"lightsNow":{},"lightsMade":{},"lights":[{}]}})",
			gGems.size(), families, blocked, gRustic, gLeftTo[1], gLeftTo[2], gLeftTo[3], gOwnLightNow.load(), gOnShowNow.load(), gParticleTextures.size(),
			gDisplays.size(), gDisplaysDark.load(), gKept.size(), gMade.load(), lights);
	}
}
