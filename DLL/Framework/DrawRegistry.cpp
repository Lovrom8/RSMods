#include "DrawRegistry.hpp"
#include "IMod.hpp"

#include <algorithm>
#include <atomic>
#include <iterator>
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

	void DrawContext::AfterDraw(std::function<void()> restore) {
		if (restore) restores.push_back(std::move(restore));
	}

	DrawContext::~DrawContext() {
		for (auto it = restores.rbegin(); it != restores.rend(); ++it) {
			(*it)();
		}
	}

	struct RegisteredInterceptor {
		const IMod* owner = nullptr;
		std::string id;
		int priority = 0;
		DrawPath path = DrawPath::Indexed;
		DrawInterceptor fn;
	};

	namespace {
		// One active list per single draw path, in DrawPath bit order.
		constexpr DrawPath singlePaths[] = { DrawPath::Indexed, DrawPath::Primitive, DrawPath::IndexedUP, DrawPath::PrimitiveUP };
		constexpr size_t pathCount = std::size(singlePaths);

		int PathSlot(DrawPath path) {
			for (size_t i = 0; i < pathCount; ++i) {
				if (singlePaths[i] == path) return static_cast<int>(i);
			}
			return -1;
		}

		template <typename Fn>
		struct OwnedCallback {
			const IMod* owner = nullptr;
			Fn fn;
		};

		template <typename Fn>
		void Upsert(std::vector<OwnedCallback<Fn>>& list, const IMod* owner, Fn fn) {
			auto it = std::find_if(list.begin(), list.end(), [owner](const OwnedCallback<Fn>& r) { return r.owner == owner; });
			if (it == list.end())
				list.push_back({ owner, std::move(fn) });
			else
				it->fn = std::move(fn);
		}
	}

	struct DrawRegistry::Impl {
		mutable std::mutex mutex;
		std::vector<RegisteredInterceptor> interceptors;
		std::vector<OwnedCallback<FrameCallback>> frameCallbacks;
		std::vector<OwnedCallback<DeviceResetCallback>> resetCallbacks;

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

		std::atomic<std::shared_ptr<const std::vector<ActiveEntry>>> active[pathCount];
		std::atomic<std::shared_ptr<const std::vector<FrameCallback>>> activeFrame;

		Impl() {
			auto empty = std::make_shared<const std::vector<ActiveEntry>>();
			for (auto& list : active) list.store(empty, std::memory_order_relaxed);
			activeFrame.store(std::make_shared<const std::vector<FrameCallback>>(), std::memory_order_relaxed);
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

	void DrawRegistry::RegisterFrame(const IMod* owner, FrameCallback fn) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		Upsert(impl->frameCallbacks, owner, std::move(fn));
	}

	void DrawRegistry::RegisterDeviceReset(const IMod* owner, DeviceResetCallback fn) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		Upsert(impl->resetCallbacks, owner, std::move(fn));
	}

	void DrawRegistry::RemoveMod(const IMod* owner) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		std::erase_if(impl->frameCallbacks, [owner](const OwnedCallback<FrameCallback>& r) { return r.owner == owner; });
		std::erase_if(impl->resetCallbacks, [owner](const OwnedCallback<DeviceResetCallback>& r) { return r.owner == owner; });
		std::erase_if(impl->interceptors, [owner](const RegisteredInterceptor& r) { return r.owner == owner; });
		std::erase_if(impl->regenCallbacks, [owner](const Impl::RegisteredRegen& r) { return r.owner == owner; });
		std::erase_if(impl->releaseCallbacks, [owner](const Impl::RegisteredRelease& r) { return r.owner == owner; });
		std::erase_if(impl->pendingReleases, [owner](const IMod* p) { return p == owner; });
	}


	void DrawRegistry::RebuildActive(std::function<bool(const IMod*)> isOwnerEnabled) {
		std::lock_guard<std::mutex> lock(impl->mutex);

		std::shared_ptr<std::vector<ActiveEntry>> lists[pathCount];
		for (auto& list : lists) list = std::make_shared<std::vector<ActiveEntry>>();

		for (const auto& reg : impl->interceptors) {
			if (!reg.owner || !isOwnerEnabled(reg.owner)) continue;

			ActiveEntry entry{
				reg.priority,
				reg.owner->Id(),
				reg.fn
			};

			for (size_t i = 0; i < pathCount; ++i) {
				if (Includes(reg.path, singlePaths[i])) lists[i]->push_back(entry);
			}
		}

		auto entryComparator = [](const ActiveEntry& a, const ActiveEntry& b) {
			if (a.priority != b.priority) return a.priority < b.priority;
			return a.ownerId < b.ownerId;
		};

		for (size_t i = 0; i < pathCount; ++i) {
			std::stable_sort(lists[i]->begin(), lists[i]->end(), entryComparator);
			impl->active[i].store(lists[i], std::memory_order_release);
		}

		// Frame callbacks in owner-Id order, so the run order doesn't depend on registration order.
		std::vector<const OwnedCallback<FrameCallback>*> enabledFrames;
		for (const auto& reg : impl->frameCallbacks) {
			if (reg.owner && reg.fn && isOwnerEnabled(reg.owner)) enabledFrames.push_back(&reg);
		}
		std::stable_sort(enabledFrames.begin(), enabledFrames.end(),
			[](const auto* a, const auto* b) { return a->owner->Id() < b->owner->Id(); });

		auto frames = std::make_shared<std::vector<FrameCallback>>();
		frames->reserve(enabledFrames.size());
		for (const auto* reg : enabledFrames) frames->push_back(reg->fn);
		impl->activeFrame.store(frames, std::memory_order_release);
	}

	std::shared_ptr<const std::vector<ActiveEntry>> DrawRegistry::ActiveSnapshot(DrawPath path) const {
		const int slot = PathSlot(path);
		if (slot < 0) {
			static const auto empty = std::make_shared<const std::vector<ActiveEntry>>();
			return empty;
		}
		return impl->active[slot].load(std::memory_order_acquire);
	}

	void DrawRegistry::RunFrame(IDirect3DDevice9* pDevice) {
		if (!pDevice) return;

		const auto frames = impl->activeFrame.load(std::memory_order_acquire);
		for (const auto& fn : *frames) {
			fn(pDevice);
		}
	}

	void DrawRegistry::RunDeviceReset(IDirect3DDevice9* pDevice) {
		if (!pDevice) return;

		std::vector<DeviceResetCallback> callbacks;
		{
			std::lock_guard<std::mutex> lock(impl->mutex);
			callbacks.reserve(impl->resetCallbacks.size());
			for (const auto& r : impl->resetCallbacks) {
				if (r.fn) callbacks.push_back(r.fn);
			}
		}

		for (const auto& cb : callbacks) {
			cb(pDevice);
		}
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
