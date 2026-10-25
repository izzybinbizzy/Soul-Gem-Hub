// Soul Gem Hub - tests for src/GemMath.h (no game needed).
// Copyright (C) 2026 izzydoingit. GPL-3.0-or-later.
//
// PC Runner\plugin build.py builds and runs them (xmake target testgems), then again under clang's address and
// undefined-behavior checkers. The run fails when a check fails, AND when fewer checks ran than kFloor (a test that stopped
// part way proves nothing - Dynamic Wards' and Illuminated's tests count theirs the same way).

#include "GemMath.h"

#include <cstdio>
#include <sstream>
#include <string>

namespace
{
	int gFailed = 0, gPassed = 0;

	void Check(bool a_ok, const char* a_what, int a_line)
	{
		if (a_ok) {
			++gPassed;
		} else {
			++gFailed;
			if (gFailed <= 30) {
				std::printf("FAIL line %d: %s\n", a_line, a_what);
			}
		}
	}
#define CHECK(x) Check((x), #x, __LINE__)

	// the fewest checks a whole run makes (raise it when checks are added; lower it only when checks are removed on purpose)
	constexpr int kFloor = 151;

	using namespace Gems;

	bool Near(float a, float b, float a_tol = 0.01f) { return std::fabs(a - b) <= a_tol; }

	void Text()
	{
		CHECK(Lower("SoulGemGrand01Full.NIF") == "soulgemgrand01full.nif");
		CHECK(SameText("Lights", "LIGHTS"));
		CHECK(!SameText("Light", "Lights"));
		CHECK(Trim("  a b \t") == "a b");
		CHECK(Trim("   ").empty());
		CHECK(BareName("Clutter\\SoulGem\\SoulGemGrand01Full.nif") == "soulgemgrand01full.nif");
		CHECK(BareName("meshes/clutter/soulgem/soulgemblack_full.nif") == "soulgemblack_full.nif");
		CHECK(BareName("soulgemdb.nif") == "soulgemdb.nif");
		CHECK(NormalPath("Meshes/DLC01/Clutter/DLC01SoulGemBowlFull.nif") == "dlc01\\clutter\\dlc01soulgembowlfull.nif");
		CHECK(NormalPath("\\Clutter\\SoulGem\\SoulgemArniel.nif") == "clutter\\soulgem\\soulgemarniel.nif");
	}

	void Light()
	{
		// a grand gem on Community Shaders reaches about 81 units, a petty one about 56 - closer together than before (his look)
		const auto& grand = kStrength[static_cast<std::size_t>(Kind::kGrand)];
		CHECK(Near(IslReach(grand.fade, grand.cutoff), 81.0f, 1.5f));
		CHECK(Near(IslReach(0.5472f, 0.95f), 47.5f, 0.5f));  // the half-reach row
		const auto& petty = kStrength[static_cast<std::size_t>(Kind::kPetty)];
		CHECK(Near(IslReach(petty.fade, petty.cutoff), 56.0f, 1.5f));
		CHECK(IslReach(0.5f, 0.0f) == 0.0f);
		CHECK(IslReach(0.0f, 0.5f) == 0.0f);
		CHECK(Near(CutoffFor(grand.fade, IslReach(grand.fade, grand.cutoff)), grand.cutoff, 0.001f));
		CHECK(CutoffFor(10.0f, 1.0f) == kHighestCutoff);
		CHECK(CutoffFor(0.001f, 1000.0f) == kLowestCutoff);
		const auto p = PlainAt(1.0f, 0.0f, 133.0f);
		CHECK(Near(p.fade, 1.14f) && Near(p.radius, 178.0f));
		CHECK(Near(PlainAt(1.0f, 500.0f, 133.0f).radius, 500.0f));  // never shorter than the radius already stated
		// the soft pulse: 0.9 at the ends, 1.1 in the middle, smooth, the same every loop
		CHECK(Near(PulseAt(0.0f), 0.9f));
		CHECK(Near(PulseAt(kPulseSeconds / 2.0f), 1.1f));
		CHECK(Near(PulseAt(kPulseSeconds), 0.9f));
		CHECK(Near(PulseAt(1.0f), PulseAt(1.0f + kPulseSeconds)));
		CHECK(PulseAt(1.0f) > 0.9f && PulseAt(1.0f) < 1.1f);
		CHECK(Near(PulseAt(-1.0f), PulseAt(1.0f)));
		const std::vector<std::pair<float, float>> keys{ { 0.0f, 0.0f }, { 1.0f, 1.0f } };
		CHECK(KeyAt(keys, 0, 0.5f) == 0.0f);
		CHECK(Near(KeyAt(keys, 1, 0.25f), 0.25f));
		CHECK(Near(KeyAt(keys, 2, 0.5f), 0.5f));
		CHECK(KeyAt({}, 1, 0.5f) == 0.0f);
		CHECK(KeyAt(keys, 1, -1.0f) == 0.0f);
		CHECK(KeyAt(keys, 1, 9.0f) == 1.0f);
	}

	void Strengths()
	{
		const auto& grand = kStrength[static_cast<std::size_t>(Kind::kGrand)];
		// the slider's percent as a factor: Vanilla a quarter brighter at every setting, the other lightings as set
		CHECK(Near(BrightnessFor(100, false), 1.0f) && Near(BrightnessFor(100, true), 1.25f) && Near(BrightnessFor(200, true), 2.5f));
		CHECK(Near(BrightnessFor(25, false), 0.25f));
		// the light's numbers as the light code asks for them: Vanilla a quarter brighter than ENB at the same settings, the reach the same
		Settings   set;
		const auto vanilla = ShownAt(Kind::kGrand, false, true, set), enb = ShownAt(Kind::kGrand, false, false, set);
		CHECK(Near(vanilla.fade, enb.fade * 1.25f) && Near(vanilla.radius, enb.radius, 0.5f));
		CHECK(Near(ShownAt(Kind::kGrand, true, false, set).fade, grand.fade));  // Community Shaders as built
		set.brightness = 200;
		CHECK(Near(ShownAt(Kind::kGrand, false, true, set).fade, enb.fade * 2.5f));
		// Community Shaders, as built: the record's numbers
		auto s = ShownFor(grand, true, 1.0f, 1.0f);
		CHECK(Near(s.fade, grand.fade) && Near(s.cutoff, grand.cutoff) && Near(s.radius, IslReach(grand.fade, grand.cutoff), 0.5f));
		// Brightness changes how bright, not how far
		auto b = ShownFor(grand, true, 2.0f, 1.0f);
		CHECK(Near(b.fade, grand.fade * 2.0f));
		CHECK(Near(b.radius, s.radius, 0.6f) || b.cutoff == kHighestCutoff);
		auto d = ShownFor(grand, true, 0.5f, 1.0f);
		CHECK(Near(d.radius, s.radius, 0.6f));
		// Reach changes how far
		auto r = ShownFor(grand, true, 1.0f, 1.5f);
		CHECK(r.radius > s.radius * 1.4f);
		// on ENB / Vanilla the same light drawn plain: a longer radius, a little more fade, no cutoff
		auto plain = ShownFor(grand, false, 1.0f, 1.0f);
		CHECK(Near(plain.fade, grand.fade * 1.14f) && Near(plain.radius, s.radius * 178.0f / 133.0f, 1.0f) && plain.cutoff == 0.0f);
		auto plainReach = ShownFor(grand, false, 1.0f, 0.5f);
		CHECK(Near(plainReach.radius, plain.radius / 2.0f, 1.0f));
		CHECK(ShownFor(grand, false, 0.0f, 1.0f).fade == 0.0f);
		CHECK(ShownFor(grand, true, 1.0f, 0.0f).radius >= 1.0f);  // never a light of no size
		// the pulse moves the fade only: the cutoff and the radius stay put while the light breathes, even at the clamp
		auto peak = ShownFor(grand, true, 1.0f, 1.0f, 1.1f);
		CHECK(Near(peak.fade, grand.fade * 1.1f) && peak.cutoff == s.cutoff && peak.radius == s.radius);
		auto hot = ShownFor(grand, true, 2.0f, 1.0f, 1.1f), hotFlat = ShownFor(grand, true, 2.0f, 1.0f);
		CHECK(hot.radius == hotFlat.radius && hot.cutoff == hotFlat.cutoff);
		CHECK(ShownFor(grand, false, 1.0f, 1.0f, 1.1f).radius == plain.radius);
		// every strength steps up from petty to grand, the stars and the Soul Cairn the strongest
		for (std::size_t i = 1; i <= static_cast<std::size_t>(Kind::kGrand); ++i) {
			CHECK(kStrength[i].fade > kStrength[i - 1].fade);
		}
		CHECK(kStrength[static_cast<std::size_t>(Kind::kStar)].fade > grand.fade);
	}

	void ModelColors()
	{
		ColorSum none;
		CHECK(!none.Result());
		none.Add(128, 128, 128);  // grey
		none.Add(255, 255, 255);  // white
		none.Add(10, 2, 4);       // nearly black
		none.Add(255, 0, 0, 0);   // see-through
		CHECK(!none.Result());
		// one colored pixel: its hue, scaled to a top channel of 255
		ColorSum one;
		one.Add(120, 60, 30);
		CHECK(one.Result() == (Rgb{ 255, 128, 64 }));
		// grey pixels beside it change nothing
		one.Add(200, 200, 200);
		CHECK(one.Result() == (Rgb{ 255, 128, 64 }));
		// a strongly colored pixel outweighs a pale one
		ColorSum mix;
		mix.Add(255, 0, 0);
		mix.Add(200, 190, 255);
		const auto m = mix.Result();
		CHECK(m && (*m)[0] == 255 && (*m)[1] < 60);
		// half alpha weighs half
		ColorSum alpha;
		alpha.Add(0, 0, 255, 255);
		alpha.Add(0, 255, 0, 128);
		const auto a = alpha.Result();
		CHECK(a && (*a)[2] == 255 && (*a)[1] > 120 && (*a)[1] < 135);
		// the families read off the model, and the ones that keep their own rows
		CHECK(ColorFromModel(Family::kGame) && ColorFromModel(Family::kRustic) && ColorFromModel(Family::kStar));
		CHECK(!ColorFromModel(Family::kPraedys) && !ColorFromModel(Family::kNamed) && !ColorFromModel(Family::kSoulCairn));
		// a normal map in a color slot never counts as a color; real textures do
		CHECK(IsNormalMap("textures\\clutter\\soulgem\\soulgemcommon0_n.dds") && IsNormalMap("Textures/X/Gem_MSN.DDS") && IsNormalMap("gem_normal.dds"));
		CHECK(!IsNormalMap("textures\\clutter\\soulgem\\soulgemlesser0.dds") && !IsNormalMap("textures\\n.dds") && !IsNormalMap("gem_nice.dds"));
		// two sums merged weigh as one sum of every pixel
		ColorSum merged, redSum, blueSum;
		redSum.Add(255, 0, 0);
		blueSum.Add(0, 0, 255);
		merged.Add(redSum);
		merged.Add(blueSum);
		CHECK(merged.Result() == (Rgb{ 255, 0, 255 }));
		// the light a model read gives: a colored texture as it is (the game's grand crystal's teal, never its glow's violet)
		const Rgb teal{ 83, 255, 234 };
		CHECK(ShownColor(teal, Kind::kGrand) == teal);
		// a pale gem (the game's soulgemcommon texture, 154,241,255) wears a pale blue (his "more paler blue")
		const Rgb paleCyan{ 154, 241, 255 };
		CHECK(IsPale(paleCyan) && !IsPale(Rgb{ 255, 38, 25 }) && !IsPale(Rgb{ 0, 0, 0 }));
		CHECK(ShownColor(paleCyan, Kind::kCommon) == (Rgb{ 152, 210, 255 }));
		CHECK(ShownColor(Rgb{ 255, 200, 230 }, Kind::kPetty)[2] == 255);  // a pale pink pastel turns blue-white, blue on top
		// a black-soul gem keeps its own purple, a star its own texture's color, pale or not
		const Rgb lilac{ 184, 132, 255 };
		CHECK(IsPale(lilac) && ShownColor(lilac, Kind::kBlack) == lilac);
		const Rgb starPale{ 157, 255, 252 };
		CHECK(ShownColor(starPale, Kind::kStar) == starPale);
	}

	void Looks()
	{
		CHECK(SizeOfName("soulgemgrand01full.nif") == Kind::kGrand);
		CHECK(SizeOfName("soulgempetty_full.nif") == Kind::kPetty);
		CHECK(SizeOfName("soulgemblack01fullcommon.nif") == Kind::kBlack);
		CHECK(!SizeOfName("soulgemarniel.nif"));
		CHECK(!SizeOfName("azurastar.nif"));
		// Praedy's filled gems (and Subtle's copies) by their 01Full models
		auto p = LookOfModel("clutter\\soulgem\\soulgemgrand01Full.nif", false);
		// their colors are their own textures' (sky blue, the black gem purple), checked as values, not against the table
		const Rgb praedyGrand{ 42, 183, 255 };
		const Rgb praedyBlack{ 154, 44, 255 };
		const Rgb praedyPetty{ 42, 196, 255 };
		CHECK(p && p->family == Family::kPraedys && p->kind == Kind::kGrand && p->rgb == praedyGrand);
		auto pb = LookOfModel("clutter\\soulgem\\soulgemblack01Full.nif", true);
		CHECK(pb && pb->family == Family::kPraedys && pb->kind == Kind::kBlack && pb->rgb == praedyBlack);
		auto pp = LookOfModel("clutter\\soulgem\\soulgempetty01Full.nif", false);
		CHECK(pp && pp->kind == Kind::kPetty && pp->rgb == praedyPetty);
		CHECK(LookOfModel("clutter\\soulgem\\soulgemgrand01fullcommon.nif", false)->family == Family::kPraedys);
		// Rustic's _full models only with its plugin loaded (another mod ships the same names)
		auto r = LookOfModel("Clutter\\SoulGem\\SoulGemLesser_full.nif", true);
		CHECK(r && r->family == Family::kRustic && r->kind == Kind::kLesser && r->rgb == kRusticRgb[1]);
		CHECK(!LookOfModel("Clutter\\SoulGem\\SoulGemLesser_full.nif", false));
		// the game's own models: no look of their own - the caller uses the size's
		CHECK(!LookOfModel("Clutter\\SoulGem\\SoulGemGrand01.nif", true));
		CHECK(!LookOfModel("", true));
		// the named gems and the Soul Cairn's pieces by their own model
		auto a = LookOfModel("clutter\\soulgem\\SoulgemArniel.nif", false);
		CHECK(a && a->family == Family::kNamed && a->kind == Kind::kNamed);
		auto m = LookOfModel("clutter\\soulgem\\SoulgemMalyn.nif", false);
		CHECK((m && m->rgb == Rgb{ 131, 200, 255 }));  // Praedy's Malyn.dds sky blue, no longer Illuminated's old pink row
		auto bowl = LookOfModel("DLC01\\Clutter\\DLC01SoulGemBowlFull.nif", false);
		CHECK(bowl && bowl->family == Family::kSoulCairn && bowl->kind == Kind::kCairn);
		CHECK(LookOfModel("dlc01\\dungeons\\castle\\animated\\scportalbowl\\casscportalbowl01.nif", false)->kind == Kind::kNamed);
		CHECK(!LookOfModel("DLC01\\Clutter\\DLC01SoulGemBowlEmpty.nif", false));
		CHECK(!LookOfModel("_pgpatcher_dups\\1\\dlc01\\clutter\\dlc01soulgemchunk01.nif", false));  // a decor copy is not the Soul Cairn's piece
		CHECK(LookOfModel("meshes\\dlc01\\clutter\\dlc01soulgemchunk01.nif", false)->family == Family::kSoulCairn);
		// what RE::Light's keys reach: a decor copy of a piece, Praedy's filled gems; never the game's own gem models
		CHECK(IsDisplayScript("dbm_dynamicdisplayscript"));
		CHECK(IsDisplayScript("MuseumDisplayCaseScript"));
		CHECK(!IsDisplayScript("defaultDisableHavokOnLoad"));
		CHECK(!IsDisplayScript("dunYngvildSoulgemScript"));
		CHECK(ReLightKeyed("_pgpatcher_dups\\1\\dlc01\\clutter\\dlc01soulgemchunk01.nif"));
		CHECK(ReLightKeyed("clutter\\soulgem\\soulgemgrand01fullcommon.nif"));
		CHECK(!ReLightKeyed("_pgpatcher_dups\\1\\clutter\\soulgem\\soulgemcommon01.nif"));
		CHECK(!ReLightKeyed("clutter\\soulgem\\soulgemdb.nif"));              // its config names the record, not the model
		CHECK(!LookOfModel("clutter\\soulgem\\SoulGemDB Empty.nif", false));  // Praedy's empty Soul Essence Gem stays dark
		// the game's look by size
		CHECK(GameLook(Kind::kCommon, false).rgb == kGameRgb[2] && GameLook(Kind::kCommon, false).kind == Kind::kCommon);
		CHECK(GameLook(Kind::kGrand, true).kind == Kind::kBlack && GameLook(Kind::kGrand, true).rgb == kGameRgb[5]);
		CHECK(FamilyName(Family::kRustic) == "Rustic Soul Gems");
		CHECK(SizeRgb(kGameRgb, Kind::kStar) == kGameRgb[5]);  // a kind past the six sizes takes the last
	}

	void Owners()
	{
		// Emissive Soul Gems' models are his whenever Light Placer is loaded (his rule 2026-10-19)
		CHECK(EmissiveModel("Clutter\\SoulGem\\SoulGemGrand_full.nif"));
		CHECK(EmissiveModel("clutter\\daedric\\azurastar.nif"));
		CHECK(EmissiveModel("clutter\\soulgem\\soulgempetty01.nif"));
		CHECK(!EmissiveModel("clutter\\soulgem\\soulgemgrand01full.nif"));
		CHECK(EmissiveModel("Meshes/Clutter/SoulGem/SoulGemPetty01.nif"));
		// a replacer's copy in another folder is not Emissive Soul Gems' (Light Placer matches whole paths)
		CHECK(!EmissiveModel("Clutter\\SoulGem\\full\\SoulGemGrand01.nif"));
		CHECK(!EmissiveModel("Clutter\\SoulGem\\Filled\\SoulGemBlack01.nif"));
		CHECK(OwnerOf("Clutter\\SoulGem\\full\\SoulGemGrand01.nif", Family::kGame, true, false, false) == Owner::kThisMod);
		// only Rustic's own names take Rustic's look, never another replacer's "..01_full.nif"
		CHECK(!LookOfModel("clutter\\soulgem\\soulgemgrand01_full.nif", true));
		CHECK(!LookOfModel("Clutter\\SoulGem\\Filled\\SoulGemGrand_full.nif", true));
		const auto rusticGrand = LookOfModel("Meshes/Clutter/SoulGem/SoulGemGrand_full.nif", true);
		CHECK(rusticGrand && rusticGrand->family == Family::kRustic);
		CHECK(OwnerOf("Clutter\\SoulGem\\SoulGemGrand_full.nif", Family::kRustic, true, false, false) == Owner::kEmissive);
		CHECK(OwnerOf("Clutter\\SoulGem\\SoulGemGrand_full.nif", Family::kRustic, false, false, false) == Owner::kThisMod);
		CHECK(OwnerOf("Clutter\\SoulGem\\SoulGemGrand_full.nif", Family::kRustic, false, false, true) == Owner::kThisMod);
		// another Light Placer config's gem is left to it, but only with Light Placer loaded
		CHECK(OwnerOf("clutter\\soulgem\\soulgemgrand01full.nif", Family::kPraedys, true, true, false) == Owner::kLightPlacer);
		CHECK(OwnerOf("clutter\\soulgem\\soulgemgrand01full.nif", Family::kPraedys, false, true, false) == Owner::kThisMod);
		// this mod's RE::Light files light Praedy's, the named gems and the Soul Cairn's pieces when RE::Light is loaded
		CHECK(OwnerOf("clutter\\soulgem\\soulgemgrand01full.nif", Family::kPraedys, false, false, true) == Owner::kReLight);
		CHECK(OwnerOf("clutter\\soulgem\\soulgemarniel.nif", Family::kNamed, false, false, true) == Owner::kReLight);
		CHECK(OwnerOf("Clutter\\SoulGem\\SoulGemGrand01.nif", Family::kGame, false, false, true) == Owner::kThisMod);
		CHECK(OwnerOf("clutter\\daedric\\azurastar.nif", Family::kStar, false, false, true) == Owner::kThisMod);
		CHECK(OwnerOf("clutter\\soulgem\\soulgemgrand01full.nif", Family::kPraedys, false, false, false) == Owner::kThisMod);
	}

	// a Light Placer formIDs token against a form (Claimed in GemLights.cpp)
	void Tokens()
	{
		CHECK(TokenNamesForm("0x2e4ff~skyrim.esm", "skyrim.esm", 0x0002E4FF));
		CHECK(TokenNamesForm("0002e4ff~skyrim.esm", "skyrim.esm", 0x0002E4FF));
		CHECK(!TokenNamesForm("0x2e4ff~dawnguard.esm", "skyrim.esm", 0x0002E4FF));
		CHECK(!TokenNamesForm("soulgemgrandfilled", "skyrim.esm", 0x0002E4FF));
		CHECK(!TokenNamesForm("0x~skyrim.esm", "skyrim.esm", 0));
		CHECK(!TokenNamesForm("0xzz~skyrim.esm", "skyrim.esm", 0));
	}

	void SettingsText()
	{
		Settings s;
		// ENBParticleGems: the first test build's switch (gone 2026-10-19 - a gem's own light is always left to it); an old file's
		// line is a key this version does not know
		std::stringstream in("; a comment\n[Settings]\nLights=0\nbrightness=150\nREACH=500\nPulse=0\nENBParticleGems=1\nUnknown=3\nBrightness=x\n");
		CHECK(ReadSettings(in, s) == 4);
		CHECK(!s.on && s.brightness == 150 && s.reach == kReachMax && !s.pulse);
		std::stringstream out;
		WriteSettings(s, out);
		Settings          back;
		std::stringstream again(out.str());
		CHECK(ReadSettings(again, back) == 4);
		CHECK(back.on == s.on && back.brightness == s.brightness && back.reach == s.reach && back.pulse == s.pulse);
		Settings          d;
		std::stringstream empty("");
		CHECK(ReadSettings(empty, d) == 0);
		CHECK(d.on && d.brightness == 100 && d.reach == 100 && d.pulse);
		std::stringstream low("Brightness=1\nReach=-5\n");
		CHECK(ReadSettings(low, d) == 2 && d.brightness == kBrightnessMin && d.reach == kReachMin);
	}
}

namespace
{
	void Permits()
	{
		const std::uint8_t a[] = { 'a' };
		CHECK(Fingerprint(a) == 0x089BE307B544F397ULL);
		CHECK(Fingerprint({}) == (0xCBF29CE484222325ULL * 0x100000001B3ULL));
		std::vector<std::uint8_t> big(kFingerprintHead + 10, 7);
		CHECK(Fingerprint(big) != Fingerprint(std::span(big).first(kFingerprintHead)));
		CHECK(std::ranges::is_sorted(kBlockedFiles, {}, &BlockedFile::hash));
		CHECK(std::ranges::all_of(kBlockedFiles, [](const BlockedFile& f) { return f.mod < kBlockedMods.size(); }));
		CHECK(BlockedModOf(kBlockedFiles.front().hash) == kBlockedMods[kBlockedFiles.front().mod]);
		CHECK(BlockedModOf(kBlockedFiles.back().hash) == kBlockedMods[kBlockedFiles.back().mod]);
		CHECK(!BlockedModOf(Fingerprint(a)));
		// the picture colors: sorted, every one found by its own fingerprint, a stranger file not
		CHECK(std::ranges::is_sorted(kSeenColors, {}, &SeenColor::hash));
		CHECK(SeenColorOf(kSeenColors.front().hash) == (Rgb{ kSeenColors.front().r, kSeenColors.front().g, kSeenColors.front().b }));
		CHECK(SeenColorOf(kSeenColors.back().hash) == (Rgb{ kSeenColors.back().r, kSeenColors.back().g, kSeenColors.back().b }));
		CHECK(!SeenColorOf(Fingerprint(a)));
		CHECK(std::ranges::none_of(kSeenColors, [](const SeenColor& s) { return BlockedModOf(s.hash).has_value(); }));
		FingerprintSum pieces;
		pieces.Add(std::span(big).first(40000));
		pieces.Add(std::span(big).subspan(40000));
		CHECK(pieces.Result() == Fingerprint(big) && pieces.Size() == big.size());
		CHECK(TexturePath("Data\\Textures/Clutter/SoulGem/Full/X.DDS") == "clutter\\soulgem\\full\\x.dds");
		CHECK(TexturePath("clutter\\x.dds") == "clutter\\x.dds");
		CHECK(std::ranges::is_sorted(kBlockedPaths, {}, &BlockedPath::path));
		CHECK(std::ranges::all_of(kBlockedPaths, [](const BlockedPath& p) { return p.mod < kBlockedMods.size() && p.path == Lower(p.path); }));
		CHECK(BlockedModOfPath(kBlockedPaths.front().path) == kBlockedMods[kBlockedPaths.front().mod]);
		CHECK(!BlockedModOfPath("clutter\\soulgem\\soulgemgrand01.nif"));  // the game's own model is never a blocked name
		const auto        field = [](std::string_view a_text) { return std::string(1, static_cast<char>(a_text.size())) + std::string(3, '\0') + std::string(a_text); };
		const std::string nif = field("NiNode") + field("Textures\\Clutter\\A\\Gem.DDS") + field("textures\\clutter\\a\\gem_n.dds") +
		                        field("textures\\clutter\\a\\gem.dds");
		const auto        named = TexturesNamed(std::span(reinterpret_cast<const std::uint8_t*>(nif.data()), nif.size()));
		CHECK(named.size() == 2 && named[0] == "clutter\\a\\gem.dds" && named[1] == "clutter\\a\\gem_n.dds");
		CHECK(ColorTexture("textures\\clutter\\a\\gem.dds") && !ColorTexture("clutter\\a\\gem_n.dds") && !ColorTexture("cubemaps\\shinyglass_e.dds"));
		CHECK(TexturesNamed({}).empty());
	}
}

int main()
{
	Text();
	Light();
	Strengths();
	ModelColors();
	Looks();
	Owners();
	Tokens();
	SettingsText();
	Permits();
	std::printf("%d passed, %d failed\n", gPassed, gFailed);
	if (gPassed < kFloor) {
		std::printf("FAIL: only %d checks ran, under the floor of %d - a test stopped part way\n", gPassed, kFloor);
		return 1;
	}
	return gFailed ? 1 : 0;
}
