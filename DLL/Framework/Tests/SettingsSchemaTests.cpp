#include "../SettingsSchema.hpp"
#include "../IMod.hpp"
#include "../../Lib/Json/json.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using Framework::SettingsSchemaRegistry;
using Framework::SettingDef;
using Framework::SettingType;
using Framework::IMod;

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
		return reinterpret_cast<const IMod*>(static_cast<std::uintptr_t>(0x3000 + n));
	}
}

int main() {
	std::cout << "SettingsSchema tests\n";

	// 1. Basic registration and querying
	{
		SettingsSchemaRegistry reg;

		std::vector<SettingDef> decls = {
			SettingDef{
				.key = "AllowRewind",
				.ini = { "Riff Repeater", "AllowRewind" },
				.type = SettingType::Bool,
				.def = "0",
				.label = "Allow rewind",
				.hint = "Enables rewinding in Riff Repeater",
				.category = "Riff Repeater"
			},
			SettingDef{
				.key = "RRSpeed",
				.ini = { "Riff Repeater", "Speed" },
				.type = SettingType::Int,
				.def = "100",
				.label = "Speed",
				.category = "Riff Repeater",
				.min = 1,
				.max = 300,
				.scale = 0.01
			},
			SettingDef{
				.key = "Theme",
				.ini = { "Visuals", "Theme" },
				.type = SettingType::Enum,
				.def = "0",
				.label = "Theme",
				.category = "Visuals",
				.choices = { "Default", "Dark", "Custom" },
				.visibleWhen = SettingDef::VisibleWhen{ "CustomThemeEnabled", "1" }
			}
		};

		std::string dup;
		bool ok = reg.Register(Owner(1), decls, &dup);
		Check("Register multiple decls returns true", ok);
		Check("Registry size is 3", reg.Size() == 3);
		Check("Has('AllowRewind')", reg.Has("AllowRewind"));
		Check("Has('RRSpeed')", reg.Has("RRSpeed"));
		Check("Has('Theme')", reg.Has("Theme"));
		Check("!Has('NonExistent')", !reg.Has("NonExistent"));

		const auto* s1 = reg.Find("AllowRewind");
		Check("Find('AllowRewind') not null", s1 != nullptr);
		if (s1) {
			Check("s1 key correct", s1->key == "AllowRewind");
			Check("s1 ini section correct", s1->ini.section == "Riff Repeater");
			Check("s1 ini name correct", s1->ini.name == "AllowRewind");
			Check("s1 type is Bool", s1->type == SettingType::Bool);
			Check("s1 default is '0'", s1->def == "0");
			Check("s1 label is 'Allow rewind'", s1->label == "Allow rewind");
			Check("s1 hint correct", s1->hint == "Enables rewinding in Riff Repeater");
		}

		const auto* s2 = reg.Find("RRSpeed");
		Check("Find('RRSpeed') not null", s2 != nullptr);
		if (s2) {
			Check("s2 type is Int", s2->type == SettingType::Int);
			Check("s2 min is 1", s2->min.has_value() && *s2->min == 1);
			Check("s2 max is 300", s2->max.has_value() && *s2->max == 300);
			Check("s2 scale is 0.01", s2->scale.has_value() && *s2->scale == 0.01);
		}

		const auto* s3 = reg.Find("Theme");
		Check("Find('Theme') not null", s3 != nullptr);
		if (s3) {
			Check("s3 type is Enum", s3->type == SettingType::Enum);
			Check("s3 choices count is 3", s3->choices.size() == 3);
			Check("s3 visibleWhen has value", s3->visibleWhen.has_value());
			if (s3->visibleWhen) {
				Check("s3 visibleWhen key correct", s3->visibleWhen->key == "CustomThemeEnabled");
				Check("s3 visibleWhen equals correct", s3->visibleWhen->equals == "1");
			}
		}
	}

	// 2. Duplicate key rejection (same batch and cross-owner)
	{
		SettingsSchemaRegistry reg;

		std::vector<SettingDef> batch1 = {
			SettingDef{ .key = "ToggleLoft", .ini = { "Mod Settings", "ToggleLoft" }, .type = SettingType::Bool, .def = "0", .label = "Toggle Loft" }
		};
		Check("First registration succeeds", reg.Register(Owner(1), batch1));

		std::vector<SettingDef> batchDup = {
			SettingDef{ .key = "ToggleLoft", .ini = { "Other", "ToggleLoft" }, .type = SettingType::Bool, .def = "1", .label = "Duplicate Key" }
		};
		std::string dupKey;
		bool okDup = reg.Register(Owner(2), batchDup, &dupKey);
		Check("Duplicate registration across owners rejected", !okDup);
		Check("Duplicate key reported as 'ToggleLoft'", dupKey == "ToggleLoft");
		Check("Registry size stayed 1", reg.Size() == 1);

		// Intra-batch collision
		std::vector<SettingDef> batchIntra = {
			SettingDef{ .key = "A", .ini = { "S", "A" }, .type = SettingType::Bool, .def = "0", .label = "A" },
			SettingDef{ .key = "A", .ini = { "S", "A2" }, .type = SettingType::Bool, .def = "1", .label = "A dup" }
		};
		std::string intraDupKey;
		bool okIntra = reg.Register(Owner(3), batchIntra, &intraDupKey);
		Check("Duplicate key within single batch rejected", !okIntra);
		Check("Intra-batch duplicate key reported as 'A'", intraDupKey == "A");
	}

	// 3. RemoveMod preserves other owners
	{
		SettingsSchemaRegistry reg;
		reg.Register(Owner(1), {
			SettingDef{ .key = "Mod1_A", .ini = { "S", "A" }, .type = SettingType::Bool, .def = "0", .label = "A" },
			SettingDef{ .key = "Mod1_B", .ini = { "S", "B" }, .type = SettingType::Bool, .def = "0", .label = "B" }
		});
		reg.Register(Owner(2), {
			SettingDef{ .key = "Mod2_C", .ini = { "S", "C" }, .type = SettingType::Bool, .def = "0", .label = "C" }
		});

		Check("Initial size is 3", reg.Size() == 3);
		reg.RemoveMod(Owner(1));
		Check("Size after RemoveMod(Owner(1)) is 1", reg.Size() == 1);
		Check("!Has('Mod1_A')", !reg.Has("Mod1_A"));
		Check("!Has('Mod1_B')", !reg.Has("Mod1_B"));
		Check("Has('Mod2_C')", reg.Has("Mod2_C"));

		// Re-registering keys of removed mod is now allowed
		Check("Re-registering Mod1_A succeeds after removal", reg.Register(Owner(3), {
			SettingDef{ .key = "Mod1_A", .ini = { "S", "A" }, .type = SettingType::Bool, .def = "1", .label = "A new" }
		}));
	}

	// 4. JSON Manifest serialization
	{
		SettingsSchemaRegistry reg;
		reg.Register(Owner(1), {
			SettingDef{
				.key = "AllowRewind",
				.ini = { "Riff Repeater", "AllowRewind" },
				.type = SettingType::Bool,
				.def = "0",
				.label = "Allow rewind",
				.hint = "Tooltip test",
				.category = "Riff Repeater",
				.min = std::nullopt,
				.max = std::nullopt,
				.scale = std::nullopt,
				.choices = {},
				.choicesSource = {},
				.visibleWhen = std::nullopt,
				.editor = {}
			},
			SettingDef{
				.key = "CustomEditorField",
				.ini = { "GuitarSpeak", "Notes" },
				.type = SettingType::String,
				.def = "",
				.label = "Guitar Speak",
				.editor = "GuitarSpeak"
			}
		});

		std::string jsonStr = reg.DumpManifestJson();
		Check("DumpManifestJson returns non-empty string", !jsonStr.empty());

		// Parse output with nlohmann::json to verify it is valid JSON
		nlohmann::json parsed = nlohmann::json::parse(jsonStr, nullptr, false);
		Check("DumpManifestJson produced valid JSON", !parsed.is_discarded());
		Check("Parsed JSON is array of size 2", parsed.is_array() && parsed.size() == 2);

		if (parsed.is_array() && parsed.size() == 2) {
			const auto& item0 = parsed[0];
			Check("item0 key == 'AllowRewind'", item0["key"] == "AllowRewind");
			Check("item0 ini.section == 'Riff Repeater'", item0["ini"]["section"] == "Riff Repeater");
			Check("item0 ini.name == 'AllowRewind'", item0["ini"]["name"] == "AllowRewind");
			Check("item0 type == 'Bool'", item0["type"] == "Bool");
			Check("item0 default == '0'", item0["default"] == "0");
			Check("item0 label == 'Allow rewind'", item0["label"] == "Allow rewind");
			Check("item0 hint == 'Tooltip test'", item0["hint"] == "Tooltip test");
			Check("item0 min is null", item0["min"].is_null());
			Check("item0 max is null", item0["max"].is_null());
			Check("item0 choices is empty array", item0["choices"].is_array() && item0["choices"].empty());
			Check("item0 editor is null", item0["editor"].is_null());

			const auto& item1 = parsed[1];
			Check("item1 key == 'CustomEditorField'", item1["key"] == "CustomEditorField");
			Check("item1 editor == 'GuitarSpeak'", item1["editor"] == "GuitarSpeak");
		}
	}

	// 5. Factory helpers & GetIntDefault
	{
		auto t = SettingDef::Toggle("KeyToggle", "IniToggle", "Toggle Label");
		Check("Toggle type == Bool", t.type == SettingType::Bool);
		Check("Toggle key == 'KeyToggle'", t.key == "KeyToggle");
		Check("Toggle default ini.section == 'Toggle Switches'", t.ini.section == "Toggle Switches");
		Check("Toggle default category == 'Toggle Switches'", t.category == "Toggle Switches");
		Check("Toggle ini.name == 'IniToggle'", t.ini.name == "IniToggle");
		Check("Toggle def == 'off'", t.def == "off");
		Check("Toggle label == 'Toggle Label'", t.label == "Toggle Label");
		Check("Toggle GetIntDefault fallback == 0", t.GetIntDefault(0) == 0);

		auto tCustom = SettingDef::Toggle("KeyCustom", "IniCustom", "Custom Label", "Custom Section", "Custom Category", "on", "Custom Hint");
		Check("Toggle custom ini.section == 'Custom Section'", tCustom.ini.section == "Custom Section");
		Check("Toggle custom category == 'Custom Category'", tCustom.category == "Custom Category");
		Check("Toggle custom def == 'on'", tCustom.def == "on");
		Check("Toggle custom hint == 'Custom Hint'", tCustom.hint == "Custom Hint");

		auto n = SettingDef::Numeric("KeyNum", "Mod Settings", "IniNum", "Num Label", "42", 0, 100, 0.5);
		Check("Numeric type == Int", n.type == SettingType::Int);
		Check("Numeric def == '42'", n.def == "42");
		Check("Numeric GetIntDefault() == 42", n.GetIntDefault() == 42);
		Check("Numeric min is 0", n.min.has_value() && *n.min == 0);
		Check("Numeric max is 100", n.max.has_value() && *n.max == 100);
		Check("Numeric scale is 0.5", n.scale.has_value() && *n.scale == 0.5);

		auto e = SettingDef::EnumChoice("KeyEnum", "Mod Settings", "IniEnum", "Enum Label", "1", { "A", "B", "C" });
		Check("EnumChoice type == Enum", e.type == SettingType::Enum);
		Check("EnumChoice choices count == 3", e.choices.size() == 3);
		Check("EnumChoice GetIntDefault() == 1", e.GetIntDefault() == 1);

		auto e2 = SettingDef::EnumChoice("KeyEnum2", "Mod Settings", "IniEnum2", "Enum Label", "song", { "song", "startup" })
			.WithVisibleWhen("KeyToggle", "on");
		Check("WithVisibleWhen sets visibleWhen key", e2.visibleWhen.has_value() && e2.visibleWhen->key == "KeyToggle");
		Check("WithVisibleWhen sets visibleWhen equals", e2.visibleWhen.has_value() && e2.visibleWhen->equals == "on");

		// Fluent builders
		auto tf = SettingDef::Toggle("KeyToggleClean", "Clean Toggle Label")
			.IniName("CustomIni")
			.Category("Custom Group")
			.Hint("Toggle Tooltip");
		Check("tf key correct", tf.key == "KeyToggleClean");
		Check("tf ini.section default", tf.ini.section == "Toggle Switches");
		Check("tf ini.name custom", tf.ini.name == "CustomIni");
		Check("tf category custom", tf.category == "Custom Group");
		Check("tf hint custom", tf.hint == "Toggle Tooltip");
		Check("tf def default", tf.def == "off");

		auto ef = SettingDef::Enum("KeyEnumFluent", "Fluent Enum")
			.Ini("Custom Section", "CustomEnumName")
			.Choices({ "opt1", "opt2", "opt3" }, "opt2")
			.WithVisibleWhen("ParentToggle", "on");
		Check("ef type Enum", ef.type == SettingType::Enum);
		Check("ef ini.section custom", ef.ini.section == "Custom Section");
		Check("ef ini.name custom", ef.ini.name == "CustomEnumName");
		Check("ef choices count 3", ef.choices.size() == 3);
		Check("ef def is opt2", ef.def == "opt2");
		Check("ef visibleWhen key", ef.visibleWhen.has_value() && ef.visibleWhen->key == "ParentToggle");
		Check("ef visibleWhen equals", ef.visibleWhen.has_value() && ef.visibleWhen->equals == "on");

		// Choices without explicit default takes front
		auto efDefaultFront = SettingDef::Enum("KeyFront", "Front Label")
			.Choices({ "first", "second" });
		Check("efDefaultFront def is first", efDefaultFront.def == "first");

		auto nf = SettingDef::Numeric("KeyNumFluent", "Fluent Num")
			.Range(10, 90)
			.Scale(0.01)
			.Default("50");
		Check("nf type Int", nf.type == SettingType::Int);
		Check("nf min 10", nf.min.has_value() && *nf.min == 10);
		Check("nf max 90", nf.max.has_value() && *nf.max == 90);
		Check("nf scale 0.01", nf.scale.has_value() && *nf.scale == 0.01);
		Check("nf def 50", nf.def == "50");
		Check("nf GetIntDefault 50", nf.GetIntDefault() == 50);
	}

	// 6. Startup ordering: schema populated before reader pass vs after
	{
		// Simulated INI storage
		std::map<std::pair<std::string, std::string>, std::string> fakeIni = {
			{ { "Toggle Switches", "GreenScreenWall" }, "on" }
		};

		// Helper mimicking Settings::ReadModSettings schema loop
		auto runSchemaReader = [&](const SettingsSchemaRegistry& schema, std::map<std::string, std::string>& outSettings) {
			for (const auto& decl : schema.GetAll()) {
				std::pair<std::string, std::string> lookup{ std::string(decl.ini.section), std::string(decl.ini.name) };
				auto it = fakeIni.find(lookup);
				if (it != fakeIni.end()) {
					outSettings[std::string(decl.key)] = it->second;
				}
				else {
					outSettings[std::string(decl.key)] = std::string(decl.def);
				}
			}
		};

		// Regressed order: ReadModSettings runs BEFORE schema is registered
		{
			SettingsSchemaRegistry regEmpty;
			std::map<std::string, std::string> modSettings;
			runSchemaReader(regEmpty, modSettings); // runs first
			// Mod registers afterwards
			regEmpty.Register(Owner(1), { SettingDef::Toggle("GreenScreenWallEnabled", "GreenScreenWall", "Wall") });
			Check("Regressed order: setting missing from modSettings on boot", modSettings.find("GreenScreenWallEnabled") == modSettings.end());
		}

		// Correct startup order: InstantiatePending/Register runs BEFORE ReadModSettings
		{
			SettingsSchemaRegistry regPopulated;
			regPopulated.Register(Owner(1), { SettingDef::Toggle("GreenScreenWallEnabled", "GreenScreenWall", "Wall") });
			std::map<std::string, std::string> modSettings;
			runSchemaReader(regPopulated, modSettings); // runs after
			Check("Correct order: setting present in modSettings on boot", modSettings.find("GreenScreenWallEnabled") != modSettings.end());
			Check("Correct order: setting loaded from INI ('on')", modSettings["GreenScreenWallEnabled"] == "on");
		}
	}

	std::cout << "\nSettingsSchema tests complete with " << g_failures << " failures.\n";
	return g_failures == 0 ? 0 : 1;
}
