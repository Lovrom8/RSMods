#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace Framework {
	class IMod;

	struct CaptureFormat {
		std::uint32_t sampleRate = 0;
		std::uint32_t channelCount = 0;
		std::uint32_t maxFrames = 0; // Largest block Process will be handed.
	};

	// Edits the guitar input before the game's pitch detection sees it. Process runs on the audio thread, so it must
	// not allocate, lock, log, block or throw (noexcept makes a throw terminate instead of unwinding into the driver).
	class IInputProcessor {
	public:
		virtual ~IInputProcessor() = default;

		// Off the audio thread, before any Process call and again whenever the tap's format changes. Allocate here.
		virtual void Prepare(const CaptureFormat& format) = 0;

		// `frames` frames of interleaved samples, edited in place. `active` is false while the owner isn't Active
		// (disabled, suppressed, faulted): pass the audio through, but keep the same delay.
		virtual void Process(float* samples, std::uint32_t frames, bool active) noexcept = 0;

		// Constant delay in frames, active or not. Rocksmith calibrates input latency once, so it must never change.
		virtual std::uint32_t GetLatencyFrames() const = 0;
	};

	// The chain of input processors. A mod hooking the audio driver (the "tap") feeds it; others only add processors.
	// The chain is fixed when the tap first prepares: the audio thread then walks a fixed array with no locks, and
	// the total latency the game calibrated against can't change.
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

		// Tap side. Call when the driver's buffers are created, while ProcessTap can't run (ASIO: createBuffers).
		// Prepares every processor and returns the chain's total latency in frames.
		std::uint32_t PrepareTap(const CaptureFormat& format);

		// Tap side, audio thread.
		void ProcessTap(float* samples, std::uint32_t frames) noexcept;

		// Tap side: buffers disposed. ProcessTap does nothing until the next PrepareTap.
		void ReleaseTap();

		[[nodiscard]] bool TapLive() const;
		[[nodiscard]] std::uint32_t LatencyFrames() const;

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};

	AudioInputChain& AudioInput();
}
