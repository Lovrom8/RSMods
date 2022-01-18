#include "Proxy.hpp"

#pragma pack(1)

typedef void(__stdcall* T_XInput_XInputEnable)(BOOL enable);
typedef DWORD(__stdcall* T_XInput_XInputGetBatteryInformation)(DWORD dwUserIndex, BYTE devType, XINPUT_BATTERY_INFORMATION* pBatteryInformation);
typedef DWORD(__stdcall* T_XInput_XInputGetCapabilities)(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* pCapabilities);
typedef DWORD(__stdcall* T_XInput_XInputGetDSoundAudioDeviceGuids)(DWORD dwUserIndex, GUID* pDSoundRenderGuid, GUID* pDSoundCaptureGuid);
typedef DWORD(__stdcall* T_XInput_XInputGetKeystroke)(DWORD dwUserIndex, DWORD dwReserved, XINPUT_KEYSTROKE* pKeystroke);
typedef DWORD(__stdcall* T_XInput_XInputGetState)(DWORD dwUserIndex, XINPUT_STATE* pState);
typedef DWORD(__stdcall* T_XInput_XInputSetState)(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);

FARPROC proxy[7] = { 0 };
HINSTANCE originalDLL;

/// <summary>
/// Shutdown Proxy to original xinput1_3.dll
/// </summary>
void Proxy::Shutdown() {
	FreeLibrary(originalDLL);
}

/// <summary>
/// Setup prooxy to original xinput1_3.dll
/// </summary>
bool Proxy::Init() {
	char winpath[MAX_PATH];

	// DLL has already been loaded, so act like we've already setup the proxy.
	if (originalDLL)
		return true;

	// Get path to "...System32/xinput1_3.dll"
	GetSystemDirectoryA(winpath, sizeof(winpath));
	strcat_s(winpath, "\\xinput1_3.dll");

	// Load the real xinput1_3.dll
	originalDLL = LoadLibraryA(winpath);

	// Verify that we have actually loaded the DLL.
	if (!originalDLL)
		return false;

	// Create an array of the addresses to call when Rocksmith asks us to do XInput calls.
	proxy[0] = GetProcAddress(originalDLL, "XInputEnable");
	proxy[1] = GetProcAddress(originalDLL, "XInputGetBatteryInformation");
	proxy[2] = GetProcAddress(originalDLL, "XInputGetCapabilities");
	proxy[3] = GetProcAddress(originalDLL, "XInputGetDSoundAudioDeviceGuids");
	proxy[4] = GetProcAddress(originalDLL, "XInputGetKeystroke");
	proxy[5] = GetProcAddress(originalDLL, "XInputGetState");
	proxy[6] = GetProcAddress(originalDLL, "XInputSetState");

	return true;
}


extern "C" {
	enum XInput_Proxy {
		Enable,
		GetBatteryInformation,
		GetCapabilities,
		GetDSoundAudioDeviceGuids,
		GetKeystroke,
		GetState,
		SetState
	};


	// Export symbol as XInputEnable, and proxy to the real XInputEnable
	void __stdcall XInput_XInputEnable(BOOL enable) {
		return ((T_XInput_XInputEnable)proxy[XInput_Proxy::Enable])(enable);
	}

	// Export symbol as XInputGetBatteryInformation, and proxy to the real XInputGetBatteryInformation
	DWORD __stdcall XInput_XInputGetBatteryInformation(DWORD dwUserIndex, BYTE devType, XINPUT_BATTERY_INFORMATION* batteryInformation) {
		return ((T_XInput_XInputGetBatteryInformation)proxy[XInput_Proxy::GetBatteryInformation])(dwUserIndex, devType, batteryInformation);
	}

	// Export symbol as XInputGetCapabilities, and proxy to the real XInputGetCapabilities
	DWORD __stdcall XInput_XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES* capabilities) {
		return ((T_XInput_XInputGetCapabilities)proxy[XInput_Proxy::GetCapabilities])(dwUserIndex, dwFlags, capabilities);
	}

	// Export symbol as XInputGetDSoundAudioDeviceGuids, and proxy to the real XInputGetDSoundAudioDeviceGuids
	DWORD __stdcall XInput_XInputGetDSoundAudioDeviceGuids(DWORD dwUserIndex, GUID* pDSoundRenderGuid, GUID* DSoundCaptureGuid) {
		return ((T_XInput_XInputGetDSoundAudioDeviceGuids)proxy[XInput_Proxy::GetDSoundAudioDeviceGuids])(dwUserIndex, pDSoundRenderGuid, DSoundCaptureGuid);
	}

	// Export symbol as XInputGetKeystroke, and proxy to the real XInputGetKeystroke
	DWORD __stdcall XInput_XInputGetKeystroke(DWORD dwUserIndex, DWORD dwReserved, XINPUT_KEYSTROKE* keystroke) {
		return ((T_XInput_XInputGetKeystroke)proxy[XInput_Proxy::GetKeystroke])(dwUserIndex, dwReserved, keystroke);
	}

	// Export symbol as XInputGetState, and proxy to the real XInputGetState
	DWORD __stdcall XInput_XInputGetState(DWORD dwUserIndex, XINPUT_STATE* state) {
		return ((T_XInput_XInputGetState)proxy[XInput_Proxy::GetState])(dwUserIndex, state);
	}

	// Export symbol as XInputSetState, and proxy to the real XInputSetState
	DWORD __stdcall  XInput_XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* vibration) {
		return ((T_XInput_XInputSetState)proxy[XInput_Proxy::SetState])(dwUserIndex, vibration);
	}
}