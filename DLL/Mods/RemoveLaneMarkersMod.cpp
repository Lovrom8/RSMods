#include "../stdafx.h"
#include "RemoveLaneMarkersMod.hpp"
#include "../D3D/D3D.hpp"
#include "../D3D/D3DHelper.hpp"

using Framework::ModContext;
using Framework::DrawContext;
using Framework::DrawResult;
using Framework::DrawOutcome;
using Framework::DrawPath;
namespace Setting = Settings::Setting;

bool RemoveLaneMarkersMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::RemoveLaneMarkersEnabled);
}

void RemoveLaneMarkersMod::OnInitialize(ModContext& c) {
	c.Draw().Register("RemoveLaneMarkers", 0, DrawPath::Indexed, [](DrawContext& ctx) -> DrawResult {
		if (ctx.inSong && IsExtraRemoved(laneMarkers, ctx.thicc)) {
			return { DrawOutcome::Hide };
		}
		return { DrawOutcome::Pass };
	});
}

static Framework::ModRegistrar<RemoveLaneMarkersMod> _removeLaneMarkersReg;
