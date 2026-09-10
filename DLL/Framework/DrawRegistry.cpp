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

	void DrawRegistry::Register(const IMod* owner, std::string id, int priority, DrawPath path,
		DrawInterceptor fn) {
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

	void DrawRegistry::RemoveMod(const IMod* owner) {
		std::lock_guard<std::mutex> lock(impl->mutex);
		std::erase_if(impl->interceptors, [owner](const RegisteredInterceptor& r) { return r.owner == owner; });
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

	DrawRegistry& Draw() {
		static DrawRegistry instance;
		return instance;
	}
}
