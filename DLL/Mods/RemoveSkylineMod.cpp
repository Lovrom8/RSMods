#include "../stdafx.h"
#include "RemoveSkylineMod.hpp"
#include "../D3D/D3D.hpp"
#include "DrawMeshTags.hpp"

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

		if (DrawMesh::StageMatchesAny(ctx, 1, { D3D::Crc::SkylinePurple, D3D::Crc::SkylineOrange })) {
			skylineOff.store(true, std::memory_order_relaxed);
			return { DrawOutcome::Hide };
		}

		if (DrawMesh::StageMatchesAny(ctx, 0, { D3D::Crc::SkylineBackground, D3D::Crc::SkylineShadow })) {
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
