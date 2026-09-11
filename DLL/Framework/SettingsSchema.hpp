#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace Framework {
	enum class SettingType {
		Bool,
		Int,
		Enum,
		String,
		Color
	};

	inline const char* SettingTypeName(SettingType t) {
		switch (t) {
			case SettingType::Bool:   return "Bool";
			case SettingType::Int:    return "Int";
			case SettingType::Enum:   return "Enum";
			case SettingType::String: return "String";
			case SettingType::Color:  return "Color";
		}
		return "Bool";
	}

	// Declarative specification of a mod setting.
	struct SettingDef {
		std::string key;                                                 // in-code key; matches a Settings::Setting constant
		struct {
			std::string section;
			std::string name; 
		} ini; // on-disk location (may differ from key)
		
		SettingType type = SettingType::Bool;
		std::string def;                                                 // default as its INI string form ("0", "17", ...)
		std::string label;                                               // GUI label
		std::string hint = {};                                           // optional GUI tooltip
		std::string category = {};                                       // GUI section grouping
		std::optional<int> min = std::nullopt;                           // Int only
		std::optional<int> max = std::nullopt;                           // Int only
		std::optional<double> scale = std::nullopt;                      // Tier 2: e.g. 0.001 to show ms as seconds
		std::vector<std::string> choices = {};                           // Enum only: display order = underlying int
		std::string choicesSource = {};                                  // Tier 2: "SystemFonts" | "MidiOutDevices" | ...
		
		struct VisibleWhen {
			std::string key;
			std::string equals;
		};

		std::optional<VisibleWhen> visibleWhen = std::nullopt;           // Tier 2: { key, equals }
		std::string editor = {};                                         // Tier 3: custom UserControl name

		SettingDef WithVisibleWhen(std::string_view parentKey, std::string_view equalsVal = "on") && {
			visibleWhen = VisibleWhen{ std::string(parentKey), std::string(equalsVal) };
			return std::move(*this);
		}

		SettingDef WithVisibleWhen(std::string_view parentKey, std::string_view equalsVal = "on") const & {
			SettingDef copy = *this;
			copy.visibleWhen = VisibleWhen{ std::string(parentKey), std::string(equalsVal) };
			return copy;
		}

		// Helper to safely parse an integer default
		int GetIntDefault(int fallback = 0) const {
			if (def.empty()) return fallback;
			try {
				return std::stoi(def);
			}
			catch (...) {
				return fallback;
			}
		}

		// Factory for standard boolean switches
		static SettingDef Toggle(std::string_view key, std::string_view iniName, std::string_view label,
			std::string_view section = "Toggle Switches", std::string_view category = "Toggle Switches",
			std::string_view def = "off", std::string_view hint = {}) {
			return SettingDef{
				.key = std::string(key),
				.ini = { std::string(section), std::string(iniName) },
				.type = SettingType::Bool,
				.def = std::string(def),
				.label = std::string(label),
				.hint = std::string(hint),
				.category = std::string(category)
			};
		}

		// Factory for integer settings
		static SettingDef Numeric(std::string_view key, std::string_view section, std::string_view iniName,
			std::string_view label, std::string_view def, std::optional<int> min = std::nullopt,
			std::optional<int> max = std::nullopt, std::optional<double> scale = std::nullopt,
			std::string_view category = "Mod Settings", std::string_view hint = {}) {
			return SettingDef{
				.key = std::string(key),
				.ini = { std::string(section), std::string(iniName) },
				.type = SettingType::Int,
				.def = std::string(def),
				.label = std::string(label),
				.hint = std::string(hint),
				.category = std::string(category),
				.min = min,
				.max = max,
				.scale = scale
			};
		}

		// Factory for static enum choice settings
		static SettingDef EnumChoice(std::string_view key, std::string_view section, std::string_view iniName,
			std::string_view label, std::string_view def, std::vector<std::string> choices,
			std::string_view category = "Mod Settings", std::string_view hint = {}) {
			return SettingDef{
				.key = std::string(key),
				.ini = { std::string(section), std::string(iniName) },
				.type = SettingType::Enum,
				.def = std::string(def),
				.label = std::string(label),
				.hint = std::string(hint),
				.category = std::string(category),
				.choices = std::move(choices)
			};
		}
	};

	using SettingDecl = SettingDef;
	using SettingDefs = std::vector<SettingDef>;

	class IMod;

	// Registry for declared mod settings.
	// Threading: Not internally synchronized; correctness rests on MainThread confinement
	// for mod registration (Register), mod retirement (RemoveMod), and settings queries (GetAll).
	class SettingsSchemaRegistry {
	public:
		SettingsSchemaRegistry() = default;

		// Registers all settings declared by a mod.
		// Returns true if all settings registered successfully.
		// Returns false and sets outDuplicateKey if a duplicate setting key is detected.
		bool Register(const IMod* owner, const SettingDefs& decls, std::string* outDuplicateKey = nullptr);

		// Removes settings registered by owner
		void RemoveMod(const IMod* owner);

		// Queries
		const SettingDef* Find(std::string_view key) const;
		bool Has(std::string_view key) const;
		size_t Size() const { return entries.size(); }

		// Returns all registered setting declarations in registration order
		SettingDefs GetAll() const;

		// Serializes the aggregate schema to JSON matching the manifest contract in settings-schema.md
		std::string DumpManifestJson() const;

		void Clear();

	private:
		struct Entry {
			const IMod* owner = nullptr;
			SettingDef decl;
		};
		std::vector<Entry> entries;
		std::unordered_map<std::string, size_t> keyToIndex;
	};

	SettingsSchemaRegistry& SettingsSchema();
}
