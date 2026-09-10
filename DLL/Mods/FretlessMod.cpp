#include "../stdafx.h"
#include "FretlessMod.hpp"
#include "../D3D/D3D.hpp"
#include "../D3D/D3DHelper.hpp"

using Framework::ModContext;
using Framework::DrawContext;
using Framework::DrawResult;
using Framework::DrawOutcome;
using Framework::DrawPath;
namespace Setting = Settings::Setting;

bool FretlessMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::FretlessModeEnabled);
}

void FretlessMod::OnInitialize(ModContext& c) {
	c.Draw().Register("Fretless", 0, DrawPath::Indexed, [](DrawContext& ctx) -> DrawResult {
		if (ctx.inSong && IsExtraRemoved(fretless, ctx.thicc)) {
			return { DrawOutcome::Hide };
		}
		return { DrawOutcome::Pass };
	});
}

static Framework::ModRegistrar<FretlessMod> _fretlessReg;
