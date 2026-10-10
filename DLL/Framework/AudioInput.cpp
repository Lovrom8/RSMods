#include "AudioInput.hpp"

#include <algorithm>
#include <atomic>
#include <exception>
#include <mutex>
#include <vector>

#include "../Log.hpp"
#include "IMod.hpp"

namespace Framework {
	namespace {
		struct Entry {
			const IMod* owner = nullptr;
			IInputProcessor* processor = nullptr;
			int order = 0;
			std::atomic<bool> active{ false };
			bool removed = false;  // Under the mutex.
			bool prepared = false; // Written by PrepareTap only, while ProcessTap can't run.
		};
	}

	struct AudioInputChain::Impl {
		std::mutex mutex;
		std::vector<std::unique_ptr<Entry>> entries; // Never changes size once fixed, so the audio thread can walk it.
		bool fixed = false;
		std::atomic<bool> live{ false };
		std::atomic<std::uint32_t> latency{ 0 };
	};

	AudioInputChain::AudioInputChain() : impl(std::make_unique<Impl>()) {}
	AudioInputChain::~AudioInputChain() = default;

	void AudioInputChain::Add(const IMod* owner, IInputProcessor& processor, int order) {
		std::lock_guard lock(impl->mutex);

		for (auto& entry : impl->entries) {
			if (entry->owner == owner && entry->processor == &processor) {
				entry->removed = false;
				if (!impl->fixed) entry->order = order;
				
				return;
			}
		}

		if (impl->fixed) {
			LOG_WARNING("[Framework] " << (owner ? owner->Id() : "(host)") 
				<< " added an audio input processor after the audio tap started; it won't run" << std::endl);
			return;
		}

		auto entry = std::make_unique<Entry>();
		entry->owner = owner;
		entry->processor = &processor;
		entry->order = order;
		impl->entries.push_back(std::move(entry));
	}

	void AudioInputChain::RemoveMod(const IMod* owner) {
		std::lock_guard lock(impl->mutex);

		if (!impl->fixed) {
			std::erase_if(impl->entries, [owner](const auto& entry) { return entry->owner == owner; });
			return;
		}

		for (auto& entry : impl->entries) {
			if (entry->owner != owner) continue;

			entry->removed = true;
			entry->active.store(false, std::memory_order_relaxed);
		}
	}

	void AudioInputChain::PublishActive(const std::function<bool(const IMod*)>& isActive) {
		std::lock_guard lock(impl->mutex);

		for (auto& entry : impl->entries) {
			entry->active.store(!entry->removed && isActive(entry->owner), std::memory_order_relaxed);
		}
	}

	std::uint32_t AudioInputChain::PrepareTap(const CaptureFormat& format) {
		std::lock_guard lock(impl->mutex);
		impl->live.store(false, std::memory_order_release);

		if (!impl->fixed) {
			impl->fixed = true;

			std::stable_sort(impl->entries.begin(), impl->entries.end(), [](const auto& a, const auto& b) {
				if (a->order != b->order) return a->order < b->order;
				if (!a->owner || !b->owner) return !a->owner && b->owner;
				return a->owner->Id() < b->owner->Id();
			});
		}

		std::uint32_t total = 0;
		for (auto& entry : impl->entries) {
			// A processor that can't prepare drops out of the chain until the next PrepareTap.
			entry->prepared = false;
			
			try {
				entry->processor->Prepare(format);
				entry->prepared = true;
				total += entry->processor->GetLatencyFrames();
			}
			catch (const std::exception& e) {
				LOG_ERROR("[Framework] " << (entry->owner ? entry->owner->Id() : "(host)")
					<< " audio input processor failed to prepare: " << e.what() << std::endl);
			}
			catch (...) {
				LOG_ERROR("[Framework] " << (entry->owner ? entry->owner->Id() : "(host)")
					<< " audio input processor failed to prepare" << std::endl);
			}
		}

		impl->latency.store(total, std::memory_order_relaxed);
		impl->live.store(true, std::memory_order_release);
		return total;
	}

	void AudioInputChain::ProcessTap(float* samples, std::uint32_t frames) noexcept {
		if (!impl->live.load(std::memory_order_acquire)) return;

		for (const auto& entry : impl->entries) {
			if (entry->prepared)
				entry->processor->Process(samples, frames, entry->active.load(std::memory_order_relaxed));
		}
	}

	void AudioInputChain::ReleaseTap() {
		impl->live.store(false, std::memory_order_release);
	}

	bool AudioInputChain::TapLive() const {
		return impl->live.load(std::memory_order_acquire);
	}

	std::uint32_t AudioInputChain::LatencyFrames() const {
		return impl->latency.load(std::memory_order_relaxed);
	}

	AudioInputChain& AudioInput() {
		static AudioInputChain instance;
		return instance;
	}
}
