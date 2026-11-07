#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "SettingsSchema.hpp"

namespace Framework {
	// A mods.manifest.json read back into declarations, so a host's mods share the Windows mods' keys, INI locations and
	// defaults. A mod that keeps the Windows mod's Id() returns ForMod(Id()) from Settings(), adding any settings of its
	// own, or picks with Setting(key) when it has only some of them or they behave differently: Setting(key).Hint("...").
	class SettingsManifest {
	public:
		// Throws if the text isn't a manifest: malformed JSON, a missing field, an unknown type or heading.
		explicit SettingsManifest(std::string_view json);

		// Both throw for what the manifest doesn't have, so a mod renamed on Windows fails its host's load (ModRegistry
		// rejects it and logs why) rather than quietly losing its settings.
		[[nodiscard]] const SettingDefs& ForMod(std::string_view modId) const;
		[[nodiscard]] const SettingDef& Setting(std::string_view key) const;

		[[nodiscard]] std::vector<std::string> Mods() const; // In manifest order.

		// One line per way `declared` contradicts the manifest, for a host's build to fail on: both platforms read one
		// RSMods.ini, so a shared key must agree on where it lives, its type, default and choices, and a new key can't
		// take a location the manifest uses. Labels and hints may differ, and a host needn't have every setting.
		[[nodiscard]] std::vector<std::string> Conflicts(const SettingDefs& declared) const;

	private:
		[[nodiscard]] const SettingDef* Find(std::string_view key) const;

		std::vector<std::pair<std::string, SettingDefs>> mods;
	};
}
