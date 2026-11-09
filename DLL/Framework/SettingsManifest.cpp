#include "SettingsManifest.hpp"
#include "../Lib/Json/json.hpp"

#include <algorithm>
#include <stdexcept>

using nlohmann::json;

namespace Framework {
	namespace {
		std::string StringOrEmpty(const json& item, const char* field) {
			auto it = item.find(field);

			return it != item.end() && !it->is_null() ? it->get<std::string>() : std::string();
		}

		template <typename T>
		std::optional<T> Optional(const json& item, const char* field) {
			auto it = item.find(field);

			return it != item.end() && !it->is_null() ? std::optional<T>(it->get<T>()) : std::nullopt;
		}

		SettingType ParseType(const std::string& name) {
			for (SettingType type : { SettingType::Bool, SettingType::Int, SettingType::Enum, SettingType::String, SettingType::Color, SettingType::Key }) {
				if (name == SettingTypeName(type))
					return type;
			}

			throw std::invalid_argument("Unknown setting type: " + name);
		}

		SettingHeading ParseHeading(const std::string& name) {
			for (SettingHeading heading : { SettingHeading::Practice, SettingHeading::Tuning, SettingHeading::HighwayAndScenery, SettingHeading::Colors,
				SettingHeading::OnScreenInfo, SettingHeading::Audio, SettingHeading::SongsAndProfiles, SettingHeading::GameAndWindow }) {
				if (name == SettingHeadingName(heading))
					return heading;
			}

			throw std::invalid_argument("Unknown setting heading: " + name);
		}

		SettingDef ParseSetting(const json& item) {
			SettingDef def{
				.key = item.at("key").get<std::string>(),
				.type = ParseType(item.at("type").get<std::string>()),
				.def = item.at("default").get<std::string>(),
				.label = item.at("label").get<std::string>(),
				.hint = StringOrEmpty(item, "hint"),
				.category = StringOrEmpty(item, "category"),
				.min = Optional<int>(item, "min"),
				.max = Optional<int>(item, "max"),
				.scale = Optional<double>(item, "scale"),
				.choicesSource = StringOrEmpty(item, "choicesSource"),
				.editor = StringOrEmpty(item, "editor"),
				.editedBy = StringOrEmpty(item, "editedBy"),
			};

			def.ini.section = item.at("ini").at("section").get<std::string>();
			def.ini.name = item.at("ini").at("name").get<std::string>();
			if (auto migration = item.find("migratesFrom"); migration != item.end() && !migration->is_null())
				def.migratesFrom = SettingDef::IniLocation{ migration->at("section").get<std::string>(), migration->at("name").get<std::string>() };

			if (auto choices = item.find("choices"); choices != item.end())
				def.choices = choices->get<std::vector<std::string>>();

			if (auto visibleWhen = item.find("visibleWhen"); visibleWhen != item.end() && !visibleWhen->is_null())
				def.visibleWhen.emplace(visibleWhen->at("key").get<std::string>(), visibleWhen->at("equals").get<std::string>());

			if (const std::string heading = StringOrEmpty(item, "heading"); !heading.empty())
				def.heading = ParseHeading(heading);

			// The manifest gives an entry's title always, and its label is the fallback a declaration leaves implicit.
			if (std::string title = StringOrEmpty(item, "entryTitle"); title != def.label)
				def.entryTitle = std::move(title);

			// listedUnder isn't in the manifest, only where it led: anything in an entry its visibleWhen doesn't explain.
			const std::string entry = StringOrEmpty(item, "entry");
			if (!entry.empty() && entry != def.key && !(def.visibleWhen && def.visibleWhen->key == entry))
				def.listedUnder = entry;

			return def;
		}
	}

	SettingsManifest::SettingsManifest(std::string_view text) {
		const json items = json::parse(text);
		if (!items.is_array())
			throw std::invalid_argument("A manifest is a JSON array");

		for (const json& item : items) {
			const std::string modId = StringOrEmpty(item, "mod");

			auto mod = std::ranges::find(mods, modId, [](const auto& entry) -> const std::string& { return entry.first; });
			if (mod == mods.end())
				mod = mods.insert(mods.end(), { modId, {} });

			mod->second.push_back(ParseSetting(item));
		}
	}

	const SettingDefs& SettingsManifest::ForMod(std::string_view modId) const {
		auto mod = std::ranges::find_if(mods, [&](const auto& entry) { return entry.first == modId; });

		if (mod == mods.end())
			throw std::out_of_range("The manifest has no mod " + std::string(modId));

		return mod->second;
	}

	const SettingDef* SettingsManifest::Find(std::string_view key) const {
		for (const auto& [id, settings] : mods) {
			auto setting = std::ranges::find_if(settings, [&](const SettingDef& def) { return def.key == key; });

			if (setting != settings.end())
				return &*setting;
		}
		return nullptr;
	}

	const SettingDef& SettingsManifest::Setting(std::string_view key) const {
		const SettingDef* setting = Find(key);

		if (!setting)
			throw std::out_of_range("The manifest has no setting " + std::string(key));

		return *setting;
	}

	std::vector<std::string> SettingsManifest::Conflicts(const SettingDefs& declared) const {
		std::vector<std::string> conflicts;

		for (const SettingDef& def : declared) {
			const SettingDef* shared = Find(def.key);
			if (!shared) {
				for (const auto& [id, settings] : mods) {
					for (const SettingDef& other : settings) {
						if (other.ini.section == def.ini.section && other.ini.name == def.ini.name)
							conflicts.push_back(def.key + " uses the INI location of " + other.key);
					}
				}
				continue;
			}

			if (def.ini.section != shared->ini.section || def.ini.name != shared->ini.name)
				conflicts.push_back(def.key + " is at [" + def.ini.section + "] " + def.ini.name + ", the manifest has [" + shared->ini.section + "] " + shared->ini.name);
			if (def.type != shared->type)
				conflicts.push_back(def.key + " is a " + SettingTypeName(def.type) + ", the manifest has " + SettingTypeName(shared->type));
			if (def.def != shared->def)
				conflicts.push_back(def.key + " defaults to '" + def.def + "', the manifest has '" + shared->def + "'");
			if (def.choices != shared->choices)
				conflicts.push_back(def.key + " has different choices from the manifest");
		}

		return conflicts;
	}

	std::vector<std::string> SettingsManifest::Mods() const {
		std::vector<std::string> ids;
		ids.reserve(mods.size());

		for (const auto& [id, settings] : mods)
			ids.push_back(id);

		return ids;
	}
}
