#include "../stdafx.h"
#include "RainbowNotesMod.hpp"
#include "ExtendedRangeMode.hpp"
#include "../D3D/D3D.hpp"
#include "../D3D/D3DHelper.hpp"
#include "../D3D/D3DHooks.hpp"

using Framework::ModContext;
using Framework::DrawContext;
using Framework::DrawResult;
using Framework::DrawOutcome;
using Framework::DrawPath;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
namespace Setting = Settings::Setting;

namespace {
	inline bool IsNoteHead(const DrawContext& ctx) {
		return IsToBeRemoved(sevenstring, ctx.mesh) || IsExtraRemoved(noteModifiers, ctx.thicc);
	}

	inline bool IsNoteStemOrAccent(const DrawContext& ctx) {
		return (ctx.mesh.Stride == 32 && ctx.mesh.PrimCount == 2 && ctx.mesh.NumVertices == 4) ||
		       (ctx.mesh.Stride == 32 && ctx.mesh.PrimCount == 4 && ctx.mesh.NumVertices == 6);
	}

	inline bool IsNoteTail(const DrawContext& ctx) {
		return ctx.mesh.Stride == 12;
	}
}

bool RainbowNotesMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::RainbowNotesEnabled) || ERMode::IsRainbowNotesEnabled();
}

void RainbowNotesMod::OnInitialize(ModContext& c) {
	c.Commands().BindSetting(
		Setting::Key::RainbowNotes,
		KeyEdge::Up,
		Availability::Active,
		[](ModContext&, const KeyEvent&) {
			ERMode::ToggleRainbowNotes();
			D3DHooks::RecreateTextures = true;
		},
		[](const ModContext& context, const KeyEvent&) {
			return context.IsOn(Setting::RainbowNotesEnabled);
		},
		"Rainbow Notes");

	// Priority 10 (above ER): note stems, bends, slides, and accents.
	// Sets rainbow texture on stage 1 and passes so higher-layering mods can interact,
	// while ER detects RainbowNotes is active and skips stems to preserve this layer.
	c.Draw().Register("RainbowNotesStems", 10, DrawPath::Indexed, [](DrawContext& ctx) -> DrawResult {
		if (!ctx.inSong) return { DrawOutcome::Pass };
		if (!ERMode::RainbowNotesEnabled.load(std::memory_order_relaxed)) return { DrawOutcome::Pass };

		int h = ERMode::customNoteColorH.load(std::memory_order_relaxed);
		if (h <= 0) return { DrawOutcome::Pass };
		if (h > 179) h %= 180;

		if (IsNoteStemOrAccent(ctx)) {
			auto crc1 = ctx.StageCRC(1);
			if (crc1 && (*crc1 == crcStemsAccents || *crc1 == crcBendSlideIndicators)) {
				if (h < static_cast<int>(ERMode::rainbowTextures.size()) && ERMode::rainbowTextures[h]) {
					return { DrawOutcome::ReplaceTexture, 1, ERMode::rainbowTextures[h] };
				}
			}
		}

		return { DrawOutcome::Pass };
	});

	// Priority 40 (below ER & Twitch): rainbow note heads and tails.
	c.Draw().Register("RainbowNotesHeadsTails", 40, DrawPath::Both, [](DrawContext& ctx) -> DrawResult {
		if (!ctx.inSong) return { DrawOutcome::Pass };
		if (!ERMode::RainbowNotesEnabled.load(std::memory_order_relaxed)) return { DrawOutcome::Pass };

		int h = ERMode::customNoteColorH.load(std::memory_order_relaxed);
		if (h <= 0) return { DrawOutcome::Pass };
		if (h > 179) h %= 180;

		if (ctx.path == DrawPath::Primitive) {
			if (IsNoteTail(ctx)) {
				if (h < static_cast<int>(ERMode::rainbowTextures.size()) && ERMode::rainbowTextures[h]) {
					return { DrawOutcome::ReplaceTexture, 1, ERMode::rainbowTextures[h] };
				}
			}
		}
		else { // Indexed
			if (IsNoteHead(ctx)) {
				if (h < static_cast<int>(ERMode::rainbowTextures.size()) && ERMode::rainbowTextures[h]) {
					return { DrawOutcome::ReplaceTexture, 1, ERMode::rainbowTextures[h] };
				}
			}
		}

		return { DrawOutcome::Pass };
	});
}

static Framework::ModRegistrar<RainbowNotesMod> _rainbowNotesReg;
