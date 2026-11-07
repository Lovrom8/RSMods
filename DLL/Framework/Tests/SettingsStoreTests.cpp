#include "../SettingsStore.hpp"
#include "../KeyNames.hpp"
#include "../ModContext.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using Framework::IMod;
using Framework::SettingDef;
using Framework::SettingType;
using Framework::SettingValues;
using Framework::UndeclaredSetting;
using Settings::When;

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

	const IMod* Owner(int n) {
		return reinterpret_cast<const IMod*>(static_cast<std::uintptr_t>(0x4000 + n));
	}

	// Stands in for CSimpleIniA: section|name -> value.
	struct FakeIni {
		std::map<std::string, std::string> entries;

		const char* GetValue(const char* section, const char* name, const char* def) const {
			auto it = entries.find(std::string(section) + "|" + name);
			return it != entries.end() ? it->second.c_str() : def;
		}

		long GetLongValue(const char* section, const char* name, long def) const {
			auto it = entries.find(std::string(section) + "|" + name);
			return it != entries.end() ? std::strtol(it->second.c_str(), nullptr, 0) : def;
		}
	};

	const UndeclaredSetting undeclared[] = {
		{ "HostFix", "Toggle Switches", "HostFixOnDisk", "off" },
	};
}

int main() {
	std::cout << "SettingsStore tests\n";

	std::vector<SettingDef> decls = {
		SettingDef{ .key = "ToggleKey", .ini = { "Toggle Switches", "ToggleOnDisk" }, .type = SettingType::Bool, .def = "on" },
		SettingDef{ .key = "SpeedKey", .ini = { "Mod Settings", "Speed" }, .type = SettingType::Int, .def = "7" },
		SettingDef{ .key = "WhenKey", .ini = { "Mod Settings", "WhenKey" }, .type = SettingType::Enum, .def = "song" },
		SettingDef{ .key = "BindKey", .ini = { "Keybinds", "BindKey" }, .type = SettingType::Enum, .def = "VK_F9" },
		SettingDef{ .key = Settings::Setting::CustomStringColors, .ini = { "Mod Settings", "CustomStringColors" }, .type = SettingType::Int, .def = "0" },
	};
	Check("Schema registers", Framework::SettingsSchema().Register(Owner(1), decls));

	auto& store = SettingValues();

	// 1. Defaults: what the mods see with no INI at all
	store.LoadDefaults(undeclared);
	Check("Default: declared toggle", store.IsOn("ToggleKey"));
	Check("Default: undeclared toggle", store.IsOff("HostFix"));
	Check("Default: int", store.Int("SpeedKey") == 7);
	Check("Default: enum", store.WhenOf("WhenKey") == When::Song);

	// 2. Load: read from the declared INI location, not the in-code key
	FakeIni ini;
	ini.entries["Toggle Switches|ToggleOnDisk"] = "off";
	ini.entries["Toggle Switches|ToggleKey"] = "on"; // The in-code key isn't what's on disk
	ini.entries["Toggle Switches|HostFixOnDisk"] = "on";
	ini.entries["Mod Settings|Speed"] = "120";
	store.Load(ini, undeclared);
	Check("Load: the declared name wins", store.IsOff("ToggleKey"));
	Check("Load: undeclared setting", store.IsOn("HostFix"));
	Check("Load: int", store.Int("SpeedKey") == 120);
	Check("Load: missing key keeps its default", store.WhenOf("WhenKey") == When::Song);

	// 3. Load replaces everything: a value set at run time doesn't survive it
	store.SetValue("RuntimeOnly", "on");
	store.SetInt("SpeedKey", 5);
	store.Load(ini, undeclared);
	Check("Reload: unknown key is gone", store.Value("RuntimeOnly").empty());
	Check("Reload: changed int comes back from the file", store.Int("SpeedKey") == 120);

	// 4. Unset values
	Check("Unset is not on", !store.IsOn("Nope"));
	Check("Unset is not off", !store.IsOff("Nope"));
	Check("Unset value is empty", store.Value("Nope").empty());
	Check("Unset int is 0", store.Int("Nope") == 0);
	Check("Unset When is Unknown", store.WhenOf("Nope") == When::Unknown);

	// 5. Int also reads a number kept as a string, and nothing else
	store.SetValue("StringNumber", "42");
	store.SetValue("StringWord", "abc");
	Check("Int: number in a string", store.Int("StringNumber") == 42);
	Check("Int: word in a string is 0", store.Int("StringWord") == 0);

	// 6. FlipToggle
	store.SetValue("Flip", "on");
	store.FlipToggle("Flip");
	Check("Flip: on -> off", store.IsOff("Flip"));
	store.FlipToggle("Flip");
	Check("Flip: off -> on", store.IsOn("Flip"));
	store.FlipToggle("FlipUnset");
	Check("Flip: unset -> on", store.IsOn("FlipUnset"));

	// 7. Every When value
	const std::vector<std::pair<std::string, When>> whens = {
		{ "manual", When::Manual }, { "startup", When::Startup }, { "song", When::Song },
		{ "tuner", When::Tuner }, { "automatic", When::Automatic }, { "Song", When::Unknown }, { "", When::Unknown },
	};
	for (const auto& [text, when] : whens) {
		store.SetValue("WhenKey", text);
		Check("When: '" + text + "'", store.WhenOf("WhenKey") == when);
	}

	// 8. Color modes stay 1:1, out-of-range included
	store.SetInt(Settings::Setting::CustomStringColors, 2);
	Check("ColorMode: Custom", store.ColorMode() == Settings::StringColorMode::Custom);
	store.SetInt(Settings::Setting::CustomStringColors, 9);
	Check("ColorMode: out of range stays out of range", static_cast<int>(store.ColorMode()) == 9);

	// 9. What mods read goes through the same store
	Framework::ModContext ctx;
	store.SetValue("CtxToggle", "on");
	store.SetInt("CtxInt", 3);
	store.SetValue("CtxWhen", "tuner");
	Check("ModContext: IsOn", ctx.IsOn("CtxToggle"));
	Check("ModContext: Value", ctx.Value("CtxToggle") == "on");
	Check("ModContext: Int", ctx.Int("CtxInt") == 3);
	Check("ModContext: When", ctx.When("CtxWhen") == When::Tuner);

	// 10. Key names
	using Framework::VirtualKeyFromName;
	Check("Key: letter", VirtualKeyFromName("A") == 0x41);
	Check("Key: digit", VirtualKeyFromName("0") == 0x30);
	Check("Key: VK name", VirtualKeyFromName("VK_F9") == 0x78);
	Check("Key: unknown is 0", VirtualKeyFromName("VK_NOPE") == 0);
	Check("Key: empty is 0", VirtualKeyFromName("") == 0);

	if (g_failures > 0) {
		std::cout << g_failures << " failure(s)\n";
		return 1;
	}
	std::cout << "All passed\n";
	return 0;
}
