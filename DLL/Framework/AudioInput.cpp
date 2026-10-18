#include "AudioInput.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <chrono>
#include <cmath>
#include <exception>
#include <mutex>
#include <string_view>
#include <thread>
#include <vector>

#include "../Log.hpp"
#include "IMod.hpp"

namespace Framework {
	namespace {
		using Clock = std::chrono::steady_clock;

		// Overloaded buffers before a processor is aborted: this many in a row, or this many of the last 64.
		constexpr unsigned kOverloadsInARow = 3;
		constexpr int kOverloadsInWindow = 8;

		enum class ProcessorState : std::uint8_t {
			Excluded, // Failed its first Prepare on this stream; adds no latency.
			Running,
			Bypassed, // A delay of its input latency runs instead.
		};

		enum AudioError : std::uint8_t { NoError, InvalidOutput, Overload };

		struct DelayLine {
			std::vector<float> buffer; // channels rings of `length` frames
			std::uint32_t length = 0;
			std::uint32_t channels = 0;
			std::uint32_t position = 0;

			void Reset(std::uint32_t channelCount, std::uint32_t frames) {
				length = frames;
				channels = channelCount;
				position = 0;
				buffer.assign(static_cast<size_t>(channelCount) * frames, 0.0f);
			}

			bool Process(float* const* samples, std::uint32_t frames) noexcept {
				if (length == 0) return false;

				for (std::uint32_t f = 0; f < frames; ++f) {
					for (std::uint32_t c = 0; c < channels; ++c) {
						float& slot = buffer[static_cast<size_t>(c) * length + position];
						const float delayed = slot;
						slot = samples[c][f];
						samples[c][f] = delayed;
					}
					position = position + 1 == length ? 0 : position + 1;
				}
				return true;
			}
		};

		// One processor on one input. Written by PrepareTap while the stream is stopped, then owned by its audio thread;
		// only `aborted` and `error` are shared.
		struct ProcessorStream {
			ProcessorState state = ProcessorState::Excluded;
			std::uint32_t inputLatency = 0;
			DelayLine bypassDelay;
			std::uint32_t overloadsInARow = 0;
			std::uint64_t overloadHistory = 0;
			std::atomic<bool> aborted{ false };
			std::atomic<std::uint8_t> error{ NoError };
		};

		struct Entry {
			const IMod* owner = nullptr;
			IInputProcessor* processor = nullptr;
			int order = 0;
			std::atomic<bool> active{ false };
			bool removed = false; // Under the chain mutex.
			std::array<ProcessorStream, kMaxAudioInputs> streams;
		};

		struct StreamState {
			std::mutex control; // PrepareTap/ReleaseTap; never taken on an audio thread.
			std::atomic<bool> live{ false };
			std::atomic<std::uint32_t> inFlight{ 0 };
			std::atomic<std::uint32_t> latency{ 0 };
			bool open = false; // Latency fixed from the first PrepareTap until ReleaseTap.
			CaptureFormat format{};

			// Seq_cst pairs with ProcessTap's increment-then-check: once this returns, no ProcessTap is running for
			// this input and none will start until live is set again.
			void Quiesce() {
				live.store(false);
				while (inFlight.load() != 0) std::this_thread::yield();
			}
		};

		std::string_view OwnerName(const IMod* owner) { return owner ? owner->Id() : std::string_view("(host)"); }

		// Zeroes non-finite samples; false if there were any.
		bool SanitizeOutput(float* const* channels, std::uint32_t channelCount, std::uint32_t frames) noexcept {
			bool valid = true;
			for (std::uint32_t c = 0; c < channelCount; ++c) {
				for (std::uint32_t f = 0; f < frames; ++f) {
					if (!std::isfinite(channels[c][f])) {
						channels[c][f] = 0.0f;
						valid = false;
					}
				}
			}
			return valid;
		}

		void Abort(ProcessorStream& stream, AudioError error) noexcept {
			stream.aborted.store(true, std::memory_order_relaxed);
			stream.error.store(error, std::memory_order_release);
		}
	}

	struct AudioInputChain::Impl {
		std::mutex mutex; // Entries' structure and `removed`.
		std::vector<std::unique_ptr<Entry>> entries; // Never changes size once fixed, so the audio threads can walk it.
		std::atomic<bool> fixed{ false };
		std::array<StreamState, kMaxAudioInputs> streams;

		// Runs before any input is live, so no audio thread walks the entries while they're sorted.
		void Fix() {
			std::lock_guard lock(mutex);
			if (fixed.load()) return;

			std::stable_sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
				if (a->order != b->order) return a->order < b->order;
				if (!a->owner || !b->owner) return !a->owner && b->owner;
				return a->owner->Id() < b->owner->Id();
			});
			fixed.store(true);
		}

		bool ProcessBuffer(std::uint32_t input, const StreamState& state, float* const* channels, std::uint32_t frames) noexcept {
			const std::uint32_t channelCount = state.format.channelCount;
			const std::uint32_t sampleRate = state.format.sampleRate;
			bool changed = false;

			for (const auto& entry : entries) {
				ProcessorStream& stream = entry->streams[input];

				if (stream.state == ProcessorState::Excluded) continue;

				if (stream.state == ProcessorState::Bypassed || stream.aborted.load(std::memory_order_relaxed)) {
					changed |= stream.bypassDelay.Process(channels, frames);
					continue;
				}

				const auto start = Clock::now();
				changed |= entry->processor->Process(input, channels, channelCount, frames,
					entry->active.load(std::memory_order_relaxed));
				const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();

				if (!SanitizeOutput(channels, channelCount, frames)) {
					Abort(stream, InvalidOutput);
					changed = true;
					continue;
				}

				// Budget: half the buffer's duration, leaving the rest to the tap, the game and the driver.
				if (sampleRate == 0) continue;
				const bool overload = static_cast<std::uint64_t>(elapsed) * sampleRate * 2
					> static_cast<std::uint64_t>(frames) * 1'000'000'000ull;
				stream.overloadHistory = (stream.overloadHistory << 1) | (overload ? 1u : 0u);
				stream.overloadsInARow = overload ? stream.overloadsInARow + 1 : 0;
				if (stream.overloadsInARow >= kOverloadsInARow || std::popcount(stream.overloadHistory) >= kOverloadsInWindow)
					Abort(stream, Overload);
			}

			return changed;
		}
	};

	AudioInputChain::AudioInputChain() : impl(std::make_unique<Impl>()) {}
	AudioInputChain::~AudioInputChain() = default;

	void AudioInputChain::Add(const IMod* owner, IInputProcessor& processor, int order) {
		std::lock_guard lock(impl->mutex);

		for (auto& entry : impl->entries) {
			if (entry->owner == owner && entry->processor == &processor) {
				entry->removed = false;
				if (!impl->fixed.load()) entry->order = order;

				return;
			}
		}

		if (impl->fixed.load()) {
			LOG_WARNING("[Framework] " << OwnerName(owner)
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

		if (!impl->fixed.load()) {
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

		StreamState& state = impl->streams[input];
		std::lock_guard control(state.control);
		state.Quiesce();

		if (format.channelCount == 0 || format.channelCount > kMaxInputChannels || format.maxFrames == 0) {
			LOG_ERROR("[Framework] audio tap input " << input << " has " << format.channelCount << " channel(s) and "
				<< format.maxFrames << " frames per buffer; it stays off" << std::endl);
			return 0;
		}

		impl->Fix();

		std::uint32_t total = 0;
		for (auto& entry : impl->entries) {
			ProcessorStream& stream = entry->streams[input];
			stream.aborted.store(false, std::memory_order_relaxed);
			stream.overloadsInARow = 0;
			stream.overloadHistory = 0;

			bool prepared = false;
			std::uint32_t latency = 0;
			try {
				entry->processor->Prepare(input, format);
				// Latency first: a processor whose latency throws would otherwise run with its delay uncounted.
				latency = entry->processor->GetLatencyFrames(input);
				prepared = true;
			}
			catch (const std::exception& e) {
				LOG_ERROR("[Framework] " << OwnerName(entry->owner) << " audio input processor failed to prepare input "
					<< input << ": " << e.what() << std::endl);
			}
			catch (...) {
				LOG_ERROR("[Framework] " << OwnerName(entry->owner) << " audio input processor failed to prepare input "
					<< input << std::endl);
			}

			if (!state.open) {
				stream.inputLatency = prepared ? latency : 0;
				stream.state = prepared ? ProcessorState::Running : ProcessorState::Excluded;
			}
			else if (prepared && latency == stream.inputLatency) {
				stream.state = ProcessorState::Running;
			}
			else {
				if (prepared) {
					LOG_ERROR("[Framework] " << OwnerName(entry->owner) << " audio input processor reports " << latency
						<< " frames on input " << input << " instead of " << stream.inputLatency
						<< "; bypassed until the stream reopens" << std::endl);
				}
				stream.state = stream.inputLatency ? ProcessorState::Bypassed : ProcessorState::Excluded;
			}

			stream.bypassDelay.Reset(format.channelCount, stream.inputLatency);
			total += stream.inputLatency;
		}

		state.format = format;
		state.open = true;
		state.latency.store(total, std::memory_order_relaxed);
		state.live.store(true);
		return total;
	}

	bool AudioInputChain::ProcessTap(std::uint32_t input, float* const* channels, std::uint32_t frames) noexcept {
		if (input >= kMaxAudioInputs || !channels) return false;

		StreamState& state = impl->streams[input];
		state.inFlight.fetch_add(1);
		bool changed = false;

		if (state.live.load()) {
			const std::uint32_t maxFrames = state.format.maxFrames;
			const std::uint32_t channelCount = state.format.channelCount;

			if (frames <= maxFrames) {
				changed = impl->ProcessBuffer(input, state, channels, frames);
			}
			else {
				std::array<float*, kMaxInputChannels> part{};
				for (std::uint32_t offset = 0; offset < frames; offset += maxFrames) {
					for (std::uint32_t c = 0; c < channelCount; ++c) part[c] = channels[c] + offset;
					changed |= impl->ProcessBuffer(input, state, part.data(), std::min(maxFrames, frames - offset));
				}
			}
		}

		state.inFlight.fetch_sub(1);
		return changed;
	}

	void AudioInputChain::ReleaseTap(std::uint32_t input) {
		if (input >= kMaxAudioInputs) return;

		StreamState& state = impl->streams[input];
		std::lock_guard control(state.control);
		state.Quiesce();
		state.open = false;
	}

	std::vector<AudioFault> AudioInputChain::TakeFaults() {
		std::vector<AudioFault> faults;
		std::lock_guard lock(impl->mutex);

		for (const auto& entry : impl->entries) {
			for (std::uint32_t input = 0; input < kMaxAudioInputs; ++input) {
				const auto error = entry->streams[input].error.exchange(NoError, std::memory_order_acquire);
				if (error == NoError) continue;

				faults.push_back({ entry->owner, input, error == InvalidOutput
					? "audio input processor produced NaN or infinity"
					: "audio input processor overloaded the audio thread" });
			}
		}

		return faults;
	}

	bool AudioInputChain::TapLive() const {
		return std::any_of(impl->streams.begin(), impl->streams.end(), [](const StreamState& s) { return s.live.load(); });
	}

	bool AudioInputChain::TapLive(std::uint32_t input) const {
		return input < kMaxAudioInputs && impl->streams[input].live.load();
	}

	std::uint32_t AudioInputChain::LatencyFrames(std::uint32_t input) const {
		return input < kMaxAudioInputs ? impl->streams[input].latency.load(std::memory_order_relaxed) : 0;
	}

	AudioInputChain& AudioInput() {
		static AudioInputChain instance;
		return instance;
	}
}
