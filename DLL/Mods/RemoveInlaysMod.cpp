#include "../stdafx.h"
#include "RemoveInlaysMod.hpp"
#include "../D3D/D3D.hpp"
#include "../D3D/D3DHelper.hpp"

using Framework::ModContext;
using Framework::DrawContext;
using Framework::DrawResult;
using Framework::DrawOutcome;
using Framework::DrawPath;
using Framework::SettingDefs;
using Framework::SettingDef;
namespace Setting = Settings::Setting;

SettingDefs RemoveInlaysMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::RemoveInlaysEnabled, "Inlays", "Remove Inlays")
	};
}

bool RemoveInlaysMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::RemoveInlaysEnabled);
}

void RemoveInlaysMod::OnInitialize(ModContext& c) {
	c.Draw().Register("RemoveInlays", 0, DrawPath::Indexed, [](DrawContext& ctx) -> DrawResult {
		if (ctx.inSong && IsExtraRemoved(inlays, ctx.thicc)) {
			return { DrawOutcome::Hide };
		}

		return { DrawOutcome::Pass };
	});
}

static Framework::ModRegistrar<RemoveInlaysMod> _removeInlaysReg;
