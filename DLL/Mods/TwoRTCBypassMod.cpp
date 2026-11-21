#include "../stdafx.h"
#include "TwoRTCBypassMod.hpp"

#include <stdexcept>

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;
namespace Setting = Settings::Setting;

namespace {
	using namespace Framework::InstructionPatch;

	SiteDescriptor<25> TwoRtcSite() {
		const auto replacement = MakeBytes(
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90);
		return SitePatch::MakeSite("two-RTC connection check", Offsets::ptr_twoRTCBypass.GetValue(),
			VersionedPattern<25>{
				ExactPattern(MakeBytes(0xE8, 0x7C, 0x22, 0xE2, 0xFF, 0x84, 0xC0, 0x74, 0x6B, 0xBA,
					0x34, 0xFE, 0x1C, 0x01, 0x8D, 0xB3, 0x4C, 0x01, 0x00, 0x00, 0xE8, 0xD8, 0xD8, 0xC4, 0xFF)),
				SitePatch::ExpandLnpPattern<25>("E8 9C 24 E2 FF 84 C0 74 6B BA @DD165C 8D B3 4C 01 00 00 E8 18 CA C4 FF")
			}, replacement);
	}
}

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
	const auto result = enable
		? bypassPatch_.Install("Two-RTC bypass", SitePatch::ReadSite, TwoRtcSite())
		: bypassPatch_.Restore("Two-RTC restore");
	if ((enable && result == MemUtil::PatchResult::Installed)
		|| (!enable && result == MemUtil::PatchResult::Original))
		return;
	throw std::runtime_error(result == MemUtil::PatchResult::RollbackIncomplete
		? "two-RTC patch outcome is indeterminate"
		: "two-RTC patch could not be installed");
}

static Framework::ModRegistrar<TwoRTCBypassMod> _twoRTCBypassReg;
