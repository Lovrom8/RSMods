#include "../stdafx.h"
#include "AudioDevices.hpp"
#include "../Framework/InstructionPatch.hpp"
#include "../VersioningStruct.h"

namespace {
	using namespace Framework::InstructionPatch;

	const auto readSite = [](std::uintptr_t address, auto& bytes) {
		return MemUtil::TryRead(address, bytes);
	};
	const auto writeSite = [](std::uintptr_t address, const auto& bytes) {
		return FromWriteResult(MemUtil::WritePatch(reinterpret_cast<LPVOID>(address), bytes.data(), bytes.size()),
			MemUtil::PatchResult::Installed, MemUtil::PatchResult::Original, MemUtil::PatchResult::RollbackIncomplete);
	};

	template <std::size_t Size>
	PatchedSite<Size> MakeSite(const char* name, std::uintptr_t address, const VersionedPattern<Size>& patterns) {
		return { name, address, GameVersion::Current(), patterns };
	}

	template <std::size_t Size>
	void LogOriginal(const char* name, std::uintptr_t address, const std::array<std::uint8_t, Size>& bytes) {
		std::ostringstream text;
		for (std::uint8_t byte : bytes)
			text << std::hex << static_cast<unsigned int>(byte) << ' ';
		LOG_INFO("(AUDIO DEVICES) " << name << " original at 0x" << std::hex << address << ": " << text.str() << std::endl);
	}

	const auto logOriginal = [](const char* name, std::uintptr_t address, const auto& bytes) {
		LogOriginal(name, address, bytes);
	};

	void LogResult(const Framework::InstructionPatch::Result& result) {
		using Framework::InstructionPatch::Failure;
		using Framework::InstructionPatch::Outcome;
		if (result.outcome == Outcome::Installed) {
			if (result.changed)
				LOG_INFO("(AUDIO DEVICES) Output sample-rate hooks installed" << std::endl);
		}
		else if (result.outcome == Outcome::RollbackIncomplete) {
			LOG_ERROR("(AUDIO DEVICES) Output sample-rate hook group is indeterminate at "
				<< (result.site ? result.site : "patch group") << std::endl);
		}
		else if (result.failure != Failure::None) {
			LOG_WARNING("(AUDIO DEVICES) Output sample-rate hooks left unchanged at "
				<< (result.site ? result.site : "patch group") << ": " << FailureName(result.failure) << std::endl);
		}
	}
}

/// <summary>
/// Setup active microphones map with all the available microphones and their respective IDs.
/// </summary>
void AudioDevices::SetupMicrophones() {
	activeMicrophones.clear();

	HRESULT coInit = CoInitialize(NULL);

	if (coInit != S_OK) {
		LOG_ERROR("Unable to get microphone volume. CoInitialize is not S_OK. Returned ");
		switch (coInit) {
		case (S_FALSE):
			LOG_NOHEAD("S_FALSE" << std::endl);
			break;
		case (RPC_E_CHANGED_MODE):
			LOG_NOHEAD("RPC_E_CHANGED_MODE" << std::endl);
			break;
		default: // Non-documented error.
			LOG_NOHEAD("NOT DOCUMENTED!" << std::endl);
			break;
		}
		return;
	}

	IMMDeviceEnumerator* deviceEnumerator = NULL;
	HRESULT coCI = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);

	// Check response of CoCreateInstance
	if (coCI != S_OK) {
		LOG_ERROR("Unable to get microphone volume. CoCreateInstance is not S_OK. Returned ");
		switch (coCI) {
		case (REGDB_E_CLASSNOTREG):
			LOG_NOHEAD("REGDB_E_CLASSNOTREG" << std::endl);
			break;
		case (CLASS_E_NOAGGREGATION):
			LOG_NOHEAD("CLASS_E_NOAGGREGATION" << std::endl);
			break;
		case (E_NOINTERFACE):
			LOG_NOHEAD("E_NOINTERFACE" << std::endl);
			break;
		case (E_POINTER):
			LOG_NOHEAD("E_POINTER" << std::endl);
			break;
		default: // Non-documented error.
			LOG_NOHEAD("NOT DOCUMENTED!" << std::endl);
			break;
		}

		return;
	}

	if (!deviceEnumerator) {
		LOG_ERROR("CoCreateInstance failed while setting up microphones. DeviceEnumerator is null" << std::endl);
		return;
	}

	// Get the list of microphones
	IMMDeviceCollection* microphones;
	deviceEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &microphones);

	// Get the number of microphones in the list.
	UINT totalMicrophones = 0;
	microphones->GetCount(&totalMicrophones);

	for (UINT i = 0; i < totalMicrophones; i++) {
		// Take a microphone
		IMMDevice* selectedMicrophone;
		microphones->Item(i, &selectedMicrophone);

		// Get the list of details about the microphone
		IPropertyStore* microphoneProperties = NULL;
		selectedMicrophone->OpenPropertyStore(STGM_READ, &microphoneProperties);

		// Get the "Device Friendly Name" of the microphone.
		PROPVARIANT microphoneName;
		PropVariantInit(&microphoneName);
		microphoneProperties->GetValue(PKEY_Device_FriendlyName, &microphoneName);

		// Get the Device ID of the microphone. We use this later so we don't need to keep a pointer to the device, utilizing IMMDeviceEnumerator->GetDevice().
		LPWSTR microphoneId = NULL;
		selectedMicrophone->GetId(&microphoneId);

		// Get the UTF8 name of the microphone as the .pszVal does not return the right name, and we cannot work with ::wstrings.
		std::wstring utf16MicrophoneName = std::wstring(microphoneName.pwszVal);
		int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &utf16MicrophoneName[0], utf16MicrophoneName.size(), NULL, 0, NULL, NULL);
		std::string utf8MicrophoneName(sizeNeeded, 0);
		WideCharToMultiByte(CP_UTF8, 0, &utf16MicrophoneName[0], utf16MicrophoneName.size(), &utf8MicrophoneName[0], sizeNeeded, NULL, NULL);

		// Add the UTF8 name and the Device ID to the list of microphones that are ready to use.
		activeMicrophones.insert(std::pair<std::string, LPWSTR>(utf8MicrophoneName, microphoneId));

		PropVariantClear(&microphoneName);
		microphoneProperties->Release();
		selectedMicrophone->Release();
		microphoneId = NULL;
		microphoneProperties = NULL;
		selectedMicrophone = NULL;
	}

	microphones->Release();
	deviceEnumerator->Release();
	microphones = NULL;
	deviceEnumerator = NULL;
}


/// <summary>
/// Sets the volume of the selected microphone
/// </summary>
/// <param name="microphone"> - Name of the microphone</param>
/// <param name="volume"> - New Volume</param>
void AudioDevices::SetMicrophoneVolume(std::string microphoneName, int volume) {
	if (auto it = activeMicrophones.find(microphoneName); it != activeMicrophones.end()) {
		LPWSTR microphoneId = it->second;
		IMMDevice* microphone = NULL;

		IMMDeviceEnumerator* deviceEnumerator = NULL;
		HRESULT coCI = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);

		if (coCI != S_OK) {
			LOG_ERROR("Unable to get microphone volume. CoCreateInstance is not S_OK. Returned ");
			switch (coCI) {
			case (REGDB_E_CLASSNOTREG):
				LOG_NOHEAD("REGDB_E_CLASSNOTREG" << std::endl);
				break;
			case (CLASS_E_NOAGGREGATION):
				LOG_NOHEAD("CLASS_E_NOAGGREGATION" << std::endl);
				break;
			case (E_NOINTERFACE):
				LOG_NOHEAD("E_NOINTERFACE" << std::endl);
				break;
			case (E_POINTER):
				LOG_NOHEAD("E_POINTER" << std::endl);
				break;
			default: // Non-documented error.
				LOG_NOHEAD("NOT DOCUMENTED!" << std::endl);
				break;
			}

			return;
		}

		if (!deviceEnumerator) {
			LOG_ERROR("CoCreateInstance failed while setting microphone volume. DeviceEnumerator is null" << std::endl);
			return;
		}

		// Get the microphone we want
		deviceEnumerator->GetDevice(microphoneId, &microphone);
		deviceEnumerator->Release();
		deviceEnumerator = NULL;

		if (!microphone) {
			LOG_ERROR("GetDevice failed to get a microphone in SetVolume." << std::endl);
			return;
		}

		// Get the endpoint volume
		IAudioEndpointVolume* microphoneVolume = NULL;
		microphone->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&microphoneVolume);

		if (volume > 100)
			volume %= 100;

		microphoneVolume->SetMasterVolumeLevelScalar((float)volume / 100, NULL);

		LOG_INFO("Set volume of microphone: \"" << microphoneName << "\" to " << volume << std::endl);

		// Clean-up
		microphone->Release();
		microphoneVolume->Release();
	}
	else
	{
		LOG_ERROR("Unable to find microphone (Set volume): \"" << microphoneName << "\"" << std::endl);
	}
}

/// <summary>
/// Gets the current volume of the selected microphone
/// </summary>
/// <param name="microphone"> - Name of the microphone</param>
/// <returns>Int 0-100 of the current volume of the microphone</returns>
int AudioDevices::GetMicrophoneVolume(std::string microphoneName) {
	if (auto it = activeMicrophones.find(microphoneName); it != activeMicrophones.end()) {
		LPWSTR microphoneId = it->second;
		IMMDevice* microphone = NULL;

		IMMDeviceEnumerator* deviceEnumerator = NULL;
		HRESULT coCI = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);

		if (coCI != S_OK) {
			LOG_ERROR("Unable to get microphone volume. CoCreateInstance is not S_OK. Returned ");
			switch (coCI) {
			case (REGDB_E_CLASSNOTREG):
				LOG_NOHEAD("REGDB_E_CLASSNOTREG" << std::endl);
				break;
			case (CLASS_E_NOAGGREGATION):
				LOG_NOHEAD("CLASS_E_NOAGGREGATION" << std::endl);
				break;
			case (E_NOINTERFACE):
				LOG_NOHEAD("E_NOINTERFACE" << std::endl);
				break;
			case (E_POINTER):
				LOG_NOHEAD("E_POINTER" << std::endl);
				break;
			default: // Non-documented error.
				LOG_NOHEAD("NOT DOCUMENTED!" << std::endl);
				break;
			}

			return 17;
		}

		if (!deviceEnumerator) {
			LOG_ERROR("CoCreateInstance failed while getting microphone volume. DeviceEnumerator is null" << std::endl);
			return 17;
		}

		// Get the microphone we want
		deviceEnumerator->GetDevice(microphoneId, &microphone);
		deviceEnumerator->Release();
		deviceEnumerator = NULL;

		if (!microphone) {
			LOG_ERROR("GetDevice failed to get a microphone in GetVolume." << std::endl);
			return 17;
		}

		// Get the endpoint volume
		IAudioEndpointVolume* microphoneVolume = NULL;
		microphone->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&microphoneVolume);

		// Get the real volume
		float currentVolume = 0.f;
		microphoneVolume->GetMasterVolumeLevelScalar(&currentVolume);

		microphone->Release();
		microphoneVolume->Release();

		return static_cast<int>(currentVolume * 100);
	}
	else
	{
		LOG_ERROR("Unable to find microphone (Get volume): \"" << microphoneName << "\"" << std::endl);
		return 17;
	}
}

/// <summary>
/// x86 ASM hook to change the sample rate requirement for audio output (headphones / speaker) devices.
/// </summary>
void __declspec(naked) hook_changeSampleRate() {
	__asm {
		mov EAX, AudioDevices::output_SampleRate					// Move user-provided sample rate into EAX
		test ESP, ESP												// ZF = 0 so the game's jnz right after keeps our rate instead of loading 24000

		push offset Offsets::ptr_sampleRateRequirementAudioOutput_JmpBck
		jmp MemUtil::JumpToVersioned
	}
}

/// <summary>
/// Fixes a divide by zero crash when using a sample rate above 48000Hz / 48kHz.
/// </summary>
void __declspec(naked) hook_sampleRate_FixDivZeroCrash() {
	__asm {
		// There is a div EBX that we are replacing, but EBX is always 1 (unless the sample rate is above 48kHz) so we just remove it to place this hook.

		mov EBX, 0x1									// Move 1 into the EBX register. This prevents the divide by 0 crash when using a sample rate above 48kHz.
		shr esi, 0x10									// Replace the original code we overwrote.

		push offset Offsets::ptr_sampleRateDivZeroCrash_JmpBck
		jmp MemUtil::JumpToVersioned
	}
}

/// <summary>
/// Reads the sample rate from output_SampleRate and tells the audio engine to approve devices with that sample rate.
/// Must be run on boot. Has no effect when run after the audio engine has initialized.
/// </summary>
void AudioDevices::ChangeOutputSampleRate() {
	static auto requirementSite = MakeSite("sample-rate requirement", Offsets::ptr_sampleRateRequirementAudioOutput.GetValue(), VersionedPattern<5>{
		ExactPattern(MakeBytes(0xB8, 0x80, 0xBB, 0x00, 0x00)),
		UnavailablePattern<5>()
	});
	static auto divideSite = MakeSite("sample-rate divide guard", Offsets::ptr_sampleRateDivZeroCrash.GetValue(), VersionedPattern<5>{
		ExactPattern(MakeBytes(0xF7, 0xF3, 0xC1, 0xEE, 0x10)),
		ExactPattern(MakeBytes(0xF7, 0xF3, 0xC1, 0xEE, 0x10))
	});
	std::array<std::uint8_t, 5> requirementJump{};
	std::array<std::uint8_t, 5> divideJump{};
	if (!BuildRelativeJump(requirementSite.Address(), reinterpret_cast<std::uintptr_t>(&hook_changeSampleRate), requirementJump)
		|| !BuildRelativeJump(divideSite.Address(), reinterpret_cast<std::uintptr_t>(&hook_sampleRate_FixDivZeroCrash), divideJump)) {
		LogResult({ Outcome::Original, Failure::Unavailable, "sample-rate hook address" });
		return;
	}
	requirementSite.SetReplacement(requirementJump);
	divideSite.SetReplacement(divideJump);
	LogResult(InstallGroup(readSite, writeSite, logOriginal, requirementSite, divideSite));
}
