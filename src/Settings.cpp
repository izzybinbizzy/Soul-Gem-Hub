// Soul Gem Hub - SKSE plugin
// Copyright (C) 2026 izzydoingit
// GPL-3.0-or-later; see LICENSE.txt and the notice at the top of main.cpp.
//
// SoulGemHub\Settings.ini, written by the menu (settings live in the SKSE menu only - his rule): the live table (GemMath.h's
// Settings, read and written by the game-free reader and writer the off-game tests run) under one lock.

#include "Plugin.h"

namespace Plugin
{
	namespace
	{
		constexpr const char* kPath = "Data/SKSE/Plugins/SoulGemHub/Settings.ini";

		std::mutex     gLock;
		Gems::Settings gS;  // under gLock
	}

	void LoadSettings()
	{
		std::ifstream  in(kPath);
		Gems::Settings read;
		bool           whoMoved = false;
		const auto     lines = Gems::ReadSettings(in, read, &whoMoved);
		if (whoMoved) {
			SKSE::log::info("settings: the file held the old default of whose light (another mod's lights) with no pick of the player's - it moves to our lights");
		}
		SetSettings(read);  // in the menu's steps of 5, as every other way in
		const auto  s = SettingsNow();
		std::string off;
		for (std::size_t i = 0; i < Gems::kGemSwitches; ++i) {
			if (!s.gems[i]) {
				off += (off.empty() ? "" : ", ") + std::string(Gems::kGemSwitchKeys[i]);
			}
		}
		std::string moved;
		for (std::size_t i = 0; i < Gems::kColorChoices; ++i) {
			if (s.side[i]) {
				moved += (moved.empty() ? "" : ", ") + std::string(Gems::kColorChoiceTexts[i].key) + " " + std::string(Gems::SideWords(Gems::kColorChoiceTexts[i], s.side[i]));
			}
		}
		std::string back;
		for (const auto& mod : s.handBack) {
			back += (back.empty() ? "" : ", ") + mod;
		}
		const char* who = s.who == Gems::WhoLights::kOurs ? "ours" : s.who == Gems::WhoLights::kGame ? "the game's own" :
		                                                                                               "another mod's";
		SKSE::log::info("settings: lights {}, brightness {}%, reach {}%, same reach {}, stars {}%, soft pulse {}, light color {}, star light {}, whose light {}, handed back: {}, model lights {}, color choices moved: {}, gems switched off: {} ({} line(s) read)",
			s.on ? "on" : "off", s.brightness, s.reach, s.sameReach ? "on" : "off", s.stars, s.pulse ? "on" : "off", s.authorPick ? "Author pick" : "the gem's own glow",
			s.starGem ? "the gem" : "the metal", who, back.empty() ? "none" : back, s.modelLights ? "kept" : "put out", moved.empty() ? "none" : moved, off.empty() ? "none" : off, lines);
	}

	// written beside the file and moved over it (as Dynamic Wards and Illuminated save theirs): a crash or a full disk mid-write
	// leaves the old file, never half of one. One save at a time, each with its own snapshot
	bool SaveSettings()
	{
		static std::mutex  writing;
		std::scoped_lock   w{ writing };
		std::ostringstream out;
		{
			std::scoped_lock l{ gLock };
			Gems::WriteSettings(gS, out);
		}
		const std::string tmp = std::string(kPath) + ".tmp";
		std::error_code   ec;
		std::filesystem::create_directories(std::filesystem::path(kPath).parent_path(), ec);
		{
			std::ofstream file(tmp, std::ios::trunc);
			file << out.str();
			file.close();
			if (!file) {
				SKSE::log::warn("settings: {} could not be written", kPath);
				std::filesystem::remove(tmp, ec);
				return false;
			}
		}
		if (std::filesystem::rename(tmp, kPath, ec); ec) {
			SKSE::log::warn("settings: {} could not be replaced ({})", kPath, ec.message());
			std::filesystem::remove(tmp, ec);
			return false;
		}
		return true;
	}

	Gems::Settings SettingsNow()
	{
		std::scoped_lock l{ gLock };
		return gS;
	}

	namespace
	{
		// clamped, in steps of 5 (the menu's steps), whoever set it - the menu, DevBench or the file; under gLock
		void Clamp(Gems::Settings& a_s)
		{
			a_s.brightness = std::clamp((std::clamp(a_s.brightness, Gems::kBrightnessMin, Gems::kBrightnessMax) + 2) / 5 * 5, Gems::kBrightnessMin, Gems::kBrightnessMax);
			a_s.reach = std::clamp((std::clamp(a_s.reach, Gems::kReachMin, Gems::kReachMax) + 2) / 5 * 5, Gems::kReachMin, Gems::kReachMax);
			a_s.who = static_cast<Gems::WhoLights>(std::clamp(static_cast<int>(a_s.who), 0, 2));
			a_s.stars = std::clamp((std::clamp(a_s.stars, Gems::kStarsMin, Gems::kStarsMax) + 2) / 5 * 5, Gems::kStarsMin, Gems::kStarsMax);
		}
	}

	void SetSettings(const Gems::Settings& a_settings)
	{
		std::scoped_lock l{ gLock };
		gS = a_settings;
		Clamp(gS);
	}

	void UpdateSettings(const std::function<void(Gems::Settings&)>& a_change)
	{
		std::scoped_lock l{ gLock };
		a_change(gS);
		Clamp(gS);
	}
}
