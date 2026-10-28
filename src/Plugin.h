// Soul Gem Hub - SKSE plugin
// Copyright (C) 2026 izzydoingit
// GPL-3.0-or-later; see LICENSE.txt and the notice at the top of main.cpp.
//
// What the files share. Each block names the file that defines it.

#pragma once

#include "GemMath.h"

namespace Plugin
{
	using Gems::Lower;

	// Runs a_work and lets no exception out: inside a hook or per-frame code one would end the game. The first failure at
	// each place is logged, later ones there are skipped quietly (Illuminated's LightKit::Guarded).
	template <class F>
	inline void Guarded(const char* a_where, F&& a_work) noexcept
	{
		static std::atomic<bool> told{ false };
		try {
			std::forward<F>(a_work)();
		} catch (const std::exception& e) {
			if (!told.exchange(true)) {
				try {  // the log line itself may throw (low memory) - never out of a noexcept function
					SKSE::log::error("{}: stopped by an error ({}); later ones here are skipped quietly", a_where, e.what());
				} catch (...) {
				}
			}
		} catch (...) {
			if (!told.exchange(true)) {
				try {
					SKSE::log::error("{}: stopped by an unknown error; later ones here are skipped quietly", a_where);
				} catch (...) {
				}
			}
		}
	}

	// a_job on the game's main thread at the next chance (SKSE's task interface); false when it could not be queued
	template <class F>
	[[nodiscard]] bool Later(const char* a_where, F&& a_job) noexcept
	{
		try {
			if (auto* tasks = SKSE::GetTaskInterface()) {
				tasks->AddTask([a_where, job = std::forward<F>(a_job)]() { Guarded(a_where, job); });
				return true;
			}
		} catch (...) {
		}
		return false;
	}

	// ------------------------------------------------------------------ Lighting.cpp: which lighting the game draws with
	enum class Lighting : int
	{
		kShaders = 0,  // Community Shaders: inverse square lights
		kEnb = 1,      // an ENB: the game's own lighting, every light drawn plain
		kVanilla = 2,  // the game's own lighting, every light drawn plain
	};
	void                      ReadLighting();   // data load, before any light is made
	[[nodiscard]] Lighting    LightingFound();  // never picked by the player (Illuminated's rule, his word)
	[[nodiscard]] const char* LightingName(Lighting a_lighting);
	[[nodiscard]] bool        InverseSquare();  // the lights are written in Community Shaders' inverse-square words
	[[nodiscard]] std::string LightingReport();

	// ------------------------------------------------------------------ Settings.cpp: SoulGemHub\Settings.ini, written by the menu
	void                         LoadSettings();
	bool                         SaveSettings();  // any thread; false when the file could not be written (logged)
	[[nodiscard]] Gems::Settings SettingsNow();
	void                         SetSettings(const Gems::Settings& a_settings);  // clamped; the caller saves and applies
	// one change made under the lock, so a menu change and a DevBench change at the same moment both land; clamped
	void UpdateSettings(const std::function<void(Gems::Settings&)>& a_change);

	// ------------------------------------------------------------------ GemLights.cpp: the gems and their lights
	void                      FindGems();       // data load: which forms are soul gems we light, and how each looks
	void                      InstallFrame();   // data load: our once-a-frame work after the player's update (main thread)
	void                      DropGemLights();  // a game loads, or a new one starts
	void                      ApplyNow();       // main thread: the settings onto every light now (the menu, DevBench)
	void                      RefreshLights();  // main thread: the settings onto the lights already hung, even while the game is paused
	[[nodiscard]] std::size_t GemLightCount();
	// what the load order dresses its filled gems in, for the menu: family name -> count of filled gem records
	[[nodiscard]] std::vector<std::pair<std::string, std::size_t>> GemFamilies();
	[[nodiscard]] std::vector<std::pair<std::string, std::size_t>> GemsBlocked();                    // mod -> gem records unlit until its author allows it
	[[nodiscard]] std::string                                      RusticFound();                    // "Sorted", "Unsorted", "both", or "" when none of its gems is lit
	[[nodiscard]] std::array<std::size_t, 4>                       GemsOtherLighters();              // gem records another light mod lights too, by Gems::Owner
	[[nodiscard]] std::size_t                                      GemsOtherLightsPutOut();          // placed gems near the player whose other light is put out for ours
	[[nodiscard]] std::size_t                                      GemParticleConfigs();             // Data\ParticleLights configs found at data load
	[[nodiscard]] bool                                             LightColorMatters(int a_gemMod);  // "Light color" changes a lit gem under this Gem colors pick
	[[nodiscard]] std::string                                      GemsReport();                     // for DevBench (JSON)
	[[nodiscard]] std::string                                      GlowNow(RE::FormID a_ref);        // main thread: the glow each lit shape of a placed gem shows now (JSON)

	// ------------------------------------------------------------------ Menu.cpp / DevBench.cpp
	void RegisterMenu();
	void OfferToDevBench();
}
