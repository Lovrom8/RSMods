#include "AudioDevices.hpp"

/// <summary>
/// Setup active microphones map with all the available microphones and their respective IDs.
/// </summary>
void AudioDevices::SetupMicrophones() {
	_LOG_INIT;
	_LOG_SETLEVEL(LogLevel::Error);

	// Initialization
	activeMicrophones.clear();

	// CoInitialize
	HRESULT coInit = CoInitialize(NULL);

	// Check response of CoInitialize
	if (coInit != S_OK) {
		_LOG("Unable to get microphone volume. CoInitialize is not S_OK. Returned ");
		switch (coInit) {
		case (S_FALSE):
			_LOG_NOHEAD("S_FALSE" << std::endl);
			break;
		case (RPC_E_CHANGED_MODE):
			_LOG_NOHEAD("RPC_E_CHANGED_MODE" << std::endl);
			break;
		default: // Non-documented error.
			_LOG_NOHEAD("NOT DOCUMENTED!" << std::endl);
			break;
		}
		return;
	}

	// CoCreateInstance
	IMMDeviceEnumerator* deviceEnumerator = NULL;
	HRESULT coCI = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);

	// Check response of CoCreateInstance
	if (coCI != S_OK) {
		_LOG("Unable to get microphone volume. CoCreateInstance is not S_OK. Returned ");
		switch (coCI) {
		case (REGDB_E_CLASSNOTREG):
			_LOG_NOHEAD("REGDB_E_CLASSNOTREG" << std::endl);
			break;
		case (CLASS_E_NOAGGREGATION):
			_LOG_NOHEAD("CLASS_E_NOAGGREGATION" << std::endl);
			break;
		case (E_NOINTERFACE):
			_LOG_NOHEAD("E_NOINTERFACE" << std::endl);
			break;
		case (E_POINTER):
			_LOG_NOHEAD("E_POINTER" << std::endl);
			break;
		default: // Non-documented error.
			_LOG_NOHEAD("NOT DOCUMENTED!" << std::endl);
			break;
		}

		return;
	}

	// Null-Check
	if (!deviceEnumerator) {
		_LOG("CoCreateInstance failed while setting up microphones. DeviceEnumerator is null" << std::endl);
		return;
	}

	// Get the list of microphones
	IMMDeviceCollection* microphones;
	deviceEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &microphones);

	// Get the number of microphones in the list.
	UINT totalMicrophones = 0;
	microphones->GetCount(&totalMicrophones);

	for (int i = 0; i < totalMicrophones; i++) {
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

		// Get the Device ID of the microphone. We use this later so we don't need to keep a pointer to the device, utilizting IMMDeviceEnumerator->GetDevice().
		LPWSTR microphoneId = NULL;
		selectedMicrophone->GetId(&microphoneId);

		// Get the UTF8 name of the microphone as the .pszVal does not return the right name, and we cannot work with ::wstrings.
		std::wstring utf16MicrophoneName = std::wstring(microphoneName.pwszVal);
		int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &utf16MicrophoneName[0], utf16MicrophoneName.size(), NULL, 0, NULL, NULL);
		std::string utf8MicrophoneName(sizeNeeded, 0);
		WideCharToMultiByte(CP_UTF8, 0, &utf16MicrophoneName[0], utf16MicrophoneName.size(), &utf8MicrophoneName[0], sizeNeeded, NULL, NULL);

		// Add the UTF8 name and the Device ID to the list of microphones that are ready to use.
		activeMicrophones.insert(std::pair<std::string, LPWSTR>(utf8MicrophoneName, microphoneId));

		// Clean-up
		PropVariantClear(&microphoneName);
		microphoneProperties->Release();
		selectedMicrophone->Release();
		microphoneId = NULL;
		microphoneProperties = NULL;
		selectedMicrophone = NULL;
	}

	// Clean-up
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
	_LOG_INIT;
	_LOG_SETLEVEL(LogLevel::Error);

	if (activeMicrophones.find(microphoneName) != activeMicrophones.end()) {

		// Initialization
		LPWSTR microphoneId = activeMicrophones[microphoneName];
		IMMDevice* microphone = NULL;

		IMMDeviceEnumerator* deviceEnumerator = NULL;
		HRESULT coCI = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);

		if (coCI != S_OK) {
			_LOG("Unable to get microphone volume. CoCreateInstance is not S_OK. Returned ");
			switch (coCI) {
			case (REGDB_E_CLASSNOTREG):
				_LOG_NOHEAD("REGDB_E_CLASSNOTREG" << std::endl);
				break;
			case (CLASS_E_NOAGGREGATION):
				_LOG_NOHEAD("CLASS_E_NOAGGREGATION" << std::endl);
				break;
			case (E_NOINTERFACE):
				_LOG_NOHEAD("E_NOINTERFACE" << std::endl);
				break;
			case (E_POINTER):
				_LOG_NOHEAD("E_POINTER" << std::endl);
				break;
			default: // Non-documented error.
				_LOG_NOHEAD("NOT DOCUMENTED!" << std::endl);
				break;
			}

			return;
		}

		if (!deviceEnumerator) {
			_LOG("CoCreateInstance failed while setting microphone volume. DeviceEnumerator is null" << std::endl);
			return;
		}

		// Get the microphone we want
		deviceEnumerator->GetDevice(microphoneId, &microphone);
		deviceEnumerator->Release();
		deviceEnumerator = NULL;

		if (!microphone) {
			_LOG("GetDevice failed to get a microphone in SetVolume." << std::endl);
			return;
		}

		// Get the endpoint volume
		IAudioEndpointVolume* microphoneVolume = NULL;
		microphone->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&microphoneVolume);

		_LOG_SETLEVEL(LogLevel::Info);

		// Set the volume
		if (volume > 100)
			volume %= 100;

		microphoneVolume->SetMasterVolumeLevelScalar((float)volume / 100, NULL);

		_LOG("Set volume of microphone: \"" << microphoneName << "\" to " << volume << std::endl);

		// Clean-up
		microphone->Release();
		microphoneVolume->Release();
	}
	else
	{
		_LOG("Unable to find microphone (Set volume): \"" << microphoneName << "\"" << std::endl);
	}
}

/// <summary>
/// Gets the current volume of the selected microphone
/// </summary>
/// <param name="microphone"> - Name of the microphone</param>
/// <returns>Int 0-100 of the current volume of the microphone</returns>
int AudioDevices::GetMicrophoneVolume(std::string microphoneName) {
	_LOG_INIT;
	_LOG_SETLEVEL(LogLevel::Error);

	if (activeMicrophones.find(microphoneName) != activeMicrophones.end()) {
		// Initialization
		LPWSTR microphoneId = activeMicrophones[microphoneName];
		IMMDevice* microphone = NULL;

		// CoCreateInstance
		IMMDeviceEnumerator* deviceEnumerator = NULL;
		HRESULT coCI = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);

		// Check response of CoCreateInstance
		if (coCI != S_OK) {
			_LOG("Unable to get microphone volume. CoCreateInstance is not S_OK. Returned ");
			switch (coCI) {
			case (REGDB_E_CLASSNOTREG):
				_LOG_NOHEAD("REGDB_E_CLASSNOTREG" << std::endl);
				break;
			case (CLASS_E_NOAGGREGATION):
				_LOG_NOHEAD("CLASS_E_NOAGGREGATION" << std::endl);
				break;
			case (E_NOINTERFACE):
				_LOG_NOHEAD("E_NOINTERFACE" << std::endl);
				break;
			case (E_POINTER):
				_LOG_NOHEAD("E_POINTER" << std::endl);
				break;
			default: // Non-documented error.
				_LOG_NOHEAD("NOT DOCUMENTED!" << std::endl);
				break;
			}

			return 17.f;
		}

		// Null-Check
		if (!deviceEnumerator) {
			_LOG("CoCreateInstance failed while getting microphone volume. DeviceEnumerator is null" << std::endl);
			return 17.f;
		}

		// Get the microphone we want
		deviceEnumerator->GetDevice(microphoneId, &microphone);
		deviceEnumerator->Release();
		deviceEnumerator = NULL;

		if (!microphone) {
			_LOG("GetDevice failed to get a microphone in GetVolume." << std::endl);
			return 17.f;
		}

		// Get the endpoint volume
		IAudioEndpointVolume* microphoneVolume = NULL;
		microphone->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&microphoneVolume);

		// Get the real volume
		float currentVolume = 0.f;
		microphoneVolume->GetMasterVolumeLevelScalar(&currentVolume);

		// Clean-up
		microphone->Release();
		microphoneVolume->Release();

		return currentVolume * 100;
	}
	else
	{
		_LOG("Unable to find microphone (Get volume): \"" << microphoneName << "\"" << std::endl);
		return 17.f;
	}
}

/// <summary>
/// x86 ASM hook to change the sample rate requirement for audio output (headphones / speaker) devices.
/// </summary>
void __declspec(naked) hook_changeSampleRate() {
	__asm {
		mov EAX, AudioDevices::output_SampleRate					// Move user-provided sample rate into EAX
		jmp Offsets::ptr_sampleRateRequirementAudioOutput_JmpBck	// Jump back to the original instruction set.
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
		jmp Offsets::ptr_sampleRateDivZeroCrash_JmpBck	// Jump back to the original instruction set.
	}
}

/// <summary>
/// Reads the sample rate from output_SampleRate and tells the audio engine to approve devices with that sample rate.
/// Must be run on boot. Has no effect when run after the audio engine has initialized.
/// </summary>
void AudioDevices::ChangeOutputSampleRate() {
	MemUtil::PlaceHook((void*)Offsets::ptr_sampleRateRequirementAudioOutput, hook_changeSampleRate, 5);
	MemUtil::PlaceHook((void*)Offsets::ptr_sampleRateDivZeroCrash, hook_sampleRate_FixDivZeroCrash, 5);
}