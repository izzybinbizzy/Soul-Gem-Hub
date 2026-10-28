// Soul Gem Hub - SKSE plugin
// Copyright (C) 2026 izzydoingit
// GPL-3.0-or-later; see LICENSE.txt and the notice at the top of main.cpp.
//
// The SKSE Menu Framework page - every setting this mod has (his rule: settings in the SKSE menu only). One page: what was
// found (the lighting, the gem looks, the gems another light mod lights too, the gem mods waiting on their authors), then the
// lights' switch, Brightness, Reach, Stars, the soft pulse, Gem colors, Light color and Their Own Lights. A change is saved and
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

		// the lights already hung take a moved slider at once: the per-frame update stops while the menu pauses the game, a task does not
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

		// "Gem colors": Automatic, or one gem mod's colors on every soul gem - for a gem mod the automatic reading misses
		void GemColors()
		{
			auto                     s = SettingsNow();
			int                      v = s.gemMod;
			std::vector<const char*> items{ T("Automatic") };
			for (const auto& pick : Gems::kGemModPicks) {
				items.push_back(pick.name.data());  // a string literal: ends in a zero
			}
			SetNextItemWidth(260.0f);
			if (Combo(T("Gem colors"), &v, items.data(), static_cast<int>(items.size())) && v != s.gemMod) {
				UpdateSettings([&](Gems::Settings& a_s) { a_s.gemMod = v; });
				Changed();
			}
			MenuStyle::Tip(T(
				"Automatic finds each gem's color from the mod that dresses it. If your gems glow in the wrong color, pick your "
				"gem mod: every plain soul gem of the six sizes then wears that mod's colors (the stars, the named quest gems and "
				"gems in textures of their own keep theirs)."));
		}

		// "Light color": each gem's own glow (the default) or the author's pick, for the gems whose glow is not their light's color
		void LightColor()
		{
			auto                             s = SettingsNow();
			int                              v = s.authorPick ? 1 : 0;
			const std::array<const char*, 2> items{ T("Gem's own glow"), T("Author pick") };
			SetNextItemWidth(260.0f);
			if (Combo(T("Light color"), &v, items.data(), static_cast<int>(items.size())) && (v == 1) != s.authorPick) {
				UpdateSettings([&](Gems::Settings& a_s) { a_s.authorPick = v == 1; });
				Changed();
			}
			MenuStyle::Tip(T(
				"Some gems glow in another color than the light the author picked for them. Gem's own glow lights them in the color "
				"they glow; Author pick keeps the author's color."));
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
			// the gems another light mod would light: lit by this mod instead, its light put out (RE::Light keeps this mod's own files)
			const auto  left = GemsOtherLighters();
			const char* leftTo[] = { nullptr, TR_MARK("Taken over from Emissive Soul Gems (Light Placer): {}"),
				TR_MARK("Taken over from other Light Placer lights: {}"), TR_MARK("Left to RE::Light (this mod's RE::Light files): {}") };
			for (std::size_t i = 1; i < left.size(); ++i) {
				if (left[i]) {
					TextColored(kMuted, "%s", Fill(T(leftTo[i]), std::to_string(left[i])).c_str());
				}
			}
			for (const auto& [mod, n] : GemsBlocked()) {
				TextColored(kMuted, "%s", Fill(T("Not lit until its author allows it - {}: {}"), mod, std::to_string(n)).c_str());
			}
			if (const auto own = GemsOtherLightsPutOut(); own) {
				TextColored(kMuted, "%s", Fill(T("Gems near you whose other light is put out for ours: {}"), std::to_string(own)).c_str());
			}

			Header(Icon::kBulb, T("Lights"));
			Switch(T("Soul gem lights"), &Gems::Settings::on,
				T("Every filled soul gem, Azura's Star, the Black Star, the named quest gems and the Soul Cairn's gems glow in their own color."));
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
				Percent(T("Reach"), &Gems::Settings::reach, Gems::kReachMin, Gems::kReachMax, T("How far the gems' light reaches. 100% is as built. On Community Shaders a short reach dims the light, and past a certain point Brightness adds no more light."));
				Percent(T("Stars"), &Gems::Settings::stars, Gems::kStarsMin, Gems::kStarsMax,
					T("How bright Azura's Star and the Black Star are, on top of Brightness. Turn it down when their light drowns the gems near them."));
				Switch(T("Soft pulse"), &Gems::Settings::pulse, T("The light breathes slowly, as the gem's own glow does."));
				GemColors();
				// shown only where its two choices light a gem differently under the Gem colors pick
				if (LightColorMatters(SettingsNow().gemMod)) {
					LightColor();
				}
				// "Use their light": a gem another mod lights keeps that light instead of ours (shown only where such a light can be:
				// Emissive Soul Gems' or another Light Placer file claims a gem, or the lighting draws a gem's own model light)
				const auto lighting = LightingFound();
				// a gem's own model light shines on an ENB, or on Community Shaders when a particle-light config names its texture
				const bool meshLights = lighting == Lighting::kEnb || (lighting == Lighting::kShaders && GemParticleConfigs() > 0);
				if (left[1] || left[2] || meshLights) {
					Header(Icon::kGem, T("Their Own Lights"));
					if (left[1]) {
						Switch(T("Use Emissive Soul Gems' own light"), &Gems::Settings::theirsEmissive,
							T("Its gems keep Emissive Soul Gems' light instead of ours. Our Brightness and Reach then leave them alone."));
					}
					if (left[2]) {
						Switch(T("Use other Light Placer lights"), &Gems::Settings::theirsLightPlacer,
							T("Gems another Light Placer file lights (Subtle Soul Gems, CS Light ...) keep that light instead of ours."));
					}
					if (meshLights) {
						Switch(T("Use the gems' own glow lights"), &Gems::Settings::theirsMesh,
							T("A gem whose model carries a light of its own (an ENB or particle light, like Praedy's) keeps it instead of ours."));
					}
				}
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
