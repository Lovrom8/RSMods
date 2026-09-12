#include "../stdafx.h"
#include "TwoRTCBypassMod.hpp"

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;
namespace Setting = Settings::Setting;

SettingDefs TwoRTCBypassMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::BypassTwoRTCMessageBox, "BypassTwoRTCMessageBox", "Bypass Two Real Tone Cables Warning")
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

// PatchTwoRTC overwrites 25 bytes of the connection check, so capture those exact live bytes
// before this mod's first patch and restore the complete instruction sequence when disabled.
// If another component (such as RS_ASIO) already applied the bypass, leave its patch alone.
void TwoRTCBypassMod::SetTwoRTCBypass(bool enable) {
	const bool isPatched = *(char*)Offsets::ptr_twoRTCBypass.Get() == Offsets::ptr_twoRTCBypass_patch_call[0];
	if (enable == isPatched)
		return;

	if (enable) {
		if (!capturedOriginal) {
			memcpy(originalBytes, (const void*)Offsets::ptr_twoRTCBypass.Get(), sizeof(originalBytes));
			capturedOriginal = true;
		}

		QualityOfLife::PatchTwoRTC();
	}
	else if (capturedOriginal) {
		MemUtil::PatchAdr((LPVOID)Offsets::ptr_twoRTCBypass.Get(), originalBytes, sizeof(originalBytes));
	}
}

static Framework::ModRegistrar<TwoRTCBypassMod> _twoRTCBypassReg;
