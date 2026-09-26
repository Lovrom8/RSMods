#pragma once

#include <algorithm>
#include <atomic>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

#include "IMod.hpp"

#if !defined(_WINDEF_) && !defined(_WINDOWS_)
using DWORD = unsigned long;
using UINT = unsigned int;
#endif

struct IDirect3DDevice9;
struct IDirect3DBaseTexture9;
struct IDirect3DSurface9;
struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;
struct tagRECT;

// Device-state events: the D3D9 calls around the draws. Hooks dispatch these instead of calling
// into mods, so a mod that needs one registers a handler and never edits D3DHooks.
namespace Framework {

	// Bind events fire after the original call succeeded, so they describe what is actually bound.
	namespace DeviceEvent {
		struct TextureBound {
			IDirect3DDevice9* device;
			DWORD stage;
			IDirect3DBaseTexture9* texture;
		};

		struct RenderTargetBound {
			IDirect3DDevice9* device;
			DWORD index;
			IDirect3DSurface9* target;
		};

		struct VertexShaderBound {
			IDirect3DDevice9* device;
			IDirect3DVertexShader9* shader;
		};

		struct PixelShaderBound {
			IDirect3DDevice9* device;
			IDirect3DPixelShader9* shader;
		};

		// Pre-call and mutable: handlers may rewrite the arguments or set `suppress` to drop the call.
		// A replaced `data` pointer only has to live until the handler chain returns.
		struct VertexShaderConstants {
			IDirect3DDevice9* device;
			UINT startRegister;
			const float* data;
			UINT vector4fCount;
			bool suppress = false;
		};

		// Pre-call and mutable, as above. A replaced rect has to live until the handler chain returns.
		struct StretchRect {
			IDirect3DDevice9* device;
			IDirect3DSurface9* source;
			const tagRECT* sourceRect;
			IDirect3DSurface9* dest;
			const tagRECT* destRect;
			bool suppress = false;
		};
	}

	enum class Observe {
		WhileEnabled, // Default. Costs nothing while the mod is off.
		Always,       // For state mirrors that would go stale if they missed binds while disabled.
	};

	// One handler per mod, run in owner-Id order. Published by Rebuild like the draw lists: the
	// render thread takes one snapshot load per dispatch and never locks.
	template <typename Event>
	class DeviceChannel {
	public:
		using Handler = std::function<void(Event&)>;

		DeviceChannel() { active.store(std::make_shared<const std::vector<Entry>>(), std::memory_order_relaxed); }
		DeviceChannel(const DeviceChannel&) = delete;
		DeviceChannel& operator=(const DeviceChannel&) = delete;

		// MainThread. Takes effect at the next Rebuild. Registering again replaces the handler.
		void Register(const IMod* owner, Observe when, Handler fn) {
			std::lock_guard lock(mutex);
			auto it = std::find_if(registered.begin(), registered.end(), [owner](const Registered& r) { return r.owner == owner; });
			if (it == registered.end())
				registered.push_back({ owner, when, std::move(fn) });
			else
				*it = { owner, when, std::move(fn) };
		}

		// MainThread. Also unpublishes at once: Always handlers would otherwise outlive a faulted mod.
		void RemoveMod(const IMod* owner) {
			std::lock_guard lock(mutex);
			std::erase_if(registered, [owner](const Registered& r) { return r.owner == owner; });

			auto current = active.load(std::memory_order_acquire);
			auto kept = std::make_shared<std::vector<Entry>>();
			std::copy_if(current->begin(), current->end(), std::back_inserter(*kept), [owner](const Entry& e) { return e.owner != owner; });
			Publish(std::move(kept));
		}

		void Rebuild(const std::function<bool(const IMod*)>& isOwnerEnabled) {
			std::lock_guard lock(mutex);
			std::vector<const Registered*> included;
			for (const auto& r : registered) {
				if (r.owner && r.fn && (r.when == Observe::Always || isOwnerEnabled(r.owner)))
					included.push_back(&r);
			}
			std::stable_sort(included.begin(), included.end(), [](const Registered* a, const Registered* b) {
				return a->owner->Id() < b->owner->Id();
			});

			auto next = std::make_shared<std::vector<Entry>>();
			next->reserve(included.size());
			for (const auto* r : included) next->push_back({ r->owner, r->fn });
			Publish(std::move(next));
		}

		// Render thread. The count check keeps an unsubscribed hook to one relaxed load.
		void Dispatch(Event& e) const {
			if (count.load(std::memory_order_relaxed) == 0)
				return;
			const auto handlers = active.load(std::memory_order_acquire);
			for (const auto& h : *handlers) h.fn(e);
		}

	private:
		struct Registered {
			const IMod* owner;
			Observe when;
			Handler fn;
		};
		struct Entry {
			const IMod* owner;
			Handler fn;
		};

		void Publish(std::shared_ptr<std::vector<Entry>> next) {
			count.store(next->size(), std::memory_order_relaxed);
			active.store(std::move(next), std::memory_order_release);
		}

		std::mutex mutex;
		std::vector<Registered> registered;
		std::atomic<std::shared_ptr<const std::vector<Entry>>> active;
		std::atomic<size_t> count = 0;
	};

	// Every device-state channel. Adding an event is a struct above, a member here, and one
	// Dispatch in the matching hook.
	struct DeviceChannels {
		DeviceChannel<DeviceEvent::TextureBound> textureBound;
		DeviceChannel<DeviceEvent::RenderTargetBound> renderTargetBound;
		DeviceChannel<DeviceEvent::VertexShaderBound> vertexShaderBound;
		DeviceChannel<DeviceEvent::PixelShaderBound> pixelShaderBound;
		DeviceChannel<DeviceEvent::VertexShaderConstants> vertexShaderConstants;
		DeviceChannel<DeviceEvent::StretchRect> stretchRect;

		void RemoveMod(const IMod* owner) {
			ForEach([owner](auto& channel) { channel.RemoveMod(owner); });
		}

		void Rebuild(const std::function<bool(const IMod*)>& isOwnerEnabled) {
			ForEach([&](auto& channel) { channel.Rebuild(isOwnerEnabled); });
		}

	private:
		template <typename Fn>
		void ForEach(Fn&& fn) {
			fn(textureBound);
			fn(renderTargetBound);
			fn(vertexShaderBound);
			fn(pixelShaderBound);
			fn(vertexShaderConstants);
			fn(stretchRect);
		}
	};
}
