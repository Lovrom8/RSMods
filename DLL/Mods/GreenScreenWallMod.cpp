#include "../stdafx.h"
#include "GreenScreenWallMod.hpp"
#include "../D3D/D3D.hpp"
#include "../D3D/D3DHelper.hpp"

using Framework::ModContext;
using Framework::DrawContext;
using Framework::DrawResult;
using Framework::DrawOutcome;
using Framework::DrawPath;
using Framework::SettingDef;
using Framework::SettingDefs;
namespace Setting = Settings::Setting;

SettingDefs GreenScreenWallMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::GreenScreenWallEnabled, "GreenScreenWall", "Green Screen Wall")
	};
}

bool GreenScreenWallMod::IsEnabled(const ModContext& c) const {
	return true;
}

void GreenScreenWallMod::SetLessonWall(bool active) {
	lessonWallActive.store(active, std::memory_order_relaxed);
}

void GreenScreenWallMod::OnSettingsChanged(ModContext& c) {
	settingEnabled.store(c.IsOn(Setting::GreenScreenWallEnabled), std::memory_order_relaxed);
}

void GreenScreenWallMod::OnInitialize(ModContext& c) {
	settingEnabled.store(c.IsOn(Setting::GreenScreenWallEnabled), std::memory_order_relaxed);

	c.Draw().Register("GreenScreenWall", 0, DrawPath::Indexed, [this](DrawContext& ctx) -> DrawResult {
		const bool active = settingEnabled.load(std::memory_order_relaxed) ||
			lessonWallActive.load(std::memory_order_relaxed);

		if (active && IsExtraRemoved(greenScreenWallMesh, ctx.thicc)) {
			return { DrawOutcome::Hide };
		}
		return { DrawOutcome::Pass };
	});
}

static Framework::ModRegistrar<GreenScreenWallMod> _greenScreenWallReg;
