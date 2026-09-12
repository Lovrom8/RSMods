#include "../stdafx.h"
#include "NonStopPlayTimerMod.hpp"

using Framework::ModContext;
using Framework::SettingDefs;
using Framework::SettingDef;
namespace Setting = Settings::Setting;

SettingDefs NonStopPlayTimerMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::UseCustomNSPTimer, "UseCustomNSPTimer", "Use Custom Non-Stop Play Timer"),
		SettingDef::Numeric(Setting::CustomNSPTimeLimit, "NSP Time Limit (seconds)")
			.Ini("Mod Settings", "CustomNSPTimeLimit")
			.Default("10000")
			.Range(2000, 60000)
			.Scale(0.001)
			.WithVisibleWhen(Setting::UseCustomNSPTimer)
	};
}

void NonStopPlayTimerMod::OnEnabled(ModContext& c) {
	ApplyTimer(c);
	active = true;
}

void NonStopPlayTimerMod::OnSettingsChanged(ModContext& c) {
	if (active)
		ApplyTimer(c);
}

void NonStopPlayTimerMod::OnDisabled(ModContext&) {
	active = false;
}

void NonStopPlayTimerMod::ApplyTimer(ModContext& c) {
	const double desired = c.IsOn(Setting::UseCustomNSPTimer)
		? c.Int(Setting::CustomNSPTimeLimit) / 1000.0
		: DefaultTimeLimit;

	const double current = SongTimer::GetNonStopPlayTimer();

	const double eps = std::numeric_limits<double>::epsilon() * std::max(1.0, std::max(std::abs(desired), std::abs(current))) * 4;
	if (std::abs(current - desired) > eps) {
		LOG_INFO("Updating NSP timer..." << std::endl);
		SongTimer::SetNonStopPlayTimer(desired);
	}
}

static Framework::ModRegistrar<NonStopPlayTimerMod> _nonStopPlayTimerReg;
