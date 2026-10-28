// Soul Gem Hub - SKSE plugin
// Copyright (C) 2026 izzydoingit
// GPL-3.0-or-later; see LICENSE.txt and the notice at the top of main.cpp.
//
// The game-free part, so tests/testgems.cpp checks it off the game (PC Runner\plugin build.py builds and runs it):
//   text      Lower, SameText, Trim, BareName - a model path as the plugin compares it
//   light     CutoffFor, IslReach, PlainAt - Illuminated's LightMath.h, the same numbers; KeyAt - Truman's KeyframeSequence maths
//   gems      which gem a model is, its color (ColorSum: off its texture pixels) and its strength
//   settings  SoulGemHub\Settings.ini as one table, its reader and its writer; ColorAt - the menu's "Gem colors" and "Light color"
//   permits   Fingerprint, BlockedModOf - the gem mods not lit until their authors allow it (GemPermissions.h)

#pragma once

#include "GemGlowColors.h"
#include "GemModPicks.h"
#include "GemPermissions.h"
#include "GemSeenColors.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <istream>
#include <iterator>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Gems
{
	// ------------------------------------------------------------------ text
	[[nodiscard]] inline std::string Lower(std::string_view a_text)
	{
		std::string out(a_text);
		std::ranges::transform(out, out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return out;
	}

	[[nodiscard]] inline bool SameText(std::string_view a, std::string_view b) noexcept
	{
		return a.size() == b.size() && std::ranges::equal(a, b, [](char x, char y) {
			return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
		});
	}

	[[nodiscard]] inline std::string_view Trim(std::string_view a_text) noexcept
	{
		constexpr std::string_view kSpace = " \t\r\n";
		const auto                 first = a_text.find_first_not_of(kSpace);
		if (first == std::string_view::npos) {
			return {};
		}
		return a_text.substr(first, a_text.find_last_not_of(kSpace) - first + 1);
	}

	// a model path as the game or a record writes it ("Clutter\SoulGem\SoulGemGrand01Full.nif", "meshes/...") -> its file
	// name in lower case ("soulgemgrand01full.nif")
	[[nodiscard]] inline std::string BareName(std::string_view a_model)
	{
		const auto cut = a_model.find_last_of("\\/");
		return Lower(cut == std::string_view::npos ? a_model : a_model.substr(cut + 1));
	}

	// does a Light Placer formIDs token ("0x2e4ff~skyrim.esm", "0002e4ff~skyrim.esm" - lower case) name the form a_local of a_plugin
	// (lower case)? An editor ID token is matched apart. Only the low 24 bits count.
	[[nodiscard]] inline bool TokenNamesForm(std::string_view a_token, std::string_view a_plugin, std::uint32_t a_local) noexcept
	{
		const auto tilde = a_token.find('~');
		if (tilde == std::string_view::npos || a_token.substr(tilde + 1) != a_plugin) {
			return false;
		}
		auto hex = a_token.substr(0, tilde);
		if (hex.starts_with("0x")) {
			hex.remove_prefix(2);
		}
		std::uint32_t v = 0;
		const auto [end, ec] = std::from_chars(hex.data(), hex.data() + hex.size(), v, 16);
		return !hex.empty() && ec == std::errc{} && end == hex.data() + hex.size() && (v & 0x00FFFFFF) == (a_local & 0x00FFFFFF);
	}

	// a whole model path as the plugin compares one: lower case, backslashes, no leading backslash or "meshes\"
	// ("Meshes/DLC01/Clutter/X.nif" -> "dlc01\clutter\x.nif")
	[[nodiscard]] inline std::string NormalPath(std::string_view a_model)
	{
		std::string out = Lower(a_model);
		std::ranges::replace(out, '/', '\\');
		while (out.starts_with('\\')) {
			out.erase(0, 1);
		}
		if (out.starts_with("meshes\\")) {
			out.erase(0, 7);
		}
		return out;
	}

	// ------------------------------------------------------------------ light (Illuminated's LightMath.h, the same numbers)
	inline constexpr float kK = 3918.88f;  // the cutoff constant every config of ours is written with (0.8 * 69.99²)
	inline constexpr float kGemSize = 1.0f;
	// never 1: Community Shaders reads a cutoff of exactly 1 as "use the default" (0.05) and draws the light ~8x farther
	inline constexpr float kLowestCutoff = 0.01f, kHighestCutoff = 0.99f;

	// cutoff = K * fade / (reach² + size²)
	[[nodiscard]] inline float CutoffFor(float a_fade, float a_reach, float a_size = kGemSize) noexcept
	{
		return std::clamp(kK * a_fade / (a_reach * a_reach + a_size * a_size), kLowestCutoff, kHighestCutoff);
	}

	// how far an inverse-square light reaches: reach² = K * fade / cutoff - size²
	[[nodiscard]] inline float IslReach(float a_fade, float a_cutoff, float a_size = kGemSize) noexcept
	{
		if (a_cutoff <= 0.0f || a_fade <= 0.0f) {
			return 0.0f;
		}
		return std::sqrt((std::max)(kK * a_fade / a_cutoff - a_size * a_size, 0.0f));
	}

	// the same light drawn by the game's own lighting (ENB, Vanilla): Dynamic Wards' house light - a reach of 133 drawn at
	// radius 178 with fade 1.14 - never shorter than the radius already stated
	struct Plain
	{
		float fade{ 0.0f }, radius{ 0.0f };
	};
	[[nodiscard]] inline Plain PlainAt(float a_fade, float a_radius, float a_reach) noexcept
	{
		constexpr float kPlainReach = 178.0f / 133.0f, kPlainFade = 1.14f;
		return { a_fade * kPlainFade, (std::max)(a_radius, a_reach * kPlainReach) };
	}

	// A fade controller's value at a_time: Truman's KeyframeSequence::Interpolate (RE::Light, controller.h, GPL-3.0; his
	// permission, credited) - step (0), linear (1) or cubic Hermite with no tangents (2). a_keys sorted by time; empty gives 0.
	[[nodiscard]] inline float KeyAt(std::span<const std::pair<float, float>> a_keys, int a_interpolation, float a_time) noexcept
	{
		if (a_keys.empty()) {
			return 0.0f;
		}
		const auto next = std::ranges::upper_bound(a_keys, a_time, {}, &std::pair<float, float>::first);
		if (next == a_keys.begin()) {
			return a_keys.front().second;
		}
		if (next == a_keys.end()) {
			return a_keys.back().second;
		}
		const auto& [t0, v0] = *std::prev(next);
		const auto& [t1, v1] = *next;
		const float dt = t1 - t0;
		if (dt <= 0.0f || a_interpolation == 0) {
			return v0;
		}
		const float u = (a_time - t0) / dt;
		if (a_interpolation == 1) {
			return (1.0f - u) * v0 + u * v1;
		}
		const float u2 = u * u, u3 = u2 * u;
		return (2.0f * u3 - 3.0f * u2 + 1.0f) * v0 + (-2.0f * u3 + 3.0f * u2) * v1;
	}

	// the soft pulse: the light breathes 10% either side of its strength over 4.5 s, as Illuminated's soul gem rows did
	inline constexpr float     kPulseSeconds = 4.5f;
	[[nodiscard]] inline float PulseAt(float a_seconds) noexcept
	{
		constexpr std::array<std::pair<float, float>, 3> keys{ { { 0.0f, 0.9f }, { kPulseSeconds / 2.0f, 1.1f }, { kPulseSeconds, 0.9f } } };
		const float                                      t = std::fmod(std::fabs(a_seconds), kPulseSeconds);
		return KeyAt(keys, 2, t);
	}

	// ------------------------------------------------------------------ gems
	// how strong a gem's light is: the petty to grand gems step up, the named quest gems shine as a grand one, the two stars and
	// the Soul Cairn's bowl and chunks the most
	enum class Kind : std::uint8_t
	{
		kPetty,
		kLesser,
		kCommon,
		kGreater,
		kGrand,
		kBlack,
		kNamed,
		kStar,
		kCairn,
	};
	inline constexpr std::size_t kKinds = 9;

	struct Strength
	{
		float fade{ 0.0f }, cutoff{ 0.0f };
	};
	// the sizes still step up, but half as far apart as Illuminated's rows were (his look: small gems lit too little, big ones too
	// much): on Community Shaders a petty gem reaches about 56 units, a grand one about 81
	inline constexpr std::array<Strength, kKinds> kStrength{ {
		{ 0.382f, 0.4752f },  // petty
		{ 0.466f, 0.4751f },  // lesser
		{ 0.551f, 0.475f },   // common
		{ 0.678f, 0.475f },   // greater
		{ 0.805f, 0.475f },   // grand
		{ 0.805f, 0.475f },   // black
		{ 0.805f, 0.475f },   // a named quest gem
		{ 1.285f, 0.475f },   // Azura's Star, the Black Star
		{ 1.285f, 0.475f },   // the Soul Cairn's gem bowl and chunks
	} };

	// whose gem it is: what decides its color
	enum class Family : std::uint8_t
	{
		kGame,       // the game's own gem models (or one this mod does not know)
		kPraedys,    // Praedy's Soul Gems' filled gems (and Subtle Soul Gems' copies of them)
		kRustic,     // Rustic Soul Gems (both downloads)
		kNamed,      // a quest gem with a model of its own
		kStar,       // Azura's Star, the Black Star
		kSoulCairn,  // the Soul Cairn's gem bowl, chunks and portal bowl
	};

	using Rgb = std::array<std::uint8_t, 3>;

	struct Look
	{
		Family             family{ Family::kGame };
		Kind               kind{ Kind::kGrand };
		Rgb                rgb{ 255, 255, 255 };
		bool               ownColor{ false };    // keeps its color under the "Gem colors" pick: a named quest gem on any model, a gem in its own swapped textures
		std::optional<Rgb> glow{};               // its own glow where that is not rgb (GemGlowColors.h): the "Light color" default
		bool               glowAlways{ false };  // that glow on "Author pick" too
	};

	// the six sizes as a gem model names them, in Kind order
	inline constexpr std::array<std::string_view, 6> kSizeWords{ "petty", "lesser", "common", "greater", "grand", "black" };

	// Each family's color per size (petty, lesser, common, greater, grand, black): the color the gem shows is the light's color.
	// Praedy's gems show their own textures' sky blue (the black gem purple); their glow's animated pink lights only where the gems
	// light themselves (ENB, Community Shaders' particle lights), never under our light. Rustic's gems each wear their own
	// texture color (the 1K and 2K downloads within 15° of each other). The game's own gems: their textures as the model read gives
	// them (soulgemcommon pale -> pale blue, soulgemgrand teal, soulgemblack purple); in game the game's family, Rustic and the stars
	// read theirs off the model (ColorFromModel) and these rows stand only when nothing reads
	inline constexpr std::array<Rgb, 6> kGameRgb{ { { 152, 210, 255 }, { 152, 210, 255 }, { 152, 210, 255 }, { 83, 255, 234 },
		{ 83, 255, 234 }, { 184, 132, 255 } } };
	inline constexpr std::array<Rgb, 6> kPraedysRgb{ { { 42, 196, 255 }, { 50, 190, 255 }, { 44, 189, 255 }, { 40, 183, 255 },
		{ 42, 183, 255 }, { 154, 44, 255 } } };
	inline constexpr std::array<Rgb, 6> kRusticRgb{ { { 128, 179, 255 }, { 76, 193, 255 }, { 127, 93, 255 }, { 26, 181, 255 },
		{ 25, 239, 255 }, { 155, 119, 255 } } };

	struct NamedModel
	{
		std::string_view path;  // the model as NormalPath writes it (a copy under another folder - a mod's decor - is not it)
		Family           family{};
		Kind             kind{};
		Rgb              rgb;
	};
	// the gems and pieces known by their own model, each in its own textures' color: the named gems' models are Praedy's Soul Gems'
	// files (soulgemdb = the full Soul Essence Gem), sky blue like its filled gems; the Soul Cairn's bowl and chunk wear the game's
	// soulgemcommon texture (pale blue)
	inline constexpr std::array<NamedModel, 7> kNamedModels{ {
		{ "clutter\\soulgem\\soulgemarniel.nif", Family::kNamed, Kind::kNamed, { 133, 194, 255 } },      // Arniel's warped gems
		{ "clutter\\soulgem\\soulgemmalyn.nif", Family::kNamed, Kind::kNamed, { 131, 200, 255 } },       // Malyn's Black Soul Gem
		{ "clutter\\soulgem\\soulgemwylandriah.nif", Family::kNamed, Kind::kNamed, { 134, 201, 255 } },  // Wylandriah's Soul Gem
		{ "clutter\\soulgem\\soulgemdb.nif", Family::kNamed, Kind::kNamed, { 136, 136, 255 } },
		{ "dlc01\\clutter\\dlc01soulgembowlfull.nif", Family::kSoulCairn, Kind::kCairn, { 152, 210, 255 } },
		{ "dlc01\\clutter\\dlc01soulgemchunk01.nif", Family::kSoulCairn, Kind::kCairn, { 152, 210, 255 } },
		{ "dlc01\\dungeons\\castle\\animated\\scportalbowl\\casscportalbowl01.nif", Family::kSoulCairn, Kind::kNamed, { 170, 90, 255 } },  // the portal's bowl
	} };

	// Azura's Star wears its own texture's teal (azurastarlite.dds; a replacer's star is read off its own model in game); the
	// Black Star is purple by his eye - its dark azurastar.dds reads a near-white blue
	inline constexpr Rgb kAzurasStarRgb{ 157, 255, 252 };
	inline constexpr Rgb kBlackStarRgb{ 170, 90, 255 };

	// which star a model is: Azura's Star wears azurastarlite.nif, the Black Star azurastar.nif - so a gem mod's own star records
	// (Andrealletius Soul Gem Overhaul's Drained ... Flawless stars) are stars too, not only the game's two records
	enum class StarModel : std::uint8_t
	{
		kNone,
		kAzura,
		kBlack,
	};
	[[nodiscard]] inline StarModel StarOf(std::string_view a_model)
	{
		const auto path = NormalPath(a_model);
		return path == "clutter\\daedric\\azurastarlite.nif" ? StarModel::kAzura :
		       path == "clutter\\daedric\\azurastar.nif"     ? StarModel::kBlack :
		                                                       StarModel::kNone;
	}

	[[nodiscard]] inline const Rgb& SizeRgb(const std::array<Rgb, 6>& a_family, Kind a_kind) noexcept
	{
		return a_family[std::min<std::size_t>(static_cast<std::size_t>(a_kind), 5)];
	}

	// the size a gem model's name carries ("soulgemgrand01full.nif", "soulgemblack_full.nif" -> grand, black), or none
	[[nodiscard]] inline std::optional<Kind> SizeOfName(std::string_view a_bare) noexcept
	{
		constexpr std::string_view kStem = "soulgem";
		if (!a_bare.starts_with(kStem)) {
			return std::nullopt;
		}
		const auto rest = a_bare.substr(kStem.size());
		for (std::size_t i = 0; i < kSizeWords.size(); ++i) {
			if (rest.starts_with(kSizeWords[i])) {
				return static_cast<Kind>(i);
			}
		}
		return std::nullopt;
	}

	// the look a model gives its gem; none = not a model of ours to judge (the caller uses the game's look for the gem's size).
	// a_rusticLoaded: Rustic Soul Gems' plugin is in the load order (its "_full" models; another mod may ship the same names)
	[[nodiscard]] inline std::optional<Look> LookOfModel(std::string_view a_model, bool a_rusticLoaded)
	{
		const auto path = NormalPath(a_model);
		for (const auto& n : kNamedModels) {
			if (path == n.path) {
				return Look{ n.family, n.kind, n.rgb };
			}
		}
		const auto bare = BareName(path);
		const auto size = SizeOfName(bare);
		if (!size) {
			return std::nullopt;
		}
		if (bare.find("01full") != std::string::npos) {  // Praedy's filled gems (CS Light's part-filled ones too: "...01fullcommon.nif")
			return Look{ Family::kPraedys, *size, SizeRgb(kPraedysRgb, *size) };
		}
		// Rustic's own models only ("clutter\soulgem\soulgemgrand_full.nif"): another replacer's "..01_full.nif" or a copy in another folder is not
		if (a_rusticLoaded && path == std::string("clutter\\soulgem\\soulgem").append(kSizeWords[static_cast<std::size_t>(*size)]).append("_full.nif")) {
			return Look{ Family::kRustic, *size, SizeRgb(kRusticRgb, *size) };
		}
		return std::nullopt;
	}

	// RE::Light lights a placed reference whose model's bare name holds a config's key, so a decor copy under another folder wears
	// this mod's RE::Light light too: true for a model holding one of the keys (the known models' bare names, Praedy's "01full")
	[[nodiscard]] inline bool ReLightKeyed(std::string_view a_model)
	{
		const auto bare = BareName(a_model);
		for (const auto& n : kNamedModels) {
			if (n.path == "clutter\\soulgem\\soulgemdb.nif") {
				continue;  // the Soul Essence Gem's config is keyed by its record, never by this model (build soul gems.py)
			}
			auto key = BareName(n.path);
			key.resize(key.size() - std::string_view(".nif").size());
			if (bare.find(key) != std::string::npos) {
				return true;
			}
		}
		return SizeOfName(bare) && bare.find("01full") != std::string::npos;
	}

	// A gem model's color off its texture pixels, so a replacer at the game's own paths shows its own colors: each pixel weighs by
	// its chroma (top channel minus bottom one) times its alpha - grey, white, see-through and nearly black pixels count for
	// nothing - and the mean is scaled so its top channel is 255. No colored pixel = no color.
	class ColorSum
	{
	public:
		void Add(std::uint8_t a_r, std::uint8_t a_g, std::uint8_t a_b, std::uint8_t a_a = 255) noexcept
		{
			const int top = (std::max)({ a_r, a_g, a_b }), bottom = (std::min)({ a_r, a_g, a_b });
			if (top < kDarkTop || top == bottom || a_a == 0) {
				return;
			}
			const double w = static_cast<double>(top - bottom) * (a_a / 255.0);
			sum[0] += a_r * w;
			sum[1] += a_g * w;
			sum[2] += a_b * w;
			weight += w;
		}

		void Add(const ColorSum& a_other) noexcept
		{
			for (std::size_t i = 0; i < 3; ++i) {
				sum[i] += a_other.sum[i];
			}
			weight += a_other.weight;
		}

		[[nodiscard]] std::optional<Rgb> Result() const noexcept
		{
			if (weight <= 0.0) {
				return std::nullopt;
			}
			const double top = (std::max)({ sum[0], sum[1], sum[2] }) / weight;  // > 0: a colored pixel has a channel above 0
			Rgb          out{};
			for (std::size_t i = 0; i < 3; ++i) {
				out[i] = static_cast<std::uint8_t>((std::min)(255.0, std::round(sum[i] / weight * 255.0 / top)));
			}
			return out;
		}

	private:
		static constexpr int  kDarkTop = 16;
		std::array<double, 3> sum{};
		double                weight{ 0.0 };
	};

	// the families whose color is read off the gem's own model in game; Praedy's (read off its textures into the table), the named
	// gems and the Soul Cairn keep their own rows
	[[nodiscard]] inline bool ColorFromModel(Family a_family) noexcept
	{
		return a_family == Family::kGame || a_family == Family::kRustic || a_family == Family::kStar;
	}

	// A texture that is a normal map, not a color (its file name ends _n, _msn or _normal, any case, before the extension). The
	// game's own soul gem models and every replacer at their paths carry an inner shape whose color slot names one
	// (soulgemcommon0_n.dds); read as a color it is a strong lavender blue that outweighed the gem's own texture.
	[[nodiscard]] inline bool IsNormalMap(std::string_view a_path)
	{
		std::string name = Lower(a_path.substr(a_path.find_last_of("/\\") == std::string_view::npos ? 0 : a_path.find_last_of("/\\") + 1));
		if (const auto dot = name.rfind('.'); dot != std::string::npos) {
			name.erase(dot);
		}
		return name.ends_with("_n") || name.ends_with("_msn") || name.ends_with("_normal");
	}

	// The light a gem read off its model wears: its textures' color (ColorSum), steady - never its glow's animated tint, which swung
	// the game's crystals violet (his "more paler blue") and Praedy's pink. A pale gem (the game's blue-white crystals, Morrowind-
	// Style's pastels) wears a pale blue; a black-soul gem and a star keep their own color whatever it is (a purple gem keeps a
	// purple light). Pale = its weakest channel at least kPaleShare of its strongest
	inline constexpr double kPaleShare = 0.5;
	inline constexpr Rgb    kPaleBlue{ 150, 190, 255 };
	inline constexpr double kPaleBlueShare = 0.6;

	[[nodiscard]] inline bool IsPale(const Rgb& a_rgb) noexcept
	{
		const int top = (std::max)({ a_rgb[0], a_rgb[1], a_rgb[2] }), bottom = (std::min)({ a_rgb[0], a_rgb[1], a_rgb[2] });
		return top > 0 && bottom >= kPaleShare * top;
	}

	[[nodiscard]] inline Rgb PaleBlue(const Rgb& a_texture) noexcept
	{
		std::array<double, 3> mix{};
		for (std::size_t i = 0; i < 3; ++i) {
			mix[i] = a_texture[i] * (1.0 - kPaleBlueShare) + kPaleBlue[i] * kPaleBlueShare;
		}
		const double top = (std::max)({ mix[0], mix[1], mix[2] });
		Rgb          out{};
		for (std::size_t i = 0; i < 3; ++i) {
			out[i] = static_cast<std::uint8_t>((std::min)(255.0, std::round(mix[i] * 255.0 / top)));
		}
		return out;
	}

	[[nodiscard]] inline Rgb ShownColor(const Rgb& a_texture, Kind a_kind) noexcept
	{
		return a_kind != Kind::kBlack && a_kind != Kind::kStar && IsPale(a_texture) ? PaleBlue(a_texture) : a_texture;
	}

	// the game's own look for a gem of this size (a_black: it holds black souls)
	[[nodiscard]] inline Look GameLook(Kind a_size, bool a_black) noexcept
	{
		const Kind kind = a_black ? Kind::kBlack : a_size;
		return Look{ Family::kGame, kind, SizeRgb(kGameRgb, kind) };
	}

	// Emissive Soul Gems (iamzachhunter, Nexus 191137): the models its Light Placer file lights - our RE::Light files never name
	// one (build soul gems.py reads this table). Whole paths, as Light Placer matches them: a replacer's copy in another folder
	// (MLP's and Vayura's "full\", Spherical's "Filled\") is not his
	inline constexpr std::array<std::string_view, 14> kEmissiveModels{ "clutter\\soulgem\\soulgempetty01.nif",
		"clutter\\soulgem\\soulgempetty_full.nif", "clutter\\soulgem\\soulgemlesser01.nif", "clutter\\soulgem\\soulgemlesser_full.nif",
		"clutter\\soulgem\\soulgemcommon01.nif", "clutter\\soulgem\\soulgemcommon_full.nif", "clutter\\soulgem\\soulgemgreater01.nif",
		"clutter\\soulgem\\soulgemgreater_full.nif", "clutter\\soulgem\\soulgemgrand01.nif", "clutter\\soulgem\\soulgemgrand_full.nif",
		"clutter\\soulgem\\soulgemblack01.nif", "clutter\\soulgem\\soulgemblack_full.nif", "clutter\\daedric\\azurastar.nif",
		"clutter\\daedric\\azurastarlite.nif" };

	[[nodiscard]] inline bool EmissiveModel(std::string_view a_model)
	{
		return std::ranges::find(kEmissiveModels, NormalPath(a_model)) != kEmissiveModels.end();
	}

	// the gems this mod's RE::Light configs light (SKSE\Plugins\ReLight\Configs\Illuminated - Soul Gem Hub\): Praedy's
	// filled gems, the named quest gems and the Soul Cairn's pieces - each keyed by its own model (the Soul Essence Gem by its
	// record). With RE::Light loaded the plugin leaves these to it; the gems that share the game's model (or are Emissive Soul
	// Gems' models) have no config, so the plugin lights them itself
	[[nodiscard]] inline bool ReLightCovers(Family a_family) noexcept
	{
		return a_family == Family::kPraedys || a_family == Family::kNamed || a_family == Family::kSoulCairn;
	}

	// a script that puts an item on show (Legacy of the Dragonborn's dbm_dynamicdisplayscript, a museum's DisplayCase...): the
	// gem it is attached to is a display, never lit
	[[nodiscard]] inline bool IsDisplayScript(std::string_view a_script)
	{
		return Lower(a_script).find("display") != std::string::npos;
	}

	// whose other light a gem would wear: none; Emissive Soul Gems' own Light Placer file's, or another Light Placer file's
	// (Subtle Soul Gems', CS Light's ...) - lit by us all the same, theirs put out unless the menu keeps that kind; or this
	// mod's own RE::Light files' (RE::Light loaded) - left to RE::Light
	enum class Owner : std::uint8_t
	{
		kThisMod,
		kEmissive,
		kLightPlacer,
		kReLight,
	};

	// which loaded Light Placer file claims a gem: none (also when Light Placer is not loaded), Emissive Soul Gems' own, another
	enum class Claim : std::uint8_t
	{
		kNone,
		kEmissive,
		kOther,
	};

	[[nodiscard]] inline Owner OwnerOf(std::string_view a_model, Family a_family, Claim a_claim, bool a_reLight)
	{
		if (a_claim == Claim::kEmissive) {
			return Owner::kEmissive;
		}
		if (a_claim == Claim::kOther) {
			return Owner::kLightPlacer;
		}
		if (a_reLight && ReLightCovers(a_family) && !EmissiveModel(a_model)) {
			return Owner::kReLight;
		}
		return Owner::kThisMod;
	}

	// ------------------------------------------------------------------ permits
	// a file's fingerprint: FNV-1a 64 over its first 64 KB, then its size folded in (perm signatures.py hashes the same way)
	inline constexpr std::size_t kFingerprintHead = 65536;

	// fed a file in pieces, so a large texture never has to sit in memory whole
	class FingerprintSum
	{
	public:
		void Add(std::span<const std::uint8_t> a_bytes) noexcept
		{
			for (const auto b : a_bytes.first(std::min(a_bytes.size(), kFingerprintHead - std::min(_size, kFingerprintHead)))) {
				_hash = (_hash ^ b) * kPrime;
			}
			_size += a_bytes.size();
		}
		[[nodiscard]] std::uint64_t Result() const noexcept { return (_hash ^ _size) * kPrime; }
		[[nodiscard]] std::size_t   Size() const noexcept { return _size; }

	private:
		static constexpr std::uint64_t kPrime = 0x100000001B3ULL;
		std::uint64_t                  _hash{ 0xCBF29CE484222325ULL };
		std::size_t                    _size{ 0 };
	};

	[[nodiscard]] inline std::uint64_t Fingerprint(std::span<const std::uint8_t> a_bytes) noexcept
	{
		FingerprintSum sum;
		sum.Add(a_bytes);
		return sum.Result();
	}

	// a texture path as GemPermissions.h writes it: lower case, below textures\ (a material may name it either way)
	[[nodiscard]] inline std::string TexturePath(std::string_view a_texture)
	{
		std::string out = Lower(a_texture);
		std::ranges::replace(out, '/', '\\');
		for (const std::string_view top : { "\\", "data\\", "textures\\" }) {
			if (out.starts_with(top)) {
				out.erase(0, top.size());
			}
		}
		return out;
	}

	// every texture a model file names, read off its bytes, in the order it stores them, as TexturePath writes it - no model
	// loader needed, so it works on VR too. A NIF keeps each path as a 4-byte length, then the text: each ".dds" end is taken with
	// the length that reaches it (a run of printable bytes is not enough - the next path's length byte is often printable too)
	[[nodiscard]] inline std::vector<std::string> TexturesNamed(std::span<const std::uint8_t> a_nif)
	{
		constexpr std::size_t    kLongestPath = 300;
		const char*              text = reinterpret_cast<const char*>(a_nif.data());
		std::vector<std::string> out;
		for (std::size_t end = 9; end <= a_nif.size(); ++end) {
			if (!SameText(std::string_view(text + end - 4, 4), ".dds")) {
				continue;
			}
			for (std::size_t start = end - 5; start >= 4 && end - start <= kLongestPath; --start) {
				std::uint32_t length = 0;
				std::memcpy(&length, a_nif.data() + start - 4, sizeof(length));  // little-endian, as the file and the PC
				if (length == end - start) {
					if (auto path = TexturePath(std::string_view(text + start, end - start)); std::ranges::find(out, path) == out.end()) {
						out.push_back(std::move(path));
					}
					break;
				}
			}
		}
		return out;
	}

	// a texture that gives a model its colors - not a cubemap, normal, specular, glow or other map, which mods share widely
	// (a shared cubemap must never mark someone else's gem as a blocked mod's)
	[[nodiscard]] inline bool ColorTexture(std::string_view a_texture)
	{
		const auto path = TexturePath(a_texture);
		if (path.starts_with("cubemaps\\") || !path.ends_with(".dds")) {
			return false;
		}
		const std::string_view stem(path.data(), path.size() - 4);
		return std::ranges::none_of(std::array<std::string_view, 8>{ "_n", "_m", "_e", "_g", "_s", "_p", "_em", "_msn" },
			[&](std::string_view a_suffix) { return stem.ends_with(a_suffix); });
	}

	// the gem mod a file belongs to while its author has not allowed this mod to light it: by its bytes ...
	[[nodiscard]] inline std::optional<std::string_view> BlockedModOf(std::uint64_t a_fingerprint) noexcept
	{
		const auto it = std::ranges::lower_bound(kBlockedFiles, a_fingerprint, {}, &BlockedFile::hash);
		if (it == kBlockedFiles.end() || it->hash != a_fingerprint) {
			return std::nullopt;
		}
		return kBlockedMods[it->mod];
	}

	// ... or by a name only such a mod ships (a model as NormalPath writes it, a texture as TexturePath does), which still
	// holds after an update of the mod or in another texture size
	[[nodiscard]] inline std::optional<std::string_view> BlockedModOfPath(std::string_view a_path) noexcept
	{
		const auto it = std::ranges::lower_bound(kBlockedPaths, a_path, {}, &BlockedPath::path);
		if (it == kBlockedPaths.end() || it->path != a_path) {
			return std::nullopt;
		}
		return kBlockedMods[it->mod];
	}

	// the color a known gem file's picture shows (a color texture's or a model's fingerprint), or none
	[[nodiscard]] inline std::optional<Rgb> SeenColorOf(std::uint64_t a_fingerprint) noexcept
	{
		const auto it = std::ranges::lower_bound(kSeenColors, a_fingerprint, {}, &SeenColor::hash);
		if (it == kSeenColors.end() || it->hash != a_fingerprint) {
			return std::nullopt;
		}
		return Rgb{ it->r, it->g, it->b };
	}

	// a known gem file's own glow, listed where it is not the color its light wears, or none
	[[nodiscard]] inline const GlowColor* GlowColorOf(std::uint64_t a_fingerprint) noexcept
	{
		const auto it = std::ranges::lower_bound(kGlowColors, a_fingerprint, {}, &GlowColor::hash);
		return it == kGlowColors.end() || it->hash != a_fingerprint ? nullptr : &*it;
	}

	struct NamedGem
	{
		std::uint32_t    id{ 0 };  // the record's id in its own plugin
		std::string_view plugin;
		Kind             kind{};  // the plain gem it wears when no mod gives it a model of its own
	};
	// Malyn's Black Soul Gem, Arniel's gem, Wylandriah's gem (an empty record), the Soul Essence Gem
	inline constexpr std::array<NamedGem, 4> kNamedGems{ { { 0x028AD9, "Skyrim.esm", Kind::kBlack },
		{ 0x06A106, "Skyrim.esm", Kind::kGrand }, { 0x043E26, "Skyrim.esm", Kind::kGrand }, { 0x00EA6A, "Dawnguard.esm", Kind::kBlack } } };

	// the color a named quest gem's picture shows on a look that gives it no model of its own (a_form: its record's id in its
	// own plugin), by the same fingerprints as SeenColorOf, or none
	[[nodiscard]] inline std::optional<Rgb> NamedSeenColorOf(std::uint32_t a_form, std::uint64_t a_fingerprint) noexcept
	{
		const auto key = [](const NamedSeenColor& a_row) { return std::pair{ a_row.form, a_row.hash }; };
		const auto it = std::ranges::lower_bound(kNamedSeenColors, std::pair{ a_form, a_fingerprint }, {}, key);
		if (it == kNamedSeenColors.end() || key(*it) != std::pair{ a_form, a_fingerprint }) {
			return std::nullopt;
		}
		return Rgb{ it->r, it->g, it->b };
	}

	[[nodiscard]] inline std::string_view FamilyName(Family a_family) noexcept
	{
		switch (a_family) {
		case Family::kPraedys:
			return "Praedy's Soul Gems";
		case Family::kRustic:
			return "Rustic Soul Gems";
		case Family::kNamed:
			return "named gems";
		case Family::kStar:
			return "stars";
		case Family::kSoulCairn:
			return "Soul Cairn";
		default:
			return "the game's own gems";
		}
	}

	// what a gem's light is drawn with now. a_brightness, a_reach: 1 = as built. Community Shaders (inverse square): the cutoff
	// goes with Brightness / Reach² (Illuminated's sliders), so Brightness changes how bright and Reach how far; the radius is the
	// reach (what the scene culls by). The game's own lighting (ENB, Vanilla): the same light drawn plain. a_pulse (the soft
	// pulse, 0.9-1.1) moves only the fade: the cutoff and the radius come from the settings alone, so they stay put while the
	// light breathes (a pulsed cutoff pinned at the clamp would move the radius every frame)
	struct Shown
	{
		float fade{ 0.0f }, radius{ 0.0f }, cutoff{ 0.0f };
	};
	// a_dim (0-1, the Stars slider) dims the light after the clamp, its fade and cutoff alike: the reach stays, only the light drops
	[[nodiscard]] inline Shown ShownFor(const Strength& a_s, bool a_inverseSquare, float a_brightness, float a_reach,
		float a_pulse = 1.0f, float a_dim = 1.0f) noexcept
	{
		const float b = (std::max)(a_brightness, 0.0f), r = (std::max)(a_reach, 0.05f), p = (std::max)(a_pulse, 0.0f);
		const float d = std::clamp(a_dim, 0.0f, 1.0f);
		if (a_inverseSquare) {
			const float wanted = a_s.cutoff * b / (r * r);
			const float cutoff = std::clamp(std::clamp(wanted, kLowestCutoff, kHighestCutoff) * d, kLowestCutoff, kHighestCutoff);
			// past the highest cutoff a shorter reach is kept by a dimmer fade (reach² goes with fade / cutoff)
			const float fade = a_s.fade * b * (wanted > kHighestCutoff ? kHighestCutoff / wanted : 1.0f) * d;
			return { fade * p, (std::max)(IslReach(fade, cutoff), 1.0f), cutoff };
		}
		const auto plain = PlainAt(a_s.fade * b * d * p, 0.0f, IslReach(a_s.fade, a_s.cutoff) * r);
		return { plain.fade, (std::max)(plain.radius, 1.0f), 0.0f };
	}

	// ------------------------------------------------------------------ settings (SoulGemHub\Settings.ini, written by the menu)
	inline constexpr int kBrightnessMin = 25, kBrightnessMax = 200, kReachMin = 50, kReachMax = 150;
	// "Stars": Azura's Star and the Black Star on top of Brightness and Reach - their big pools can drown every gem near them
	inline constexpr int kStarsMin = 10, kStarsMax = 100;

	// on Vanilla lighting a gem's light read too dark at the default, so it is drawn this much brighter there
	inline constexpr float kVanillaBoost = 1.25f;

	// the Brightness slider's percent as the factor a light is drawn with
	[[nodiscard]] inline float BrightnessFor(int a_percent, bool a_vanilla) noexcept
	{
		return static_cast<float>(a_percent) / 100.0f * (a_vanilla ? kVanillaBoost : 1.0f);
	}

	struct Settings
	{
		bool on{ true };         // every soul gem light
		int  brightness{ 100 };  // percent
		int  reach{ 100 };       // percent
		int  stars{ 50 };        // percent of Brightness and of Reach for the two stars, so every gem reads about as bright
		bool pulse{ true };      // the soft pulse
		// "Use their light": gems another mod lights keep that light and get none of ours (our sliders then leave them alone)
		bool theirsEmissive{ false };     // Emissive Soul Gems' Light Placer lights
		bool theirsLightPlacer{ false };  // any other Light Placer file's (Subtle Soul Gems', CS Light's ...)
		bool theirsMesh{ false };         // a light in the gem's own model (an ENB or particle light: Praedy's, Subtle's, Rudy's ...)
		int  gemMod{ 0 };                 // "Gem colors": 0 = Automatic, else kGemModPicks[gemMod - 1] colors every gem of the six sizes
		bool authorPick{ false };         // "Light color": false = a gem's own glow where it differs from its light, true = Author pick
	};

	// the "Gem colors" pick a Settings.ini value names (a kGemModPicks key, any case); 0 = Automatic, also for a value this
	// version does not know
	[[nodiscard]] inline int GemModIndex(std::string_view a_key) noexcept
	{
		for (std::size_t i = 0; i < kGemModPicks.size(); ++i) {
			if (SameText(a_key, kGemModPicks[i].key)) {
				return static_cast<int>(i) + 1;
			}
		}
		return 0;
	}

	[[nodiscard]] inline const GemModPick* GemModAt(int a_index) noexcept
	{
		return a_index >= 1 && static_cast<std::size_t>(a_index) <= kGemModPicks.size() ? &kGemModPicks[static_cast<std::size_t>(a_index) - 1] : nullptr;
	}

	[[nodiscard]] inline std::string_view GemModName(int a_index) noexcept
	{
		const auto* pick = GemModAt(a_index);
		return pick ? pick->name : std::string_view("Automatic");
	}

	// a picked gem mod whose own glow differs from its colors at some size: "Light color" then changes what that pick shows
	[[nodiscard]] inline bool PickHasOwnGlow(int a_index) noexcept
	{
		const auto* pick = GemModAt(a_index);
		return pick && pick->glow != pick->rgb;
	}

	// a gem the "Gem colors" pick recolors: a soul gem of the six sizes (the game's or a replacer's, Praedy's, Rustic's) - never a
	// star, a named quest gem (on any model), a gem in its own swapped textures or a Soul Cairn piece, which keep their own
	[[nodiscard]] inline bool PickRecolors(const Look& a_look) noexcept
	{
		const bool sized = a_look.family == Family::kGame || a_look.family == Family::kPraedys || a_look.family == Family::kRustic;
		return sized && !a_look.ownColor && a_look.kind <= Kind::kBlack;
	}

	// the color a gem's light wears: the picked gem mod's color for its size when a pick recolors it, else the color found for it
	// at data load. "Light color": by default a gem's own glow where it differs from that color, on Author pick the color as found
	[[nodiscard]] inline Rgb ColorAt(const Look& a_look, const Settings& a_s) noexcept
	{
		const auto* pick = GemModAt(a_s.gemMod);
		if (!pick || !PickRecolors(a_look)) {
			return a_look.glow && (a_look.glowAlways || !a_s.authorPick) ? *a_look.glow : a_look.rgb;
		}
		return (a_s.authorPick ? pick->rgb : pick->glow)[static_cast<std::size_t>(a_look.kind)];
	}

	// whether "Light color" changes anything: a_glows = lit gems whose own glow is worn on the default choice only, a_glowsKept =
	// those of them no pick recolors; under a pick, also the pick's own glow set where it differs from its colors
	[[nodiscard]] inline bool LightColorShown(int a_gemMod, std::size_t a_glows, std::size_t a_glowsKept) noexcept
	{
		return GemModAt(a_gemMod) ? PickHasOwnGlow(a_gemMod) || a_glowsKept > 0 : a_glows > 0;
	}

	// a gem whose other light the menu keeps: ours is not hung on it and theirs stays - only when that light is really there
	// (GemLights.cpp OtherLights): a_placedLight, another mod's light hangs on it; a_meshLight, its model carries a light that
	// shines in this game. A claim alone is no light: a file may claim a gem it does not light here
	[[nodiscard]] inline bool TheirsKept(Owner a_owner, bool a_placedLight, bool a_meshLight, const Settings& a_s) noexcept
	{
		return (a_placedLight && ((a_owner == Owner::kEmissive && a_s.theirsEmissive) || (a_owner == Owner::kLightPlacer && a_s.theirsLightPlacer))) ||
		       (a_meshLight && a_s.theirsMesh);
	}

	// what a gem of this kind is drawn with for the menu's settings on the lighting found (GemLights.cpp Follow): a_inverseSquare =
	// Community Shaders' lights, a_vanilla = the game's own lighting without an ENB
	[[nodiscard]] inline Shown ShownAt(Kind a_kind, bool a_inverseSquare, bool a_vanilla, const Settings& a_s, float a_pulse = 1.0f) noexcept
	{
		const float stars = a_kind == Kind::kStar ? static_cast<float>(a_s.stars) / 100.0f : 1.0f;
		return ShownFor(kStrength[static_cast<std::size_t>(a_kind)], a_inverseSquare, BrightnessFor(a_s.brightness, a_vanilla),
			static_cast<float>(a_s.reach) / 100.0f * stars, a_pulse, stars);
	}

	// a setting's value as a number: digits, or true / false, on / off, yes / no, as a hand-edited file may hold it
	[[nodiscard]] inline std::optional<int> SettingNumber(std::string_view a_value) noexcept
	{
		constexpr std::array<std::pair<std::string_view, int>, 6> kWords{ { { "true", 1 }, { "on", 1 }, { "yes", 1 }, { "false", 0 },
			{ "off", 0 }, { "no", 0 } } };
		for (const auto& [word, n] : kWords) {
			if (SameText(a_value, word)) {
				return n;
			}
		}
		int v = 0;
		const auto [end, ec] = std::from_chars(a_value.data(), a_value.data() + a_value.size(), v);
		return ec == std::errc{} && end == a_value.data() + a_value.size() ? std::optional(v) : std::nullopt;
	}

	// the file's text over a_out's values (the defaults, for a fresh table); keys in any case; a note after a value (; or #) is
	// left out; a value that does not read keeps the one it had; every number clamped to its slider. Returns the key lines read.
	inline std::size_t ReadSettings(std::istream& a_in, Settings& a_out)
	{
		std::size_t lines = 0;
		std::string line;
		while (a_in && std::getline(a_in, line)) {
			auto text = Trim(line);
			if (text.starts_with("\xEF\xBB\xBF")) {
				text = Trim(text.substr(3));  // the UTF-8 mark an editor may put before the first line
			}
			if (text.empty() || text.front() == ';' || text.front() == '#' || text.front() == '[') {
				continue;
			}
			const auto eq = text.find('=');
			if (eq == std::string_view::npos) {
				continue;
			}
			const auto key = Trim(text.substr(0, eq));
			auto       val = Trim(text.substr(eq + 1));
			val = Trim(val.substr(0, val.find_first_of(";#")));
			if (SameText(key, "GemMod")) {  // a gem mod's key (as written here), or its number in the menu's order (as DevBench takes it)
				int n = 0;
				const auto [end, ec] = std::from_chars(val.data(), val.data() + val.size(), n);
				a_out.gemMod = ec == std::errc{} && end == val.data() + val.size() && GemModAt(n) ? n : GemModIndex(val);
				++lines;
				continue;
			}
			const auto read = SettingNumber(val);
			if (!read) {
				continue;
			}
			const int v = *read;
			++lines;
			if (SameText(key, "Lights")) {
				a_out.on = v != 0;
			} else if (SameText(key, "Brightness")) {
				a_out.brightness = std::clamp(v, kBrightnessMin, kBrightnessMax);
			} else if (SameText(key, "Reach")) {
				a_out.reach = std::clamp(v, kReachMin, kReachMax);
			} else if (SameText(key, "Stars")) {
				a_out.stars = std::clamp(v, kStarsMin, kStarsMax);
			} else if (SameText(key, "Pulse")) {
				a_out.pulse = v != 0;
			} else if (SameText(key, "TheirsEmissive")) {
				a_out.theirsEmissive = v != 0;
			} else if (SameText(key, "TheirsLightPlacer")) {
				a_out.theirsLightPlacer = v != 0;
			} else if (SameText(key, "TheirsMesh")) {
				a_out.theirsMesh = v != 0;
			} else if (SameText(key, "AuthorPick")) {
				a_out.authorPick = v != 0;
			} else {
				--lines;  // a key this version does not know
			}
		}
		return lines;
	}

	inline void WriteSettings(const Settings& a_s, std::ostream& a_out)
	{
		const auto* pick = GemModAt(a_s.gemMod);
		a_out << "; Illuminated - Soul Gem Hub - written by its menu (SKSE Menu Framework). Change these in game, not here.\n"
			  << "[Settings]\nLights=" << (a_s.on ? 1 : 0) << "\nBrightness=" << a_s.brightness << "\nReach=" << a_s.reach << "\nStars=" << a_s.stars
			  << "\nPulse=" << (a_s.pulse ? 1 : 0) << "\nTheirsEmissive=" << (a_s.theirsEmissive ? 1 : 0)
			  << "\nTheirsLightPlacer=" << (a_s.theirsLightPlacer ? 1 : 0) << "\nTheirsMesh=" << (a_s.theirsMesh ? 1 : 0) << "\nGemMod="
			  << (pick ? pick->key : std::string_view("auto")) << "\nAuthorPick=" << (a_s.authorPick ? 1 : 0) << "\n";
	}
}
