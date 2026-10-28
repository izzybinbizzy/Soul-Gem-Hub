// Soul Gem Hub - SKSE plugin
// Copyright (C) 2026 izzydoingit
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. It is distributed WITHOUT ANY WARRANTY; see the
// GNU General Public License in LICENSE.txt for details.
//
// Every soul gem in the world glows in its own color - the game's own gems, Praedy's Soul Gems, Rustic Soul Gems, Azura's
// Star, the Black Star, the named quest gems and the Soul Cairn's gem bowl and chunks - with lights made in memory, on
// Community Shaders (with or without Light Placer), ENB and Vanilla, the lighting found by itself. No plugin file, nothing in
// the save; the settings live in its SKSE Menu Framework page (and the file the page writes). Another mod's light on a gem we light
// is put out while ours is on it (its menu's Their Own Lights gives a kind back); this mod's own RE::Light files light their gems.
//
// THE FILES, AND WHAT EACH ONE IS FOR (laid out as Illuminated's and Dynamic Wards')
//   main.cpp        this file - the order of the data-load work, and the SKSE messages
//   Plugin.h        what the files share; PCH.h what every file includes
//   GemMath.h       game-free: the gems' looks and strengths, the light maths, the settings file - tested off the game
//   GemLights.cpp   which forms are gems we light, and the lights themselves (once a frame, on the main thread)
//   Lighting.cpp    Community Shaders, ENB or Vanilla (Illuminated's automatic detection)
//   Settings.cpp    the settings file, read at load and saved after each change
//   Menu.cpp        the SKSE Menu Framework page          DevBench.cpp  DevBench's inspect and control (optional)
//   SKSEMenuFramework.h, DevBenchAPI.*  those mods' own files; DevBenchGlue.h, MenuStyle.h, Translation.h are shared word for
//                   word with our other plugins (master: Dynamic Wards' src\)
//   tests\testgems.cpp  the off-game tests of GemMath.h

#include "Plugin.h"
#include "RuntimeGuard.h"
#include "Translation.h"

using namespace Plugin;

namespace
{
	void LoadEverything()
	{
		const auto started = std::chrono::steady_clock::now();
		ReadLighting();
		FindGems();
		InstallFrame();  // before the menu's steps: a throw in them leaves the gems lit
		SKSE::log::info("{}", Translation::Load("Data/SKSE/Plugins/SoulGemHub/Translation.json"));
		RegisterMenu();
		SKSE::log::info("ready in {:.1f} ms", std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count());
	}

	// an exception never leaves into the game: one thrown by the load work is logged, and the steps before it keep what they did
	void OnDataLoaded()
	{
		try {
			LoadEverything();
		} catch (const std::exception& e) {
			SKSE::log::critical("the data-load work stopped part way: {} - the steps before it keep their work, the rest is not done this session",
				e.what());
		}
	}

	void Handle(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kPostLoad:
			LoadSettings();  // before DevBench can change one and save the rest
			OfferToDevBench();
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			OnDataLoaded();
			break;
		case SKSE::MessagingInterface::kPreLoadGame:
		case SKSE::MessagingInterface::kNewGame:
			DropGemLights();  // the gems they hang on are the old game's
			break;
		default:
			break;
		}
	}

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		if (a_msg) {
			Guarded("SKSE message", [a_msg]() { Handle(a_msg); });
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	// the wrong game: loaded but idle - a false here makes SKSE add its own "exit game?" box over the guard's
	if (!RuntimeGuard::RightGame("Illuminated - Soul Gem Hub", "Illuminated - Soul Gem Hub", "Illuminated - Soul Gem Hub - VR")) {
		return true;
	}
	// the start line is the only work here that can throw (low memory): it never leaves the plugin
	try {
		// the version the DLL declares (xmake.lua set_version), never a typed number
		const auto* decl = SKSE::PluginDeclaration::GetSingleton();
		const auto  ver = decl ? std::format("{}.{}", decl->GetVersion().major(), decl->GetVersion().minor()) : std::string("?");
		SKSE::log::info("Illuminated - Soul Gem Hub {} - every soul gem glows in its own color, on any lighting", ver);
	} catch (...) {
	}
	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
	return true;
}
