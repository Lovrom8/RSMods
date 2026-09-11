#include "../stdafx.h"
#include "RemoveHeadstockMod.hpp"
#include "../D3D/D3D.hpp"
#include "../D3D/D3DHelper.hpp"
#include "DrawMeshTags.hpp"
#include <algorithm>

using Framework::ModContext;
using Framework::DrawContext;
using Framework::DrawResult;
using Framework::DrawOutcome;
using Framework::DrawPath;
using Settings::When;
using Framework::GamePhase;
using Framework::SettingDefs;
using Framework::SettingDef;
namespace Setting = Settings::Setting;

SettingDefs RemoveHeadstockMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::RemoveHeadstockEnabled, "Headstock", "Remove Headstock"),
		SettingDef::Enum(Setting::RemoveHeadstockWhen, "Remove Headstock Mode")
			.Choices({ "song", "startup" }, "song")
			.WithVisibleWhen(Setting::RemoveHeadstockEnabled)
	};
}

bool RemoveHeadstockMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::RemoveHeadstockEnabled);
}

void RemoveHeadstockMod::SyncState(ModContext& c) {
	const When when = c.When(Setting::RemoveHeadstockWhen);
	removeHeadstockInThisMenu.store(when == When::Startup || (when == When::Song && c.phase == GamePhase::Song), std::memory_order_relaxed);
}

void RemoveHeadstockMod::AddHeadstockTexture(LPDIRECT3DTEXTURE9 tex) {
	if (!tex) return;
	auto current = cachedTextures.load(std::memory_order_acquire);
	if (current && std::ranges::find(*current, tex) != current->end()) {
		return;
	}

	auto updated = current ? std::make_shared<std::vector<LPDIRECT3DTEXTURE9>>(*current)
	                       : std::make_shared<std::vector<LPDIRECT3DTEXTURE9>>();
	updated->push_back(tex);
	cachedTextures.store(std::move(updated), std::memory_order_release);
}

bool RemoveHeadstockMod::IsHeadstockTextureCached(LPDIRECT3DTEXTURE9 tex) const {
	if (!tex) return false;
	auto snap = cachedTextures.load(std::memory_order_acquire);
	return snap && std::ranges::find(*snap, tex) != snap->end();
}

void RemoveHeadstockMod::ClearCachedTextures() {
	cachedTextures.store(std::make_shared<const std::vector<LPDIRECT3DTEXTURE9>>(), std::memory_order_release);
}

size_t RemoveHeadstockMod::GetCachedTextureCount() const {
	auto snap = cachedTextures.load(std::memory_order_acquire);
	return snap ? snap->size() : 0;
}

void RemoveHeadstockMod::UpdateHeadstockCacheForMenu() {
	const std::string& currentMenu = GameState::currentMenu;
	if (!GameState::Menus::IsInTuningMenus() || currentMenu == "MissionMenu") {
		resetHeadstockCache.store(true, std::memory_order_relaxed);
	}

	if (previousMenu != currentMenu && GameState::Menus::IsInTuningMenus()) {
		resetHeadstockCache.store(true, std::memory_order_relaxed);
		ClearCachedTextures();
		calculatedHeadstocks.store(false, std::memory_order_relaxed);
	}
	previousMenu = currentMenu;
}

void RemoveHeadstockMod::OnInitialize(ModContext& c) {
	ClearCachedTextures();

	// Remove Headstock Artifacts
	c.Draw().Register("RemoveHeadstockArtifacts", 0, DrawPath::Indexed, [this](DrawContext& ctx) -> DrawResult {
		if (!removeHeadstockInThisMenu.load(std::memory_order_relaxed))
			return { DrawOutcome::Pass };

		if (!GameState::Menus::IsInTuningMenus())
			return { DrawOutcome::Pass };

		if (IsExtraRemoved(tuningLetters, ctx.thicc) ||
			IsExtraRemoved(tunerHighlight, ctx.thicc) ||
			IsExtraRemoved(leftyFix, ctx.thicc)) {
			return { DrawOutcome::Hide };
		}

		return { DrawOutcome::Pass };
	});

	// Remove Headstock
	c.Draw().Register("RemoveHeadstock", 1, DrawPath::Indexed, [this](DrawContext& ctx) -> DrawResult {
		const UINT stride = ctx.mesh.Stride;
		const bool possibleHeadstock = (stride == 44 || stride == 56 || stride == 60 || stride == 68 || stride == 76 || stride == 84);
		if (!possibleHeadstock)
			return { DrawOutcome::Pass };

		if (!removeHeadstockInThisMenu.load(std::memory_order_relaxed))
			return { DrawOutcome::Show };

		// Need to reset cache, and this is a headstock texture
		if (resetHeadstockCache.load(std::memory_order_relaxed) && IsExtraRemoved(headstockThicc, ctx.thicc)) {
			if (!ctx.StageCRC(1))
				return { DrawOutcome::Hide };

			if (DrawMesh::StageMatchesAny(ctx, 1, { D3D::Crc::Headstock0, D3D::Crc::Headstock1,
			                                        D3D::Crc::Headstock2, D3D::Crc::Headstock3, D3D::Crc::Headstock4 })) {
				LPDIRECT3DBASETEXTURE9 pBase = nullptr;
				if (SUCCEEDED(ctx.device->GetTexture(1, &pBase)) && pBase) {
					auto pTex = reinterpret_cast<LPDIRECT3DTEXTURE9>(pBase);
					pBase->Release();
					AddHeadstockTexture(pTex);
				}
			}

			const size_t limit = GameState::Menus::IsInMultiplayerTunerMenus() ? 6 : 3;
			if (GetCachedTextureCount() >= limit) {
				calculatedHeadstocks.store(true, std::memory_order_relaxed);
				resetHeadstockCache.store(false, std::memory_order_relaxed);
			}

			return { DrawOutcome::Hide };
		}

		// We've already cached the headstocks we're using, so find the one we are working with and remove it
		if (calculatedHeadstocks.load(std::memory_order_relaxed)) {
			LPDIRECT3DBASETEXTURE9 pBase = nullptr;
			if (SUCCEEDED(ctx.device->GetTexture(1, &pBase)) && pBase) {
				auto pTex = reinterpret_cast<LPDIRECT3DTEXTURE9>(pBase);
				pBase->Release();
				if (IsHeadstockTextureCached(pTex)) {
					return { DrawOutcome::Hide };
				}
			}
		}

		return { DrawOutcome::Pass };
	});
}

void RemoveHeadstockMod::OnEnabled(ModContext& c) {
	SyncState(c);
	active = true;
}

void RemoveHeadstockMod::OnSettingsChanged(ModContext& c) {
	if (active)
		SyncState(c);
}

void RemoveHeadstockMod::OnSongEnter(ModContext& c) {
	SyncState(c);
}

void RemoveHeadstockMod::OnSongExit(ModContext& c) {
	SyncState(c);
}

void RemoveHeadstockMod::OnMenuTick(ModContext&) {
	UpdateHeadstockCacheForMenu();
}

void RemoveHeadstockMod::OnDisabled(ModContext&) {
	active = false;
	removeHeadstockInThisMenu.store(false, std::memory_order_relaxed);
	resetHeadstockCache.store(true, std::memory_order_relaxed);
	calculatedHeadstocks.store(false, std::memory_order_relaxed);
	ClearCachedTextures();
}

static Framework::ModRegistrar<RemoveHeadstockMod> _removeHeadstockReg;
