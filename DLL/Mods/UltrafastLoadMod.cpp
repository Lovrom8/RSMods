#include "../stdafx.h"
#include "UltrafastLoadMod.hpp"

using Framework::SettingDefs;
using Framework::SettingDef;
namespace Setting = Settings::Setting;

namespace {
	constexpr char iniSection[] = "Mod Settings";
	constexpr char iniName[] = "UltrafastLoad";
}

SettingDefs UltrafastLoadMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::UltrafastLoad, iniName, "Ultrafast Load", iniSection, iniSection, "on")
			.Hint("Starts the game and adds your songs much faster, most of all with a big song library.\n"
				"Turn it off if you see problems at startup or with your song list.\n"
				"Takes effect the next time you start the game.")
			.Heading(Framework::SettingHeading::GameAndWindow),
	};
}

bool UltrafastLoad::Enabled() {
	if (GameVersion::Current() == Framework::GameVersion::Build::Unknown) {
		static const bool logged = [] {
			LOG_WARNING("(STARTUP) Ultrafast Load disabled: unknown executable" << std::endl);
			return true;
		}();
		(void)logged;
		return false;
	}
	static const bool enabled = [] {
		CSimpleIniA reader;
		if (reader.LoadFile("RSMods.ini") < 0)
			return true;
		return std::string(reader.GetValue(iniSection, iniName, "on")) != "off";
	}();
	return enabled;
}

static Framework::ModRegistrar<UltrafastLoadMod> _ultrafastLoadReg;
