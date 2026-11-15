// Soul Gem Hub - SKSE plugin
// Copyright (C) 2026 izzydoingit
// GPL-3.0-or-later; see LICENSE.txt and the notice at the top of main.cpp.
//
// Which lighting the game draws with: the shared GameLighting.h finds it once, at data load (Community Shaders, ENB or Vanilla,
// never picked by the player); this file keeps the answer for the rest of the plugin. On Community Shaders a light's reach comes
// from its fade and cutoff, in Community Shaders' words, where a shader draws inverse square; elsewhere, and on a Community Shaders
// with no such shader, every light of ours is drawn plain, reaching its own radius (GemMath.h ShownFor, GemLights.cpp SetOwnLinear).

#include "Plugin.h"

#include "GameLighting.h"

namespace Plugin
{
	static_assert(static_cast<int>(Lighting::kShaders) == static_cast<int>(GameLighting::Lighting::kShaders) &&
				  static_cast<int>(Lighting::kEnb) == static_cast<int>(GameLighting::Lighting::kEnb) &&
				  static_cast<int>(Lighting::kVanilla) == static_cast<int>(GameLighting::Lighting::kVanilla));

	namespace
	{
		std::atomic<Lighting> gFound{ Lighting::kVanilla };
		std::atomic<bool>     gShaderFiles{ false };  // an inverse square shader file is in Data (it counts only with the DLL)
		std::atomic<bool>     gCsLoaded{ false };     // CommunityShaders.dll loaded: Light Limit Fix reads every light's first words
		std::atomic<bool>     gInverseSquare{ false };
		std::atomic<bool>     gEnbRunning{ false }, gEnbFiles{ false };
	}

	void ReadLighting()
	{
		const auto found = GameLighting::Read();
		gShaderFiles = found.shaderAddOn || found.shaderInLightLimitFix;
		gCsLoaded = found.communityShaders;
		gInverseSquare = found.inverseSquare;
		gEnbRunning = found.enb;
		gEnbFiles = found.enbFilesLeft;
		gFound = static_cast<Lighting>(found.lighting);
		const auto lines = GameLighting::Describe(found);
		SKSE::log::info("{}", lines.info);
		for (const auto& warning : lines.warnings) {
			SKSE::log::warn("{}", warning);
		}
	}

	Lighting LightingFound()
	{
		return gFound.load();
	}

	const char* LightingName(Lighting a_lighting)
	{
		return GameLighting::Name(static_cast<GameLighting::Lighting>(a_lighting));
	}

	bool InverseSquare()
	{
		return gInverseSquare.load();
	}

	bool LightLimitFix()
	{
		return gCsLoaded.load();
	}

	std::string LightingReport()
	{
		return std::format(
			R"({{"lighting":"{}","detected":true,"inverseSquareFiles":{},"communityShadersLoaded":{},"enbRunning":{},"enbFiles":{},"inverseSquare":{}}})",
			LightingName(gFound), gShaderFiles.load(), gCsLoaded.load(), gEnbRunning.load(), gEnbFiles.load(), InverseSquare());
	}
}
