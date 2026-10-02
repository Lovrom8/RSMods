#include "../stdafx.h"
#include "MetronomeAudioHook.hpp"

#include <atomic>

#include "../MemUtil.hpp"

using Metronome::AudioBlock;
using Metronome::ClickMixer;

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

	enum StatusFlag : int {
		kCodecHookInstalled = 1 << 0,
		kFactoryHooked = 1 << 1,
		kFactoryHookFailed = 1 << 2,
		kDecoderHooked = 1 << 3,
		kDecoderHookFailed = 1 << 4,
	};

	ClickMixer* mixer = nullptr;
	tRegisterCodec originalRegisterCodec = nullptr;
	AkCreateFileSourceCallback* originalFileFactory = nullptr;
	DecoderOutputFunction originalDecoderOutput = nullptr;

	std::atomic<bool> factoryHookAttempted = false;
	std::atomic<bool> decoderHookAttempted = false;
	std::atomic<bool> decoderHooked = false;
	std::atomic<int> pendingStatus = 0;

	bool IsStereoMusicBlock(const DecoderOutput* output) {
		return output != nullptr
			&& output->samples != nullptr
			&& output->channelMask == AK_SPEAKER_SETUP_STEREO
			&& output->frameCount > 0
			&& output->frameCount <= output->maxFrameCount
			&& output->sampleRate > 0;
	}

	// Audio thread: no locks, allocations or logging.
	void __fastcall DecoderOutputHook(void* decoder, void* unusedEdx, DecoderOutput* output) {
		originalDecoderOutput(decoder, unusedEdx, output);

		if (!IsStereoMusicBlock(output)) return;
		mixer->MixInto(AudioBlock{ output->samples, output->frameCount, output->positionStart, output->totalFrames, output->sampleRate });
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
}

bool Metronome::AudioHook::IsHooked() {
	return decoderHooked.load();
}

void Metronome::AudioHook::LogStatus() {
	const int status = pendingStatus.exchange(0);
	if (status & kCodecHookInstalled) LOG_INFO("(Metronome) Waiting for Wwise to register its Vorbis codec" << std::endl);
	if (status & kFactoryHooked) LOG_INFO("(Metronome) Hooked the Vorbis file-source factory" << std::endl);
	if (status & kFactoryHookFailed) LOG_ERROR("(Metronome) Couldn't hook the Vorbis file-source factory" << std::endl);
	if (status & kDecoderHooked) LOG_INFO("(Metronome) Hooked the Vorbis decoder output" << std::endl);
	if (status & kDecoderHookFailed) LOG_ERROR("(Metronome) Couldn't hook the Vorbis decoder output" << std::endl);
}
