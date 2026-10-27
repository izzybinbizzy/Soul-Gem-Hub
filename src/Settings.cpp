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
		const auto     lines = Gems::ReadSettings(in, read);
		SetSettings(read);  // in the menu's steps of 5, as every other way in (the code review 2026-10-19)
		const auto s = SettingsNow();
		SKSE::log::info("settings: lights {}, brightness {}%, reach {}%, soft pulse {}, gem colors {} ({} line(s) read)", s.on ? "on" : "off",
			s.brightness, s.reach, s.pulse ? "on" : "off", Gems::GemModName(s.gemMod), lines);
	}

	// written beside the file and moved over it (as Dynamic Wards and Illuminated save theirs): a crash or a full disk mid-write
	// leaves the old file, never half of one. One save at a time, each with its own snapshot
	void SaveSettings()
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
				return;
			}
		}
		if (std::filesystem::rename(tmp, kPath, ec); ec) {
			SKSE::log::warn("settings: {} could not be replaced ({})", kPath, ec.message());
			std::filesystem::remove(tmp, ec);
		}
	}

	Gems::Settings SettingsNow()
	{
		std::scoped_lock l{ gLock };
		return gS;
	}

	void SetSettings(const Gems::Settings& a_settings)
	{
		std::scoped_lock l{ gLock };
		gS = a_settings;
		// clamped, in steps of 5 (the menu's steps), whoever set it - the menu, DevBench or the file
		gS.brightness = std::clamp((std::clamp(gS.brightness, Gems::kBrightnessMin, Gems::kBrightnessMax) + 2) / 5 * 5, Gems::kBrightnessMin, Gems::kBrightnessMax);
		gS.reach = std::clamp((std::clamp(gS.reach, Gems::kReachMin, Gems::kReachMax) + 2) / 5 * 5, Gems::kReachMin, Gems::kReachMax);
		gS.gemMod = Gems::GemModAt(gS.gemMod) ? gS.gemMod : 0;
	}
}
