#include "ModContext.hpp"

// Out of line so this header stays free of the settings store.
#include "SettingsStore.hpp"

namespace Framework {
	bool ModContext::IsOn(std::string_view key) const { return SettingValues().IsOn(key); }
	bool ModContext::IsOff(std::string_view key) const { return SettingValues().IsOff(key); }
	std::string ModContext::Value(std::string_view key) const { return SettingValues().Value(key); }
	int ModContext::Int(std::string_view key) const { return SettingValues().Int(key); }

	Settings::When ModContext::When(std::string_view key) const { return SettingValues().WhenOf(key); }
	Settings::StringColorMode ModContext::ColorMode() const { return SettingValues().ColorMode(); }
	Settings::NoteColorMode ModContext::NoteColorMode() const { return SettingValues().NoteColorMode(); }
}
