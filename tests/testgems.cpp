// Soul Gem Hub - tests for src/GemMath.h (no game needed).
// Copyright (C) 2026 izzydoingit. GPL-3.0-or-later.
//
// PC Runner\plugin build.py builds and runs them (xmake target testgems), then again under clang's address and
// undefined-behavior checkers. The run fails when a check fails, AND when fewer checks ran than kFloor (a test that stopped
// part way proves nothing - Dynamic Wards' and Illuminated's tests count theirs the same way).

#include "FloorLightChecks.h"
#include "GemMath.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <format>
#include <initializer_list>
#include <limits>
#include <sstream>
#include <string>
#include <utility>

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
	constexpr int kFloor = 243 + FloorLightChecks::kCount;

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
		// his report: at Brightness 200 % the Reach slider jumped - the cutoff never reaches 1 (Community Shaders' "default"), and
		// on every lighting, kind and Brightness a higher Reach never draws a shorter light
		CHECK(kHighestCutoff < 1.0f && ShownFor(grand, true, 2.0f, 0.75f).cutoff < 1.0f);
		CHECK(ShownFor(grand, true, 2.0f, 0.75f).radius < ShownFor(grand, true, 2.0f, 1.0f).radius);
		bool steady = true;
		for (const auto& strength : kStrength) {
			for (int brightness = kBrightnessMin; brightness <= kBrightnessMax; brightness += 25) {
				for (const bool isl : { true, false }) {
					float last = 0.0f;
					for (int reach = kReachMin; reach <= kReachMax; reach += 5) {
						const auto shown = ShownFor(strength, isl, brightness / 100.0f, reach / 100.0f);
						steady = steady && shown.radius >= last - 0.01f && shown.cutoff < 1.0f;
						last = shown.radius;
					}
				}
			}
		}
		CHECK(steady);
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
		// a grey texture (Animated's soul cloud tile, mean chroma 0.3) is colorless; a soul gem texture is not, nor is a mix
		ColorSum cloud;
		for (int i = 0; i < 300; ++i) {
			cloud.Add(200, 200, 200);
		}
		cloud.Add(201, 200, 199);
		CHECK(cloud.Colorless() && ColorSum{}.Colorless());
		ColorSum crystal;
		for (int i = 0; i < 300; ++i) {
			crystal.Add(150, 230, 255);
		}
		CHECK(!crystal.Colorless());
		crystal.Add(cloud);
		CHECK(!crystal.Colorless());
		// a faint but colored glow (alpha 20) is no grey texture: pixels weigh by alpha as the color does
		ColorSum faint;
		for (int i = 0; i < 300; ++i) {
			faint.Add(0, 0, 255, 20);
			faint.Add(0, 0, 0, 0);
		}
		CHECK(!faint.Colorless());
		// a shader's color scaled to a top channel of 255 (Animated Filled's halo 0.1475, 0.4517, 1.0; its black gem's violet)
		ColorSum halo, violet, grey, broken;
		halo.AddShaderColor(0.1475f, 0.4517f, 1.0f);
		violet.AddShaderColor(0.2915f, 0.002f, 0.6497f);
		CHECK(halo.Result() == (Rgb{ 38, 115, 255 }) && violet.Result() == (Rgb{ 114, 1, 255 }));
		// over-bright or dark, the same hue; grey, black, negative or broken carries none
		ColorSum bright, dim;
		bright.AddShaderColor(0.2f, 0.4f, 2.0f);
		dim.AddShaderColor(0.05f, 0.1f, 0.5f);
		CHECK(bright.Result() == dim.Result() && bright.Result() == (Rgb{ 26, 51, 255 }));
		grey.AddShaderColor(0.5f, 0.5f, 0.5f);
		grey.AddShaderColor(0.0f, 0.0f, 0.0f);
		grey.AddShaderColor(-1.0f, -0.5f, -0.2f);
		broken.AddShaderColor(std::numeric_limits<float>::quiet_NaN(), 1.0f, 0.0f);
		broken.AddShaderColor(std::numeric_limits<float>::infinity(), 1.0f, 0.0f);
		CHECK(!grey.Result() && !broken.Result());
		// the light a model read gives: a colored texture as it is (the game's grand crystal's teal, never its glow's violet)
		const Rgb teal{ 83, 255, 234 };
		CHECK(ShownColor(teal, Kind::kGrand) == teal);
		// a pale gem (the game's soulgemcommon texture, 154,241,255) wears its own color too - no pull toward pale blue
		const Rgb paleCyan{ 154, 241, 255 };
		CHECK(IsPale(paleCyan) && !IsPale(Rgb{ 255, 38, 25 }) && !IsPale(Rgb{ 0, 0, 0 }));
		CHECK(ShownColor(paleCyan, Kind::kCommon) == paleCyan);
		CHECK(ShownColor(Rgb{ 255, 200, 230 }, Kind::kPetty) == (Rgb{ 255, 200, 230 }));
		// a black-soul gem keeps its own purple, a star its own texture's color, pale or not
		const Rgb lilac{ 184, 132, 255 };
		CHECK(IsPale(lilac) && ShownColor(lilac, Kind::kBlack) == lilac);
		const Rgb starPale{ 157, 255, 252 };
		CHECK(ShownColor(starPale, Kind::kStar) == starPale);
		// every light keeps its hue and top channel, its saturation lifted to 1 - (1 - s)^3; grey and pure colors stay as they are
		const auto satOf = [](const Rgb& a_c) {
			const int top = (std::max)({ a_c[0], a_c[1], a_c[2] }), bottom = (std::min)({ a_c[0], a_c[1], a_c[2] });
			return top > 0 ? static_cast<double>(top - bottom) / top : 0.0;
		};
		const Rgb    rustBlack{ 168, 175, 255 };
		const Rgb    richBlack = Richer(rustBlack);
		const double rustSat = satOf(rustBlack);
		CHECK(richBlack[2] == 255 && std::abs(satOf(richBlack) - (1.0 - (1.0 - rustSat) * (1.0 - rustSat) * (1.0 - rustSat))) < 0.01);
		CHECK(richBlack[0] < richBlack[1] && richBlack[1] < richBlack[2]);  // the hue's channel order kept
		CHECK(richBlack == (Rgb{ 73, 88, 255 }));                           // and the exact result, so a hue shift fails too
		CHECK(Richer(Rgb{ 200, 200, 200 }) == (Rgb{ 200, 200, 200 }) && Richer(Rgb{ 0, 0, 0 }) == (Rgb{ 0, 0, 0 }));
		CHECK(Richer(Rgb{ 255, 0, 0 }) == (Rgb{ 255, 0, 0 }) && Richer(Rgb{ 0, 232, 255 }) == (Rgb{ 0, 232, 255 }));
		CHECK(satOf(Richer(paleCyan)) > satOf(paleCyan) && satOf(Richer(teal)) >= satOf(teal));
		{
			Look look{};
			look.kind = Kind::kGrand;
			look.rgb = paleCyan;
			CHECK(ColorAt(look, Settings{}) == Richer(FoundColor(look, Settings{})) && ColorAt(look, Settings{}) != paleCyan);
			// a star on Metal and on Gem, a color choice on both sides, both Light color choices: each wears its found color, richer
			Look star{ Family::kStar, Kind::kStar, kAzurasStarRgb };
			star.gemRgb = Rgb{ 157, 255, 252 };
			Look chosen = look;
			ApplyHisColor(chosen, *HisColorOf(0xEF44C3E71611A925ULL));
			Settings metal, gemChoice, other, otherAuthor;
			metal.starGem = false;
			gemChoice.starGem = true;
			other.side.fill(1);
			otherAuthor = other;
			otherAuthor.authorPick = true;
			for (const auto* s : { &metal, &gemChoice, &other, &otherAuthor }) {
				CHECK(ColorAt(star, *s) == Richer(FoundColor(star, *s)) && ColorAt(look, *s) == Richer(FoundColor(look, *s)) &&
					  ColorAt(chosen, *s) == Richer(FoundColor(chosen, *s)));
			}
			CHECK(ColorAt(star, metal) != kAzurasStarRgb && ColorAt(chosen, other) != FoundColor(chosen, other));
		}
		// each star's default is its body's color: the Black Star a cool white (blue on top), Azura's Star a pale aqua
		CHECK(kBlackStarRgb[2] == 255 && kBlackStarRgb[0] > 200 && kBlackStarRgb[1] > 200);
		CHECK(kAzurasStarRgb[1] > kAzurasStarRgb[0] && kAzurasStarRgb[2] > kAzurasStarRgb[0] && kAzurasStarRgb[0] > 200);
		// a star's body counts every pixel the same: a grey body with teal stripes reads near-white, its stripes alone teal
		ColorSum star;
		for (int i = 0; i < 9; ++i) {
			star.Add(100, 100, 104);
		}
		star.Add(40, 200, 190);
		const auto body = star.Body(), stripes = star.Result();
		CHECK(body && (*body)[0] > 200 && (*body)[2] == 255);
		CHECK(stripes && (*stripes)[0] < 100);
		ColorSum clear;
		clear.Add(0, 0, 0, 0);
		CHECK(!clear.Body());
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
		CHECK(KnownDisplay("legacyofthedragonborn.esm", 0x9212C6) && KnownDisplay("LegacyoftheDragonborn.esm", 0x01C59B));
		CHECK(!KnownDisplay("LegacyoftheDragonborn.esm", 0x02E4E2) && !KnownDisplay("Skyrim.esm", 0x9212C6));
		// the game's own bowl file: his pink over its picture row; any other file's bowl with no row of its own takes the same
		CHECK(CairnEyeColorOf(0xACA08866A6FF488CULL) == kGameCairnRgb && SeenColorOf(0xACA08866A6FF488CULL));
		CHECK(bowl->rgb == kGameCairnRgb);
		// the game's gems: only the greater is blue, each size's color worn as given; a star past the six sizes takes the last
		Look gameGreater = GameLook(Kind::kGreater, false);
		WearGameSetColor(gameGreater, true);
		Look gameGrand = GameLook(Kind::kGrand, false);
		gameGrand.glow = Rgb{ 9, 9, 9 };
		WearGameSetColor(gameGrand, true);
		CHECK((gameGreater.rgb == Rgb{ 114, 125, 255 }) && (gameGrand.rgb == Rgb{ 220, 130, 255 }) && gameGrand.exact && !gameGrand.glow);
		// on ENB and Vanilla the greater and the grand are baby blue, the other sizes as on Community Shaders
		Look plainGreater = GameLook(Kind::kGreater, false), plainGrand = GameLook(Kind::kGrand, false), plainPetty = GameLook(Kind::kPetty, false);
		Look plainBlack = GameLook(Kind::kBlack, true);
		for (auto* plain : { &plainGreater, &plainGrand, &plainPetty, &plainBlack }) {
			WearGameSetColor(*plain, false);
		}
		CHECK((plainGreater.rgb == Rgb{ 137, 207, 240 }) && (plainGrand.rgb == Rgb{ 137, 207, 240 }) && plainGrand.exact);
		CHECK(plainPetty.rgb == SizeRgb(kGameSetRgb, Kind::kPetty) && plainBlack.rgb == SizeRgb(kGameSetRgb, Kind::kBlack));
		CHECK(ColorAt(gameGrand, Settings{}) == gameGrand.rgb);
		// Morrowind-Style's grand model: bright orange, worn as given
		Look mwGrand = GameLook(Kind::kGrand, false);
		ApplyHisColor(mwGrand, *HisColorOf(0x8F64473DB36CF862ULL));
		CHECK((ColorAt(mwGrand, Settings{}) == Rgb{ 255, 128, 0 }));
		// Morrowind-Style's common texture: the Soul Cairn's pieces take his eye call, its common gem keeps its own row
		CHECK((CairnEyeColorOf(0x2B8D94EE2E85315DULL) == Rgb{ 252, 168, 255 }));
		CHECK(!CairnEyeColorOf(0x369A766BBF5EF72CULL) && !SeenColorOf(0x2B8D94EE2E85315DULL));
		CHECK((SeenColorOf(0x369A766BBF5EF72CULL) == Rgb{ 232, 173, 255 }));
		// the game's own Azura's Star texture: its metal a little more teal than its reading; the Black Star's texture, none
		CHECK((StarEyeColorOf(0x3F25E706DF4BF1D9ULL) == Rgb{ 184, 255, 253 }) && !SeenColorOf(0x3F25E706DF4BF1D9ULL));
		CHECK(!StarEyeColorOf(0x7223424CEA9747D1ULL) && !StarEyeColorOf(0x2B8D94EE2E85315DULL));
		CHECK((*StarEyeColorOf(0x3F25E706DF4BF1D9ULL))[0] < kAzurasStarRgb[0] && (*StarEyeColorOf(0x3F25E706DF4BF1D9ULL))[1] == 255);
		// Azura's Star Electrified's metal texture: silver, and still near white after the richer rule (it lit yellow from its
		// texture's warm grey)
		CHECK((StarEyeColorOf(0x1A45769125A0CA09ULL) == Rgb{ 249, 255, 251 }));
		CHECK((std::ranges::min(Richer(*StarEyeColorOf(0x1A45769125A0CA09ULL))) >= 230));
		// ElSopa's Black Star texture: the blue-grey it shows in game, blue the top channel after the richer rule (it lit magenta)
		CHECK((StarEyeColorOf(0xE27812D9AE3242A1ULL) == Rgb{ 234, 240, 255 }));
		CHECK((Richer(*StarEyeColorOf(0xE27812D9AE3242A1ULL))[2] == 255 && Richer(*StarEyeColorOf(0xE27812D9AE3242A1ULL))[0] < 210));
		// the colors set by eye on his gem color page are laid onto the generated tables: a regen that loses one fails here
		constexpr std::array<std::pair<std::uint64_t, Rgb>, 3> byEye{ { { 0xB0ECEC48C8938C35ULL, { 169, 107, 255 } },
			{ 0xFEE719CDD30DD0A9ULL, { 169, 107, 255 } }, { 0x40F7CB51AF04EE0CULL, { 98, 213, 255 } } } };
		for (const auto& [print, rgb] : byEye) {
			CHECK(SeenColorOf(print) == rgb);
		}
		// Animated Filled ships no gem texture: no row keyed to its own models, its light follows the texture it wears
		for (const auto print : { 0x7BC926C7EA564A46ULL, 0x792501A608331F18ULL, 0x28C1994DF9E895DBULL, 0x8F446D95A1428120ULL, 0x5D22ACF84C7D75B6ULL, 0x5AC6A6D3E9FFDBD5ULL }) {
			CHECK(!SeenColorOf(print));
		}
		// Better's common texture went teal; its Soul Cairn pieces keep their blue
		CHECK((CairnEyeColorOf(0x40F7CB51AF04EE0CULL) == Rgb{ 98, 169, 255 }));
		const auto* enhancedGrand = GlowColorOf(0x8EE44DE6FF690A05ULL);
		const auto* enhancedBlack = GlowColorOf(0xB0EA3AF8535358D8ULL);
		CHECK(enhancedGrand && enhancedGrand->r == 74 && enhancedGrand->g == 88 && enhancedGrand->b == 255);
		CHECK(enhancedBlack && enhancedBlack->r == 45 && enhancedBlack->g == 103 && enhancedBlack->b == 255);
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
		// the models Emissive Soul Gems' own Light Placer file lights, by whole path (our RE::Light files never name one)
		CHECK(EmissiveModel("Clutter\\SoulGem\\SoulGemGrand_full.nif"));
		CHECK(EmissiveModel("clutter\\daedric\\azurastar.nif"));
		CHECK(EmissiveModel("clutter\\soulgem\\soulgempetty01.nif"));
		CHECK(!EmissiveModel("clutter\\soulgem\\soulgemgrand01full.nif"));
		CHECK(EmissiveModel("Meshes/Clutter/SoulGem/SoulGemPetty01.nif"));
		// a replacer's copy in another folder is not Emissive Soul Gems' (Light Placer matches whole paths)
		CHECK(!EmissiveModel("Clutter\\SoulGem\\full\\SoulGemGrand01.nif"));
		CHECK(!EmissiveModel("Clutter\\SoulGem\\Filled\\SoulGemBlack01.nif"));
		CHECK(OwnerOf("Clutter\\SoulGem\\full\\SoulGemGrand01.nif", Family::kGame, Claim::kNone, false) == Owner::kThisMod);
		// only Rustic's own names take Rustic's look, never another replacer's "..01_full.nif"
		CHECK(!LookOfModel("clutter\\soulgem\\soulgemgrand01_full.nif", true));
		CHECK(!LookOfModel("Clutter\\SoulGem\\Filled\\SoulGemGrand_full.nif", true));
		const auto rusticGrand = LookOfModel("Meshes/Clutter/SoulGem/SoulGemGrand_full.nif", true);
		CHECK(rusticGrand && rusticGrand->family == Family::kRustic);
		CHECK(OwnerOf("Clutter\\SoulGem\\SoulGemGrand_full.nif", Family::kRustic, Claim::kEmissive, false) == Owner::kEmissive);
		CHECK(OwnerOf("Clutter\\SoulGem\\SoulGemGrand_full.nif", Family::kRustic, Claim::kNone, false) == Owner::kThisMod);
		CHECK(OwnerOf("Clutter\\SoulGem\\SoulGemGrand_full.nif", Family::kRustic, Claim::kNone, true) == Owner::kThisMod);
		// another Light Placer file's claim, on Emissive Soul Gems' paths too (CS Light on the game's gems), answers to its own switch
		CHECK(OwnerOf("clutter\\soulgem\\soulgemgrand01full.nif", Family::kPraedys, Claim::kOther, false) == Owner::kLightPlacer);
		CHECK(OwnerOf("Clutter\\SoulGem\\SoulGemGrand01.nif", Family::kGame, Claim::kOther, false) == Owner::kLightPlacer);
		// this mod's RE::Light files light Praedy's, the named gems and the Soul Cairn's pieces when RE::Light is loaded
		CHECK(OwnerOf("clutter\\soulgem\\soulgemgrand01full.nif", Family::kPraedys, Claim::kNone, true) == Owner::kReLight);
		CHECK(OwnerOf("clutter\\soulgem\\soulgemarniel.nif", Family::kNamed, Claim::kNone, true) == Owner::kReLight);
		CHECK(OwnerOf("Clutter\\SoulGem\\SoulGemGrand01.nif", Family::kGame, Claim::kNone, true) == Owner::kThisMod);
		CHECK(OwnerOf("clutter\\daedric\\azurastar.nif", Family::kStar, Claim::kNone, true) == Owner::kThisMod);
		CHECK(OwnerOf("clutter\\soulgem\\soulgemgrand01full.nif", Family::kPraedys, Claim::kNone, false) == Owner::kThisMod);
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
		// line is a key this version does not know, and so is Shadows (the shadow switch is gone - shadows are the shader mod's job)
		std::stringstream in("; a comment\n[Settings]\nLights=0\nbrightness=150\nREACH=500\nPulse=0\nShadows=1\nTheirsLightPlacer=1\nENBParticleGems=1\nUnknown=3\nBrightness=x\n");
		CHECK(ReadSettings(in, s) == 4);
		CHECK(!s.on && s.brightness == 150 && s.reach == kReachMax && !s.pulse);
		CHECK(s.who == WhoLights::kOthers);  // an old file's Theirs lines are read as nothing
		std::stringstream out;
		WriteSettings(s, out);
		Settings          back;
		std::stringstream again(out.str());
		CHECK(ReadSettings(again, back) == 9 + kGemSwitches + kColorChoices);
		CHECK(out.str().find("WhoLights=2\n") != std::string::npos && out.str().find("Theirs") == std::string::npos && out.str().find("Defaults") == std::string::npos);
		CHECK(out.str().find("GemMod") == std::string::npos && out.str().find("ColorNeVP=0\n") != std::string::npos);
		CHECK(out.str().find("AuthorPick=0\n") != std::string::npos && !back.authorPick);
		CHECK(back.on == s.on && back.brightness == s.brightness && back.reach == s.reach && back.pulse == s.pulse);
		CHECK(out.str().find("Shadows") == std::string::npos);
		CHECK(back.who == WhoLights::kOthers);
		Settings          d;
		std::stringstream empty("");
		CHECK(ReadSettings(empty, d) == 0);
		CHECK(d.on && d.brightness == 100 && d.reach == 100 && d.pulse && d.stars == 50);
		// a hand-edited file: an editor's UTF-8 mark, a note after a value, true / false and on / off words
		std::stringstream hand("\xEF\xBB\xBFLights = off ; mine\nBrightness=150 # brighter\nPulse=false\nWhoLights=1\nReach=far\n");
		Settings          h;
		CHECK(ReadSettings(hand, h) == 4 && !h.on && h.brightness == 150 && !h.pulse && h.who == WhoLights::kGame && h.reach == 100);
		// the one choice of whose light: read and clamped; an old file's switches are read as nothing
		std::stringstream oldFile("Defaults=2\nTheirsEmissive=0\nTheirsMesh=0\n"), oursFile("WhoLights=0\n"), tooBig("WhoLights=9\n");
		Settings          o, n, big;
		CHECK(ReadSettings(oldFile, o) == 0 && o.who == WhoLights::kOthers);
		CHECK(ReadSettings(oursFile, n) == 1 && n.who == WhoLights::kOurs);
		CHECK(ReadSettings(tooBig, big) == 1 && big.who == WhoLights::kOthers);
		// a light switch per gem: every gem on by default, one switched off read, written back and kept off the light
		CHECK(std::ranges::all_of(d.gems, [](bool a_on) { return a_on; }));
		std::stringstream gemsIn("lightblackstar=0\nLightPetty=off\n");
		Settings          g;
		CHECK(ReadSettings(gemsIn, g) == 2 && !g.gems[static_cast<std::size_t>(GemSwitch::kBlackStar)] &&
			  !g.gems[static_cast<std::size_t>(GemSwitch::kPetty)] && g.gems[static_cast<std::size_t>(GemSwitch::kAzurasStar)]);
		std::stringstream gemsOut;
		WriteSettings(g, gemsOut);
		CHECK(gemsOut.str().find("LightBlackStar=0\n") != std::string::npos && gemsOut.str().find("LightAzurasStar=1\n") != std::string::npos);
		Look blackStar{ Family::kStar, Kind::kStar, kBlackStarRgb };
		blackStar.blackStar = true;
		const Look azura{ Family::kStar, Kind::kStar, kAzurasStarRgb };
		const Look cairn{ Family::kSoulCairn, Kind::kNamed, kBlackStarRgb };  // the portal's bowl
		CHECK(SwitchOf(blackStar) == GemSwitch::kBlackStar && SwitchOf(azura) == GemSwitch::kAzurasStar && SwitchOf(cairn) == GemSwitch::kSoulCairn);
		CHECK(SwitchOf(GameLook(Kind::kGreater, false)) == GemSwitch::kGreater && SwitchOf(GameLook(Kind::kGrand, true)) == GemSwitch::kBlack);
		CHECK(!GemLit(blackStar, g) && GemLit(azura, g) && !GemLit(GameLook(Kind::kPetty, false), g) && GemLit(GameLook(Kind::kGrand, false), g));
		// a named quest gem on a plain gem's model (Malyn's Black Soul on the black gem's) goes by its own switch, not its size's
		Look malyn = GameLook(Kind::kGrand, true);
		malyn.named = true;
		g.gems[static_cast<std::size_t>(GemSwitch::kNamed)] = false;
		CHECK(SwitchOf(malyn) == GemSwitch::kNamed && !GemLit(malyn, g) && GemLit(GameLook(Kind::kGrand, true), g));
		g.on = false;
		CHECK(!GemLit(azura, g));
		// "Stars" (his checklist 309 / 321 answers): clamped to its range, written back, and only the two stars dimmed and
		// shortened by it
		std::stringstream starsIn("Stars=5\n");
		Settings          st;
		CHECK(ReadSettings(starsIn, st) == 1 && st.stars == kStarsMin);
		st.stars = 50;
		std::stringstream starsOut;
		WriteSettings(st, starsOut);
		CHECK(starsOut.str().find("Stars=50\n") != std::string::npos);
		Settings full;
		full.stars = 100;
		const auto plainFull = ShownAt(Kind::kStar, false, false, full), plainHalf = ShownAt(Kind::kStar, false, false, st);
		CHECK(Near(plainHalf.fade, plainFull.fade * 0.5f) && Near(plainHalf.radius, plainFull.radius * 0.5f, 0.5f));
		CHECK(Near(ShownAt(Kind::kGrand, false, false, st).fade, ShownAt(Kind::kGrand, false, false, full).fade));
		CHECK(Near(ShownAt(Kind::kGrand, true, false, st).radius, ShownAt(Kind::kGrand, true, false, full).radius));
		// ShownFor's dim comes after Community Shaders' cutoff clamp: at a reach short enough to clamp, half the fade, the same reach
		const auto& starStrength = kStrength[static_cast<std::size_t>(Kind::kStar)];
		const auto  clampFull = ShownFor(starStrength, true, 1.0f, 0.5f), clampHalf = ShownFor(starStrength, true, 1.0f, 0.5f, 1.0f, 0.5f);
		CHECK(Near(clampHalf.fade, clampFull.fade * 0.5f, 0.001f) && Near(clampHalf.radius, clampFull.radius, 0.5f));
		// on Community Shaders at every Reach a star at 50 % is dimmer and reaches less far
		for (const int reach : { kReachMin, 100, kReachMax }) {
			Settings atReach = full;
			atReach.reach = reach;
			Settings halfStars = atReach;
			halfStars.stars = 50;
			const auto atFull = ShownAt(Kind::kStar, true, false, atReach), atHalf = ShownAt(Kind::kStar, true, false, halfStars);
			CHECK(atHalf.fade < atFull.fade && atHalf.radius < atFull.radius);
		}
		CHECK(d.who == WhoLights::kOthers);  // another mod's light stays by default (his step-aside order)
		// ours on a gem: always on our lights, never on the game's, only where no other light hangs on another mod's
		CHECK(OursOn(WhoLights::kOurs, true) && OursOn(WhoLights::kOurs, false));
		CHECK(!OursOn(WhoLights::kGame, true) && !OursOn(WhoLights::kGame, false));
		CHECK(!OursOn(WhoLights::kOthers, true) && OursOn(WhoLights::kOthers, false));
		std::stringstream low("Brightness=1\nReach=-5\n");
		CHECK(ReadSettings(low, d) == 2 && d.brightness == kBrightnessMin && d.reach == kReachMin);
	}

	// his color calls (2.x of the menu: the gem mod is found, never picked): a fixed color, or a two-way choice per gem mod whose
	// empty side keeps the color found; an old file's GemMod line is a key this version does not know
	void HisColorCalls()
	{
		Settings          s;
		std::stringstream old("GemMod=praedy\nGemMod=3\n");
		CHECK(ReadSettings(old, s) == 0 && s.side == (std::array<std::uint8_t, kColorChoices>{}));
		std::stringstream in("ColorNeVP=1\ncolorrusticblack=yes\n");
		CHECK(ReadSettings(in, s) == 2 && s.side[0] == 1 && s.side[1] == 0 && s.side[2] == 1);
		std::stringstream out;
		WriteSettings(s, out);
		CHECK(out.str().find("ColorNeVP=1\nColorRemasteredBlack=0\nColorRusticBlack=1\n") != std::string::npos);
		// a choice holds only the sides it has
		std::stringstream sides("ColorNeVP=2\nColorRusticBlack=2\n");
		CHECK(ReadSettings(sides, s) == 2 && s.side[0] == 1 && s.side[2] == 2);
		std::stringstream tooFar("ColorRusticBlack=9\nColorRemasteredBlack=-3\n");
		CHECK(ReadSettings(tooFar, s) == 2 && s.side[2] == 2 && s.side[1] == 0);

		// a model's color: its glowing textures, its other textures, a grey texture's emissive, its own light shape, in that order
		ColorSum none, texture, grey, halo;
		texture.Add(176, 229, 255);
		grey.AddShaderColor(0.2f, 0.2f, 1.0f);
		halo.AddShaderColor(0.15f, 0.45f, 1.0f);
		CHECK(ModelColorOf(texture, texture, grey, halo) == texture.Result());
		CHECK(ModelColorOf(none, texture, grey, halo) == texture.Result());
		CHECK(ModelColorOf(none, none, grey, halo) == grey.Result());
		CHECK(ModelColorOf(none, none, none, halo) == halo.Result());
		CHECK(!ModelColorOf(none, none, none, none));

		// the measured table: sorted, every row found by its fingerprint, nothing for a file it does not list
		CHECK(std::ranges::is_sorted(kMeasuredColors, {}, &MeasuredColor::hash) && !MeasuredColorOf(0));
		bool measuredOk = true;
		for (const auto& row : kMeasuredColors) {
			measuredOk = measuredOk && MeasuredColorOf(row.hash) == (Rgb{ row.r, row.g, row.b });
		}
		CHECK(measuredOk);
		// the first listed file of a gem decides
		const auto&                        firstRow = kMeasuredColors.front();
		const std::array<std::uint64_t, 2> gemFiles{ 0, firstRow.hash };
		CHECK(MeasuredColorIn(gemFiles) == (Rgb{ firstRow.r, firstRow.g, firstRow.b }));
		CHECK(!MeasuredColorIn(std::array<std::uint64_t, 1>{ 0 }));
		// what a gem wears decides: its own files' rows only while it wears them, a pack's texture its own row, an unknown pack none
		constexpr std::uint64_t            otherFile = 0x1234;
		const std::array<std::uint64_t, 2> ownFiles{ firstRow.hash, otherFile };
		const std::array<std::uint64_t, 1> wearsOwn{ firstRow.hash }, wearsOther{ 0x5678 };
		CHECK(WearsNamed(ownFiles, {}) && WearsNamed(ownFiles, wearsOwn) && !WearsNamed(ownFiles, wearsOther));
		CHECK(MeasuredColorWorn(ownFiles, wearsOwn) == (Rgb{ firstRow.r, firstRow.g, firstRow.b }));
		CHECK(!MeasuredColorWorn(ownFiles, wearsOther));
		const std::array<std::uint64_t, 1> noFile{ 0x9 }, packFile{ firstRow.hash };
		CHECK(MeasuredColorWorn(noFile, packFile) == (Rgb{ firstRow.r, firstRow.g, firstRow.b }));
		CHECK(WornPairPrint(1, 2) == 0x6CD000005C805ULL);
		// a game gem under a pack takes only its model's row in that texture: none without a model, none for a pair no take measured
		const std::array<std::uint64_t, 0> noNamed{};
		CHECK(!MeasuredPairColor(noNamed, packFile) && !MeasuredPairColor(noFile, wearsOther));
		CHECK(GameFilesIn(kGameGemFiles) && !GameFilesIn(noFile));
		// every row keyed once, by a model; a choice row names a choice, a fixed row its color
		bool rowsOk = true;
		for (const auto& row : kHisColors) {
			rowsOk = rowsOk && HisColorOf(row.model) == &row && (row.choice != ColorChoice::kNone || (row.first && !row.second));
		}
		CHECK(rowsOk && !HisColorOf(0) && kColorChoiceTexts.size() == kColorChoices);

		const Rgb found{ 1, 2, 3 };
		Look      mlp = GameLook(Kind::kCommon, false);
		mlp.rgb = found;
		mlp.glow = Rgb{ 9, 9, 9 };
		ApplyHisColor(mlp, *HisColorOf(0x0FA73CA86BC804F3ULL));
		Settings d, second;
		second.side.fill(1);
		CHECK(FoundColor(mlp, d) == (Rgb{ 255, 224, 109 }) && FoundColor(mlp, second) == (Rgb{ 255, 224, 109 }) && !mlp.glow);
		Look nevp = GameLook(Kind::kGrand, false);
		nevp.rgb = found;
		ApplyHisColor(nevp, *HisColorOf(0xAD06DD431D6CAE34ULL));
		CHECK(FoundColor(nevp, d) == (Rgb{ 255, 225, 160 }) && FoundColor(nevp, second) == (Rgb{ 148, 176, 255 }));
		// an author's color is worn as given: the richer rule leaves it, and still richens a color read by us
		CHECK(nevp.exact && ColorAt(nevp, d) == FoundColor(nevp, d) && !mlp.exact && ColorAt(mlp, d) == Richer(FoundColor(mlp, d)));
		Look remrem = GameLook(Kind::kBlack, true);
		remrem.rgb = found;
		ApplyHisColor(remrem, *HisColorOf(0x83B296B357133540ULL));
		CHECK(FoundColor(remrem, d) == (Rgb{ 69, 83, 255 }) && FoundColor(remrem, second) == found);
		// one choice moves only its own gem mod's gems
		Settings onlyRustic;
		onlyRustic.side[2] = 1;
		CHECK(FoundColor(remrem, onlyRustic) == (Rgb{ 69, 83, 255 }) && FoundColor(nevp, onlyRustic) == (Rgb{ 255, 225, 160 }));
		// Rustic's black gem: purple by default, then green, then blue
		Look rustic = GameLook(Kind::kBlack, true);
		ApplyHisColor(rustic, *HisColorOf(0xEF44C3E71611A925ULL));
		Settings rusticBlue;
		rusticBlue.side[2] = 2;
		CHECK(FoundColor(rustic, d) == (Rgb{ 164, 113, 255 }) && FoundColor(rustic, onlyRustic) == (Rgb{ 119, 255, 176 }) && FoundColor(rustic, rusticBlue) == (Rgb{ 90, 158, 255 }));
		// a side a gem mod does not have wears its second; another choice's side leaves it alone
		Settings remremPastEnd;
		remremPastEnd.side[1] = 2;
		CHECK(FoundColor(remrem, remremPastEnd) == found && FoundColor(remrem, rusticBlue) == (Rgb{ 69, 83, 255 }));
		CHECK(ChoiceSides(kColorChoiceTexts[2]) == 3 && ChoiceSides(kColorChoiceTexts[0]) == 2);
		CHECK(SideWords(kColorChoiceTexts[2], 0) == "Purple" && SideWords(kColorChoiceTexts[2], 1) == "Green" && SideWords(kColorChoiceTexts[2], 2) == "Blue");
		// a gem mod's own star records are stars by the model they wear (Andrealletius Soul Gem Overhaul's star variants)
		CHECK(StarOf("Clutter\\Daedric\\AzuraStarLite.nif") == StarModel::kAzura && StarOf("meshes/clutter/daedric/azurastar.nif") == StarModel::kBlack &&
			  StarOf("clutter\\soulgem\\soulgemgrand01.nif") == StarModel::kNone && StarOf("decor\\clutter\\daedric\\azurastar.nif") == StarModel::kNone);
	}

	// the menu's "Star light": a star wears its metal's color, or on Gem the gem it holds; a pale Black Star reading (the game's
	// dark gem) wears its own purple; no other gem changes
	void StarLightPick()
	{
		Settings s;
		CHECK(s.starGem);
		// a file that only holds the old default moves to the gem once; a player's own pick stays
		std::stringstream oldFile("StarGem=0\n");
		CHECK(ReadSettings(oldFile, s) == 1 && s.starGem);
		std::stringstream in("StarGem=0\nStarPicked=1\n");
		CHECK(ReadSettings(in, s) == 2 && !s.starGem);
		std::stringstream out;
		WriteSettings(s, out);
		CHECK(out.str().find("StarGem=0\nStarPicked=1\n") != std::string::npos);

		Look azura{ Family::kStar, Kind::kStar, kAzurasStarRgb };
		azura.gemRgb = StarGemColor(Rgb{ 157, 255, 252 }, false);
		Look black{ Family::kStar, Kind::kStar, kBlackStarRgb };
		black.blackStar = true;
		black.gemRgb = StarGemColor(Rgb{ 192, 203, 255 }, true);
		Settings metal, gem;
		metal.starGem = false;
		gem.starGem = true;
		CHECK(FoundColor(azura, metal) == kAzurasStarRgb && FoundColor(azura, gem) == (Rgb{ 157, 255, 252 }));
		CHECK(FoundColor(black, metal) == kBlackStarRgb && FoundColor(black, gem) == kBlackStarGemRgb);
		// a colored Black Star reading (Praedy's dark star) keeps its own; none read = the game's star's gem
		CHECK(StarGemColor(Rgb{ 114, 84, 255 }, true) == (Rgb{ 114, 84, 255 }));
		// a pale Black Star reading of another hue (a replacer's own pale gem) keeps its own; only a pale blue-white is the game's
		CHECK(StarGemColor(Rgb{ 252, 255, 190 }, true) == (Rgb{ 252, 255, 190 }));
		CHECK(StarGemColor(std::nullopt, false) == kAzurasStarGemRgb && StarGemColor(std::nullopt, true) == kBlackStarGemRgb);
		// a pale Azura's Star reading is its gem as read (only the Black Star's dark gem hides in its texture)
		CHECK(StarGemColor(Rgb{ 252, 255, 190 }, false) == (Rgb{ 252, 255, 190 }));
		// a star with no gem color read keeps its metal on Gem; a plain gem never changes with Star light
		Look bare{ Family::kStar, Kind::kStar, kAzurasStarRgb };
		Look plain = GameLook(Kind::kGrand, false);
		CHECK(FoundColor(bare, gem) == kAzurasStarRgb && FoundColor(plain, gem) == FoundColor(plain, metal));
	}

	// the menu's "Light color": by default a gem whose own glow is not its light's color wears its glow, on Author pick the
	// color as found; a glow marked always (ASGO's Solar gems) on both
	void LightColorPick()
	{
		Settings s;
		CHECK(!s.authorPick);  // the gem's own glow is the default
		std::stringstream in("AuthorPick=1\n");
		CHECK(ReadSettings(in, s) == 1 && s.authorPick);
		std::stringstream out;
		WriteSettings(s, out);
		CHECK(out.str().find("AuthorPick=1\n") != std::string::npos);

		Look praedyBlack{ Family::kPraedys, Kind::kBlack, { 125, 108, 255 } };
		praedyBlack.glow = Rgb{ 233, 120, 255 };
		Look plain = GameLook(Kind::kGrand, false);
		Look solar{ Family::kGame, Kind::kGrand, { 255, 194, 65 } };
		solar.glow = Rgb{ 255, 110, 80 };
		solar.glowAlways = true;
		Settings d, author;
		author.authorPick = true;
		CHECK(FoundColor(praedyBlack, d) == *praedyBlack.glow && FoundColor(praedyBlack, author) == praedyBlack.rgb);
		CHECK(FoundColor(plain, d) == plain.rgb && FoundColor(plain, author) == plain.rgb);  // no own glow listed: one color
		CHECK(FoundColor(solar, d) == *solar.glow && FoundColor(solar, author) == *solar.glow);
		// the glow table: sorted, found by its fingerprint, nothing for a file it does not list
		CHECK(std::ranges::is_sorted(kGlowColors, {}, &GlowColor::hash));
		bool found = true;
		for (const auto& row : kGlowColors) {
			found = found && GlowColorOf(row.hash) == &row;
		}
		CHECK(found && !GlowColorOf(0));
		// the table carries ASGO's Solar glow on both choices
		CHECK(std::ranges::any_of(kGlowColors, &GlowColor::always));
		// the game's own gems glow by kind on a game model laid out as the game's, whatever texture pack dresses it
		const std::vector<std::string>     classic{ "clutter\\soulgem\\soulgemcommon.dds", "clutter\\soulgem\\soulgemcommon_n.dds",
			"clutter\\soulgem\\soulgems01_g.dds", "cubemaps\\chitin_e.dds" };
		const std::vector<std::string>     pbrSet{ "pbr\\clutter\\soulgem\\soulgemgrand.dds", "pbr\\clutter\\soulgem\\soulgemgrand_n.dds",
			"pbr\\clutter\\soulgem\\soulgemgrand_p.dds", "pbr\\clutter\\soulgem\\soulgemgrand_rmaos.dds",
			"pbr\\clutter\\soulgem\\soulgemgrandcoat_n.dds", "pbr\\clutter\\soulgem\\soulgemgrandcoat.dds" };
		const std::vector<std::string>     animatedSet{ "clutter\\soulgem\\soulgemcommon.dds", "clutter\\soulgem\\soulgemcommon_n.dds",
			"clutter\\soulgem\\soulgems01_g.dds", "cubemaps\\shinyglass_e.dds" };
		const std::vector<std::string>     praedySet{ "praedythxvi\\soulgems\\soulgemsempty\\soulgememptycommon\\soulgemcommon.dds",
			"praedythxvi\\soulgems\\soulgemcommon\\soulgemcommon_n.dds", "praedythxvi\\cubemaps\\vindition\\v-frostedglass.dds" };
		const std::array<std::uint64_t, 2> retexture{ 0x98986517EDDE3260ULL, 0x1234ULL };
		const auto                         modRow = std::ranges::find_if(kSeenColors, [](const SeenColor& a_row) { return std::ranges::find(kGameGemFiles, a_row.hash) == kGameGemFiles.end(); });
		CHECK(modRow != kSeenColors.end());
		const std::array<std::uint64_t, 2> modPrint{ modRow->hash, 0x1234ULL };
		const std::array<std::uint64_t, 2> gameModel{ 0x7F3555FAF7E28371ULL, 0x72BA88B6D473B38FULL };
		CHECK(GameGlowOf("Clutter\\SoulGem\\SoulGemCommon01.nif", classic, retexture) == kGameGlows[2].rgb);
		CHECK(GameGlowOf("Clutter\\SoulGem\\SoulGemCommon01.nif", classic, gameModel) == kGameGlows[2].rgb);
		// Kanjs' models: the game's layout plus the engine's black filler
		auto kanjsSet = classic;
		kanjsSet.insert(kanjsSet.begin(), "black.dds");
		CHECK(GameGlowOf("clutter\\soulgem\\soulgempetty01.nif", kanjsSet, retexture) == kGameGlows[0].rgb);
		CHECK(GameGlowOf("meshes\\clutter\\soulgem\\soulgemgrand01.nif", pbrSet, retexture) == kGameGlows[4].rgb);
		CHECK(!GameGlowOf("Clutter\\SoulGem\\SoulGemCommon01.nif", animatedSet, gameModel));
		CHECK(!GameGlowOf("Clutter\\SoulGem\\SoulGemCommon01.nif", praedySet, retexture));
		CHECK(!GameGlowOf("Clutter\\SoulGem\\SoulGemCommon01.nif", classic, modPrint));
		CHECK(!GameGlowOf("clutter\\soulgem\\soulgempetty01_filled.nif", classic, retexture));
		CHECK(!GameGlowOf("Clutter\\SoulGem\\SoulGemCommon01.nif", {}, retexture));
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
		// the table may be empty: a mod whose author asks out is the only one in it
		CHECK(kBlockedFiles.empty() || BlockedModOf(kBlockedFiles.front().hash) == kBlockedMods[kBlockedFiles.front().mod]);
		CHECK(kBlockedFiles.empty() || BlockedModOf(kBlockedFiles.back().hash) == kBlockedMods[kBlockedFiles.back().mod]);
		CHECK(!BlockedModOf(Fingerprint(a)));
		CHECK(!BlockedModAt(kBlockedMods.size()) && !BlockedModAt(255));
		// the picture colors: sorted, every one found by its own fingerprint, a stranger file not
		CHECK(std::ranges::is_sorted(kSeenColors, {}, &SeenColor::hash));
		CHECK(SeenColorOf(kSeenColors.front().hash) == (Rgb{ kSeenColors.front().r, kSeenColors.front().g, kSeenColors.front().b }));
		CHECK(SeenColorOf(kSeenColors.back().hash) == (Rgb{ kSeenColors.back().r, kSeenColors.back().g, kSeenColors.back().b }));
		CHECK(!SeenColorOf(Fingerprint(a)));
		CHECK(std::ranges::none_of(kSeenColors, [](const SeenColor& s) { return BlockedModOf(s.hash).has_value(); }));
		// the named quest gems' own rows: sorted by record then file, found by both, only the four named records, a plain lookup
		// never reaching them
		const auto namedKey = [](const NamedSeenColor& n) { return std::pair{ n.form, n.hash }; };
		CHECK(!kNamedSeenColors.empty() && std::ranges::is_sorted(kNamedSeenColors, {}, namedKey));
		CHECK(std::ranges::all_of(kNamedSeenColors,
			[](const NamedSeenColor& n) { return std::ranges::find(kNamedGems, n.form, &NamedGem::id) != kNamedGems.end(); }));
		// every row gives back its own color, so two records that share one file each keep theirs
		CHECK(std::ranges::all_of(kNamedSeenColors,
			[](const NamedSeenColor& n) { return NamedSeenColorOf(n.form, n.hash) == (Rgb{ n.r, n.g, n.b }); }));
		const auto& n0 = kNamedSeenColors.front();
		CHECK(!NamedSeenColorOf(0x012345, n0.hash) && !NamedSeenColorOf(n0.form, Fingerprint(a)));
		CHECK(std::ranges::none_of(kNamedSeenColors, [](const NamedSeenColor& n) { return BlockedModOf(n.hash).has_value(); }));
		FingerprintSum pieces;
		pieces.Add(std::span(big).first(40000));
		pieces.Add(std::span(big).subspan(40000));
		CHECK(pieces.Result() == Fingerprint(big) && pieces.Size() == big.size());
		CHECK(TexturePath("Data\\Textures/Clutter/SoulGem/Full/X.DDS") == "clutter\\soulgem\\full\\x.dds");
		CHECK(TexturePath("clutter\\x.dds") == "clutter\\x.dds");
		CHECK(std::ranges::is_sorted(kBlockedPaths, {}, &BlockedPath::path));
		CHECK(std::ranges::all_of(kBlockedPaths, [](const BlockedPath& p) { return p.mod < kBlockedMods.size() && p.path == Lower(p.path); }));
		CHECK(kBlockedPaths.empty() || BlockedModOfPath(kBlockedPaths.front().path) == kBlockedMods[kBlockedPaths.front().mod]);
		CHECK(!BlockedModOfPath("clutter\\soulgem\\soulgemgrand01.nif"));  // the game's own model is never a blocked name
		const auto        field = [](std::string_view a_text) { return std::string(1, static_cast<char>(a_text.size())) + std::string(3, '\0') + std::string(a_text); };
		const std::string nif = field("NiNode") + field("Textures\\Clutter\\A\\Gem.DDS") + field("textures\\clutter\\a\\gem_n.dds") +
		                        field("textures\\clutter\\a\\gem.dds");
		const auto        named = TexturesNamed(std::span(reinterpret_cast<const std::uint8_t*>(nif.data()), nif.size()));
		CHECK(named.size() == 2 && named[0] == "clutter\\a\\gem.dds" && named[1] == "clutter\\a\\gem_n.dds");
		CHECK(ColorTexture("textures\\clutter\\a\\gem.dds") && !ColorTexture("clutter\\a\\gem_n.dds") && !ColorTexture("cubemaps\\shinyglass_e.dds") && !ColorTexture("textures\\black.dds"));
		CHECK(TexturesNamed({}).empty());
		// a path followed by one 32-126 characters long: that length's first byte is printable (')' for 41), which a scan of
		// printable runs glued onto the path before it and lost it (the audit's H1, Animated Filled Soul Gems' gem models)
		const std::string glued = field("NiNode") + field("textures\\clutter\\a\\b.dds") + field("textures\\clutter\\soulgem\\soulgemblack.dds");
		const auto        both = TexturesNamed(std::span(reinterpret_cast<const std::uint8_t*>(glued.data()), glued.size()));
		CHECK(both.size() == 2 && both[0] == "clutter\\a\\b.dds" && both[1] == "clutter\\soulgem\\soulgemblack.dds");
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
	HisColorCalls();
	LightColorPick();
	StarLightPick();
	Permits();
	FloorLightChecks::Run(Check);
	std::printf("%d passed, %d failed\n", gPassed, gFailed);
	if (gPassed < kFloor) {
		std::printf("FAIL: only %d checks ran, under the floor of %d - a test stopped part way\n", gPassed, kFloor);
		return 1;
	}
	return gFailed ? 1 : 0;
}
