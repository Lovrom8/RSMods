#include "AudioInput.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <exception>
#include <mutex>
#include <thread>
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
			bool removed = false; // Under the mutex.
			std::array<bool, kMaxAudioInputs> prepared{}; // Per input, written by PrepareTap only while that input is idle.
		};

		struct InputState {
			std::atomic<bool> live{ false };
			std::atomic<std::uint32_t> inFlight{ 0 }; // ProcessTap calls past their live check.
			std::atomic<std::uint32_t> latency{ 0 };

			// Seq_cst pairs with ProcessTap's increment-then-check: once this returns, no ProcessTap is running for
			// this input and none will start until live is set again.
			void Quiesce() {
				live.store(false);
				while (inFlight.load() != 0) std::this_thread::yield();
			}
		};
	}

	struct AudioInputChain::Impl {
		std::mutex mutex;
		std::vector<std::unique_ptr<Entry>> entries; // Never changes size once fixed, so the audio threads can walk it.
		bool fixed = false;
		std::array<InputState, kMaxAudioInputs> inputs;
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

	std::uint32_t AudioInputChain::PrepareTap(std::uint32_t input, const CaptureFormat& format) {
		if (input >= kMaxAudioInputs) {
			LOG_ERROR("[Framework] audio tap prepared input " << input << ", past the last one ("
				<< kMaxAudioInputs - 1 << ")" << std::endl);
			return 0;
		}

		std::lock_guard lock(impl->mutex);
		InputState& state = impl->inputs[input];
		state.Quiesce();

		// No input has been live yet, so no audio thread is walking the entries while they're sorted.
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
			// A processor that can't prepare drops out of this input's chain until its next PrepareTap.
			entry->prepared[input] = false;

			try {
				entry->processor->Prepare(input, format);
				entry->prepared[input] = true;
				total += entry->processor->GetLatencyFrames(input);
			}
			catch (const std::exception& e) {
				LOG_ERROR("[Framework] " << (entry->owner ? entry->owner->Id() : "(host)")
					<< " audio input processor failed to prepare input " << input << ": " << e.what() << std::endl);
			}
			catch (...) {
				LOG_ERROR("[Framework] " << (entry->owner ? entry->owner->Id() : "(host)")
					<< " audio input processor failed to prepare input " << input << std::endl);
			}
		}

		state.latency.store(total, std::memory_order_relaxed);
		state.live.store(true);
		return total;
	}

	void AudioInputChain::ProcessTap(std::uint32_t input, float* samples, std::uint32_t frames) noexcept {
		if (input >= kMaxAudioInputs) return;

		InputState& state = impl->inputs[input];
		state.inFlight.fetch_add(1);

		if (state.live.load()) {
			for (const auto& entry : impl->entries) {
				if (entry->prepared[input])
					entry->processor->Process(input, samples, frames, entry->active.load(std::memory_order_relaxed));
			}
		}

		state.inFlight.fetch_sub(1);
	}

	void AudioInputChain::ReleaseTap(std::uint32_t input) {
		if (input >= kMaxAudioInputs) return;

		std::lock_guard lock(impl->mutex);
		impl->inputs[input].Quiesce();
	}

	bool AudioInputChain::TapLive() const {
		return std::any_of(impl->inputs.begin(), impl->inputs.end(), [](const InputState& s) { return s.live.load(); });
	}

	bool AudioInputChain::TapLive(std::uint32_t input) const {
		return input < kMaxAudioInputs && impl->inputs[input].live.load();
	}

	std::uint32_t AudioInputChain::LatencyFrames(std::uint32_t input) const {
		return input < kMaxAudioInputs ? impl->inputs[input].latency.load(std::memory_order_relaxed) : 0;
	}

	AudioInputChain& AudioInput() {
		static AudioInputChain instance;
		return instance;
	}
}
