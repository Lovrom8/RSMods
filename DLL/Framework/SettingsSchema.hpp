#pragma once

#include <algorithm>
#include <cassert>
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

			VisibleWhen() = default;
			VisibleWhen(std::string_view k, std::string_view eq)
				: key(k), equals(eq) {}
		};

		std::optional<VisibleWhen> visibleWhen = std::nullopt;           // Tier 2: { key, equals }
		std::string editor = {};                                         // Tier 3: custom UserControl name

		// --- Fluent Builders (paired && and const & overloads for temporary safety) ---

		SettingDef Ini(std::string_view section, std::string_view name) && {
			ini.section = std::string(section);
			ini.name = std::string(name);
			return std::move(*this);
		}
		SettingDef Ini(std::string_view section, std::string_view name) const & {
			SettingDef copy = *this;
			copy.ini.section = std::string(section);
			copy.ini.name = std::string(name);
			return copy;
		}

		SettingDef IniName(std::string_view name) && {
			ini.name = std::string(name);
			return std::move(*this);
		}
		SettingDef IniName(std::string_view name) const & {
			SettingDef copy = *this;
			copy.ini.name = std::string(name);
			return copy;
		}

		SettingDef Default(std::string_view defaultVal) && {
			def = std::string(defaultVal);
			return std::move(*this);
		}
		SettingDef Default(std::string_view defaultVal) const & {
			SettingDef copy = *this;
			copy.def = std::string(defaultVal);
			return copy;
		}

		SettingDef Category(std::string_view cat) && {
			category = std::string(cat);
			return std::move(*this);
		}
		SettingDef Category(std::string_view cat) const & {
			SettingDef copy = *this;
			copy.category = std::string(cat);
			return copy;
		}

		SettingDef Hint(std::string_view h) && {
			hint = std::string(h);
			return std::move(*this);
		}
		SettingDef Hint(std::string_view h) const & {
			SettingDef copy = *this;
			copy.hint = std::string(h);
			return copy;
		}

		SettingDef Choices(std::vector<std::string> choiceList, std::string_view defaultChoice = {}) && {
			choices = std::move(choiceList);
			if (!defaultChoice.empty()) {
				assert(std::ranges::find(choices, defaultChoice) != choices.end() && "Enum default value must be present in choices list");
				def = std::string(defaultChoice);
			}
			else if (def.empty() && !choices.empty()) {
				def = choices.front();
			}
			return std::move(*this);
		}
		SettingDef Choices(std::vector<std::string> choiceList, std::string_view defaultChoice = {}) const & {
			SettingDef copy = *this;
			copy.choices = std::move(choiceList);
			if (!defaultChoice.empty()) {
				assert(std::ranges::find(copy.choices, defaultChoice) != copy.choices.end() && "Enum default value must be present in choices list");
				copy.def = std::string(defaultChoice);
			}
			else if (copy.def.empty() && !copy.choices.empty()) {
				copy.def = copy.choices.front();
			}
			return copy;
		}

		SettingDef Range(int minVal, int maxVal) && {
			min = minVal;
			max = maxVal;
			return std::move(*this);
		}
		SettingDef Range(int minVal, int maxVal) const & {
			SettingDef copy = *this;
			copy.min = minVal;
			copy.max = maxVal;
			return copy;
		}

		SettingDef Scale(double scaleVal) && {
			scale = scaleVal;
			return std::move(*this);
		}
		SettingDef Scale(double scaleVal) const & {
			SettingDef copy = *this;
			copy.scale = scaleVal;
			return copy;
		}

		SettingDef WithVisibleWhen(std::string_view parentKey, std::string_view equalsVal = "on") && {
			visibleWhen.emplace(parentKey, equalsVal);
			return std::move(*this);
		}
		SettingDef WithVisibleWhen(std::string_view parentKey, std::string_view equalsVal = "on") const & {
			SettingDef copy = *this;
			copy.visibleWhen.emplace(parentKey, equalsVal);
			return copy;
		}

		SettingDef ChoicesSource(std::string_view source) && {
			choicesSource = source;
			return std::move(*this);
		}
		SettingDef ChoicesSource(std::string_view source) const & {
			SettingDef copy = *this;
			copy.choicesSource = source;
			return copy;
		}

		SettingDef WithEditor(std::string_view editorName) && {
			editor = editorName;
			return std::move(*this);
		}
		SettingDef WithEditor(std::string_view editorName) const & {
			SettingDef copy = *this;
			copy.editor = editorName;
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

		// --- Base Factories for Common Kinds ---

		// Standard toggle switch (defaults section/category to "Toggle Switches", def to "off", iniName to key)
		static SettingDef Toggle(std::string_view key, std::string_view label) {
			return SettingDef{
				.key = std::string(key),
				.ini = { "Toggle Switches", std::string(key) },
				.type = SettingType::Bool,
				.def = "off",
				.label = std::string(label),
				.category = "Toggle Switches"
			};
		}

		// Toggle with custom INI name (and optional full tailoring)
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

		// Enum choice setting (chain with .Choices({...}, defaultChoice))
		static SettingDef Enum(std::string_view key, std::string_view label) {
			return SettingDef{
				.key = std::string(key),
				.ini = { "Toggle Switches", std::string(key) },
				.type = SettingType::Enum,
				.label = std::string(label),
				.category = "Toggle Switches"
			};
		}

		// Integer numeric setting (chain with .Range(min, max), .Default(...))
		static SettingDef Numeric(std::string_view key, std::string_view label) {
			return SettingDef{
				.key = std::string(key),
				.ini = { "Mod Settings", std::string(key) },
				.type = SettingType::Int,
				.def = "0",
				.label = std::string(label),
				.category = "Mod Settings"
			};
		}

		// String setting (chain with .Ini(), .Default(), etc.)
		static SettingDef String(std::string_view key, std::string_view label) {
			return SettingDef{
				.key = std::string(key),
				.ini = { "Toggle Switches", std::string(key) },
				.type = SettingType::String,
				.def = "",
				.label = std::string(label),
				.category = "Toggle Switches"
			};
		}

		// Positional factories for backward compatibility
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

	// Free factory functions for SettingDef so mods can use:
	//   using Framework::Toggle; using Framework::Numeric; using Framework::Enum; using Framework::String;
	inline SettingDef Toggle(std::string_view key, std::string_view label) {
		return SettingDef::Toggle(key, label);
	}
	inline SettingDef Toggle(std::string_view key, std::string_view iniName, std::string_view label,
		std::string_view section = "Toggle Switches", std::string_view category = "Toggle Switches",
		std::string_view def = "off", std::string_view hint = {}) {
		return SettingDef::Toggle(key, iniName, label, section, category, def, hint);
	}
	inline SettingDef Enum(std::string_view key, std::string_view label) {
		return SettingDef::Enum(key, label);
	}
	inline SettingDef Numeric(std::string_view key, std::string_view label) {
		return SettingDef::Numeric(key, label);
	}
	inline SettingDef String(std::string_view key, std::string_view label) {
		return SettingDef::String(key, label);
	}

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
