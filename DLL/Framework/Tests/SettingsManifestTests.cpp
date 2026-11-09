#include "../SettingsManifest.hpp"
#include "../SettingsStore.hpp"
#include "../IMod.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

using Framework::IMod;
using Framework::SettingDef;
using Framework::SettingDefs;
using Framework::SettingHeading;
using Framework::SettingsManifest;
using Framework::SettingsSchemaRegistry;
using Framework::SettingType;

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

	// Another host's mod: only its Id() matches the Windows one.
	class NamedMod final : public IMod {
	public:
		explicit NamedMod(std::string id) : id(std::move(id)) {}
		std::string_view Id() const override { return id; }

	private:
		std::string id;
	};

	std::string Dump(const SettingDefs& settings, std::string_view modId) {
		NamedMod mod{ std::string(modId) };
		SettingsSchemaRegistry schema;
		schema.Register(&mod, settings);
		return schema.DumpManifestJson();
	}

	// Registers every mod, in the manifest's order, the way a host would.
	std::string DumpAll(const SettingsManifest& manifest) {
		std::vector<std::unique_ptr<NamedMod>> mods;
		SettingsSchemaRegistry schema;
		for (const std::string& id : manifest.Mods()) {
			mods.push_back(std::make_unique<NamedMod>(id));
			schema.Register(mods.back().get(), manifest.ForMod(id));
		}
		return schema.DumpManifestJson();
	}

	std::string Normalized(std::string text) {
		std::erase(text, '\r');
		while (!text.empty() && (text.back() == '\n' || text.back() == ' '))
			text.pop_back();
		return text;
	}

	template <typename Fn>
	bool Throws(Fn fn) {
		try {
			fn();
		}
		catch (const std::exception&) {
			return true;
		}
		return false;
	}

	struct NoIni {
		const char* GetValue(const char*, const char*, const char* def) const { return def; }
		long GetLongValue(const char*, const char*, long def) const { return def; }
	};
}

int main() {
	std::cout << "SettingsManifest tests\n";

	// 1. Every field a declaration can have survives a round trip
	{
		SettingDefs declared = {
			Framework::Toggle("Parent", "Parent setting").Heading(SettingHeading::Audio).EntryTitle("The Entry").Hint("A hint").MigratesFrom("Old", "Parent"),
			SettingDef::Numeric("Speed", "Mod Settings", "SpeedOnDisk", "Speed", "5").Range(0, 10).Scale(0.5).WithVisibleWhen("Parent"),
			SettingDef::Enum("Choice", "Choice").Choices({ "one", "two" }, "two").ListedUnder("Parent"),
			Framework::String("Font", "Font").ChoicesSource("SystemFonts").WithEditor("FontEditor"),
			Framework::String("Owned", "Owned").EditedBy("FontEditor"),
			Framework::KeyBind("Bind", "Bind", "VK_F9"),
		};

		const std::string dumped = Dump(declared, "SomeMod");
		const SettingDefs read = SettingsManifest(dumped).ForMod("SomeMod");
		Check("Same number of settings", read.size() == declared.size());
		Check("The dump is unchanged", Dump(read, "SomeMod") == dumped);

		const auto find = [&](std::string_view key) {
			return std::ranges::find_if(read, [&](const SettingDef& d) { return d.key == key; });
		};
		Check("INI location", find("Speed")->ini.section == "Mod Settings" && find("Speed")->ini.name == "SpeedOnDisk");
		Check("Type and default", find("Speed")->type == SettingType::Int && find("Speed")->def == "5");
		Check("Range", find("Speed")->min == 0 && find("Speed")->max == 10);
		Check("Scale", find("Speed")->scale == 0.5);
		Check("visibleWhen", find("Speed")->visibleWhen && find("Speed")->visibleWhen->key == "Parent");
		Check("visibleWhen isn't taken for listedUnder", find("Speed")->listedUnder.empty());
		Check("Choices and default", find("Choice")->choices.size() == 2 && find("Choice")->def == "two");
		Check("listedUnder comes back", find("Choice")->listedUnder == "Parent");
		Check("Heading", find("Parent")->heading == SettingHeading::Audio);
		Check("Migration location", find("Parent")->migratesFrom && find("Parent")->migratesFrom->section == "Old" && find("Parent")->migratesFrom->name == "Parent");
		Check("Entry title", find("Parent")->entryTitle == "The Entry");
		Check("A title that's the label stays implicit", find("Font")->entryTitle.empty());
		Check("A key bind stays a key bind", find("Bind")->type == SettingType::Key);
	}

	// 2. The real manifest, byte for byte
	{
		const auto path = std::filesystem::path(__FILE__).parent_path() / ".." / ".." / ".." / "mods.manifest.json";
		std::ifstream file(path, std::ios::binary);
		Check("mods.manifest.json is there", file.good());
		const std::string text{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

		const SettingsManifest manifest(text);
		Check("It has mods", !manifest.Mods().empty());
		Check("mods.manifest.json round-trips unchanged", Normalized(DumpAll(manifest)) == Normalized(text));

		SettingDefs all;
		for (const std::string& id : manifest.Mods())
			all.insert(all.end(), manifest.ForMod(id).begin(), manifest.ForMod(id).end());
		Check("mods.manifest.json has no conflicts with itself", manifest.Conflicts(all).empty());
	}

	// 3. A mod takes only its own settings
	{
		NamedMod a{ "ModA" };
		NamedMod b{ "ModB" };
		SettingsSchemaRegistry schema;
		schema.Register(&a, { Framework::Toggle("AToggle", "A") });
		schema.Register(&b, { Framework::Toggle("BToggle", "B") });

		const SettingsManifest manifest(schema.DumpManifestJson());
		Check("Mods in order", manifest.Mods() == std::vector<std::string>({ "ModA", "ModB" }));
		Check("Only its own", manifest.ForMod("ModB").size() == 1 && manifest.ForMod("ModB")[0].key == "BToggle");
		Check("An unknown mod throws", Throws([&] { (void)manifest.ForMod("Nope"); }));
		Check("Setting finds a key in any mod", manifest.Setting("AToggle").label == "A");
		Check("An unknown setting throws", Throws([&] { (void)manifest.Setting("Nope"); }));

		const SettingDef reworded = manifest.Setting("AToggle").Hint("Differs on this host");
		Check("A host can reword a setting", reworded.hint == "Differs on this host");
		Check("Keeping the rest", reworded.ini.name == "AToggle");
		Check("Rewording leaves the manifest as it was", manifest.Setting("AToggle").hint.empty());
	}

	// 3b. A host's own manifest: shared mods from the Windows one, its own, and none of the Windows-only ones
	{
		NamedMod shared{ "Shared" };
		NamedMod windowsOnly{ "WindowsOnly" };
		SettingsSchemaRegistry windows;
		windows.Register(&shared, { Framework::Toggle("SharedToggle", "Shared"), Framework::KeyBind("SharedKey", "Key", "A") });
		windows.Register(&windowsOnly, { Framework::Toggle("WindowsToggle", "Windows") });
		const SettingsManifest windowsManifest(windows.DumpManifestJson());

		SettingDefs sharedOnHost = windowsManifest.ForMod("Shared");
		sharedOnHost.push_back(Framework::Toggle("HostExtra", "Host extra"));

		NamedMod hostOnly{ "HostOnly" };
		SettingsSchemaRegistry host;
		host.Register(&shared, sharedOnHost);
		host.Register(&hostOnly, { Framework::Toggle("HostToggle", "Host") });
		const SettingsManifest hostManifest(host.DumpManifestJson());

		Check("Its mods, not the Windows-only one", hostManifest.Mods() == std::vector<std::string>({ "Shared", "HostOnly" }));
		Check("A shared setting comes through", hostManifest.Setting("SharedKey").def == "A");
		Check("A setting added to a shared mod comes through", hostManifest.ForMod("Shared").size() == 3);
		Check("A host-only mod comes through", hostManifest.Setting("HostToggle").label == "Host");
	}

	// 4. A setting listed under one another host doesn't have just starts its own entry
	{
		SettingDefs declared = {
			Framework::Toggle("Parent", "Parent"),
			Framework::Toggle("Child", "Child").ListedUnder("Parent"),
		};
		SettingDefs read = SettingsManifest(Dump(declared, "SomeMod")).ForMod("SomeMod");
		read.erase(read.begin());

		NamedMod mod{ "SomeMod" };
		SettingsSchemaRegistry schema;
		Check("The rest registers", schema.Register(&mod, read));
		Check("It starts its own entry", schema.Entries().size() == 1 && schema.Entries()[0].key == "Child");
	}

	// 5. Defaults reach the values mods read
	{
		SettingDefs declared = { SettingDef::Numeric("Speed", "Mod Settings", "SpeedOnDisk", "Speed", "5").Range(0, 10) };
		NamedMod mod{ "SomeMod" };
		Framework::SettingsSchema().Register(&mod, SettingsManifest(Dump(declared, "SomeMod")).ForMod("SomeMod"));
		Framework::SettingValues().Load(NoIni{}, {});
		Check("A manifest default is what a mod reads", Framework::SettingValues().Int("Speed") == 5);
		Framework::SettingsSchema().Clear();
	}

	// 6. What a host's declarations may and may not change
	{
		SettingDefs declared = {
			Framework::Toggle("Toggle", "Toggle").Default("on"),
			SettingDef::Numeric("Speed", "Mod Settings", "SpeedOnDisk", "Speed", "5"),
			Framework::Enum("Choice", "Choice").Choices({ "one", "two" }),
		};
		const SettingsManifest manifest(Dump(declared, "SomeMod"));

		const auto conflictsOf = [&](SettingDef changed) { return manifest.Conflicts({ std::move(changed) }).size(); };

		Check("Unchanged settings don't conflict", manifest.Conflicts(manifest.ForMod("SomeMod")).empty());
		Check("Having only some of them doesn't", conflictsOf(manifest.Setting("Speed")) == 0);
		Check("Rewording doesn't", conflictsOf(manifest.Setting("Speed").Hint("Differs here")) == 0);
		Check("A setting of its own doesn't", conflictsOf(Framework::Toggle("HostOnly", "Host only")) == 0);

		Check("Another default does", conflictsOf(manifest.Setting("Speed").Default("6")) == 1);
		Check("Another INI location does", conflictsOf(manifest.Setting("Speed").IniName("Elsewhere")) == 1);
		Check("Another type does", conflictsOf(SettingDef::Numeric("Toggle", "Toggle Switches", "Toggle", "Toggle", "on")) == 1);
		Check("Other choices do", conflictsOf(manifest.Setting("Choice").Choices({ "one", "three" })) == 1);
		Check("A new key on a taken INI location does", conflictsOf(Framework::Toggle("Other", "Other").Ini("Toggle Switches", "Toggle")) == 1);
	}

	// 7. What isn't a manifest
	Check("Malformed JSON throws", Throws([] { SettingsManifest("[{"); }));
	Check("Not an array throws", Throws([] { SettingsManifest("{}"); }));
	Check("A missing field throws", Throws([] { SettingsManifest(R"([{"key":"A"}])"); }));
	Check("An unknown type throws", Throws([] {
		SettingsManifest(R"([{"key":"A","ini":{"section":"S","name":"A"},"type":"Nope","default":"","label":"A"}])");
	}));
	Check("An unknown heading throws", Throws([] {
		SettingsManifest(R"([{"key":"A","ini":{"section":"S","name":"A"},"type":"Bool","default":"","label":"A","heading":"Nope"}])");
	}));
	Check("An empty manifest is fine", SettingsManifest("[]").Mods().empty());

	if (g_failures > 0) {
		std::cout << g_failures << " failure(s)\n";
		return 1;
	}
	std::cout << "All passed\n";
	return 0;
}
