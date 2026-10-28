// Soul Gem Hub - SKSE plugin
// Copyright (C) 2026 izzydoingit
// GPL-3.0-or-later; see LICENSE.txt and the notice at the top of main.cpp.
//
// DevBench, when it is in the load order (it is optional; nothing here runs without it):
//   inspect kind=soulgemhub   the lighting found, the settings, the gems known (by look, and those left to another
//                             light mod), and every gem light near the player now (its gem, color, fade, radius, place)
//   soulgemhub.control        action = set (setting + value: one menu setting changed the way the menu does - saved,
//                             then applied on the main thread; the reply is the state after it) | rescan (look for
//                             the gems around the player again now)

#include "Plugin.h"

#include "DevBenchGlue.h"

#include <charconv>

namespace Plugin
{
	namespace
	{
		using DevBenchGlue::json;

		constexpr const char* kKey = "soulgemhub";

		constexpr const char* kDescriptor =
			R"json({"description":"Soul Gem Hub - the lighting found (automatic), the settings, the soul gems this plugin lights (by look: the game's own, Praedy's, Rustic, named, stars, Soul Cairn) and those it leaves to another light mod (Emissive Soul Gems on Light Placer, other Light Placer configs, RE::Light, and the gems near the player whose own model's light shines here - ENB particle lights, or a particle-light config on Community Shaders), and every gem light near the player now with its color, fade, radius and place. Read only.","inputSchema":{"type":"object","properties":{}},"readOnly":true})json";

		constexpr const char* kTool =
			R"json({"description":"Soul Gem Hub (lights every soul gem in the world): change its settings the way its menu does, or look for gems again. action=set changes one setting (saved, then applied on the main thread) and replies with the state after it: lights (0/1), brightness (25-200, percent), reach (50-150, percent), stars (10-100, percent of Brightness for Azura's Star and the Black Star), pulse (0/1, the soft pulse), theirsEmissive / theirsLightPlacer / theirsMesh (0/1, gems Emissive Soul Gems, another Light Placer file or their own model light keep that light instead of ours), gemMod (0 = Automatic, 1 and up = a gem mod in the menu's order, whose colors every soul gem then wears; the state's gemModName names it, a number past the last gem mod is refused), authorPick (0/1, the Light color: 0 = a gem's own glow where it differs from its light, 1 = Author pick). action=rescan looks for the gems around the player again at once. action=glow (ref = a placed gem's form id in hex) replies with the glow color each of its shapes shows right now. Read state with inspect kind=soulgemhub.","inputSchema":{"type":"object","properties":{"action":{"type":"string","enum":["set","rescan","glow"]},"ref":{"type":"string"},"setting":{"type":"string","enum":["lights","brightness","reach","stars","pulse","theirsEmissive","theirsLightPlacer","theirsMesh","gemMod","authorPick"]},"value":{"type":"number"}},"required":["action"]}})json";

		json SettingsJson(const Gems::Settings& a_s)
		{
			return json{ { "lights", a_s.on }, { "brightness", a_s.brightness }, { "reach", a_s.reach }, { "stars", a_s.stars }, { "pulse", a_s.pulse },
				{ "theirsEmissive", a_s.theirsEmissive }, { "theirsLightPlacer", a_s.theirsLightPlacer }, { "theirsMesh", a_s.theirsMesh },
				{ "gemMod", a_s.gemMod }, { "gemModName", std::string(Gems::GemModName(a_s.gemMod)) }, { "authorPick", a_s.authorPick } };
		}

		// everything, read on the main thread (the lights are the main thread's)
		json StateNow()
		{
			auto now = DevBenchGlue::OnMainThread([]() {
				json j = json::parse(GemsReport(), nullptr, false);
				if (!j.is_object()) {
					j = json::object();
				}
				j["lighting"] = json::parse(LightingReport(), nullptr, false);
				j["settings"] = SettingsJson(SettingsNow());
				return j;
			});
			return now ? *now : DevBenchGlue::NotRunYet();
		}

		void Handler(void*, const char*, void* a_sink, DevBenchAPI::WriteFn a_write) noexcept
		{
			try {
				DevBenchGlue::Reply(a_sink, a_write, StateNow());
			} catch (...) {
				try {  // the refusal is JSON too and may throw (low memory) - never out of a noexcept function
					DevBenchGlue::Reply(a_sink, a_write, DevBenchGlue::Refusal("the soul gem report could not be made"));
				} catch (...) {
				}
			}
		}

		json Act(const json& a_args)
		{
			const auto action = DevBenchGlue::Text(a_args, "action");
			if (action == "glow") {
				// the whole text must be one 32-bit hex number (an optional 0x first): no sign, nothing after it
				const auto       ref = DevBenchGlue::Text(a_args, "ref");
				std::string_view hex = ref;
				if (hex.starts_with("0x") || hex.starts_with("0X")) {
					hex.remove_prefix(2);
				}
				RE::FormID id = 0;
				const auto [end, error] = std::from_chars(hex.data(), hex.data() + hex.size(), id, 16);
				if (hex.empty() || error != std::errc{} || end != hex.data() + hex.size()) {
					return DevBenchGlue::Refusal("glow needs ref, a placed gem's form id in hex");
				}
				auto now = DevBenchGlue::OnMainThread([id]() { return json::parse(GlowNow(id), nullptr, false); });
				return now ? *now : DevBenchGlue::NotRunYet();
			}
			if (action == "rescan") {
				if (!Later("devbench rescan", []() { ApplyNow(); })) {
					return DevBenchGlue::NotQueued();
				}
				return json{ { "ok", true }, { "queued", true } };
			}
			if (action != "set") {
				return DevBenchGlue::Refusal("unknown action '" + action + "'", json::array({ "set", "rescan", "glow" }));
			}
			const auto key = DevBenchGlue::Text(a_args, "setting");
			const auto value = DevBenchGlue::Number(a_args, "value");
			if (!value) {
				return DevBenchGlue::Refusal("set needs a number value");
			}
			const int v = static_cast<int>(std::clamp(*value, -100000.0, 100000.0));  // finite (Number); in range before the cast
			// the one field, changed under the settings lock - a menu change at the same moment is kept
			std::function<void(Gems::Settings&)> change;
			if (key == "lights") {
				change = [v](Gems::Settings& a_s) { a_s.on = v != 0; };
			} else if (key == "brightness") {
				change = [v](Gems::Settings& a_s) { a_s.brightness = v; };
			} else if (key == "reach") {
				change = [v](Gems::Settings& a_s) { a_s.reach = v; };
			} else if (key == "stars") {
				change = [v](Gems::Settings& a_s) { a_s.stars = v; };
			} else if (key == "pulse") {
				change = [v](Gems::Settings& a_s) { a_s.pulse = v != 0; };
			} else if (key == "theirsEmissive") {
				change = [v](Gems::Settings& a_s) { a_s.theirsEmissive = v != 0; };
			} else if (key == "theirsLightPlacer") {
				change = [v](Gems::Settings& a_s) { a_s.theirsLightPlacer = v != 0; };
			} else if (key == "theirsMesh") {
				change = [v](Gems::Settings& a_s) { a_s.theirsMesh = v != 0; };
			} else if (key == "gemMod") {
				if (v != 0 && !Gems::GemModAt(v)) {
					return DevBenchGlue::Refusal(std::format("gemMod is 0 (Automatic) to {}", Gems::kGemModPicks.size()));
				}
				change = [v](Gems::Settings& a_s) { a_s.gemMod = v; };
			} else if (key == "authorPick") {
				change = [v](Gems::Settings& a_s) { a_s.authorPick = v != 0; };
			} else {
				return DevBenchGlue::Refusal("unknown setting '" + key + "'",
					json::array({ "lights", "brightness", "reach", "stars", "pulse", "theirsEmissive", "theirsLightPlacer", "theirsMesh", "gemMod", "authorPick" }));
			}
			UpdateSettings(change);
			SaveSettings();
			auto now = DevBenchGlue::OnMainThread([]() {
				ApplyNow();
				return SettingsJson(SettingsNow());
			});
			if (!now) {
				return DevBenchGlue::NotRunYet();
			}
			if (DevBenchGlue::Refused(*now)) {
				return *now;
			}
			return json{ { "ok", true }, { "setting", key }, { "settings", *now } };
		}

		void Control(void*, const char* a_args, void* a_sink, DevBenchAPI::WriteFn a_write) noexcept
		{
			try {
				DevBenchGlue::Reply(a_sink, a_write, Act(DevBenchGlue::Args(a_args)));
			} catch (...) {
				try {
					DevBenchGlue::Reply(a_sink, a_write, DevBenchGlue::Refusal("Soul Gem Hub could not handle that"));
				} catch (...) {
				}
			}
		}
	}

	void OfferToDevBench()
	{
		auto* devbench = DevBenchAPI::GetDevBenchInterface001();
		if (!devbench) {
			SKSE::log::info("[DEVBENCH] DevBench is not in this load order");
			return;
		}
		const auto build = devbench->GetBuildNumber();
		if (build < DevBenchGlue::kNeedsBuild) {
			SKSE::log::warn("[DEVBENCH] DevBench build {} is older than {} and cannot take an inspect extension", build, DevBenchGlue::kNeedsBuild);
			return;
		}
		const bool fresh = devbench->RegisterToolExtension("inspect", kKey, kDescriptor, Handler, nullptr);
		devbench->RegisterTool("soulgemhub.control", kTool, Control, nullptr);
		SKSE::log::info("[DEVBENCH] offered to DevBench build {}: inspect kind={} ({}), the tool soulgemhub.control", build, kKey,
			fresh ? "a new entry" : "replacing one already registered");
	}
}
