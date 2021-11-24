#include "VolumeControl.hpp"

/// <summary>
/// Increase Volume of Mixer's Backend
/// </summary>
/// <param name="amountToIncrease"> - How much should we increase by?</param>
/// <param name="mixerToIncrease"> - Name of Mixer Value</param>
void VolumeControl::IncreaseVolume(int amountToIncrease, std::string mixerToIncrease) {
	float volume = 0;
	RTPCValue_type type = RTPCValue_GameObject;

	if (!MemHelpers::IsInStringArray(mixerToIncrease, mixerNames)) {
		std::cout << "That mixer doesn't exist" << std::endl;
		return;
	}

	WwiseVariables::Wwise_Sound_Query_GetRTPCValue_Char(mixerToIncrease.c_str(), 0xffffffff, &volume, &type); // Fill Volume Variable With Current Volume

	if (volume <= (100.0f - amountToIncrease))
		volume += amountToIncrease;
	else
		volume = 100.0f; // Incase the volume is within the amountToIncrease we can't throw it over 100.
	
	// Set Volume
	WwiseVariables::Wwise_Sound_SetRTPCValue_Char(mixerToIncrease.c_str(), (float)volume, 0xffffffff, 0, AkCurveInterpolation_Linear);
	WwiseVariables::Wwise_Sound_SetRTPCValue_Char(mixerToIncrease.c_str(), (float)volume, 0x00001234, 0, AkCurveInterpolation_Linear);

	std::cout << "Increase volume of " << mixerToIncrease << " by " << amountToIncrease << " with a new volume of " << volume << std::endl;
}

/// <summary>
/// Decrease Volume of Mixer's Backend
/// </summary>
/// <param name="amountToDecrease"> - How much show we decrease by?</param>
/// <param name="mixerToDecrease"> - Name of Mixer Value</param>
void VolumeControl::DecreaseVolume(int amountToDecrease, std::string mixerToDecrease) {
	float volume = 0;
	RTPCValue_type type = RTPCValue_GameObject;

	if (!MemHelpers::IsInStringArray(mixerToDecrease, mixerNames)) {
		std::cout << "That mixer doesn't exist" << std::endl;
		return;
	}

	WwiseVariables::Wwise_Sound_Query_GetRTPCValue_Char(mixerToDecrease.c_str(), 0xffffffff, &volume, &type); // Fill Volume Variable With Current Volume

	if (volume >= (0.0f + amountToDecrease))
		volume -= amountToDecrease;
	else
		volume = 0.0f; // Incase the volume is within the amountToDecrease we can't throw it below 0.

	// Set Volume
	WwiseVariables::Wwise_Sound_SetRTPCValue_Char(mixerToDecrease.c_str(), (float)volume, 0xffffffff, 0, AkCurveInterpolation_Linear);
	WwiseVariables::Wwise_Sound_SetRTPCValue_Char(mixerToDecrease.c_str(), (float)volume, 0x00001234, 0, AkCurveInterpolation_Linear);
	
	std::cout << "Decrease volume of " << mixerToDecrease << " by " << amountToDecrease << " with a new volume of " << volume << std::endl;
}

/// <summary>
/// Disables the song previews when hovering over a song.
/// </summary>
void VolumeControl::DisableSongPreviewAudio() {
	if (!disabledSongPreviewAudio) {
		MemUtil::PatchAdr((void*)Offsets::patch_SongPreviewWwiseEvent, "Play_%s_Invalid", 16);
		disabledSongPreviewAudio = true;
	}
	else {
		std::cout << "Tried to disable song previews when they are already disabled!" << std::endl;
	}
}

/// <summary>
/// Enables the song previews when hovering over a song.
/// </summary>
void VolumeControl::EnableSongPreviewAudio() {
	if (disabledSongPreviewAudio) {
		MemUtil::PatchAdr((void*)Offsets::patch_SongPreviewWwiseEvent, "Play_%s_Preview", 16);
		disabledSongPreviewAudio = false;
	}
	else {
		std::cout << "Tried to enable song previews when they are already enabled!" << std::endl;
	}
	
}

/// <summary>
/// Setup active microphones map with all the available microphones and their respective IDs.
/// </summary>
void VolumeControl::SetupMicrophones() {
	// Initialization
	activeMicrophones.clear();

	HRESULT coInit = CoInitialize(NULL);
	if (coInit != S_OK) {
		std::cout << "Unable to get microphone volume. CoInitialize is not S_OK. Returned ";
		switch (coInit) {
		case (S_FALSE):
			std::cout << "S_FALSE" << std::endl;
			break;
		case (RPC_E_CHANGED_MODE):
			std::cout << "RPC_E_CHANGED_MODE" << std::endl;
			break;
		default: // Non-documented error.
			std::cout << "NOT DOCUMENTED!" << std::endl;
			break;
		}
		return;
	}

	IMMDeviceEnumerator* deviceEnumerator = NULL;
	HRESULT coCI = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);

	if (coCI != S_OK) {
		std::cout << "Unable to get microphone volume. CoCreateInstance is not S_OK. Returned ";
		switch (coCI) {
		case (REGDB_E_CLASSNOTREG):
			std::cout << "REGDB_E_CLASSNOTREG" << std::endl;
			break;
		case (CLASS_E_NOAGGREGATION):
			std::cout << "CLASS_E_NOAGGREGATION" << std::endl;
			break;
		case (E_NOINTERFACE):
			std::cout << "E_NOINTERFACE" << std::endl;
			break;
		case (E_POINTER):
			std::cout << "E_POINTER" << std::endl;
			break;
		default: // Non-documented error.
			std::cout << "NOT DOCUMENTED!" << std::endl;
			break;
		}

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
void VolumeControl::SetMicrophoneVolume(std::string microphoneName, int volume) {
	if (activeMicrophones.find(microphoneName) != activeMicrophones.end()) {

		// Initialization
		LPWSTR microphoneId = activeMicrophones[microphoneName];
		IMMDevice* microphone = NULL;

		HRESULT coInit = CoInitialize(NULL);
		if (coInit != S_OK) {
			std::cout << "Unable to get microphone volume. CoInitialize is not S_OK. Returned ";
			switch (coInit) {
			case (S_FALSE):
				std::cout << "S_FALSE" << std::endl;
				break;
			case (RPC_E_CHANGED_MODE):
				std::cout << "RPC_E_CHANGED_MODE" << std::endl;
				break;
			default: // Non-documented error.
				std::cout << "NOT DOCUMENTED!" << std::endl;
				break;
			}
			return;
		}

		IMMDeviceEnumerator* deviceEnumerator = NULL;
		HRESULT coCI = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);

		if (coCI != S_OK) {
			std::cout << "Unable to get microphone volume. CoCreateInstance is not S_OK. Returned ";
			switch (coCI) {
			case (REGDB_E_CLASSNOTREG):
				std::cout << "REGDB_E_CLASSNOTREG" << std::endl;
				break;
			case (CLASS_E_NOAGGREGATION):
				std::cout << "CLASS_E_NOAGGREGATION" << std::endl;
				break;
			case (E_NOINTERFACE):
				std::cout << "E_NOINTERFACE" << std::endl;
				break;
			case (E_POINTER):
				std::cout << "E_POINTER" << std::endl;
				break;
			default: // Non-documented error.
				std::cout << "NOT DOCUMENTED!" << std::endl;
				break;
			}

			return;
		}

		// Get the microphone we want
		deviceEnumerator->GetDevice(microphoneId, &microphone);
		deviceEnumerator->Release();
		deviceEnumerator = NULL;

		// Get the endpoint volume
		IAudioEndpointVolume* microphoneVolume = NULL;
		microphone->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&microphoneVolume);

		// Set the volume
		if (volume > 100)
			volume %= 100;

		microphoneVolume->SetMasterVolumeLevelScalar((float)volume / 100, NULL);

		std::cout << "Set volume of microphone: \"" << microphoneName << "\" to " << volume << std::endl;

		// Clean-up
		microphone->Release();
		microphoneVolume->Release();
	}
	else
	{
		std::cout << "Unable to find microphone (Set volume): \"" << microphoneName << "\"" << std::endl;
	}
}

/// <summary>
/// Gets the current volume of the selected microphone
/// </summary>
/// <param name="microphone"> - Name of the microphone</param>
/// <returns>Int 0-100 of the current volume of the microphone</returns>
int VolumeControl::GetMicrophoneVolume(std::string microphoneName) {
	if (activeMicrophones.find(microphoneName) != activeMicrophones.end()) {
		// Initialization
		LPWSTR microphoneId = activeMicrophones[microphoneName];
		IMMDevice* microphone = NULL;

		HRESULT coInit = CoInitialize(NULL);
		if (coInit != S_OK) {
			std::cout << "Unable to get microphone volume. CoInitialize is not S_OK. Returned ";
			switch (coInit) {
				case (S_FALSE):
					std::cout << "S_FALSE" << std::endl;
					break;
				case (RPC_E_CHANGED_MODE):
					std::cout << "RPC_E_CHANGED_MODE" << std::endl;
					break;
				default: // Non-documented error.
					std::cout << "NOT DOCUMENTED!" << std::endl;
					break;
			}
			return;
		}

		IMMDeviceEnumerator* deviceEnumerator = NULL;
		HRESULT coCI = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);

		if (coCI != S_OK) {
			std::cout << "Unable to get microphone volume. CoCreateInstance is not S_OK. Returned ";
			switch (coCI) {
				case (REGDB_E_CLASSNOTREG):
					std::cout << "REGDB_E_CLASSNOTREG" << std::endl;
					break;
				case (CLASS_E_NOAGGREGATION):
					std::cout << "CLASS_E_NOAGGREGATION" << std::endl;
					break;
				case (E_NOINTERFACE):
					std::cout << "E_NOINTERFACE" << std::endl;
					break;
				case (E_POINTER):
					std::cout << "E_POINTER" << std::endl;
					break;
				default: // Non-documented error.
					std::cout << "NOT DOCUMENTED!" << std::endl;
					break;
			}
			
			return;
		}

		// Get the microphone we want
		deviceEnumerator->GetDevice(microphoneId, &microphone);
		deviceEnumerator->Release();
		deviceEnumerator = NULL;

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
		std::cout << "Unable to find microphone (Get volume): \"" << microphoneName << "\"" << std::endl;
	}
}