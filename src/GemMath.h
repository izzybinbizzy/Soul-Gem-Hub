// Soul Gem Hub - SKSE plugin
// Copyright (C) 2026 izzydoingit
// GPL-3.0-or-later; see LICENSE.txt and the notice at the top of main.cpp.
//
// The game-free part, so tests/testgems.cpp checks it off the game (PC Runner\plugin build.py builds and runs it):
//   text      Lower, SameText, Trim, BareName - a model path as the plugin compares it
//   light     CutoffFor, IslReach, PlainAt - Illuminated's LightMath.h, the same numbers; KeyAt - Truman's KeyframeSequence maths
//   gems      which gem a model is, its color (ColorSum: off its texture pixels) and its strength
//   settings  SoulGemHub\Settings.ini as one table, its reader and its writer
//   permits   Fingerprint, BlockedModOf - the gem mods not lit until their authors allow it (GemPermissions.h)

#pragma once

#include "GemPermissions.h"
#include "GemSeenColors.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
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
	inline constexpr float kLowestCutoff = 0.01f, kHighestCutoff = 1.0f;

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
	[[nodiscard]] inline float KeyAt(const std::vector<std::pair<float, float>>& a_keys, int a_interpolation, float a_time) noexcept
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
		static const std::vector<std::pair<float, float>> keys{ { 0.0f, 0.9f }, { kPulseSeconds / 2.0f, 1.1f }, { kPulseSeconds, 0.9f } };
		const float                                       t = std::fmod(std::fabs(a_seconds), kPulseSeconds);
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
		Family family{ Family::kGame };
		Kind   kind{ Kind::kGrand };
		Rgb    rgb{ 255, 255, 255 };
		bool   seen{ false };  // rgb is the color the gem's picture shows (GemSeenColors.h): no color maths on it
	};

	// the six sizes as a gem model names them, in Kind order
	inline constexpr std::array<std::string_view, 6> kSizeWords{ "petty", "lesser", "common", "greater", "grand", "black" };

	// Each family's color per size (petty, lesser, common, greater, grand, black): the color the gem shows is the light's color.
	// Praedy's gems show their own textures' sky blue (the black gem purple); their glow's animated pink lights only where the gems
	// light themselves (ENB, Community Shaders' particle lights), never under our light. Rustic's gems each wear their own
	// texture color (the 1K and 2K downloads within 15° of each other). The game's own gems keep the colors Illuminated gave them;
	// in game the game's family and Rustic read theirs off the model (ColorFromModel)
	inline constexpr std::array<Rgb, 6> kGameRgb{ { { 120, 150, 255 }, { 120, 150, 255 }, { 120, 150, 255 }, { 120, 150, 255 },
		{ 120, 150, 255 }, { 170, 90, 255 } } };
	inline constexpr std::array<Rgb, 6> kPraedysRgb{ { { 42, 196, 255 }, { 50, 190, 255 }, { 44, 189, 255 }, { 40, 183, 255 },
		{ 42, 183, 255 }, { 154, 44, 255 } } };
	inline constexpr std::array<Rgb, 6> kRusticRgb{ { { 128, 179, 255 }, { 76, 193, 255 }, { 127, 93, 255 }, { 26, 181, 255 },
		{ 25, 239, 255 }, { 155, 119, 255 } } };

	struct NamedModel
	{
		std::string_view path;  // the model as NormalPath writes it (a copy under another folder - a mod's decor - is not it)
		Family           family;
		Kind             kind;
		Rgb              rgb;
	};
	// the gems and pieces known by their own model (their colors are Illuminated's rows for them)
	inline constexpr std::array<NamedModel, 7> kNamedModels{ {
		{ "clutter\\soulgem\\soulgemarniel.nif", Family::kNamed, Kind::kNamed, { 185, 255, 255 } },     // Arniel's warped gems
		{ "clutter\\soulgem\\soulgemmalyn.nif", Family::kNamed, Kind::kNamed, { 255, 131, 250 } },      // Malyn's Black Soul Gem
		{ "clutter\\soulgem\\soulgemwylandriah.nif", Family::kNamed, Kind::kNamed, { 170, 90, 255 } },  // Wylandriah's Soul Gem
		{ "clutter\\soulgem\\soulgemdb.nif", Family::kNamed, Kind::kNamed, { 188, 255, 255 } },         // the Soul Essence Gem (full)
		{ "dlc01\\clutter\\dlc01soulgembowlfull.nif", Family::kSoulCairn, Kind::kCairn, { 170, 90, 255 } },
		{ "dlc01\\clutter\\dlc01soulgemchunk01.nif", Family::kSoulCairn, Kind::kCairn, { 170, 90, 255 } },
		{ "dlc01\\dungeons\\castle\\animated\\scportalbowl\\casscportalbowl01.nif", Family::kSoulCairn, Kind::kNamed, { 170, 90, 255 } },  // the portal's bowl
	} };

	inline constexpr Rgb kAzurasStarRgb{ 111, 217, 255 };  // the teal of its texture's colored panels, as it shows in game
	inline constexpr Rgb kBlackStarRgb{ 150, 70, 255 };    // its art is a dark purple

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

	// the families whose color is read off the gem's own model in game; Praedy's (read off its textures into the table), the stars,
	// the named gems and the Soul Cairn keep their own rows
	[[nodiscard]] inline bool ColorFromModel(Family a_family) noexcept
	{
		return a_family == Family::kGame || a_family == Family::kRustic;
	}

	// A texture that is a normal map, not a color (its file name ends _n, _msn or _normal, any case, before the extension). The
	// game's own soul gem models and every replacer at their paths carry an inner shape whose color slot names one
	// (soulgemcommon0_n.dds); read as a color it is a strong lavender blue that outweighed the gem's own texture.
	[[nodiscard]] inline bool IsNormalMap(std::string_view a_path)
	{
		std::string name(a_path.substr(a_path.find_last_of("/\\") == std::string_view::npos ? 0 : a_path.find_last_of("/\\") + 1));
		for (auto& c : name) {
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
		if (const auto dot = name.rfind('.'); dot != std::string::npos) {
			name.erase(dot);
		}
		return name.ends_with("_n") || name.ends_with("_msn") || name.ends_with("_normal");
	}

	// one lit shape's glow as the game shows it this frame: its emissive color and multiple
	struct Glow
	{
		float red{ 0.0f }, green{ 0.0f }, blue{ 0.0f }, mult{ 0.0f };
	};

	// The color a gem shows now: its texture color (ColorSum) tinted by the glow its shapes show this frame - every glowing shape's
	// emissive color weighed by its multiple, multiplied channel by channel with the texture color, scaled to a top channel of 255.
	// Against close pictures of eleven gem mods this is about half as far off as the texture alone or the glow alone, and every gem
	// model animates its glow, so the light follows it. Nothing glowing, or no color left after the tint: the texture color.
	// A pale gem (the game's blue-white crystals, Morrowind-Style's pastels) wears a pale blue light, not its glow's pink or
	// violet. Pale = its weakest channel at least kPaleShare of its strongest
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

	[[nodiscard]] inline Rgb LiveColor(const Rgb& a_texture, std::span<const Glow> a_glows) noexcept
	{
		if (IsPale(a_texture)) {
			return PaleBlue(a_texture);
		}
		std::array<double, 3> mix{};
		for (const auto& g : a_glows) {
			const double w = (std::max)(0.0f, g.mult);
			mix[0] += (std::max)(0.0f, g.red) * w;
			mix[1] += (std::max)(0.0f, g.green) * w;
			mix[2] += (std::max)(0.0f, g.blue) * w;
		}
		std::array<double, 3> tinted{};
		for (std::size_t i = 0; i < 3; ++i) {
			tinted[i] = a_texture[i] * mix[i];
		}
		const double top = (std::max)({ tinted[0], tinted[1], tinted[2] });
		if (!(top > 0.0)) {
			return a_texture;
		}
		Rgb out{};
		for (std::size_t i = 0; i < 3; ++i) {
			out[i] = static_cast<std::uint8_t>((std::min)(255.0, std::round(tinted[i] * 255.0 / top)));
		}
		return out;
	}

	// the game's own look for a gem of this size (a_black: it holds black souls)
	[[nodiscard]] inline Look GameLook(Kind a_size, bool a_black) noexcept
	{
		const Kind kind = a_black ? Kind::kBlack : a_size;
		return Look{ Family::kGame, kind, SizeRgb(kGameRgb, kind) };
	}

	// Emissive Soul Gems (iamzachhunter, Nexus 191137): the models its Light Placer file lights. The two mods work together: with
	// Light Placer loaded these models are his, whether or not his mod is installed; without Light Placer (which his mod needs)
	// this mod lights them.
	// Whole paths, as Light Placer matches them: a replacer's copy in another folder (MLP's and Vayura's "full\", Spherical's
	// "Filled\") is not his - its own Light Placer file, if any, claims it
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

	// who lights a gem: this plugin, or a light mod it leaves the gem to
	enum class Owner : std::uint8_t
	{
		kThisMod,
		kEmissive,     // Light Placer is loaded and the model is Emissive Soul Gems'
		kLightPlacer,  // Light Placer is loaded and one of its configs claims the gem
		kReLight,      // RE::Light is loaded and this mod's RE::Light configs light the gem
	};

	[[nodiscard]] inline Owner OwnerOf(std::string_view a_model, Family a_family, bool a_lightPlacer, bool a_claimedByLightPlacer,
		bool a_reLight)
	{
		if (a_lightPlacer && EmissiveModel(a_model)) {
			return Owner::kEmissive;
		}
		if (a_lightPlacer && a_claimedByLightPlacer) {
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

	// every texture a model file names, read off its bytes (a NIF keeps each path as plain text after its length), as
	// TexturePath writes it - no model loader needed, so it works on VR too
	[[nodiscard]] inline std::vector<std::string> TexturesNamed(std::span<const std::uint8_t> a_nif)
	{
		std::vector<std::string> out;
		std::size_t              start = 0;
		for (std::size_t i = 0; i <= a_nif.size(); ++i) {
			if (i < a_nif.size() && a_nif[i] >= 0x20 && a_nif[i] < 0x7F) {
				continue;
			}
			const std::string_view run(reinterpret_cast<const char*>(a_nif.data()) + start, i - start);
			if (run.size() > 4 && SameText(run.substr(run.size() - 4), ".dds")) {
				if (auto path = TexturePath(run); std::ranges::find(out, path) == out.end()) {
					out.push_back(std::move(path));
				}
			}
			start = i + 1;
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
	[[nodiscard]] inline Shown ShownFor(const Strength& a_s, bool a_inverseSquare, float a_brightness, float a_reach,
		float a_pulse = 1.0f) noexcept
	{
		const float b = (std::max)(a_brightness, 0.0f), r = (std::max)(a_reach, 0.05f), p = (std::max)(a_pulse, 0.0f);
		if (a_inverseSquare) {
			const float cutoff = std::clamp(a_s.cutoff * b / (r * r), kLowestCutoff, kHighestCutoff);
			const float fade = a_s.fade * b;
			return { fade * p, (std::max)(IslReach(fade, cutoff), 1.0f), cutoff };
		}
		const auto plain = PlainAt(a_s.fade * b * p, 0.0f, IslReach(a_s.fade, a_s.cutoff) * r);
		return { plain.fade, (std::max)(plain.radius, 1.0f), 0.0f };
	}

	// ------------------------------------------------------------------ settings (SoulGemHub\Settings.ini, written by the menu)
	inline constexpr int kBrightnessMin = 25, kBrightnessMax = 200, kReachMin = 50, kReachMax = 150;

	struct Settings
	{
		bool on{ true };         // every soul gem light
		int  brightness{ 100 };  // percent
		int  reach{ 100 };       // percent
		bool pulse{ true };      // the soft pulse
	};

	// the file's text over a_out's values (the defaults, for a fresh table); keys in any case; a number that does not read
	// keeps the value it had; every number clamped to its slider. Returns the key lines read.
	inline std::size_t ReadSettings(std::istream& a_in, Settings& a_out)
	{
		std::size_t lines = 0;
		std::string line;
		while (a_in && std::getline(a_in, line)) {
			const auto text = Trim(line);
			if (text.empty() || text.front() == ';' || text.front() == '#' || text.front() == '[') {
				continue;
			}
			const auto eq = text.find('=');
			if (eq == std::string_view::npos) {
				continue;
			}
			const auto key = Trim(text.substr(0, eq));
			const auto val = Trim(text.substr(eq + 1));
			int        v = 0;
			const auto [end, ec] = std::from_chars(val.data(), val.data() + val.size(), v);
			if (ec != std::errc{} || end != val.data() + val.size()) {
				continue;
			}
			++lines;
			if (SameText(key, "Lights")) {
				a_out.on = v != 0;
			} else if (SameText(key, "Brightness")) {
				a_out.brightness = std::clamp(v, kBrightnessMin, kBrightnessMax);
			} else if (SameText(key, "Reach")) {
				a_out.reach = std::clamp(v, kReachMin, kReachMax);
			} else if (SameText(key, "Pulse")) {
				a_out.pulse = v != 0;
			} else {
				--lines;  // a key this version does not know
			}
		}
		return lines;
	}

	inline void WriteSettings(const Settings& a_s, std::ostream& a_out)
	{
		a_out << "; Illuminated - Soul Gem Hub - written by its menu (SKSE Menu Framework). Change these in game, not here.\n"
			  << "[Settings]\nLights=" << (a_s.on ? 1 : 0) << "\nBrightness=" << a_s.brightness << "\nReach=" << a_s.reach
			  << "\nPulse=" << (a_s.pulse ? 1 : 0) << "\n";
	}
}
