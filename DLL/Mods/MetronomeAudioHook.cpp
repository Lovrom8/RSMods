#include "../stdafx.h"
#include "MetronomeAudioHook.hpp"

#include <atomic>
#include <mmreg.h>

#include "../MemUtil.hpp"
#include "../Offsets.hpp"
#include "MetronomeSongClock.hpp"

using Metronome::ClickMixer;
using Metronome::OutputBuffer;
using Metronome::SampleFormat;
using Metronome::SongClock;

namespace {
	constexpr AkUInt32 kAudiokineticCompanyId = 0;
	constexpr AkUInt32 kVorbisCodecId = 4;
	constexpr int kDecoderOutputVtableSlot = 10;

	// The decoder's output block, filled in by its output function. Only the fields used here are named.
	struct DecoderOutput {
		int16_t* samples; // Interleaved.
		uint32_t channelMask;
		uint32_t unknown08;
		uint16_t maxFrameCount;
		uint16_t frameCount;
		uint32_t unknown10;
		uint32_t unknown14;
		uint32_t positionStart; // Absolute frame of the block in the stream.
		uint32_t unknown1C;
		uint32_t totalFrames;
		uint32_t sampleRate;
	};
	static_assert(offsetof(DecoderOutput, frameCount) == 0x0E);
	static_assert(offsetof(DecoderOutput, positionStart) == 0x18);
	static_assert(offsetof(DecoderOutput, sampleRate) == 0x24);

	using DecoderOutputFunction = void(__fastcall*)(void* decoder, void* unusedEdx, DecoderOutput* output);

	using OutputCallback = int(__cdecl*)(const void* input, void* output, unsigned long frameCount,
		const void* timeInfo, unsigned long statusFlags, void* userData);

	constexpr unsigned char kOutputCallbackPrologue[] = { 0x55, 0x8B, 0xEC, 0x56, 0x8B, 0x75, 0x1C, 0x57, 0x8D, 0x7E, 0x10, 0x57, 0xFF, 0x15 };

	// Wwise's output ring, the output callback's userData. Read from the callback's code: it copies slot [+0xAC] of
	// the ring to the device and advances it; Wwise renders into slot [+0xA8].
	constexpr uintptr_t kRingWriteSlot = 0xA8;
	constexpr uintptr_t kRingReadSlot = 0xAC;
	constexpr uintptr_t kRingFormat = 0xBA; // WAVEFORMATEXTENSIBLE

	constexpr uint16_t kWaveFormatPcm = 1;
	constexpr uint16_t kWaveFormatFloat = 3;
	constexpr uint16_t kWaveFormatExtensible = 0xFFFE;

	enum StatusFlag : int {
		kCodecHookInstalled = 1 << 0,
		kFactoryHooked = 1 << 1,
		kFactoryHookFailed = 1 << 2,
		kDecoderHooked = 1 << 3,
		kDecoderHookFailed = 1 << 4,
		kOutputHookInstalled = 1 << 5,
		kOutputHookFailed = 1 << 6,
		kOutputFormatUnsupported = 1 << 7,
		kOutputFaulted = 1 << 8,
	};

	ClickMixer* mixer = nullptr;
	tRegisterCodec originalRegisterCodec = nullptr;
	AkCreateFileSourceCallback* originalFileFactory = nullptr;
	DecoderOutputFunction originalDecoderOutput = nullptr;
	OutputCallback originalOutputCallback = nullptr;
	SongClock songClock;

	std::atomic<bool> factoryHookAttempted = false;
	std::atomic<bool> decoderHookAttempted = false;
	std::atomic<bool> decoderHooked = false;
	std::atomic<uintptr_t> outputRing = 0;
	std::atomic<bool> outputFaulted = false;
	std::atomic<bool> unsupportedFormatReported = false;
	std::atomic<int> pendingStatus = 0;

	bool IsStereoMusicBlock(const DecoderOutput* output) {
		return output != nullptr
			&& output->samples != nullptr
			&& output->channelMask == AK_SPEAKER_SETUP_STEREO
			&& output->frameCount > 0
			&& output->frameCount <= output->maxFrameCount
			&& output->sampleRate > 0;
	}

	// Wwise's render thread: no locks, allocations or logging. Tags the ring slot being rendered with the song position.
	void __fastcall DecoderOutputHook(void* decoder, void* unusedEdx, DecoderOutput* output) {
		originalDecoderOutput(decoder, unusedEdx, output);

		if (!IsStereoMusicBlock(output) || !mixer->IsSongStream(output->totalFrames, output->sampleRate)) return;

		uint32_t slot = 0;
		if (MemUtil::TryRead(outputRing.load() + kRingWriteSlot, slot))
			songClock.TagSlot(slot, output->positionStart, output->sampleRate);
	}

	std::optional<SampleFormat> ReadSampleFormat(uintptr_t ring, uint16_t& channels, uint32_t& sampleRate) {
		const auto* format = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(ring + kRingFormat);
		channels = format->Format.nChannels;
		sampleRate = format->Format.nSamplesPerSec;

		uint16_t tag = format->Format.wFormatTag;
		if (tag == kWaveFormatExtensible) tag = static_cast<uint16_t>(format->SubFormat.Data1);

		const uint16_t bits = format->Format.wBitsPerSample;
		if (tag == kWaveFormatFloat && bits == 32) return SampleFormat::Float32;
		if (tag == kWaveFormatPcm && bits == 16) return SampleFormat::Int16;
		if (tag == kWaveFormatPcm && bits == 32) return SampleFormat::Int32;
		return std::nullopt;
	}

	void MixIntoOutput(uintptr_t ring, void* output, unsigned long frameCount, uint32_t playedSlot) {
		uint16_t channels = 0;
		uint32_t sampleRate = 0;
		const std::optional<SampleFormat> format = ReadSampleFormat(ring, channels, sampleRate);
		const Metronome::ClockReading clock = songClock.Read(playedSlot, frameCount, sampleRate);

		if (!format || channels == 0) {
			if (!unsupportedFormatReported.exchange(true)) pendingStatus.fetch_or(kOutputFormatUnsupported);
			return;
		}
		mixer->MixInto(OutputBuffer{ output, *format, channels, frameCount, sampleRate }, clock);
	}

	// A fault here must not take down the audio thread: the game would wait for audio forever. Mixing stops instead.
	void MixIntoOutputSafely(uintptr_t ring, void* output, unsigned long frameCount, uint32_t playedSlot) {
		__try {
			MixIntoOutput(ring, output, frameCount, playedSlot);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			outputFaulted.store(true);
			pendingStatus.fetch_or(kOutputFaulted);
		}
	}

	// The device's audio thread, once per buffer: the original copies the next rendered slot into `output`.
	int __cdecl OutputCallbackHook(const void* input, void* output, unsigned long frameCount,
		const void* timeInfo, unsigned long statusFlags, void* userData) {
		const uintptr_t ring = reinterpret_cast<uintptr_t>(userData);
		uint32_t playedSlot = 0;
		const bool slotKnown = MemUtil::TryRead(ring + kRingReadSlot, playedSlot);

		const int result = originalOutputCallback(input, output, frameCount, timeInfo, statusFlags, userData);

		outputRing.store(ring);
		if (output != nullptr && slotKnown && !outputFaulted.load())
			MixIntoOutputSafely(ring, output, frameCount, playedSlot);
		return result;
	}

	void HookOutputCallback() {
		const uintptr_t target = Offsets::func_wwiseOutputCallback.Get();
		if (target == 0 || !MemUtil::MatchesBytes(target, kOutputCallbackPrologue)) {
			pendingStatus.fetch_or(kOutputHookFailed);
			return;
		}

		originalOutputCallback = reinterpret_cast<OutputCallback>(DetourFunction(
			reinterpret_cast<PBYTE>(target), reinterpret_cast<PBYTE>(OutputCallbackHook)));
		pendingStatus.fetch_or(originalOutputCallback != nullptr ? kOutputHookInstalled : kOutputHookFailed);
	}

	// Every Vorbis file decoder shares one vtable, so hooking the first decoder's output covers them all.
	void HookDecoderOutput(IAkSoftwareCodec* decoder) {
		if (decoder == nullptr || decoderHookAttempted.exchange(true)) return;

		auto* vtable = *reinterpret_cast<uintptr_t**>(decoder);
		if (vtable == nullptr || MemUtil::IsBadReadPtr(vtable)) {
			pendingStatus.fetch_or(kDecoderHookFailed);
			return;
		}

		originalDecoderOutput = reinterpret_cast<DecoderOutputFunction>(DetourFunction(
			reinterpret_cast<PBYTE>(vtable[kDecoderOutputVtableSlot]), reinterpret_cast<PBYTE>(DecoderOutputHook)));
		decoderHooked.store(originalDecoderOutput != nullptr);
		pendingStatus.fetch_or(originalDecoderOutput != nullptr ? kDecoderHooked : kDecoderHookFailed);
	}

	IAkSoftwareCodec* __cdecl FileFactoryHook(void* context) {
		IAkSoftwareCodec* decoder = originalFileFactory(context);
		HookDecoderOutput(decoder);
		return decoder;
	}

	// The file factory makes decoders for streamed music; the bank factory, for menu sound effects.
	void HookFileFactory(AkCreateFileSourceCallback* fileFactory, AkCreateBankSourceCallback* bankFactory) {
		if (factoryHookAttempted.exchange(true)) return;

		if (fileFactory == nullptr || fileFactory == bankFactory) {
			pendingStatus.fetch_or(kFactoryHookFailed);
			return;
		}

		originalFileFactory = reinterpret_cast<AkCreateFileSourceCallback*>(DetourFunction(
			reinterpret_cast<PBYTE>(fileFactory), reinterpret_cast<PBYTE>(FileFactoryHook)));
		pendingStatus.fetch_or(originalFileFactory != nullptr ? kFactoryHooked : kFactoryHookFailed);
	}

	AKRESULT __cdecl RegisterCodecHook(AkUInt32 companyId, AkUInt32 codecId,
		AkCreateFileSourceCallback fileFactory, AkCreateBankSourceCallback bankFactory) {
		if (companyId == kAudiokineticCompanyId && codecId == kVorbisCodecId)
			HookFileFactory(fileFactory, bankFactory);

		return originalRegisterCodec(companyId, codecId, fileFactory, bankFactory);
	}
}

void Metronome::AudioHook::Install(ClickMixer& clickMixer) {
	if (originalRegisterCodec != nullptr) return;

	const uintptr_t registerCodec = Wwise::Exports::func_Wwise_Sound_RegisterCodec.Get();
	if (registerCodec == 0 || MemUtil::IsBadReadPtr(reinterpret_cast<void*>(registerCodec))) {
		LOG_ERROR("(Metronome) Couldn't find Wwise's RegisterCodec; the metronome will be silent" << std::endl);
		return;
	}

	mixer = &clickMixer;
	originalRegisterCodec = reinterpret_cast<tRegisterCodec>(DetourFunction(
		reinterpret_cast<PBYTE>(registerCodec), reinterpret_cast<PBYTE>(RegisterCodecHook)));
	if (originalRegisterCodec == nullptr) {
		LOG_ERROR("(Metronome) Couldn't hook Wwise's RegisterCodec; the metronome will be silent" << std::endl);
		return;
	}

	pendingStatus.fetch_or(kCodecHookInstalled);
	HookOutputCallback();
}

bool Metronome::AudioHook::IsHooked() {
	return decoderHooked.load() && originalOutputCallback != nullptr;
}

void Metronome::AudioHook::LogStatus() {
	const int status = pendingStatus.exchange(0);
	if (status & kCodecHookInstalled) LOG_INFO("(Metronome) Waiting for Wwise to register its Vorbis codec" << std::endl);
	if (status & kFactoryHooked) LOG_INFO("(Metronome) Hooked the Vorbis file-source factory" << std::endl);
	if (status & kFactoryHookFailed) LOG_ERROR("(Metronome) Couldn't hook the Vorbis file-source factory" << std::endl);
	if (status & kDecoderHooked) LOG_INFO("(Metronome) Hooked the Vorbis decoder output" << std::endl);
	if (status & kDecoderHookFailed) LOG_ERROR("(Metronome) Couldn't hook the Vorbis decoder output" << std::endl);
	if (status & kOutputHookInstalled) LOG_INFO("(Metronome) Hooked Wwise's output callback" << std::endl);
	if (status & kOutputHookFailed) LOG_ERROR("(Metronome) Couldn't find Wwise's output callback in this game version; the metronome will be silent" << std::endl);
	if (status & kOutputFormatUnsupported) LOG_ERROR("(Metronome) The game's output format isn't supported; the metronome will be silent" << std::endl);
	if (status & kOutputFaulted) LOG_ERROR("(Metronome) Mixing the clicks failed and was switched off; the game's audio is unaffected" << std::endl);
}
