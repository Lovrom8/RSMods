#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#if !defined(_WINDEF_) && !defined(_WINDOWS_)
using DWORD = unsigned long;
#endif

struct IDirect3DDevice9;
struct IDirect3DTexture9;
#ifndef LPDIRECT3DTEXTURE9
using LPDIRECT3DTEXTURE9 = IDirect3DTexture9*;
#endif

struct Mesh;
struct ThiccMesh;

namespace Framework {
	class IMod;

	// Bit flags: register on any combination, a draw reports exactly one. The user-pointer (UP)
	// variants are opt-in so existing mesh-matching interceptors never see those draws.
	enum class DrawPath : unsigned {
		Indexed     = 1u << 0, // DrawIndexedPrimitive
		Primitive   = 1u << 1, // DrawPrimitive
		Both        = Indexed | Primitive,
		IndexedUP   = 1u << 2, // DrawIndexedPrimitiveUP
		PrimitiveUP = 1u << 3, // DrawPrimitiveUP
		All         = Both | IndexedUP | PrimitiveUP,
	};

	constexpr bool Includes(DrawPath registered, DrawPath single) {
		return (static_cast<unsigned>(registered) & static_cast<unsigned>(single)) != 0;
	}
	enum class DrawOutcome { Pass, Show, Hide, ReplaceTexture };

	struct DrawResult {
		DrawOutcome outcome = DrawOutcome::Pass;
		DWORD stage = 1;                       // for ReplaceTexture
		LPDIRECT3DTEXTURE9 texture = nullptr;  // for ReplaceTexture
	};

	struct DrawContext {
		IDirect3DDevice9* device = nullptr;
		const Mesh& mesh;
		const ThiccMesh& thicc;
		DrawPath path = DrawPath::Indexed;
		bool inSong = false;

		using CrcSource = std::function<std::optional<DWORD>(IDirect3DDevice9*, DWORD stage)>;

		explicit DrawContext(
			IDirect3DDevice9* dev,
			const Mesh& m,
			const ThiccMesh& t,
			DrawPath p = DrawPath::Indexed,
			bool song = false,
			CrcSource customCrc = nullptr
		);

		// Computes GetTexture(stage) + D3D::CRCForTexture once per (stage) per draw and caches.
		// Returns std::nullopt if the stage has no bound texture.
		std::optional<DWORD> StageCRC(DWORD stage);

		// Global default CRC source used when no custom provider is passed to DrawContext.
		// Set by the host/D3D hook layer during startup.
		static void SetDefaultCrcProvider(CrcSource provider);

		// Undo a device-state change once this draw is over, whether it was drawn, shown early or hidden.
		// Runs newest-first when the hook's DrawContext goes out of scope, i.e. after the original draw.
		// Use for per-draw state such as viewport or scissor.
		void AfterDraw(std::function<void()> restore);

		~DrawContext();
		DrawContext(const DrawContext&) = delete;
		DrawContext& operator=(const DrawContext&) = delete;

	private:
		CrcSource crcSource;
		mutable std::vector<std::pair<DWORD, std::optional<DWORD>>> crcCache;
		std::vector<std::function<void()>> restores;
	};

	// Render-thread interceptor callback. MUST NOT query Settings by string.
	using DrawInterceptor = std::function<DrawResult(DrawContext&)>;

	// Texture regeneration callback called at frame boundaries (EndScene).
	using TextureRegenCallback = std::function<void(IDirect3DDevice9*)>;
	// Texture release callback called at frame boundaries (EndScene) when a release was requested.
	using TextureReleaseCallback  = std::function<void()>;
	// Render thread, once per frame from EndScene, enabled mods only. Runs before the HUD is laid out.
	using FrameCallback = std::function<void(IDirect3DDevice9*)>;
	// Render thread, after a successful IDirect3DDevice9::Reset, every registered mod (enabled or not).
	// Drop anything keyed by D3D object pointers: the game recreates its resources and addresses get reused.
	using DeviceResetCallback = std::function<void(IDirect3DDevice9*)>;

	struct ActiveEntry {
		int priority = 0;
		std::string_view ownerId;
		DrawInterceptor fn;
	};

	class DrawRegistry {
	public:
		DrawRegistry();
		~DrawRegistry();

		DrawRegistry(const DrawRegistry&) = delete;
		DrawRegistry& operator=(const DrawRegistry&) = delete;

		// MainThread: register an interceptor owned by `owner`. Lower priority runs earlier.
		// Ties break on owner Id() to stay deterministic.
		void Register(const IMod* owner, std::string id, int priority, DrawPath path,
			DrawInterceptor fn);

		// MainThread: register texture regen AND release callbacks together.
		// Prefer this over separate RegisterTextureRegen when the mod also owns D3D textures,
		// so the release path is always declared alongside the regen path.
		void RegisterTextureLifecycle(const IMod* owner, TextureRegenCallback regenFn, TextureReleaseCallback releaseFn);

		// MainThread: register a texture regeneration callback owned by `owner` (regen-only; no release).
		void RegisterTextureRegen(const IMod* owner, TextureRegenCallback fn);

		// MainThread: queue a deferred texture release for `owner`. Safe to call from OnDisabled.
		// The release callback registered via RegisterTextureLifecycle is invoked once on the
		// render thread at the next RunPendingReleases call (EndScene), after all DIP/DP are done.
		void RequestTextureRelease(const IMod* owner);

		// MainThread: cancel a previously requested deferred texture release for `owner`.
		void CancelTextureRelease(const IMod* owner);

		// MainThread: register this mod's per-frame callback (replaces any earlier one).
		void RegisterFrame(const IMod* owner, FrameCallback fn);

		// MainThread: register this mod's device-reset callback (replaces any earlier one).
		void RegisterDeviceReset(const IMod* owner, DeviceResetCallback fn);

		// MainThread: drop every interceptor, regen, release, frame, and reset callback owned by this mod.
		void RemoveMod(const IMod* owner);

		// MainThread: rebuild the active snapshot from currently-enabled owners.
		void RebuildActive(std::function<bool(const IMod*)> isOwnerEnabled);

		// Render thread: lock-free load of the immutable, priority-sorted active list for one path
		// (Indexed, Primitive, IndexedUP or PrimitiveUP; a combined value returns an empty list).
		[[nodiscard]] std::shared_ptr<const std::vector<ActiveEntry>> ActiveSnapshot(DrawPath path) const;

		// Render thread (EndScene): run enabled mods' frame callbacks, lock-free.
		void RunFrame(IDirect3DDevice9* pDevice);

		// Render thread (after a successful Reset): run every registered reset callback.
		void RunDeviceReset(IDirect3DDevice9* pDevice);

		// Render thread (EndScene): execute all registered texture regeneration callbacks.
		void RegenerateAllTextures(IDirect3DDevice9* pDevice);

		// Render thread (EndScene): drain and invoke all pending release callbacks.
		// Must be called after RegenerateAllTextures so regen always wins over a same-frame release.
		void RunPendingReleases();

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};

	DrawRegistry& Draw();
}
