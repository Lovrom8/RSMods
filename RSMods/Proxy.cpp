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
/// Shutdown Proxy to original DLL
/// </summary>
void Proxy::Shutdown() {
	FreeLibrary(originalDLL);
}

/// <summary>
/// Proxy to original DLL
/// </summary>
bool Proxy::Init() {
	char winpath[MAX_PATH];

	if (originalDLL)
		return true;

	GetSystemDirectoryA(winpath, sizeof(winpath));
	strcat_s(winpath, "\\xinput1_3.dll");

	originalDLL = LoadLibraryA(winpath);

	if (!originalDLL)
		return false;

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
	// XInputEnable
	void __cdecl XInput_XInputEnable(BOOL enable) {
		return ((T_XInput_XInputEnable)proxy[0])(enable);
	}

	// XInputGetBatteryInformation
	DWORD __cdecl XInput_XInputGetBatteryInformation(DWORD dwUserIndex, BYTE devType, XINPUT_BATTERY_INFORMATION batteryInformation) {
		return ((T_XInput_XInputGetBatteryInformation)proxy[1])(dwUserIndex, devType, &batteryInformation);
	}

	// XInputGetCapabilities
	DWORD __cdecl XInput_XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags, XINPUT_CAPABILITIES capabilities) {
		return ((T_XInput_XInputGetCapabilities)proxy[2])(dwUserIndex, dwFlags, &capabilities);
	}

	// XInputGetDSoundAudioDeviceGuids
	DWORD __cdecl XInput_XInputGetDSoundAudioDeviceGuids(DWORD dwUserIndex, GUID pDSoundRenderGuid, GUID DSoundCaptureGuid) {
		return ((T_XInput_XInputGetDSoundAudioDeviceGuids)proxy[3])(dwUserIndex, &pDSoundRenderGuid, &DSoundCaptureGuid);
	}

	// XInputGetKeystroke
	DWORD __cdecl XInput_XInputGetKeystroke(DWORD dwUserIndex, DWORD dwReserved, XINPUT_KEYSTROKE keystroke) {
		return ((T_XInput_XInputGetKeystroke)proxy[4])(dwUserIndex, dwReserved, &keystroke);
	}

	// XInputGetState
	DWORD __cdecl XInput_XInputGetState(DWORD dwUserIndex, XINPUT_STATE state) {
		return ((T_XInput_XInputGetState)proxy[5])(dwUserIndex, &state);
	}

	// XInputSetState
	DWORD __cdecl XInput_XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION vibration) {
		return ((T_XInput_XInputSetState)proxy[6])(dwUserIndex, &vibration);
	}
}
