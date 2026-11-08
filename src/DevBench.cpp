// Soul Gem Hub - SKSE plugin
// Copyright (C) 2026 izzydoingit
// GPL-3.0-or-later; see LICENSE.txt and the notice at the top of main.cpp.
//
// DevBench, when it is in the load order (it is optional; nothing here runs without it):
//   inspect kind=soulgemhub   the lighting found, the settings, the gems known (by look, and those left to another
//                             light mod), and every gem light near the player now (its gem, color, fade, radius, place)
//   soulgemhub.control        action = set (setting + value: one menu setting changed the way the menu does - saved,
//                             then applied on the main thread; the reply is the state after it) | rescan (look for
//                             the gems around the player again now) | glow (ref: the glow each shape of a placed gem shows)

#include "Plugin.h"

#include "DevBenchGlue.h"
#include "FrameCost.h"

#include <charconv>

namespace Plugin
{
	namespace
	{
		using DevBenchGlue::json;

		constexpr const char* kKey = "soulgemhub";

		constexpr const char* kDescriptor =
			R"json({"description":"Soul Gem Hub - the lighting found (automatic), the settings, the soul gems this plugin lights (by look: the game's own, Praedy's, Rustic, named, stars, Soul Cairn) and those another light mod lights too (taken over from Emissive Soul Gems and other Light Placer files, left to RE::Light), the gems near the player whose other light is put out for ours, and every gem light near the player now with its color, fade, radius and place. Read only.","inputSchema":{"type":"object","properties":{}},"readOnly":true})json";

		constexpr const char* kTool =
			R"json({"description":"Soul Gem Hub (lights every soul gem in the world): change its settings the way its menu does, or look for gems again. action=set changes one setting (saved, then applied on the main thread) and replies with the state after it: lights (0/1), brightness (25-200, percent), reach (50-150, percent), stars (10-100, percent of Brightness and of Reach for Azura's Star and the Black Star), pulse (0/1, the soft pulse), theirsEmissive / theirsLightPlacer / theirsMesh (0/1, gems Emissive Soul Gems, another Light Placer file or their own model light keep that light instead of ours; 1 by default), ColorNeVP / ColorRemasteredBlack / ColorRusticBlack (0/1, that gem mod's color choice: 0 = its first side, 1 = its other; the state's colorChoices lists them), authorPick (0/1, the Light color: 0 = a gem's own glow where it differs from its light, 1 = Author pick), starGem (0/1, the Star light: 0 = the star's metal, 1 = the gem it holds), LightPetty / LightLesser / LightCommon / LightGreater / LightGrand / LightBlack / LightNamed / LightAzurasStar / LightBlackStar / LightSoulCairn (0/1, the light switch of that gem; the state's gems lists them). action=rescan looks for the gems around the player again at once. action=glow (ref = a placed gem's form id in hex) replies with the glow color each of its shapes shows right now. Read state with inspect kind=soulgemhub.","inputSchema":{"type":"object","properties":{"action":{"type":"string","enum":["set","rescan","glow"]},"ref":{"type":"string"},"setting":{"type":"string","enum":["lights","brightness","reach","stars","pulse","theirsEmissive","theirsLightPlacer","theirsMesh","ColorNeVP","ColorRemasteredBlack","ColorRusticBlack","authorPick","starGem","LightPetty","LightLesser","LightCommon","LightGreater","LightGrand","LightBlack","LightNamed","LightAzurasStar","LightBlackStar","LightSoulCairn"]},"value":{"type":"number"}},"required":["action"]}})json";

		json SettingsJson(const Gems::Settings& a_s)
		{
			json gems = json::object();
			for (std::size_t i = 0; i < Gems::kGemSwitches; ++i) {
				gems[std::string(Gems::kGemSwitchKeys[i])] = a_s.gems[i];
			}
			json choices = json::object();
			for (std::size_t i = 0; i < Gems::kColorChoices; ++i) {
				const auto& text = Gems::kColorChoiceTexts[i];
				choices[std::string(text.key)] = { { "value", a_s.second[i] ? 1 : 0 }, { "wears", std::string(a_s.second[i] ? text.second : text.first) },
					{ "offered", ColorChoiceFound(static_cast<Gems::ColorChoice>(i + 1)) } };
			}
			return json{ { "lights", a_s.on }, { "brightness", a_s.brightness }, { "reach", a_s.reach }, { "stars", a_s.stars }, { "pulse", a_s.pulse },
				{ "theirsEmissive", a_s.theirsEmissive }, { "theirsLightPlacer", a_s.theirsLightPlacer }, { "theirsMesh", a_s.theirsMesh },
				{ "authorPick", a_s.authorPick }, { "starGem", a_s.starGem }, { "colorChoices", std::move(choices) }, { "gems", std::move(gems) } };
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
				j["cost"] = json::array();  // each pass's main-thread time (FrameCost.h)
				for (const auto& s : FrameCost::Slots()) {
					if (const char* name = s.name.load(std::memory_order_relaxed)) {
						j["cost"].push_back({ { "pass", name }, { "usAvg", s.usAvg.load(std::memory_order_relaxed) },
							{ "usMax", s.usMax.load(std::memory_order_relaxed) } });
					}
				}
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
			} else if (const auto choice = std::ranges::find(Gems::kColorChoiceTexts, key, &Gems::ColorChoiceText::key); choice != Gems::kColorChoiceTexts.end()) {
				const auto i = static_cast<std::size_t>(choice - Gems::kColorChoiceTexts.begin());
				change = [v, i](Gems::Settings& a_s) { a_s.second[i] = v != 0; };
			} else if (key == "authorPick") {
				change = [v](Gems::Settings& a_s) { a_s.authorPick = v != 0; };
			} else if (key == "starGem") {
				change = [v](Gems::Settings& a_s) { a_s.starGem = v != 0; };
			} else if (const auto it = std::ranges::find_if(Gems::kGemSwitchKeys, [&](std::string_view a_k) { return Gems::SameText(key, a_k); });
				it != Gems::kGemSwitchKeys.end()) {
				const auto i = static_cast<std::size_t>(it - Gems::kGemSwitchKeys.begin());
				change = [v, i](Gems::Settings& a_s) { a_s.gems[i] = v != 0; };
			} else {
				auto known = json::array({ "lights", "brightness", "reach", "stars", "pulse", "theirsEmissive", "theirsLightPlacer", "theirsMesh", "authorPick", "starGem" });
				for (const auto k : Gems::kGemSwitchKeys) {
					known.push_back(std::string(k));
				}
				for (const auto& c : Gems::kColorChoiceTexts) {
					known.push_back(std::string(c.key));
				}
				return DevBenchGlue::Refusal("unknown setting '" + key + "'", std::move(known));
			}
			UpdateSettings(change);
			const bool saved = SaveSettings();
			auto       now = DevBenchGlue::OnMainThread([]() {
				ApplyNow();
				return SettingsJson(SettingsNow());
			});
			if (now && DevBenchGlue::Refused(*now) && now->value("queued", true)) {
				return *now;
			}
			if (!now || DevBenchGlue::Refused(*now)) {
				// the live table holds the change already: the player's next frame takes it, whatever the task queue did
				return json{ { "ok", true }, { "setting", key }, { "saved", saved }, { "applied", "on the next frame" },
					{ "settings", SettingsJson(SettingsNow()) } };
			}
			return json{ { "ok", true }, { "setting", key }, { "saved", saved }, { "settings", *now } };
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
