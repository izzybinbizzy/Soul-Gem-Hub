// Soul Gem Hub - SKSE plugin
// Copyright (C) 2026 izzydoingit
// GPL-3.0-or-later; see LICENSE.txt and the notice at the top of main.cpp.
//
// GENERATED - never edited by hand (see README.md, "The permission table"). The soul gem mods this plugin may not light
// until their authors allow it (their Nexus permissions): the fingerprint (FNV-1a 64 of the first 64 KB, then the size) of
// each of their gem models and the textures those name, minus every file the game or an allowed mod ships too, and the
// file names only such mods ship. GemLights.cpp looks a gem's model and its color textures up in both.

#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace Gems
{
	inline constexpr std::array<std::string_view, 0> kBlockedMods{ {} };

	struct BlockedFile
	{
		std::uint64_t hash;
		std::uint8_t  mod;  // index into kBlockedMods
	};

	// sorted by hash
	inline constexpr std::array<BlockedFile, 0> kBlockedFiles{ {} };

	struct BlockedPath
	{
		std::string_view path;  // below meshes\ or textures\, lower case
		std::uint8_t     mod{ 0 };
	};

	// sorted by path
	inline constexpr std::array<BlockedPath, 0> kBlockedPaths{ {} };
}
