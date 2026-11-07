#pragma once

#include <functional>
#include <map>
#include <shared_mutex>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "SettingsSchema.hpp"

namespace Settings {
	enum class When { Unknown, Manual, Startup, Song, Tuner, Automatic };
	enum class StringColorMode { Default = 0, Zag = 1, Custom = 2, Test = 3 };
	enum class NoteColorMode { SameAsStrings = 0, Default = 1, Custom = 2 };

	namespace Setting {
		inline constexpr char CustomStringColors[]     = "CustomStringColors";
		inline constexpr char SeparateNoteColorsMode[] = "SeparateNoteColorsMode";
	}
}

namespace Framework {
	// A host's own setting: no mod declares it, so the schema can't say where it lives.
	struct UndeclaredSetting {
		const char* key;
		const char* section;
		const char* name;
		const char* def;
	};

	// The current value of every setting, read from any thread (the render thread, CrowdControl). Int settings keep
	// ints, the rest strings. Reads never insert, so a miss can't mutate the maps under another reader.
	class SettingsStore {
	public:
		// Every declared setting at its default, for when there's no INI to load.
		void LoadDefaults(std::span<const UndeclaredSetting> undeclared);

		// Replaces every value at once. IniReader is CSimpleIniA, or anything with its GetValue and GetLongValue: which
		// text conversion SimpleIni builds with differs per platform, so the host owns the reader and the file's path.
		template <typename IniReader>
		void Load(const IniReader& reader, std::span<const UndeclaredSetting> undeclared) {
			Values next;
			for (const UndeclaredSetting& setting : undeclared)
				next.strings[setting.key] = reader.GetValue(setting.section, setting.name, setting.def);

			for (const auto& decl : SettingsSchema().GetAll()) {
				if (decl.type == SettingType::Int)
					next.ints[decl.key] = static_cast<int>(reader.GetLongValue(decl.ini.section.c_str(), decl.ini.name.c_str(), decl.GetIntDefault()));
				else
					next.strings[decl.key] = reader.GetValue(decl.ini.section.c_str(), decl.ini.name.c_str(), decl.def.c_str());
			}
			Publish(std::move(next));
		}

		[[nodiscard]] std::string Value(std::string_view key) const;
		[[nodiscard]] int Int(std::string_view key) const; // An int setting, or a string one holding a number; 0 otherwise.

		[[nodiscard]] bool IsOn(std::string_view key) const;
		// Deliberately not !IsOn: an unset or unrecognized value is neither on nor off.
		[[nodiscard]] bool IsOff(std::string_view key) const;

		[[nodiscard]] Settings::When WhenOf(std::string_view key) const; // Unrecognized or empty is When::Unknown.
		// The stored int maps 1:1 onto the enum, so an out-of-range value stays out of range for consumers' default cases.
		[[nodiscard]] Settings::StringColorMode ColorMode() const;
		[[nodiscard]] Settings::NoteColorMode NoteColorMode() const;

		void SetValue(std::string_view key, std::string_view value);
		void SetInt(std::string_view key, int value);
		void FlipToggle(std::string_view key);

	private:
		struct Values {
			std::map<std::string, std::string, std::less<>> strings;
			std::map<std::string, int, std::less<>> ints;
		};

		void Publish(Values next);

		mutable std::shared_mutex mutex;
		Values values;
	};

	SettingsStore& SettingValues();
}
