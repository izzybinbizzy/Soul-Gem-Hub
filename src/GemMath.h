// Soul Gem Hub - SKSE plugin
// Copyright (C) 2026 izzydoingit
// GPL-3.0-or-later; see LICENSE.txt and the notice at the top of main.cpp.
//
// The game-free part, so tests/testgems.cpp checks it off the game (PC Runner\plugin build.py builds and runs it):
//   text      Lower, SameText, Trim, BareName - a model path as the plugin compares it
//   light     CutoffFor, IslReach, PlainAt - Illuminated's LightMath.h, the same numbers; KeyAt - Truman's KeyframeSequence maths
//   gems      which gem a model is, its color (ColorSum: off its texture pixels) and its strength
//   settings  SoulGemHub\Settings.ini as one table, its reader and its writer; ColorAt - the menu's color choices and "Light color"
//   permits   Fingerprint, BlockedModOf - the gem mods not lit until their authors allow it (GemPermissions.h)

#pragma once

#include "GemGlowColors.h"
#include "GemMeasuredColors.h"
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
#include <ranges>
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

	// a two-way color choice the menu offers for one gem mod's gems (his calls, kHisColors), in Settings::second order
	enum class ColorChoice : std::uint8_t
	{
		kNone,
		kNeVP,    // NeV-P Extreme Crystals: the gem's color or its flare glow
		kRemRem,  // Remastered Remastered's black gem: blue or pink
		kRustic,  // Rustic Soul Gems' black gem: blue or green
		kCount,
	};
	inline constexpr std::size_t kColorChoices = static_cast<std::size_t>(ColorChoice::kCount) - 1;

	struct Look
	{
		Family             family{ Family::kGame };
		Kind               kind{ Kind::kGrand };
		Rgb                rgb{ 255, 255, 255 };
		ColorChoice        choice{ ColorChoice::kNone };  // its mod's two-way color choice: rgb is the first, second the other
		Rgb                second{ 255, 255, 255 };
		std::optional<Rgb> glow{};               // its own glow where that is not rgb (GemGlowColors.h): the "Light color" default
		bool               glowAlways{ false };  // that glow on "Author pick" too
		bool               blackStar{ false };   // a star: the Black Star (its own light switch), else Azura's Star
		bool               named{ false };       // a named quest gem, on its own model or a plain gem's (its own light switch)
		std::optional<Rgb> gemRgb{};             // a star: the color of the gem it holds, the "Star light" Gem choice
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

	// each star wears its body's color, not its glowing stripes' (his 393 answer): the game's own textures read as a whole
	// (azurastarlite.dds a pale aqua, the dark azurastar.dds a cool white); in game a star is read off the model that wins it
	inline constexpr Rgb kAzurasStarRgb{ 215, 255, 253 };
	inline constexpr Rgb kBlackStarRgb{ 222, 228, 255 };
	// the gem a star holds ("Star light" on Gem): the colored part of its texture; the game's Black Star's dark gem reads pale
	// there, so it wears his own call for it, purple
	inline constexpr Rgb kAzurasStarGemRgb{ 157, 255, 252 };
	inline constexpr Rgb kBlackStarGemRgb{ 170, 90, 255 };

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
			if (a_a == 0) {
				return;
			}
			pixels += a_a / 255.0;
			plain[0] += a_r * (a_a / 255.0);
			plain[1] += a_g * (a_a / 255.0);
			plain[2] += a_b * (a_a / 255.0);
			const int top = (std::max)({ a_r, a_g, a_b }), bottom = (std::min)({ a_r, a_g, a_b });
			if (top < kDarkTop || top == bottom) {
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
				plain[i] += a_other.plain[i];
			}
			weight += a_other.weight;
			pixels += a_other.pixels;
		}

		// a shader's color (0..1, or above when over-bright) as one pixel scaled to a top channel of 255: its multiple only makes
		// it brighter. A grey, black or broken one carries no color
		void AddShaderColor(float a_r, float a_g, float a_b) noexcept
		{
			if (!std::isfinite(a_r) || !std::isfinite(a_g) || !std::isfinite(a_b)) {
				return;
			}
			const float top = (std::max)({ a_r, a_g, a_b });
			if (top <= 0.0f) {
				return;
			}
			const auto byte = [top](float a_c) { return static_cast<std::uint8_t>(std::clamp(std::round(a_c / top * 255.0f), 0.0f, 255.0f)); };
			Add(byte(a_r), byte(a_g), byte(a_b));
		}

		// pixels that carry almost no color (a grey cloud tile, clear glass): what such a texture's shape shows comes from its
		// shader's color. The game's own gem textures carry 17-128, a cloud tile 0.3
		[[nodiscard]] bool Colorless() const noexcept { return weight < kColorlessChroma * pixels || weight <= 0.0; }

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

		// every pixel counted the same, grey and dark ones too: the color the texture shows as a whole (a star's body, not its
		// glowing stripes - his 393 answer), scaled to a top channel of 255; none for a texture that is all black or see-through
		[[nodiscard]] std::optional<Rgb> Body() const noexcept
		{
			const double top = (std::max)({ plain[0], plain[1], plain[2] });
			if (pixels <= 0.0 || top <= 0.0) {
				return std::nullopt;
			}
			Rgb out{};
			for (std::size_t i = 0; i < 3; ++i) {
				out[i] = static_cast<std::uint8_t>((std::min)(255.0, std::round(plain[i] * 255.0 / top)));
			}
			return out;
		}

	private:
		static constexpr int    kDarkTop = 16;
		static constexpr double kColorlessChroma = 1.0;  // mean chroma per pixel below which a texture is grey
		std::array<double, 3>   sum{};
		std::array<double, 3>   plain{};  // every pixel's color weighed by its alpha alone (Body)
		double                  weight{ 0.0 };
		double                  pixels{ 0.0 };  // every pixel, colored or not, weighed by its alpha as the color is
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

	// The light a gem read off its model wears: its textures' color (ColorSum), steady - never its glow's animated tint. Pale = its
	// weakest channel at least kPaleShare of its strongest (the Black Star's gem reading only)
	inline constexpr double kPaleShare = 0.5;

	[[nodiscard]] inline bool IsPale(const Rgb& a_rgb) noexcept
	{
		const int top = (std::max)({ a_rgb[0], a_rgb[1], a_rgb[2] }), bottom = (std::min)({ a_rgb[0], a_rgb[1], a_rgb[2] });
		return top > 0 && bottom >= kPaleShare * top;
	}

	// a star's gem color off its texture's colored part (none read = the game's star's); a Black Star whose reading is a pale
	// blue-white holds the game's dark gem, which the texture average cannot show, so it wears kBlackStarGemRgb - a replacer's
	// pale gem of another hue keeps its own
	[[nodiscard]] inline Rgb StarGemColor(const std::optional<Rgb>& a_read, bool a_black) noexcept
	{
		const Rgb own = a_black ? kBlackStarGemRgb : kAzurasStarGemRgb;
		if (!a_read) {
			return own;
		}
		const auto& c = *a_read;
		return a_black && IsPale(c) && c[2] >= (std::max)(c[0], c[1]) ? own : c;
	}

	// a gem read off its own model wears the color read, pale or not - no pull toward pale blue (he wants each gem's own color)
	[[nodiscard]] inline Rgb ShownColor(const Rgb& a_texture, [[maybe_unused]] Kind a_kind) noexcept { return a_texture; }

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

	// the real gems Legacy of the Dragonborn puts on show (its display references, read off LegacyoftheDragonborn.esm): known by
	// their own place too, so they stay unlit where the script check cannot run (Skyrim VR)
	[[nodiscard]] inline bool KnownDisplay(std::string_view a_plugin, std::uint32_t a_localId) noexcept
	{
		constexpr std::array<std::uint32_t, 3> kLegacyDisplays{ 0x01C59B, 0x9212C6, 0x9212C7 };  // a petty soul gem, Azura's Star, the Black Star
		return SameText(a_plugin, "LegacyoftheDragonborn.esm") && std::ranges::find(kLegacyDisplays, a_localId) != kLegacyDisplays.end();
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
	// (a shared cubemap must never mark someone else's gem as a blocked mod's); the engine's plain black filler carries no color
	[[nodiscard]] inline bool ColorTexture(std::string_view a_texture)
	{
		const auto path = TexturePath(a_texture);
		if (path.starts_with("cubemaps\\") || !path.ends_with(".dds") || path == "black.dds") {
			return false;
		}
		const std::string_view stem(path.data(), path.size() - 4);
		return std::ranges::none_of(std::array<std::string_view, 8>{ "_n", "_m", "_e", "_g", "_s", "_p", "_em", "_msn" },
			[&](std::string_view a_suffix) { return stem.ends_with(a_suffix); });
	}

	// a blocked mod's name by its index; the table may hold none (every gem mod lit)
	[[nodiscard]] inline std::optional<std::string_view> BlockedModAt(std::size_t a_index) noexcept
	{
		if constexpr (kBlockedMods.empty()) {
			return std::nullopt;
		} else {
			return a_index < kBlockedMods.size() ? std::optional(kBlockedMods[a_index]) : std::nullopt;
		}
	}

	// the gem mod a file belongs to while its author has asked this mod not to light it: by its bytes ...
	[[nodiscard]] inline std::optional<std::string_view> BlockedModOf(std::uint64_t a_fingerprint) noexcept
	{
		const auto it = std::ranges::lower_bound(kBlockedFiles, a_fingerprint, {}, &BlockedFile::hash);
		if (it == kBlockedFiles.end() || it->hash != a_fingerprint) {
			return std::nullopt;
		}
		return BlockedModAt(it->mod);
	}

	// ... or by a name only such a mod ships (a model as NormalPath writes it, a texture as TexturePath does), which still
	// holds after an update of the mod or in another texture size
	[[nodiscard]] inline std::optional<std::string_view> BlockedModOfPath(std::string_view a_path) noexcept
	{
		const auto it = std::ranges::lower_bound(kBlockedPaths, a_path, {}, &BlockedPath::path);
		if (it == kBlockedPaths.end() || it->path != a_path) {
			return std::nullopt;
		}
		return BlockedModAt(it->mod);
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

	// a color set by eye in a small table keyed by a file's fingerprint, or none
	[[nodiscard]] inline std::optional<Rgb> EyeColorIn(std::span<const SeenColor> a_table, std::uint64_t a_fingerprint) noexcept
	{
		const auto it = std::ranges::find(a_table, a_fingerprint, &SeenColor::hash);
		return it == a_table.end() ? std::nullopt : std::optional(Rgb{ it->r, it->g, it->b });
	}

	// a Soul Cairn piece's color set by eye, by a file its model reads, where the gem sharing that texture wears another: Morrowind-
	// Style Soulgems' bowl and chunks pink-lavender, Better Soul Gems' keep the blue its common gem wore before that gem went teal
	inline constexpr std::array<SeenColor, 2> kCairnEyeColors{ { { 0x2B8D94EE2E85315DULL, 252, 168, 255 },
		{ 0x40F7CB51AF04EE0CULL, 98, 169, 255 } } };

	[[nodiscard]] inline std::optional<Rgb> CairnEyeColorOf(std::uint64_t a_fingerprint) noexcept
	{
		return EyeColorIn(kCairnEyeColors, a_fingerprint);
	}

	// a star's metal color set by eye, by its own color texture: the game's own Azura's Star a little more teal than its texture
	// reads - a star in another mod's texture keeps its own reading
	inline constexpr std::array<SeenColor, 1> kStarEyeColors{ { { 0x3F25E706DF4BF1D9ULL, 184, 255, 253 } } };

	// His color calls on gem mods no close picture had read, keyed by the gem's own MODEL fingerprint. A row with a choice gives
	// the menu two colors for that mod; an empty side keeps the color found for the gem. Each color is that gem's lit-room color,
	// its own glow alone in the dark, or one hue cluster of that glow.
	struct HisColor
	{
		std::uint64_t      model{ 0 };
		ColorChoice        choice{ ColorChoice::kNone };
		std::optional<Rgb> first{};   // the default
		std::optional<Rgb> second{};  // the menu's other side (with a choice only)
	};
	inline constexpr std::array<HisColor, 12> kHisColors{ {
		// MultiLayer Parallax: the yellow common gem in its own yellow glow, the purple black gem in its lit-room purple
		{ 0x0FA73CA86BC804F3ULL, ColorChoice::kNone, Rgb{ 255, 224, 109 }, std::nullopt },
		{ 0x5F778791314ED928ULL, ColorChoice::kNone, Rgb{ 179, 149, 255 }, std::nullopt },
		// Remastered Remastered's black gem (and its non-ENB patch's): its own blue glow by default, or the pink found for it
		{ 0x83B296B357133540ULL, ColorChoice::kRemRem, Rgb{ 69, 83, 255 }, std::nullopt },
		{ 0x6EF0B85853AE79FAULL, ColorChoice::kRemRem, Rgb{ 69, 83, 255 }, std::nullopt },
		// Rustic's black gem (and Rudy HQ's Rustic patch's): the blue of its glow by default, or the green of it
		{ 0xEF44C3E71611A925ULL, ColorChoice::kRustic, Rgb{ 90, 158, 255 }, Rgb{ 119, 255, 176 } },
		{ 0xD2F7DD7556D48362ULL, ColorChoice::kRustic, Rgb{ 90, 158, 255 }, Rgb{ 119, 255, 176 } },
		// NeV-P Extreme Crystals: the gem's body color under a white light by default, or its flare effect's glow in the dark
		{ 0x45DB22C948A5CD42ULL, ColorChoice::kNeVP, Rgb{ 255, 242, 184 }, Rgb{ 196, 239, 255 } },  // petty
		{ 0xD10837D75B463783ULL, ColorChoice::kNeVP, Rgb{ 227, 255, 222 }, Rgb{ 205, 255, 232 } },  // lesser
		{ 0x3E42F44E140870B7ULL, ColorChoice::kNeVP, Rgb{ 217, 223, 255 }, Rgb{ 180, 230, 255 } },  // common
		{ 0x5D4ADD91C377A27EULL, ColorChoice::kNeVP, Rgb{ 208, 240, 255 }, Rgb{ 168, 235, 255 } },  // greater
		{ 0xAD06DD431D6CAE34ULL, ColorChoice::kNeVP, Rgb{ 238, 247, 255 }, Rgb{ 112, 172, 255 } },  // grand
		{ 0x8B71C96E1B769D60ULL, ColorChoice::kNeVP, Rgb{ 243, 226, 255 }, Rgb{ 85, 102, 255 } },   // black
	} };

	// the color the game shows for a gem, measured off its own pictures (GemMeasuredColors.h), by one of its files' fingerprints
	[[nodiscard]] inline std::optional<Rgb> MeasuredColorOf(std::uint64_t a_fingerprint) noexcept
	{
		const auto it = std::ranges::lower_bound(kMeasuredColors, a_fingerprint, {}, &MeasuredColor::hash);
		return it == kMeasuredColors.end() || it->hash != a_fingerprint ? std::nullopt : std::optional(Rgb{ it->r, it->g, it->b });
	}

	[[nodiscard]] inline const HisColor* HisColorOf(std::uint64_t a_model) noexcept
	{
		const auto it = std::ranges::find(kHisColors, a_model, &HisColor::model);
		return it == kHisColors.end() ? nullptr : &*it;
	}

	// a look takes his call: its first color now, the other kept for the menu's choice; its own glow no longer applies
	// a color that stands over the gem's own glow: its light wears it on every Light color choice
	inline void WearFixedColor(Look& a_look, const Rgb& a_rgb) noexcept
	{
		a_look.rgb = a_rgb;
		a_look.glow.reset();
		a_look.glowAlways = false;
	}

	inline void ApplyHisColor(Look& a_look, const HisColor& a_his) noexcept
	{
		const Rgb found = a_look.rgb;
		WearFixedColor(a_look, a_his.first.value_or(found));
		a_look.second = a_his.second.value_or(found);
		a_look.choice = a_his.choice;
	}

	[[nodiscard]] inline std::optional<Rgb> StarEyeColorOf(std::uint64_t a_fingerprint) noexcept
	{
		return EyeColorIn(kStarEyeColors, a_fingerprint);
	}

	// a known gem file's own glow, listed where it is not the color its light wears, or none
	[[nodiscard]] inline const GlowColor* GlowColorOf(std::uint64_t a_fingerprint) noexcept
	{
		const auto it = std::ranges::lower_bound(kGlowColors, a_fingerprint, {}, &GlowColor::hash);
		return it == kGlowColors.end() || it->hash != a_fingerprint ? nullptr : &*it;
	}

	// the game's own six gem models glow these colors alone in the dark (our light off), in the game's textures and in the
	// retextures filmed (Vanilla Remastered, Skyland, Vanilla PBR with Faultier's PBR meshes) - the default of the menu's "Light
	// color": their lit-room colors lit pale blue and teal pools under violet gems
	inline constexpr std::array<std::uint64_t, 9>    kGameGemFiles{ 0x3CE5F8F2CB137059ULL, 0x40E64FA415FA17CFULL, 0x6CF55016FDB5D7A9ULL,
		0x72BA88B6D473B38FULL, 0x7946C9087AC9C2C3ULL, 0x7F3555FAF7E28371ULL, 0x9DCF4B6BA39D84BFULL, 0xB8F31CDE569730CDULL,
		0xD0BA24B919394013ULL };
	inline constexpr std::array<std::string_view, 3> kGameGemDiffuses{ "clutter\\soulgem\\soulgemcommon.dds",
		"clutter\\soulgem\\soulgemgrand.dds", "clutter\\soulgem\\soulgemblack.dds" };

	struct GameGlow
	{
		std::string_view model;
		Rgb              rgb{};
	};
	inline constexpr std::array<GameGlow, 6> kGameGlows{ { { "soulgempetty01.nif", { 159, 110, 255 } },
		{ "soulgemlesser01.nif", { 176, 116, 255 } }, { "soulgemcommon01.nif", { 206, 145, 255 } },
		{ "soulgemgreater01.nif", { 95, 106, 255 } }, { "soulgemgrand01.nif", { 86, 116, 255 } },
		{ "soulgemblack01.nif", { 197, 74, 255 } } } };

	// the own glow of a game gem model laid out as the game's (its color textures at the game's gem paths with the game's chitin
	// reflection, or all at the PBR gem paths), or none. Texture BYTES cannot tell it: every texture pack differs, and Animated
	// Soul Gems ships meshes of its own over a byte copy of the game's texture (another reflection map). A file a gem mod's
	// picture row knows (the game's own files aside) means that mod's look: its rows answer.
	// a_textures: every texture path the model names (TexturePath form); a_prints: its color textures' and its own fingerprints
	[[nodiscard]] inline std::optional<Rgb> GameGlowOf(std::string_view a_model, std::span<const std::string> a_textures,
		std::span<const std::uint64_t> a_prints)
	{
		const auto gameDiffuse = [](const std::string& a_path) { return std::ranges::find(kGameGemDiffuses, a_path) != kGameGemDiffuses.end(); };
		const auto pbr = [](const std::string& a_path) { return a_path.starts_with("pbr\\clutter\\soulgem\\soulgem"); };
		const auto modFile = [](std::uint64_t a_print) {
			return std::ranges::find(kGameGemFiles, a_print) == kGameGemFiles.end() && SeenColorOf(a_print).has_value();
		};
		auto       colors = a_textures | std::views::filter([](const std::string& a_path) { return ColorTexture(a_path); });
		const bool classic = std::ranges::all_of(colors, gameDiffuse) && std::ranges::find(a_textures, "cubemaps\\chitin_e.dds") != a_textures.end();
		if (std::ranges::empty(colors) || !(classic || std::ranges::all_of(colors, pbr)) || std::ranges::any_of(a_prints, modFile)) {
			return std::nullopt;
		}
		const auto bare = BareName(a_model);
		const auto it = std::ranges::find(kGameGlows, std::string_view(bare), &GameGlow::model);
		return it == kGameGlows.end() ? std::nullopt : std::optional(it->rgb);
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

	// the menu's light switch per gem, in menu order: the six sizes and the named quest gems in Kind order
	enum class GemSwitch : std::uint8_t
	{
		kPetty,
		kLesser,
		kCommon,
		kGreater,
		kGrand,
		kBlack,
		kNamed,
		kAzurasStar,
		kBlackStar,
		kSoulCairn,
	};
	inline constexpr std::size_t kGemSwitches = 10;

	// each switch's key in Settings.ini and DevBench (the menu's labels are in Menu.cpp, for its translation)
	inline constexpr std::array<std::string_view, kGemSwitches> kGemSwitchKeys{ "LightPetty", "LightLesser", "LightCommon", "LightGreater",
		"LightGrand", "LightBlack", "LightNamed", "LightAzurasStar", "LightBlackStar", "LightSoulCairn" };

	[[nodiscard]] inline GemSwitch SwitchOf(const Look& a_look) noexcept
	{
		if (a_look.family == Family::kSoulCairn || a_look.kind == Kind::kCairn) {
			return GemSwitch::kSoulCairn;  // the portal's bowl is a named kind of the Soul Cairn's family
		}
		if (a_look.kind == Kind::kStar) {
			return a_look.blackStar ? GemSwitch::kBlackStar : GemSwitch::kAzurasStar;
		}
		return a_look.named ? GemSwitch::kNamed : static_cast<GemSwitch>(a_look.kind);  // petty .. named: the same order
	}

	struct Settings
	{
		bool on{ true };         // every soul gem light
		int  brightness{ 100 };  // percent
		int  reach{ 100 };       // percent
		int  stars{ 50 };        // percent of Brightness and of Reach for the two stars, so every gem reads about as bright
		bool pulse{ true };      // the soft pulse
		// "Use their light": gems another mod lights keep that light and get none of ours (our sliders then leave them alone)
		bool                            theirsEmissive{ false };                                             // Emissive Soul Gems' Light Placer lights
		bool                            theirsLightPlacer{ false };                                          // any other Light Placer file's (Subtle Soul Gems', CS Light's ...)
		bool                            theirsMesh{ false };                                                 // a light in the gem's own model (an ENB or particle light: Praedy's, Subtle's, Rudy's ...)
		std::array<bool, kColorChoices> second{};                                                            // a color choice on its other side (ColorChoice order, kNone left out)
		bool                            authorPick{ false };                                                 // "Light color": false = a gem's own glow where it differs from its light, true = Author pick
		bool                            starGem{ false };                                                    // "Star light": false = a star's metal (its body), true = the gem it holds
		std::array<bool, kGemSwitches>  gems{ true, true, true, true, true, true, true, true, true, true };  // GemSwitch order
	};

	// whether this gem gets our light: the gems switch and its own (a gem switched off keeps any other mod's light on it)
	[[nodiscard]] inline bool GemLit(const Look& a_look, const Settings& a_s) noexcept
	{
		return a_s.on && a_s.gems[static_cast<std::size_t>(SwitchOf(a_look))];
	}

	// each color choice's key in Settings.ini and DevBench, and the menu's words for it and its two sides (Menu.cpp translates them)
	struct ColorChoiceText
	{
		std::string_view key, first, second;
	};
	inline constexpr std::array<ColorChoiceText, kColorChoices> kColorChoiceTexts{ {
		{ "ColorNeVP", "Gem color", "Flare glow" },
		{ "ColorRemasteredBlack", "Blue", "Pink" },
		{ "ColorRusticBlack", "Blue", "Green" },
	} };

	// A light keeps its hue and its brightest channel, its saturation lifted to s (2 - s): read beside the gems' own glow, the lights
	// found for them were paler than the gems, and the floor takes about a sixth more off. A grey light has no hue to keep
	[[nodiscard]] inline Rgb Richer(const Rgb& a_rgb) noexcept
	{
		const int top = (std::max)({ a_rgb[0], a_rgb[1], a_rgb[2] }), bottom = (std::min)({ a_rgb[0], a_rgb[1], a_rgb[2] });
		if (top <= 0 || top == bottom) {
			return a_rgb;
		}
		// each channel's distance below the top grows by the new saturation over the old one, s (2 - s) / s
		const double grow = 2.0 - static_cast<double>(top - bottom) / top;
		Rgb          out{};
		for (std::size_t i = 0; i < 3; ++i) {
			out[i] = static_cast<std::uint8_t>(std::clamp(std::round(top - (top - a_rgb[i]) * grow), 0.0, 255.0));
		}
		return out;
	}

	// the color found for a gem's light at data load; its mod's color choice on its other side wears the other color. "Light
	// color": by default a gem's own glow where it differs from that color, on Author pick the color as found
	[[nodiscard]] inline Rgb FoundColor(const Look& a_look, const Settings& a_s) noexcept
	{
		if (a_look.kind == Kind::kStar) {
			return a_s.starGem && a_look.gemRgb ? *a_look.gemRgb : a_look.rgb;
		}
		if (a_look.choice != ColorChoice::kNone) {
			return a_s.second[static_cast<std::size_t>(a_look.choice) - 1] ? a_look.second : a_look.rgb;
		}
		return a_look.glow && (a_look.glowAlways || !a_s.authorPick) ? *a_look.glow : a_look.rgb;
	}

	// the color a gem's light wears: the color found for it, richer
	[[nodiscard]] inline Rgb ColorAt(const Look& a_look, const Settings& a_s) noexcept { return Richer(FoundColor(a_look, a_s)); }

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
			} else if (SameText(key, "StarGem")) {
				a_out.starGem = v != 0;
			} else if (const auto it = std::ranges::find_if(kGemSwitchKeys, [&](std::string_view a_k) { return SameText(key, a_k); });
				it != kGemSwitchKeys.end()) {
				a_out.gems[static_cast<std::size_t>(it - kGemSwitchKeys.begin())] = v != 0;
			} else if (const auto c = std::ranges::find_if(kColorChoiceTexts, [&](const ColorChoiceText& a_c) { return SameText(key, a_c.key); });
				c != kColorChoiceTexts.end()) {
				a_out.second[static_cast<std::size_t>(c - kColorChoiceTexts.begin())] = v != 0;
			} else {
				--lines;  // a key this version does not know
			}
		}
		return lines;
	}

	inline void WriteSettings(const Settings& a_s, std::ostream& a_out)
	{
		a_out << "; Illuminated - Soul Gem Hub - written by its menu (SKSE Menu Framework). Change these in game, not here.\n"
			  << "[Settings]\nLights=" << (a_s.on ? 1 : 0) << "\nBrightness=" << a_s.brightness << "\nReach=" << a_s.reach << "\nStars=" << a_s.stars
			  << "\nPulse=" << (a_s.pulse ? 1 : 0) << "\nTheirsEmissive=" << (a_s.theirsEmissive ? 1 : 0)
			  << "\nTheirsLightPlacer=" << (a_s.theirsLightPlacer ? 1 : 0) << "\nTheirsMesh=" << (a_s.theirsMesh ? 1 : 0)
			  << "\nAuthorPick=" << (a_s.authorPick ? 1 : 0) << "\nStarGem=" << (a_s.starGem ? 1 : 0) << "\n";
		for (std::size_t i = 0; i < kGemSwitches; ++i) {
			a_out << kGemSwitchKeys[i] << '=' << (a_s.gems[i] ? 1 : 0) << '\n';
		}
		for (std::size_t i = 0; i < kColorChoices; ++i) {
			a_out << kColorChoiceTexts[i].key << '=' << (a_s.second[i] ? 1 : 0) << '\n';
		}
	}
}
