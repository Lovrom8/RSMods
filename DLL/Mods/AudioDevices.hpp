#pragma once

#include "Windows.h"
#include <string>
#include <map>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <Functiondiscoverykeys_devpkey.h>

#include "../Log.hpp"
#include "../MemUtil.hpp"
#include "../Offsets.hpp"


namespace AudioDevices {
	void SetupMicrophones();
	void SetMicrophoneVolume(std::string microphoneName, int volume);
	int GetMicrophoneVolume(std::string microphoneName);
	void ChangeOutputSampleRate();

	inline std::map<std::string, LPWSTR> activeMicrophones;

	inline int output_SampleRate = 48000;
};