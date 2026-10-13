#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace Framework {
	class IMod;

	// Separate capture streams the tap can feed, each with its own format, latency and audio thread. The tap picks
	// the index; Rocksmith's capture streams map to it by player (0 = player 1, 1 = player 2).
	inline constexpr std::uint32_t kMaxAudioInputs = 2;

	struct CaptureFormat {
		std::uint32_t sampleRate = 0;
		std::uint32_t channelCount = 0;
		std::uint32_t maxFrames = 0; // Largest block Process will be handed.
	};

	// Edits the guitar input before the game's pitch detection sees it. Process runs on the audio thread, so it must
	// not allocate, lock, log, block or throw (noexcept makes a throw terminate instead of unwinding into the driver).
	// Every call carries the input index; keep state per input, since one input can be prepared while another runs.
	class IInputProcessor {
	public:
		virtual ~IInputProcessor() = default;

		// Off that input's audio thread, before its first Process call and again whenever its format changes.
		// Allocate here. It may overlap Process for a different input.
		virtual void Prepare(std::uint32_t input, const CaptureFormat& format) = 0;

		// `frames` frames of interleaved samples, edited in place. `active` is false while the owner isn't Active
		// (disabled, suppressed, faulted): pass the audio through, but keep the same delay.
		virtual void Process(std::uint32_t input, float* samples, std::uint32_t frames, bool active) noexcept = 0;

		// Constant delay in frames for this input, active or not. Rocksmith calibrates input latency once, so it must
		// never change between Prepare calls.
		virtual std::uint32_t GetLatencyFrames(std::uint32_t input) const = 0;
	};

	// The chain of input processors. A mod hooking the audio driver (the "tap") feeds it; others only add processors.
	// The chain is fixed when the tap first prepares any input: the audio threads then walk a fixed array with no
	// locks, and the total latency the game calibrated against can't change.
	class AudioInputChain {
	public:
		AudioInputChain();
		~AudioInputChain();

		AudioInputChain(const AudioInputChain&) = delete;
		AudioInputChain& operator=(const AudioInputChain&) = delete;

		// MainThread. Lower `order` runs first; ties go by owner Id. Once the tap has prepared, a new processor is
		// logged and ignored; adding one that's already in (a retried mod re-running OnInitialize) is fine.
		void Add(const IMod* owner, IInputProcessor& processor, int order);

		// Fault or shutdown. Before the tap prepares the owner's processors are dropped; after, they stay in the
		// chain inactive, keeping their latency.
		void RemoveMod(const IMod* owner);

		// MainThread: which owners are Active, i.e. what each processor's `active` is.
		void PublishActive(const std::function<bool(const IMod*)>& isActive);

		// Tap side, any thread except from inside ProcessTap for the same input. Waits for an in-flight ProcessTap on
		// that input to return, prepares every processor for it and returns that input's total latency in frames
		// (0 for an index past kMaxAudioInputs).
		std::uint32_t PrepareTap(std::uint32_t input, const CaptureFormat& format);

		// Tap side, that input's audio thread.
		void ProcessTap(std::uint32_t input, float* samples, std::uint32_t frames) noexcept;

		// Tap side: that stream is gone. Waits like PrepareTap; ProcessTap does nothing for it until the next one.
		void ReleaseTap(std::uint32_t input);

		[[nodiscard]] bool TapLive() const; // Any input.
		[[nodiscard]] bool TapLive(std::uint32_t input) const;
		[[nodiscard]] std::uint32_t LatencyFrames(std::uint32_t input) const;

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};

	AudioInputChain& AudioInput();
}
