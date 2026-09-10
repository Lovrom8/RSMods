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

	enum class DrawPath { Indexed, Primitive, Both };
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

	private:
		CrcSource crcSource;
		mutable std::vector<std::pair<DWORD, std::optional<DWORD>>> crcCache;
	};

	// Render-thread interceptor callback. MUST NOT query Settings by string.
	using DrawInterceptor = std::function<DrawResult(DrawContext&)>;

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

		// MainThread: drop every interceptor owned by this mod.
		void RemoveMod(const IMod* owner);

		// MainThread: rebuild the active snapshot from currently-enabled owners.
		void RebuildActive(std::function<bool(const IMod*)> isOwnerEnabled);

		// Render thread: lock-free load of the immutable, priority-sorted active list for a path.
		[[nodiscard]] std::shared_ptr<const std::vector<ActiveEntry>> ActiveSnapshot(DrawPath path) const;

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};

	DrawRegistry& Draw();
}
