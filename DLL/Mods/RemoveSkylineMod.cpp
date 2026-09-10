#include "../stdafx.h"
#include "RemoveSkylineMod.hpp"
#include "../D3D/D3D.hpp"

using Framework::ModContext;
using Framework::DrawContext;
using Framework::DrawResult;
using Framework::DrawOutcome;
using Framework::DrawPath;
using Settings::When;
namespace Setting = Settings::Setting;

bool RemoveSkylineMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::RemoveSkylineEnabled);
}

void RemoveSkylineMod::OnInitialize(ModContext& c) {
	c.Draw().Register("RemoveSkyline", 0, DrawPath::Indexed, [this](DrawContext& ctx) -> DrawResult {
		if (!toggleSkyline.load(std::memory_order_relaxed)) {
			return { DrawOutcome::Pass };
		}

		if (ctx.mesh.Stride != 16) {
			return { DrawOutcome::Pass };
		}

		if (drawSkylineInMenu.load(std::memory_order_relaxed)) {
			skylineOff.store(false, std::memory_order_relaxed);
			return { DrawOutcome::Show };
		}

		auto crc1 = ctx.StageCRC(1);
		if (crc1 && (*crc1 == crcSkylinePurple || *crc1 == crcSkylineOrange)) {
			skylineOff.store(true, std::memory_order_relaxed);
			return { DrawOutcome::Hide };
		}

		auto crc0 = ctx.StageCRC(0);
		if (crc0 && (*crc0 == crcSkylineBackground || *crc0 == crcSkylineShadow)) {
			skylineOff.store(true, std::memory_order_relaxed);
			return { DrawOutcome::Hide };
		}

		return { DrawOutcome::Pass };
	});
}

void RemoveSkylineMod::OnEnabled(ModContext& c) {
	skylineOff.store(false, std::memory_order_relaxed);
	ApplyStartup(c);
}

void RemoveSkylineMod::OnDisabled(ModContext&) {
	toggleSkyline.store(false, std::memory_order_relaxed);
	skylineOff.store(false, std::memory_order_relaxed);
	drawSkylineInMenu.store(false, std::memory_order_relaxed);
}

void RemoveSkylineMod::OnSettingsChanged(ModContext& c) {
	ApplyStartup(c);
}

void RemoveSkylineMod::OnSongTick(ModContext& c) {
	if (c.When(Setting::ToggleSkylineWhen) == When::Song) {
		if (!skylineOff.load(std::memory_order_relaxed)) {
			toggleSkyline.store(true, std::memory_order_relaxed);
		}
		
		drawSkylineInMenu.store(false, std::memory_order_relaxed);
	}

	ApplyStartup(c);
}

void RemoveSkylineMod::OnMenuTick(ModContext& c) {
	// Coming back from a song with the skyline off: request the toggle and let it draw in the menu again.
	if (skylineOff.load(std::memory_order_relaxed) && c.When(Setting::ToggleSkylineWhen) == When::Song) {
		toggleSkyline.store(true, std::memory_order_relaxed);
		drawSkylineInMenu.store(true, std::memory_order_relaxed);
	}

	ApplyStartup(c);
}

void RemoveSkylineMod::ApplyStartup(ModContext& c) {
	if (!skylineOff.load(std::memory_order_relaxed) && c.When(Setting::ToggleSkylineWhen) == When::Startup) {
		toggleSkyline.store(true, std::memory_order_relaxed);
	}
}

static Framework::ModRegistrar<RemoveSkylineMod> _removeSkylineReg;
