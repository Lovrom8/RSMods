#include "SettingsSchema.hpp"
#include "../Lib/Json/json.hpp"
#include <unordered_set>

using ordered_json = nlohmann::ordered_json;

namespace Framework {
	bool SettingsSchemaRegistry::Register(const IMod* owner, const SettingDefs& decls, std::string* outDuplicateKey) {
		std::unordered_set<std::string> batchKeys;

		// First pass: check for collisions within the batch and with existing registrations
		for (const auto& decl : decls) {
			if (keyToIndex.find(decl.key) != keyToIndex.end() || !batchKeys.insert(decl.key).second) {
				if (outDuplicateKey) {
					*outDuplicateKey = decl.key;
				}
				return false;
			}
		}

		// Second pass: insert entries
		for (const auto& decl : decls) {
			size_t index = entries.size();
			entries.push_back(Entry{ owner, decl });
			keyToIndex[decl.key] = index;
		}

		return true;
	}

	void SettingsSchemaRegistry::RemoveMod(const IMod* owner) {
		std::vector<Entry> remaining;
		remaining.reserve(entries.size());

		for (auto& entry : entries) {
			if (entry.owner != owner) {
				remaining.push_back(std::move(entry));
			}
		}

		entries = std::move(remaining);
		keyToIndex.clear();
		for (size_t i = 0; i < entries.size(); ++i) {
			keyToIndex[entries[i].decl.key] = i;
		}
	}

	const SettingDef* SettingsSchemaRegistry::Find(std::string_view key) const {
		auto it = keyToIndex.find(std::string(key));
		if (it == keyToIndex.end()) {
			return nullptr;
		}
		return &entries[it->second].decl;
	}

	bool SettingsSchemaRegistry::Has(std::string_view key) const {
		return keyToIndex.find(std::string(key)) != keyToIndex.end();
	}

	SettingDefs SettingsSchemaRegistry::GetAll() const {
		SettingDefs result;
		result.reserve(entries.size());
		for (const auto& entry : entries) {
			result.push_back(entry.decl);
		}
		return result;
	}

	namespace {
		ordered_json StringOrNull(const std::string& s) {
			return s.empty() ? ordered_json(nullptr) : ordered_json(s);
		}
	}

	void to_json(ordered_json& j, const SettingDef::VisibleWhen& v) {
		j = ordered_json{
			{ "key", v.key },
			{ "equals", v.equals }
		};
	}

	void to_json(ordered_json& j, const SettingDef& d) {
		j = ordered_json{
			{ "key", d.key },
			{ "ini", { { "section", d.ini.section }, { "name", d.ini.name } } },
			{ "type", SettingTypeName(d.type) },
			{ "default", d.def },
			{ "label", d.label },
			{ "hint", d.hint },
			{ "category", d.category },
			{ "min", d.min ? ordered_json(*d.min) : nullptr },
			{ "max", d.max ? ordered_json(*d.max) : nullptr },
			{ "scale", d.scale ? ordered_json(*d.scale) : nullptr },
			{ "choices", d.choices },
			{ "choicesSource", StringOrNull(d.choicesSource) },
			{ "visibleWhen", d.visibleWhen ? ordered_json(*d.visibleWhen) : nullptr },
			{ "editor", StringOrNull(d.editor) }
		};
	}

	std::string SettingsSchemaRegistry::DumpManifestJson() const {
		ordered_json j = ordered_json::array();
		for (const auto& entry : entries) {
			j.push_back(entry.decl);
		}
		return j.dump(2);
	}

	void SettingsSchemaRegistry::Clear() {
		entries.clear();
		keyToIndex.clear();
	}

	static SettingsSchemaRegistry g_settingsSchema;
	SettingsSchemaRegistry& SettingsSchema() {
		return g_settingsSchema;
	}
}
