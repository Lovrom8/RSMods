#include "../stdafx.h"
#include "TwitchMod.hpp"
#include "ExtendedRangeMode.hpp"
#include "../D3D/D3D.hpp"
#include "../D3D/D3DHelper.hpp"
#include "../Offsets.hpp"
#include "../MemUtil.hpp"
#include "../Settings.hpp"
#include "DrawMeshTags.hpp"

using Framework::ModContext;
using Framework::DrawContext;
using Framework::DrawResult;
using Framework::DrawOutcome;
using Framework::DrawPath;
namespace Setting = Settings::Setting;


void TwitchMod::SyncState() {
	bool remove = Settings::IsTwitchSettingEnabled(Setting::Twitch::RemoveNotes);
	bool trans = Settings::IsTwitchSettingEnabled(Setting::Twitch::TransparentNotes);
	bool solid = Settings::IsTwitchSettingEnabled(Setting::Twitch::SolidNotes);

	s_removeNotes.store(remove, std::memory_order_relaxed);
	s_transparentNotes.store(trans, std::memory_order_relaxed);
	s_solidNotes.store(solid, std::memory_order_relaxed);
	s_fYourFC.store(Settings::IsTwitchSettingEnabled(Setting::Twitch::FYourFC), std::memory_order_relaxed);
	s_drunkMode.store(Settings::IsTwitchSettingEnabled(Setting::Twitch::DrunkMode), std::memory_order_relaxed);

	if (solid) {
		if (Settings::ReturnSettingValue(Setting::SolidNoteColor) == "random") {
			int idx = currentRandomTexture;
			if (idx >= 0 && idx < static_cast<int>(randomTextures.size())) {
				s_activeSolidTexture.store(randomTextures[idx], std::memory_order_relaxed);
			}
			else {
				s_activeSolidTexture.store(nullptr, std::memory_order_relaxed);
			}
		}
		else {
			s_activeSolidTexture.store(twitchUserDefinedTexture, std::memory_order_relaxed);
		}
	}
	else {
		s_activeSolidTexture.store(nullptr, std::memory_order_relaxed);
	}
}

void TwitchMod::OnInitialize(ModContext& c) {
	SyncState();

	// Priority 30: Twitch note modifications (Remove, Transparent, Solid).
	c.Draw().Register("TwitchNotes", 30, DrawPath::Both, [](DrawContext& ctx) -> DrawResult {
		if (!ctx.inSong) return { DrawOutcome::Pass };

		if (s_removeNotes.load(std::memory_order_relaxed)) {
			if (ctx.path == DrawPath::Primitive) {
				if (DrawMesh::IsNoteTail(ctx)) return { DrawOutcome::Hide };
			}
			else { // Indexed
				if (DrawMesh::IsNoteHead(ctx)) return { DrawOutcome::Hide };
				if (DrawMesh::IsNoteStemOrAccent(ctx) && DrawMesh::IsNoteStemCrc(ctx)) {
					return { DrawOutcome::Hide };
				}
			}
			return { DrawOutcome::Pass };
		}

		if (s_transparentNotes.load(std::memory_order_relaxed)) {
			if (ctx.path == DrawPath::Primitive) {
				if (DrawMesh::IsNoteTail(ctx)) return { DrawOutcome::ReplaceTexture, 1, nonexistentTexture };
			}
			else { // Indexed
				if (DrawMesh::IsNoteHead(ctx)) return { DrawOutcome::ReplaceTexture, 1, nonexistentTexture };
				if (DrawMesh::IsNoteStemOrAccent(ctx) && DrawMesh::IsNoteStemCrc(ctx)) {
					return { DrawOutcome::ReplaceTexture, 1, nonexistentTexture };
				}
			}
			return { DrawOutcome::Pass };
		}

		if (s_solidNotes.load(std::memory_order_relaxed)) {
			LPDIRECT3DTEXTURE9 tex = s_activeSolidTexture.load(std::memory_order_relaxed);
			if (tex) {
				if (ctx.path == DrawPath::Primitive) {
					if (DrawMesh::IsNoteTail(ctx)) return { DrawOutcome::ReplaceTexture, 1, tex };
				}
				else { // Indexed
					if (DrawMesh::IsNoteHead(ctx)) return { DrawOutcome::ReplaceTexture, 1, tex };
					if (DrawMesh::IsNoteStemOrAccent(ctx) && DrawMesh::IsNoteStemCrc(ctx)) {
						return { DrawOutcome::ReplaceTexture, 1, tex };
					}
				}
			}
			return { DrawOutcome::Pass };
		}

		return { DrawOutcome::Pass };
	});

	c.Draw().RegisterTextureLifecycle(&TwitchMod::RegenerateTextures, &TwitchMod::ReleaseTextures);
}

void TwitchMod::OnEnabled(ModContext& c) {
	c.Draw().CancelTextureRelease();
	SyncState();
	D3DHooks::RecreateTextures = true;
}

void TwitchMod::OnSongTick(ModContext&) {
	SyncState();
}

void TwitchMod::RunPerFrameEffects(IDirect3DDevice9*) {
	if (!GameState::IsInSong()) return;

	// Reset note streak
	if (s_fYourFC.load(std::memory_order_relaxed)) {
		uintptr_t currentNoteStreak = 0;

		if (GameState::Menus::IsInLearnASongModes()) {
			currentNoteStreak = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_currentNoteStreak, Offsets::ptr_currentNoteStreakLASOffsets);
		}
		else if (GameState::Menus::IsInScoreAttackModes()) {
			currentNoteStreak = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_currentNoteStreak, Offsets::ptr_currentNoteStreakSAOffsets);
		}

		if (currentNoteStreak != 0) {
			*(BYTE*)currentNoteStreak = 0;
		}
	}

	// DrunkMode camera jitter
	if (s_drunkMode.load(std::memory_order_relaxed)) {
		static std::uniform_real_distribution<> keepValueWithin(-1.5, 1.5);
		MemUtil::SetStaticValue(Offsets::ptr_drunkShit.Get(), (float)keepValueWithin(rng), sizeof(float));
	}
}

void TwitchMod::OnDisabled(ModContext& c) {
	s_activeSolidTexture.store(nullptr, std::memory_order_release);
	D3DHooks::RecreateTextures = true;
	c.Draw().RequestTextureRelease();
}

void TwitchMod::OnSettingsChanged(ModContext&) {
	SyncState();
}

void TwitchMod::OnShutdown(ModContext&) {
	ReleaseTextures();
}

void TwitchMod::RegenerateTextures(IDirect3DDevice9* pDevice) {
	if (!pDevice) return;

	GenerateRandomTextures(pDevice);
	RegenerateUserDefinedTexture(pDevice);
}

void TwitchMod::GenerateRandomTextures(IDirect3DDevice9* pDevice) {
	if (!pDevice) return;

	if (randomTextureColors.size() != randomTextureCount) {
		randomTextureColors.resize(randomTextureCount);
	}

	static std::uniform_real_distribution<> urd(0.0f, 1.0f);

	for (int textIdx = 0; textIdx < randomTextureCount; ++textIdx) {
		RSColor rndColor;
		rndColor.r = (float)urd(rng);
		rndColor.g = (float)urd(rng);
		rndColor.b = (float)urd(rng);

		randomTextureColors[textIdx] = rndColor;

		ColorList colorSet(16, rndColor);
		D3D::GenerateGradientTexture(pDevice, &randomTextures[textIdx], colorSet);
	}
}

void TwitchMod::RegenerateUserDefinedTexture(IDirect3DDevice9* pDevice) {
	if (!pDevice) return;

	RSColor userDefColor = Settings::ConvertHexToColor(Settings::ReturnSettingValue(Setting::SolidNoteColor));

	ColorList customColorList(16, userDefColor);
	D3D::GenerateGradientTexture(pDevice, &twitchUserDefinedTexture, customColorList);

	ERMode::customSolidColor.clear();
	for (int str = 0; str < 6; str++) {
		ERMode::customSolidColor.push_back(userDefColor);
	}
}

void TwitchMod::ReleaseTextures() {
	D3D::ReleaseTexture(&twitchUserDefinedTexture);
	for (auto& tex : randomTextures) {
		D3D::ReleaseTexture(&tex);
	}
}

static Framework::ModRegistrar<TwitchMod> _twitchReg;
