#include "../stdafx.h"
#include "RemoveLyricsMod.hpp"
#include "../D3D/D3D.hpp"
#include "../D3D/D3DHelper.hpp"

using Framework::ModContext;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using Framework::DrawContext;
using Framework::DrawResult;
using Framework::DrawOutcome;
using Framework::DrawPath;
using Framework::SettingDefs;
using Framework::SettingDef;
using Settings::When;
namespace Setting = Settings::Setting;

SettingDefs RemoveLyricsMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::RemoveLyricsEnabled, "Lyrics", "Remove Lyrics"),
		SettingDef::EnumChoice(Setting::RemoveLyricsWhen, "Toggle Switches", "RemoveLyricsWhen",
			"Remove Lyrics Mode", "manual", { "manual", "startup" }, "Toggle Switches")
			.WithVisibleWhen(Setting::RemoveLyricsEnabled)
	};
}

bool RemoveLyricsMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::RemoveLyricsEnabled);
}

void RemoveLyricsMod::OnInitialize(ModContext& c) {
	c.Commands().BindSetting(
		Setting::Key::RemoveLyrics,
		KeyEdge::Up,
		Availability::Initialized,
		[this](ModContext&, const KeyEvent&) {
			removeLyricsActive.store(!removeLyricsActive.load(std::memory_order_relaxed), std::memory_order_relaxed);
		},
		[](const ModContext& context, const KeyEvent&) {
			return context.When(Setting::RemoveLyricsWhen) == When::Manual;
		},
		"Remove Lyrics");

	c.Draw().Register("RemoveLyrics", 0, DrawPath::Indexed, [this](DrawContext& ctx) -> DrawResult {
		if (ctx.inSong && removeLyricsActive.load(std::memory_order_relaxed) && IsExtraRemoved(lyrics, ctx.thicc)) {
			return { DrawOutcome::Hide };
		}

		return { DrawOutcome::Pass };
	});
}

void RemoveLyricsMod::OnDisabled(ModContext&) {
	removeLyricsActive.store(false, std::memory_order_relaxed);
}

void RemoveLyricsMod::OnSongTick(ModContext& c) {
	ApplyStartup(c);
}

void RemoveLyricsMod::OnMenuTick(ModContext& c) {
	ApplyStartup(c);
}

// "Startup" mode latches lyric removal on once loaded; the manual keybinding and the
// render-time setting check own every other case, so this is the mod's only policy.
void RemoveLyricsMod::ApplyStartup(ModContext& c) {
	if (!removeLyricsActive.load(std::memory_order_relaxed) && c.When(Setting::RemoveLyricsWhen) == When::Startup) {
		removeLyricsActive.store(true, std::memory_order_relaxed);
	}
}

static Framework::ModRegistrar<RemoveLyricsMod> _removeLyricsReg;
