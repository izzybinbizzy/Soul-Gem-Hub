// Soul Gem Hub - SKSE plugin
// Copyright (C) 2026 izzydoingit
// GPL-3.0-or-later; see LICENSE.txt and the notice at the top of main.cpp.
//
// The SKSE Menu Framework page - every setting this mod has (his rule: settings in the SKSE menu only). One page: what was
// found (the lighting, the gem looks, the gems another light mod lights too, the gem mods waiting on their authors), then the
// lights' switch, Brightness, Reach, the soft pulse, a switch per gem (the stars' strength and color beside theirs), the color choices, Light color and
// whose light a gem wears. A change is saved and
// applied on the game's main thread when it is let go.

#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#	define NOMINMAX  // the build defines it too (xmake.lua)
#endif
#include "Plugin.h"

// SKSE Menu Framework's own header (theirs, MIT): its warnings are not ours, and ours are errors (xmake.lua) - under MSVC and
// under clang-cl, which checks every file too
#pragma warning(push, 0)
#if defined(__clang__)
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Weverything"
#endif
#include "SKSEMenuFramework.h"
#if defined(__clang__)
#	pragma clang diagnostic pop
#endif
#pragma warning(pop)
#include "Translation.h"
#include "MenuStyle.h"

namespace Plugin
{
	namespace
	{
		using namespace ImGuiMCP;
		using MenuStyle::Header;
		using MenuStyle::kMuted;
		using Translation::Fill;
		using Translation::T;
		namespace Icon = MenuStyle::Icon;
#define TR_MARK(x) x  // a line Translation.json carries, though it is not written inside T( ) here

		// the menu draws on the RENDER thread: the save and the lights follow in ONE task on the main thread
		void Changed()
		{
			static std::atomic<bool> queued{ false };
			if (queued.exchange(true)) {
				return;
			}
			if (!Later("menu change", []() {
					queued = false;
					SaveSettings();
					ApplyNow();
				})) {
				queued = false;
				SKSE::log::warn("menu: the change could not reach the main thread - it is kept for this session but not saved");
			}
		}

		// the lights already hung take a moved slider at once, while the menu pauses the game too
		void Live()
		{
			static std::atomic<bool> queued{ false };
			if (!queued.exchange(true) && !Later("menu slider", []() {
					queued = false;
					RefreshLights();
				})) {
				queued = false;
				static std::atomic<bool> told{ false };
				if (!told.exchange(true)) {
					SKSE::log::warn("menu: a moved slider could not reach the main thread - the lights take it with the next change");
				}
			}
		}

		// a percent slider; applied while dragged, saved when let go
		void Percent(const char* a_label, int Gems::Settings::* a_field, int a_min, int a_max, const char* a_tip)
		{
			auto s = SettingsNow();
			int  v = s.*a_field;
			SetNextItemWidth(260.0f);
			if (SliderInt(a_label, &v, a_min, a_max, "%d%%")) {
				v = std::clamp((std::clamp(v, a_min, a_max) + 2) / 5 * 5, a_min, a_max);  // in steps of 5, rounded only when moved
				if (v != s.*a_field) {
					UpdateSettings([&](Gems::Settings& a_s) { a_s.*a_field = v; });
					Live();
				}
			}
			if (IsItemDeactivatedAfterEdit()) {
				Changed();
			}
			MenuStyle::Tip(a_tip);
		}

		void Switch(const char* a_label, bool Gems::Settings::* a_field, const char* a_tip)
		{
			auto s = SettingsNow();
			bool v = s.*a_field;
			if (Checkbox(a_label, &v)) {
				UpdateSettings([&](Gems::Settings& a_s) { a_s.*a_field = v; });
				Changed();
			}
			MenuStyle::Tip(a_tip);
		}

		// a gem mod's color choice, offered only while that gem mod is loaded (the gem mod itself is found, never picked)
		void ColorChoices()
		{
			// GemMath.h kColorChoiceTexts' words, here for their translation: the label, then the sides
			constexpr std::array<std::array<const char*, 4>, Gems::kColorChoices> kWords{ {
				{ TR_MARK("NeV-P Extreme Crystals"), TR_MARK("Gem color"), TR_MARK("Flare glow"), "" },
				{ TR_MARK("Remastered Remastered black gem"), TR_MARK("Blue"), TR_MARK("Pink"), "" },
				{ TR_MARK("Rustic black gem"), TR_MARK("Purple"), TR_MARK("Green"), TR_MARK("Blue") },
			} };
			for (std::size_t i = 0; i < Gems::kColorChoices; ++i) {
				if (!ColorChoiceFound(static_cast<Gems::ColorChoice>(i + 1))) {
					continue;
				}
				const auto                       s = SettingsNow();
				int                              v = s.side[i];
				const std::array<const char*, 3> items{ T(kWords[i][1]), T(kWords[i][2]), T(kWords[i][3]) };
				SetNextItemWidth(260.0f);
				if (Combo(T(kWords[i][0]), &v, items.data(), Gems::ChoiceSides(Gems::kColorChoiceTexts[i])) && v != s.side[i]) {
					UpdateSettings([&](Gems::Settings& a_s) { a_s.side[i] = static_cast<std::uint8_t>(v); });
					Changed();
				}
				MenuStyle::Tip(T("The color these gems light up in."));
			}
		}

		// New Filled Soul Gems' pink on the filled or on the empty gems (his checklist 574), shown only while it is loaded
		void PinkChoice()
		{
			const int                        now = PinkOnEmptyNow() ? 1 : 0;
			int                              v = now;
			const std::array<const char*, 2> items{ T("On the filled gems"), T("On the empty gems") };
			SetNextItemWidth(260.0f);
			if (Combo(T("New Filled Soul Gems pink"), &v, items.data(), static_cast<int>(items.size())) && v != now) {
				UpdateSettings([&](Gems::Settings& a_s) { a_s.pink = static_cast<std::uint8_t>(v + 1); });
				Changed();
			}
			MenuStyle::Tip(T(
				"Puts New Filled Soul Gems' pink look on the filled gems or the empty ones; a gem in view changes when it next loads."));
		}

		// "Light color": each gem's own glow (the default) or the author's pick, for the gems whose glow is not their light's color
		void LightColor()
		{
			auto                             s = SettingsNow();
			int                              v = s.authorPick ? 1 : 0;
			const std::array<const char*, 2> items{ T("Gem's own glow"), T("My pick") };
			SetNextItemWidth(260.0f);
			if (Combo(T("Light color"), &v, items.data(), static_cast<int>(items.size())) && (v == 1) != s.authorPick) {
				UpdateSettings([&](Gems::Settings& a_s) { a_s.authorPick = v == 1; });
				Changed();
			}
			MenuStyle::Tip(T(
				"Lights a gem in the color it glows, or in the color I picked."));
		}

		// "Star light": Azura's Star and the Black Star lit in the color of their metal or of the gem they hold
		void StarLight()
		{
			auto                             s = SettingsNow();
			int                              v = s.starGem ? 1 : 0;
			const std::array<const char*, 2> items{ T("Metal"), T("Gem") };
			SetNextItemWidth(260.0f);
			if (Combo(T("Star light"), &v, items.data(), static_cast<int>(items.size())) && (v == 1) != s.starGem) {
				UpdateSettings([&](Gems::Settings& a_s) { a_s.starGem = v == 1; });
				Changed();
			}
			MenuStyle::Tip(T("Azura's Star and the Black Star light up in the color of their metal, or of the gem they hold."));
		}

		// whose light a gem wears: one choice, always shown; by default ours is on every gem and any other light on it is put out
		void WhoLightsChoice()
		{
			const auto                       s = SettingsNow();
			int                              v = static_cast<int>(s.who);
			const std::array<const char*, 3> items{ T("Use my lights"), T("Use the game's own lights"), T("Use another mod's lights") };
			SetNextItemWidth(260.0f);
			if (Combo(T("Whose light"), &v, items.data(), static_cast<int>(items.size())) && v != static_cast<int>(s.who)) {
				UpdateSettings([&](Gems::Settings& a_s) { a_s.who = static_cast<Gems::WhoLights>(std::clamp(v, 0, 2)); });
				Changed();
			}
			MenuStyle::Tip(T(
				"Whose light a gem wears: mine, the game's own, or another mod's where one lights it."));
		}

		// a mod's gems handed back to its own light, or taken again
		void HandBackSwitch(const Gems::Settings& a_now, const std::string& a_mod, const char* a_label, const char* a_tip)
		{
			bool v = Gems::HandedBack(a_now, a_mod);
			if (Checkbox(a_label, &v)) {
				UpdateSettings([&](Gems::Settings& a_s) { Gems::HandBack(a_s, a_mod, v); });
				Changed();
			}
			MenuStyle::Tip(a_tip);
		}

		// on our lights: a switch per other mod that lights these gems, each giving them back to that mod's light with ours off them
		void HandBackSwitches()
		{
			const auto s = SettingsNow();
			const auto mods = GemsLitBy();
			const bool unnamed = GemsUnnamedLightSeen() || Gems::HandedBack(s, Gems::kUnnamedMod);
			const bool model = GemsModelLightSeen() || s.modelLights;
			if (s.who != Gems::WhoLights::kOurs || (mods.empty() && !unnamed && !model)) {
				return;
			}
			Header(Icon::kGem, T("Hand Gems Back"));
			const char* tip = T("Ticked, another mod's light stays on these gems and mine comes off them.");
			for (const auto& [mod, n] : mods) {
				HandBackSwitch(s, mod, Fill(T("{} lights its own gems ({})"), mod, std::to_string(n)).c_str(), tip);
			}
			if (unnamed) {
				HandBackSwitch(s, std::string(Gems::kUnnamedMod), T("A light no mod's file names stays on its gem"), tip);
			}
			if (model) {
				Switch(T("A gem model's own light stays on it"), &Gems::Settings::modelLights, tip);
			}
		}

		// the next control on the same line, or on the next one when the window is too narrow for it (Dynamic Wards' way)
		void BesideIfRoom(float a_width)
		{
			SameLine(0.0f, 24.0f);
			if (GetContentRegionAvail().x < a_width) {
				NewLine();
			}
		}

		// "Which Gems Glow": a light switch per gem, in GemSwitch order; the two stars share a line with their own controls beside them
		void GemSwitches()
		{
			constexpr std::array<const char*, Gems::kGemSwitches> kLabels{ TR_MARK("Petty soul gems"), TR_MARK("Lesser soul gems"),
				TR_MARK("Common soul gems"), TR_MARK("Greater soul gems"), TR_MARK("Grand soul gems"), TR_MARK("Black soul gems"),
				TR_MARK("Named quest gems"), TR_MARK("Azura's Star"), TR_MARK("The Black Star"), TR_MARK("The Soul Cairn's gems") };
			constexpr auto                                        kAzura = static_cast<std::size_t>(Gems::GemSwitch::kAzurasStar);
			constexpr auto                                        kBlack = static_cast<std::size_t>(Gems::GemSwitch::kBlackStar);
			Header(Icon::kGem, T("Which Gems Glow"));
			for (std::size_t i = 0; i < kLabels.size(); ++i) {
				if (i == kBlack) {
					SameLine(0.0f, 24.0f);
				}
				bool       v = SettingsNow().gems[i];
				const bool changed = Checkbox(T(kLabels[i]), &v);
				// a hover tip belongs to the item just drawn, so each switch carries its own
				MenuStyle::Tip(T("A gem switched off gets none of my light; another mod's light on it stays."));
				if (changed) {
					UpdateSettings([&](Gems::Settings& a_s) { a_s.gems[i] = v; });
					Changed();
				}
				if (i != kBlack) {
					continue;
				}
				// the stars' strength, and their color while one is lit, beside their switches (his rule: a setting's controls beside it)
				BesideIfRoom(260.0f + CalcTextSize(T("Stars")).x);
				Percent(T("Stars"), &Gems::Settings::stars, Gems::kStarsMin, Gems::kStarsMax,
					T("How bright and far Azura's Star and the Black Star shine, on top of Brightness and Reach."));
				if (const auto s = SettingsNow(); s.gems[kAzura] || s.gems[kBlack]) {
					BesideIfRoom(260.0f + CalcTextSize(T("Star light")).x);
					StarLight();
				}
			}
		}

		void RenderPage();

		// SKSE Menu Framework calls this from its own DLL on the render thread: nothing may be thrown back across
		void __stdcall RenderSettings()
		{
			Guarded("menu page", [] { RenderPage(); });  // a lambda of its own: its own "told once" flag in Guarded
		}

		void RenderPage()
		{
			MenuStyle::Page page;

			Header(Icon::kGem, T("Soul Gem Lighting"));
			TextColored(kMuted, "%s", Fill(T("Lighting: Automatic - {}"), T(LightingName(LightingFound()))).c_str());
			const auto families = GemFamilies();
			for (const auto& [name, n] : families) {
				MenuStyle::Status(true, Fill(T("{}: {}"), T(name.c_str()), std::to_string(n)).c_str());
			}
			if (const auto rustic = RusticFound(); !rustic.empty()) {
				TextColored(kMuted, "%s", Fill(T("Rustic Soul Gems found ({})"), T(rustic.c_str())).c_str());
			}
			// the gems another light mod lights, named by the folder its Light Placer file sits in, and what the choice of whose light
			// does with them; RE::Light keeps this mod's own files
			const auto left = GemsOtherLighters();
			const auto now = SettingsNow();
			for (const auto& [mod, n] : GemsLitBy()) {
				const bool  theirs = now.who == Gems::WhoLights::kOthers || (now.who == Gems::WhoLights::kOurs && Gems::HandedBack(now, mod));
				const char* line = theirs                            ? T("Lit by {}: {}") :
				                   now.who == Gems::WhoLights::kOurs ? T("Taken over from {}: {}") :
				                                                       T("Put out for the game's own look - {}: {}");
				TextColored(kMuted, "%s", Fill(line, mod, std::to_string(n)).c_str());
			}
			if (left[3]) {
				TextColored(kMuted, "%s", Fill(T("Left to RE::Light (my RE::Light files): {}"), std::to_string(left[3])).c_str());
			}
			for (const auto& [mod, n] : GemsBlocked()) {
				TextColored(kMuted, "%s", Fill(T("Not lit, at its author's request - {}: {}"), mod, std::to_string(n)).c_str());
			}
			if (const auto own = GemsOtherLightsPutOut(); own) {
				TextColored(kMuted, "%s", Fill(T("Gems near you whose other light is put out: {}"), std::to_string(own)).c_str());
			}
			const auto nearYou = GemsLeftNear();
			for (const auto& [mod, n] : nearYou.byMod) {
				TextColored(kMuted, "%s", Fill(T("Gems near you lit by {}: {}"), mod, std::to_string(n)).c_str());
			}
			for (const auto& [look, n] : nearYou.byModel) {
				TextColored(kMuted, "%s", Fill(T("Gems near you keeping their model's light ({}): {}"), T(look.c_str()), std::to_string(n)).c_str());
			}

			Header(Icon::kBulb, T("Lights"));
			Switch(T("Soul gem lights"), &Gems::Settings::on,
				T("Lights every filled soul gem, both stars and the Soul Cairn's gems, each in its own color."));
			const bool on = SettingsNow().on;
			{
				// ended however the block is left, so a caught throw never leaves the next frame disabled
				struct Disabled
				{
					explicit Disabled(bool a_off) { BeginDisabled(a_off); }
					~Disabled() { EndDisabled(); }
					Disabled(const Disabled&) = delete;
					Disabled& operator=(const Disabled&) = delete;
				} disabled{ !on };
				Percent(T("Brightness"), &Gems::Settings::brightness, Gems::kBrightnessMin, Gems::kBrightnessMax,
					T("How bright the gems' light is. 100% is as built."));
				Percent(T("Reach"), &Gems::Settings::reach, Gems::kReachMin, Gems::kReachMax, T("How far the gems' light reaches. 100% is as built; on Community Shaders a short reach also dims it."));
				BesideIfRoom(CalcTextSize(T("Same reach")).x + 40.0f);
				Switch(T("Same reach"), &Gems::Settings::sameReach,
					T("Gives every gem a common gem's reach; the two stars keep the Stars slider."));
				Switch(T("Soft pulse"), &Gems::Settings::pulse, T("The light breathes slowly, as the gem's own glow does."));
				GemSwitches();
				// shown only while a gem mod with a color choice is loaded, or where Light color's two choices light a gem differently
				bool choices = false;
				for (std::size_t i = 1; i <= Gems::kColorChoices; ++i) {
					choices = choices || ColorChoiceFound(static_cast<Gems::ColorChoice>(i));
				}
				if (choices || LightColorMatters() || PinkFound()) {
					Header(Icon::kGem, T("Colors"));
					if (PinkFound()) {
						// the pink is the gem mod's look, not our light: movable with the lights off; greyed again after it, a throw too
						EndDisabled();
						struct Again
						{
							bool off{ false };
							~Again() { BeginDisabled(off); }
						} again{ !on };
						PinkChoice();
					}
					ColorChoices();
					if (LightColorMatters()) {
						LightColor();
					}
				}
				Header(Icon::kGem, T("Whose Light"));
				WhoLightsChoice();
				HandBackSwitches();
			}
			TextColored(kMuted, "%s", Fill(T("Gems lit near you: {}"), std::to_string(GemLightCount())).c_str());
		}
	}

	void RegisterMenu()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			SKSE::log::warn("SKSE Menu Framework is not installed, so there is no settings page; the settings file still applies");
			return;
		}
		SKSEMenuFramework::SetSection(T("Illuminated - Soul Gem Hub"));
		SKSEMenuFramework::AddSectionItem(T("Settings"), RenderSettings);
		SKSE::log::info("settings page added to SKSE Menu Framework {}", SKSEMenuFramework::GetMenuFrameworkVersion());
	}
}
