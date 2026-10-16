#pragma once

#include <algorithm>
#include <atomic>
#include <exception>
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

	// Render thread: a device handler threw. Its channel has already stopped running it.
	using DeviceFaultSink = std::function<void(const IMod* owner, const char* where, const char* detail)>;

	// One handler per mod, run in owner-Id order. Published by Rebuild like the draw lists: the
	// render thread takes one snapshot load per dispatch and never locks.
	template <typename Event>
	class DeviceChannel {
	public:
		using Handler = std::function<void(Event&)>;

		explicit DeviceChannel(const char* name) : name(name) {
			active.store(std::make_shared<const std::vector<Entry>>(), std::memory_order_relaxed);
		}
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
			std::erase(suspended, owner);
			UnpublishLocked(owner);
		}

		// Any thread. Stops the owner's handler, Always included, until RemoveMod: its mod is being faulted.
		void Suspend(const IMod* owner) {
			std::lock_guard lock(mutex);
			if (std::ranges::find(suspended, owner) == suspended.end())
				suspended.push_back(owner);
			UnpublishLocked(owner);
		}

		// Set once, before the first Dispatch.
		void SetFaultSink(DeviceFaultSink fn) { sink = std::move(fn); }

		void Rebuild(const std::function<bool(const IMod*)>& isOwnerEnabled) {
			std::lock_guard lock(mutex);
			std::vector<const Registered*> included;
			for (const auto& r : registered) {
				if (std::ranges::find(suspended, r.owner) != suspended.end()) continue;
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

		// Render thread. The count check keeps an unsubscribed hook to one load. A handler that throws is
		// dropped and reported to the sink; the others still run and the game's call goes ahead.
		void Dispatch(Event& e) {
			if (count.load(std::memory_order_acquire) == 0)
				return;
			const auto handlers = active.load(std::memory_order_acquire);
			for (const auto& h : *handlers) {
				try {
					h.fn(e);
				}
				catch (const std::exception& ex) {
					Fault(h.owner, ex.what());
				}
				catch (...) {
					Fault(h.owner, "threw an unknown exception");
				}
			}
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

		// Snapshot first: a Dispatch that sees the new count also sees the list it counts.
		void Publish(std::shared_ptr<std::vector<Entry>> next) {
			const size_t size = next->size();
			active.store(std::move(next), std::memory_order_release);
			count.store(size, std::memory_order_release);
		}

		void UnpublishLocked(const IMod* owner) {
			auto current = active.load(std::memory_order_acquire);
			auto kept = std::make_shared<std::vector<Entry>>();
			std::copy_if(current->begin(), current->end(), std::back_inserter(*kept), [owner](const Entry& e) { return e.owner != owner; });
			Publish(std::move(kept));
		}

		void Fault(const IMod* owner, const char* detail) {
			Suspend(owner);
			if (sink) sink(owner, name, detail);
		}

		const char* name;
		DeviceFaultSink sink;
		std::mutex mutex;
		std::vector<Registered> registered;
		std::vector<const IMod*> suspended; // Faulted on the render thread, until RemoveMod.
		std::atomic<std::shared_ptr<const std::vector<Entry>>> active;
		std::atomic<size_t> count = 0;
	};

	// Every device-state channel. Adding an event is a struct above, a member here, and one
	// Dispatch in the matching hook.
	struct DeviceChannels {
		DeviceChannel<DeviceEvent::TextureBound> textureBound{ "TextureBound handler" };
		DeviceChannel<DeviceEvent::RenderTargetBound> renderTargetBound{ "RenderTargetBound handler" };
		DeviceChannel<DeviceEvent::VertexShaderBound> vertexShaderBound{ "VertexShaderBound handler" };
		DeviceChannel<DeviceEvent::PixelShaderBound> pixelShaderBound{ "PixelShaderBound handler" };
		DeviceChannel<DeviceEvent::VertexShaderConstants> vertexShaderConstants{ "VertexShaderConstants handler" };
		DeviceChannel<DeviceEvent::StretchRect> stretchRect{ "StretchRect handler" };

		void RemoveMod(const IMod* owner) {
			ForEach([owner](auto& channel) { channel.RemoveMod(owner); });
		}

		void Suspend(const IMod* owner) {
			ForEach([owner](auto& channel) { channel.Suspend(owner); });
		}

		void SetFaultSink(const DeviceFaultSink& fn) {
			ForEach([&](auto& channel) { channel.SetFaultSink(fn); });
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
