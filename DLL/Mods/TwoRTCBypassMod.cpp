#include "../stdafx.h"
#include "TwoRTCBypassMod.hpp"

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;
namespace Setting = Settings::Setting;

SettingDefs TwoRTCBypassMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::BypassTwoRTCMessageBox, "BypassTwoRTCMessageBox", "Bypass Two Real Tone Cables Warning")
			.Hint("Skips the warning Rocksmith shows when two Real Tone Cables are detected.")
			.Heading(Framework::SettingHeading::GameAndWindow)
	};
}

bool TwoRTCBypassMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::BypassTwoRTCMessageBox);
}

void TwoRTCBypassMod::OnEnabled(ModContext&) {
	SetTwoRTCBypass(true);
}

void TwoRTCBypassMod::OnDisabled(ModContext&) {
	SetTwoRTCBypass(false);
}

void TwoRTCBypassMod::SetTwoRTCBypass(bool enable) {
	if (enable && bypassInstalled)
		return;
	if (!enable && !bypassInstalled && !bypassNeedsResolution)
		return;

	const auto result = QualityOfLife::PatchTwoRTC(enable);
	if (result.outcome == Framework::InstructionPatch::Outcome::Installed) {
		bypassInstalled = enable;
		bypassNeedsResolution = false;
	}
	else if (result.outcome == Framework::InstructionPatch::Outcome::RollbackIncomplete) {
		bypassNeedsResolution = true;
	}
}

static Framework::ModRegistrar<TwoRTCBypassMod> _twoRTCBypassReg;
