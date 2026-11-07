// A host with no game and no window, doing what the README's "Other hosts" asks of one: if this stops working,
// so does a non-Windows host. dllmain.cpp is the real thing on Windows.

#include "../Framework.hpp"
#include "../KeyNames.hpp"
#include "../SettingsStore.hpp"

#include <chrono>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace Framework;

namespace {
	int g_failures = 0;

	void Check(const std::string& name, bool ok) {
		if (ok) {
			std::cout << "  PASS  " << name << "\n";
			return;
		}
		++g_failures;
		std::cout << "  FAIL  " << name << "\n";
	}

	// The registry owns the mod, so the test reads what it did from here.
	struct Observed {
		int enabled = 0;
		int disabled = 0;
		int ticks = 0;
		int songEntered = 0;
		int songExited = 0;
		int presses = 0;
		int settingsChanged = 0;
	} g_observed;

	class HostedMod final : public IMod {
	public:
		MOD_ID(HostedMod)

		SettingDefs Settings() const override {
			return {
				Toggle("HostedEnabled", "Hosted mod"),
				KeyBind("HostedKey", "Hosted key", "VK_F9"),
			};
		}

		bool IsEnabled(const ModContext& c) const override { return c.IsOn("HostedEnabled"); }

		void OnInitialize(ModContext& c) override {
			c.Commands().BindSetting("HostedKey", KeyEdge::Down, Availability::Active,
				[](ModContext&, const KeyEvent&) { ++g_observed.presses; });
		}

		void OnSettingsChanged(ModContext&) override { ++g_observed.settingsChanged; }
		void OnEnabled(ModContext&) override { ++g_observed.enabled; }
		void OnDisabled(ModContext&) override { ++g_observed.disabled; }
		void OnSongEnter(ModContext&) override { ++g_observed.songEntered; }
		void OnSongExit(ModContext&) override { ++g_observed.songExited; }

		void OnTick(ModContext& c) override {
			++g_observed.ticks;
			HudText text;
			text.visible = true;
			text.text = "hosted";
			c.Hud().Set("line", { HudAnchor::TopLeft }, text);
		}
	};

	// What a mod renamed on Windows does on a host reading the manifest: SettingsManifest::ForMod throws.
	class RenamedMod final : public IMod {
	public:
		MOD_ID(RenamedMod)
		SettingDefs Settings() const override { throw std::out_of_range("The manifest has no mod RenamedMod"); }
	};

	ModRegistrar<HostedMod> registrar;
	ModRegistrar<RenamedMod> renamedRegistrar;

	// Stands in for CSimpleIniA: section|name -> value.
	struct FakeIni {
		std::map<std::string, std::string> entries;

		const char* GetValue(const char* section, const char* name, const char* def) const {
			auto it = entries.find(std::string(section) + "|" + name);
			return it != entries.end() ? it->second.c_str() : def;
		}

		long GetLongValue(const char* section, const char* name, long def) const {
			auto it = entries.find(std::string(section) + "|" + name);
			return it != entries.end() ? std::stol(it->second) : def;
		}
	};

	void LoadSettings(const FakeIni& ini) {
		SettingValues().Load(ini, {});
	}

	void Press(unsigned int virtualKey) {
		KeyEvent event;
		event.virtualKey = virtualKey;
		event.edge = KeyEdge::Down;
		Inbox().PostKeyEvent(event);
	}

	// One pass of the host's loop: commands, then the maintenance tick.
	void Pass(GamePhase phase, bool gameLoaded = true) {
		Inbox().WaitUntil(std::chrono::steady_clock::now());
		Registry().DispatchCommands(phase, gameLoaded);
		Registry().Tick(phase);
	}

	bool HudShowsLine() {
		return !Hud().SnapshotVisible().empty();
	}
}

int main() {
	std::cout << "Headless host tests\n";

	const unsigned int f9 = VirtualKeyFromName("VK_F9");
	const unsigned int f10 = VirtualKeyFromName("VK_F10");

	// Startup, in dllmain.cpp's order: the schema exists before the first load.
	Registry().InstantiatePending();
	Check("The mod's settings reach the schema", SettingsSchema().Has("HostedEnabled") && SettingsSchema().Has("HostedKey"));
	Check("The manifest can be dumped without the DLL",
		SettingsSchema().DumpManifestJson().find("HostedKey") != std::string::npos);

	SettingValues().LoadDefaults({});
	Commands().SetKeyResolver([](std::string_view setting) {
		return VirtualKeyFromName(SettingValues().Value(setting));
	});

	FakeIni ini;
	ini.entries["Toggle Switches|HostedEnabled"] = "on";
	LoadSettings(ini);
	Registry().DispatchInitialize();

	const auto registered = Registry().StatusSnapshot();
	Check("A mod whose settings can't load is rejected, the others stay",
		registered.size() == 1 && registered[0].id == "HostedMod");

	// Loading
	Press(f9);
	Pass(GamePhase::Loading, false);
	Check("Enabled by the INI", g_observed.enabled == 1);
	Check("Keys before the game loads are dropped", g_observed.presses == 0);

	// Menu
	Press(f9);
	Pass(GamePhase::Menu);
	Check("The default key bind fires", g_observed.presses == 1);
	Check("The mod ticks", g_observed.ticks > 0);
	Check("Its HUD line shows", HudShowsLine());

	// A key from another thread, as a window callback or hook would post it
	std::thread([&] { Press(f9); }).join();
	Pass(GamePhase::Menu);
	Check("A key posted from another thread fires", g_observed.presses == 2);

	// Song
	Pass(GamePhase::Song);
	Check("Song entered", g_observed.songEntered == 1);
	Pass(GamePhase::Menu);
	Check("Song exited", g_observed.songExited == 1);

	// The INI changes while the game runs: a new key bind
	ini.entries["Keybinds|HostedKey"] = "VK_F10";
	Registry().EnqueueSettingsUpdate([&] { LoadSettings(ini); });
	Pass(GamePhase::Menu);
	Check("Told about the settings", g_observed.settingsChanged == 1);
	Press(f9);
	Pass(GamePhase::Menu);
	Check("The old key no longer fires", g_observed.presses == 2);
	Press(f10);
	Pass(GamePhase::Menu);
	Check("The new key fires", g_observed.presses == 3);

	// ...and the mod turned off
	ini.entries["Toggle Switches|HostedEnabled"] = "off";
	Registry().EnqueueSettingsUpdate([&] { LoadSettings(ini); });
	Pass(GamePhase::Menu);
	Check("Disabled by the INI", g_observed.disabled == 1);
	Check("Its HUD line is gone", !HudShowsLine());
	Press(f10);
	Pass(GamePhase::Menu);
	Check("A disabled mod's key doesn't fire", g_observed.presses == 3);

	Registry().Shutdown();

	if (g_failures > 0) {
		std::cout << g_failures << " failure(s)\n";
		return 1;
	}
	std::cout << "All passed\n";
	return 0;
}
