#include "../stdafx.h"
#include "LoftMod.hpp"
#include "Loft.hpp"
#include "GreenScreenWallMod.hpp"

using Framework::ModContext;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using Settings::When;
namespace Setting = Settings::Setting;

bool LoftMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::ToggleLoftEnabled);
}

void LoftMod::OnInitialize(ModContext& c) {
	c.Commands().BindSetting(
		Setting::Key::ToggleLoft,
		KeyEdge::Up,
		Availability::Active,
		[](ModContext&, const KeyEvent&) { Loft::ToggleLoft(); },
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn(Setting::ToggleLoftEnabled);
		},
		"Toggle Loft");
}

void LoftMod::OnDisabled(ModContext&) {
	if (loftOff) {
		Loft::ToggleLoft();
		loftOff = false;
	}
	GreenScreenWallMod::SetLessonWall(false);
}

void LoftMod::OnSongTick(ModContext& c) {
	if (c.When(Setting::ToggleLoftWhen) == When::Song) {
		if (!loftOff) {
			Loft::ToggleLoft();
		}
		loftOff = true;
	}
	ApplyAlwaysOn(c);
}

void LoftMod::OnMenuTick(ModContext& c) {
	if (c.When(Setting::ToggleLoftWhen) == When::Song) {
		if (loftOff) {
			Loft::ToggleLoft();
			loftOff = false;
		}
		
		if (!GameState::LessonMode) GreenScreenWallMod::SetLessonWall(false);
	}
	ApplyAlwaysOn(c);
}

// Runs after the phase policy so lesson mode takes precedence.
void LoftMod::ApplyAlwaysOn(ModContext& c) {
	const When when = c.When(Setting::ToggleLoftWhen);
	if (when == When::Manual) {
		if (loftOff) {
			Loft::ToggleLoft();
			loftOff = false;
		}
		GreenScreenWallMod::SetLessonWall(false);
		return;
	}

	// Lesson videos require the loft; GreenScreenWall preserves the no-loft appearance.
	if (GameState::LessonMode && when != When::Manual) {
		if (loftOff)
			Loft::ToggleLoft();
		loftOff = false;
		GreenScreenWallMod::SetLessonWall(true);
	}

	if (!loftOff && !GameState::LessonMode && when == When::Startup) {
		Loft::ToggleLoft();
		loftOff = true;
		GreenScreenWallMod::SetLessonWall(false);
	}
}

static Framework::ModRegistrar<LoftMod> _loftReg;
