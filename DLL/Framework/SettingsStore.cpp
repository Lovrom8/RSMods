#include "SettingsStore.hpp"

#include <mutex>

namespace Framework {
	namespace {
		Settings::When ParseWhen(std::string_view value) {
			using Settings::When;
			
			if (value == "manual")    return When::Manual;
			if (value == "startup")   return When::Startup;
			if (value == "song")      return When::Song;
			if (value == "tuner")     return When::Tuner;
			if (value == "automatic") return When::Automatic;

			return When::Unknown;
		}
	}

	void SettingsStore::LoadDefaults(std::span<const UndeclaredSetting> undeclared) {
		Values next;
		for (const UndeclaredSetting& setting : undeclared)
			next.strings[setting.key] = setting.def;

		for (const auto& decl : SettingsSchema().GetAll()) {
			if (decl.type == SettingType::Int)
				next.ints.try_emplace(decl.key, decl.GetIntDefault());
			else
				next.strings.try_emplace(decl.key, decl.def);
		}
		Publish(std::move(next));
	}

	void SettingsStore::Publish(Values next) {
		std::unique_lock lock(mutex);

		values = std::move(next);
	}

	std::string SettingsStore::Value(std::string_view key) const {
		std::shared_lock lock(mutex);

		auto it = values.strings.find(key);
		return it != values.strings.end() ? it->second : std::string();
	}

	int SettingsStore::Int(std::string_view key) const {
		std::shared_lock lock(mutex);

		auto it = values.ints.find(key);
		if (it != values.ints.end())
			return it->second;

		auto itString = values.strings.find(key);
		if (itString != values.strings.end() && !itString->second.empty()) {
			try {
				return std::stoi(itString->second);
			} catch (...) {}
		}

		return 0;
	}

	bool SettingsStore::IsOn(std::string_view key) const { return Value(key) == "on"; }
	bool SettingsStore::IsOff(std::string_view key) const { return Value(key) == "off"; }

	Settings::When SettingsStore::WhenOf(std::string_view key) const { return ParseWhen(Value(key)); }

	Settings::StringColorMode SettingsStore::ColorMode() const {
		return static_cast<Settings::StringColorMode>(Int(Settings::Setting::CustomStringColors));
	}

	Settings::NoteColorMode SettingsStore::NoteColorMode() const {
		return static_cast<Settings::NoteColorMode>(Int(Settings::Setting::SeparateNoteColorsMode));
	}

	void SettingsStore::SetValue(std::string_view key, std::string_view value) {
		std::unique_lock lock(mutex);

		values.strings.insert_or_assign(std::string(key), std::string(value));
	}

	void SettingsStore::SetInt(std::string_view key, int value) {
		std::unique_lock lock(mutex);

		values.ints.insert_or_assign(std::string(key), value);
	}

	void SettingsStore::FlipToggle(std::string_view key) {
		std::unique_lock lock(mutex);

		std::string& value = values.strings[std::string(key)];
		value = value == "on" ? "off" : "on";
	}

	SettingsStore& SettingValues() {
		static SettingsStore instance;

		return instance;
	}
}
