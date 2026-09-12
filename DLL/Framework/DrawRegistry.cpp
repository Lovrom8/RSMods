#include "DrawRegistry.hpp"
#include "IMod.hpp"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <utility>

namespace Framework {
	static DrawContext::CrcSource s_defaultCrcProvider = nullptr;

	void DrawContext::SetDefaultCrcProvider(CrcSource provider) {
		s_defaultCrcProvider = std::move(provider);
	}

	DrawContext::DrawContext(
		IDirect3DDevice9* dev,
		const Mesh& m,
		const ThiccMesh& t,
		DrawPath p,
		bool song,
		CrcSource customCrc
	) : device(dev), mesh(m), thicc(t), path(p), inSong(song), crcSource(std::move(customCrc)) {
		crcCache.reserve(4);
	}

	std::optional<DWORD> DrawContext::StageCRC(DWORD stage) {
		for (const auto& [cachedStage, crcVal] : crcCache) {
			if (cachedStage == stage) return crcVal;
		}

		std::optional<DWORD> computed;
		if (crcSource) {
			computed = crcSource(device, stage);
		}
		else if (s_defaultCrcProvider) {
			computed = s_defaultCrcProvider(device, stage);
		}

		crcCache.emplace_back(stage, computed);
		return computed;
	}

	struct RegisteredInterceptor {
		const IMod* owner = nullptr;
		std::string id;
		int priority = 0;
		DrawPath path = DrawPath::Indexed;
		DrawInterceptor fn;
	};

	struct DrawRegistry::Impl {
		mutable std::mutex mutex;
		std::vector<RegisteredInterceptor> interceptors;

		struct RegisteredRegen {
			const IMod* owner = nullptr;
			TextureRegenCallback fn;
		};
		struct RegisteredRelease {
			const IMod* owner = nullptr;
			TextureReleaseCallback fn;
		};

		std::vector<RegisteredRegen>  regenCallbacks;
		std::vector<RegisteredRelease> releaseCallbacks;
		std::vector<const IMod*> pendingReleases;  // owners waiting for deferred release

		std::atomic<std::shared_ptr<const std::vector<ActiveEntry>>> activeIndexed;
		std::atomic<std::shared_ptr<const std::vector<ActiveEntry>>> activePrimitive;

		Impl() {
			auto empty = std::make_shared<const std::vector<ActiveEntry>>();
			activeIndexed.store(empty, std::memory_order_relaxed);
			activePrimitive.store(empty, std::memory_order_relaxed);
		}
	};


	DrawRegistry::DrawRegistry() : impl(std::make_unique<Impl>()) {}
	DrawRegistry::~DrawRegistry() = default;

	void DrawRegistry::Register(const IMod* owner, std::string id, int priority, DrawPath path, DrawInterceptor fn) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		auto it = std::find_if(impl->interceptors.begin(), impl->interceptors.end(),
			[&](const RegisteredInterceptor& r) { return r.owner == owner && r.id == id; });

		if (it == impl->interceptors.end()) {
			impl->interceptors.push_back(RegisteredInterceptor{
				owner, std::move(id), priority, path, std::move(fn)
			});
		}
		else {
			it->priority = priority;
			it->path = path;
			it->fn = std::move(fn);
		}
	}

	void DrawRegistry::RegisterTextureLifecycle(const IMod* owner, TextureRegenCallback regenFn, TextureReleaseCallback releaseFn) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		// Regen
		{
			auto it = std::find_if(impl->regenCallbacks.begin(), impl->regenCallbacks.end(),
				[owner](const Impl::RegisteredRegen& r) { return r.owner == owner; });
			if (it == impl->regenCallbacks.end())
				impl->regenCallbacks.push_back({ owner, std::move(regenFn) });
			else
				it->fn = std::move(regenFn);
		}

		// Release
		{
			auto it = std::find_if(impl->releaseCallbacks.begin(), impl->releaseCallbacks.end(),
				[owner](const Impl::RegisteredRelease& r) { return r.owner == owner; });
			if (it == impl->releaseCallbacks.end())
				impl->releaseCallbacks.push_back({ owner, std::move(releaseFn) });
			else
				it->fn = std::move(releaseFn);
		}
	}

	void DrawRegistry::RegisterTextureRegen(const IMod* owner, TextureRegenCallback fn) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		auto it = std::find_if(impl->regenCallbacks.begin(), impl->regenCallbacks.end(),
			[owner](const Impl::RegisteredRegen& r) { return r.owner == owner; });
		
		if (it == impl->regenCallbacks.end()) {
			impl->regenCallbacks.push_back({ owner, std::move(fn) });
		}
		else {
			it->fn = std::move(fn);
		}
	}

	void DrawRegistry::RequestTextureRelease(const IMod* owner) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		// Only enqueue if not already pending (idempotent)
		auto it = std::find(impl->pendingReleases.begin(), impl->pendingReleases.end(), owner);
		if (it == impl->pendingReleases.end())
			impl->pendingReleases.push_back(owner);
	}

	void DrawRegistry::CancelTextureRelease(const IMod* owner) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		std::erase_if(impl->pendingReleases, [owner](const IMod* p) { return p == owner; });
	}

	void DrawRegistry::RemoveMod(const IMod* owner) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		
		std::erase_if(impl->interceptors, [owner](const RegisteredInterceptor& r) { return r.owner == owner; });
		std::erase_if(impl->regenCallbacks, [owner](const Impl::RegisteredRegen& r) { return r.owner == owner; });
		std::erase_if(impl->releaseCallbacks, [owner](const Impl::RegisteredRelease& r) { return r.owner == owner; });
		std::erase_if(impl->pendingReleases, [owner](const IMod* p) { return p == owner; });
	}


	void DrawRegistry::RebuildActive(std::function<bool(const IMod*)> isOwnerEnabled) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		auto newIndexed = std::make_shared<std::vector<ActiveEntry>>();
		auto newPrimitive = std::make_shared<std::vector<ActiveEntry>>();

		for (const auto& reg : impl->interceptors) {
			if (!reg.owner || !isOwnerEnabled(reg.owner)) continue;

			ActiveEntry entry{
				reg.priority,
				reg.owner->Id(),
				reg.fn
			};

			if (reg.path == DrawPath::Indexed || reg.path == DrawPath::Both) {
				newIndexed->push_back(entry);
			}
			if (reg.path == DrawPath::Primitive || reg.path == DrawPath::Both) {
				newPrimitive->push_back(entry);
			}
		}

		auto entryComparator = [](const ActiveEntry& a, const ActiveEntry& b) {
			if (a.priority != b.priority) return a.priority < b.priority;
			return a.ownerId < b.ownerId;
		};

		std::stable_sort(newIndexed->begin(), newIndexed->end(), entryComparator);
		std::stable_sort(newPrimitive->begin(), newPrimitive->end(), entryComparator);

		impl->activeIndexed.store(newIndexed, std::memory_order_release);
		impl->activePrimitive.store(newPrimitive, std::memory_order_release);
	}

	std::shared_ptr<const std::vector<ActiveEntry>> DrawRegistry::ActiveSnapshot(DrawPath path) const {
		if (path == DrawPath::Primitive) {
			return impl->activePrimitive.load(std::memory_order_acquire);
		}
		return impl->activeIndexed.load(std::memory_order_acquire);
	}

	void DrawRegistry::RegenerateAllTextures(IDirect3DDevice9* pDevice) {
		if (!pDevice) return;

		std::vector<TextureRegenCallback> callbacks;
		{
			std::lock_guard<std::mutex> lock(impl->mutex);
			callbacks.reserve(impl->regenCallbacks.size());
			for (const auto& r : impl->regenCallbacks) {
				if (r.fn) callbacks.push_back(r.fn);
			}
		}

		for (const auto& cb : callbacks) {
			cb(pDevice);
		}
	}

	void DrawRegistry::RunPendingReleases() {
		std::vector<TextureReleaseCallback> callbacks;
		{
			std::lock_guard<std::mutex> lock(impl->mutex);
			if (impl->pendingReleases.empty()) return;

			callbacks.reserve(impl->pendingReleases.size());
			for (const IMod* owner : impl->pendingReleases) {
				auto it = std::find_if(impl->releaseCallbacks.begin(), impl->releaseCallbacks.end(),
					[owner](const Impl::RegisteredRelease& r) { return r.owner == owner; });
				if (it != impl->releaseCallbacks.end() && it->fn)
					callbacks.push_back(it->fn);
			}
			impl->pendingReleases.clear();
		}

		for (const auto& cb : callbacks) {
			cb();
		}
	}

	DrawRegistry& Draw() {
		static DrawRegistry instance;
		return instance;
	}
}
