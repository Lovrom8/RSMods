#pragma once

// Full-stage bitmaps on ultrawide displays.
//
// The interface is confined to the centre 16:9 band draw by draw, which is right for chrome
// but wrong for the few bitmaps that are meant to cover the whole screen: they stop at the
// band and the side extensions show through. Three of them, all 1280x720 managed textures
// drawn as unit quads through the same ortho in vertex constants c0/c1 into the scene target:
//
//   - the modal dim plate (carbon pattern, DXT5, one 10-primitive Scaleform batch) behind
//     every dialog: tinted the centre only, the extensions stayed bright. Widened to the
//     full backbuffer (viewport and scissor), fade kept.
//   - the Scaleform title bitmap (logo, DXT5, 10 primitives) and the white boot quad under
//     it (X8R8G8B8, 2 primitives, the game's own shader): left the extensions black on the
//     title screen. On the title screen the title bitmap is scaled uniformly (viewport,
//     scissor and the y row of its ortho) so the logo keeps its proportions; under the
//     sign-in dialogs, where the game keeps drawing both beneath the plate, they are dropped
//     so the loft shows there. The cards before the title (pick logo, PEGI, engine logo)
//     are text over the same bitmap and are left as they are; the title screen is the
//     bitmap coming back after those cards' fade to black.
//
// Texture pointers differ per run, so an asset is recognised by an FNV-1a hash of the first
// 8 KB of level 0. Pointer verdicts are re-validated against the descriptor on every hit and
// trusted for one second only: menus churn textures and a reused pointer must not inherit a
// verdict. Only draws the ultrawide path has already confined are looked at, so at 16:9
// nothing here runs.
#include <atomic>

namespace UltrawideFullStage {
	enum class Decision { None, Skip, Stretch, StretchWide };

	constexpr unsigned int plateHash = 0xa1e6925a;
	constexpr unsigned int titleHash = 0xb2af9b13;
	constexpr unsigned int bootHash = 0x8983f4d2;

	struct CacheEntry {
		IDirect3DBaseTexture9* texture = nullptr;
		unsigned int hash = 0;
		ULONGLONG checkedAt = 0;
	};
	inline CacheEntry cache[8]{};
	// Set once per frame from the menu state: no menu yet, pre_enter_prompt, TitleScreen and
	// SelectionListDialog (the "connecting" stage between Begin and the first dialog).
	inline std::atomic<bool> onTitle = false;

	inline unsigned int HashTexture(IDirect3DTexture9* texture) {
		D3DLOCKED_RECT locked{};
		if (FAILED(texture->LockRect(0, &locked, nullptr, D3DLOCK_READONLY)))
			return 0;
		unsigned int hash = 2166136261u;
		const unsigned char* bytes = static_cast<const unsigned char*>(locked.pBits);
		for (int i = 0; i < 8192; ++i)
			hash = (hash ^ bytes[i]) * 16777619u;
		texture->UnlockRect(0);
		return hash;
	}

	// Hash of the stage-0 texture if it is a full-stage managed bitmap, 0 otherwise.
	inline unsigned int StageZeroHash(IDirect3DDevice9* device) {
		IDirect3DBaseTexture9* base = nullptr;
		if (FAILED(device->GetTexture(0, &base)) || !base)
			return 0;
		unsigned int hash = 0;
		if (base->GetType() == D3DRTYPE_TEXTURE) {
			IDirect3DTexture9* texture = static_cast<IDirect3DTexture9*>(base);
			D3DSURFACE_DESC desc{};
			if (SUCCEEDED(texture->GetLevelDesc(0, &desc)) && desc.Width == 1280 && desc.Height == 720
				&& desc.Pool == D3DPOOL_MANAGED) {
				const ULONGLONG now = GetTickCount64();
				CacheEntry* slot = nullptr;
				for (CacheEntry& entry : cache)
					if (entry.texture == base)
						slot = &entry;
				if (slot && now - slot->checkedAt < 1000) {
					hash = slot->hash;
				}
				else {
					hash = HashTexture(texture);
					if (!slot) {
						slot = &cache[0];
						for (CacheEntry& entry : cache)
							if (entry.checkedAt < slot->checkedAt)
								slot = &entry;
					}
					slot->texture = base;
					slot->hash = hash;
					slot->checkedAt = now;
				}
			}
		}
		base->Release();
		return hash;
	}

	// Called inside the DrawScope after Apply, for confined draws only.
	inline Decision Decide(IDirect3DDevice9* device, unsigned int primCount, bool confined) {
		if (!confined || (primCount != 10 && primCount != 2))
			return Decision::None;
		const unsigned int hash = StageZeroHash(device);
		if (hash == plateHash) {
			// Viewport and scissor only: this shader's c1 is not the ortho row, scaling it
			// changes the plate's colour.
			return Decision::StretchWide;
		}
		if (hash == titleHash) {
			// The intro cards are drawn over this same bitmap, so the draw alone cannot tell
			// the cards from the title screen. The cards end with a fade to black in which the
			// bitmap is not drawn for a few seconds; the title screen is the bitmap coming back
			// after that gap. Everything before it is left alone.
			static ULONGLONG lastTitleDraw = 0;
			static bool titlePhase = false;
			const ULONGLONG now = GetTickCount64();
			if (!titlePhase && lastTitleDraw && now - lastTitleDraw > 1000)
				titlePhase = true;
			lastTitleDraw = now;
			if (!titlePhase)
				return Decision::None;
			return onTitle.load(std::memory_order_relaxed) ? Decision::Stretch : Decision::Skip;
		}
		if (hash == bootHash) {
			// Never touched before or on the title screen, dropped under the sign-in dialogs.
			return onTitle.load(std::memory_order_relaxed) ? Decision::None : Decision::Skip;
		}
		return Decision::None;
	}

	inline RECT savedScissor{};
	inline bool scissorWidened = false;
	inline float savedRow1[4]{};
	inline bool rowScaled = false;

	// Widens the confined viewport and scissor back to the whole backbuffer for this one draw,
	// which scales x by backbuffer / band; with `uniform` the y row of the ortho in c1 is
	// scaled by the same factor so the image grows about the centre and overflows top and
	// bottom instead of being pulled wide. The DrawScope destructor restores the viewport,
	// Restore() the scissor and the row.
	inline void StretchViewport(IDirect3DDevice9* device, unsigned int backBufferWidth, bool uniform) {
		D3DVIEWPORT9 viewport{};
		if (FAILED(device->GetViewport(&viewport)) || viewport.Width == 0)
			return;
		const float k = static_cast<float>(backBufferWidth) / static_cast<float>(viewport.Width);
		viewport.X = 0;
		viewport.Width = backBufferWidth;
		device->SetViewport(&viewport);
		DWORD scissorOn = 0;
		if (SUCCEEDED(device->GetRenderState(D3DRS_SCISSORTESTENABLE, &scissorOn)) && scissorOn
			&& SUCCEEDED(device->GetScissorRect(&savedScissor))) {
			RECT wide = savedScissor;
			wide.left = 0;
			wide.right = static_cast<LONG>(backBufferWidth);
			device->SetScissorRect(&wide);
			scissorWidened = true;
		}
		if (uniform && SUCCEEDED(device->GetVertexShaderConstantF(1, savedRow1, 1))) {
			const float scaled[4] = { savedRow1[0] * k, savedRow1[1] * k, savedRow1[2], savedRow1[3] * k };
			device->SetVertexShaderConstantF(1, scaled, 1);
			rowScaled = true;
		}
	}

	inline void Restore(IDirect3DDevice9* device) {
		if (scissorWidened) {
			scissorWidened = false;
			device->SetScissorRect(&savedScissor);
		}
		if (rowScaled) {
			rowScaled = false;
			device->SetVertexShaderConstantF(1, savedRow1, 1);
		}
	}
}
