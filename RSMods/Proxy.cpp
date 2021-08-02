// DON'T TOUCH THIS FILE!!!!!! This is what allows us to get our code running.

#include "windows.h"
#include "Proxy.hpp"
#pragma pack(1)

FARPROC proxy[13] = { 0 };
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

	proxy[0] = GetProcAddress(originalDLL, "DllMain");
	proxy[1] = GetProcAddress(originalDLL, "entry");
	proxy[2] = GetProcAddress(originalDLL, "Ordinal_100");
	proxy[3] = GetProcAddress(originalDLL, "Ordinal_101");
	proxy[4] = GetProcAddress(originalDLL, "Ordinal_102");
	proxy[5] = GetProcAddress(originalDLL, "Ordinal_103");
	proxy[6] = GetProcAddress(originalDLL, "XInputEnable");
	proxy[7] = GetProcAddress(originalDLL, "XInputGetBatteryInformation");
	proxy[8] = GetProcAddress(originalDLL, "XInputGetCapabilities");
	proxy[9] = GetProcAddress(originalDLL, "XInputGetDSoundAudioDeviceGuids");
	proxy[10] = GetProcAddress(originalDLL, "XInputGetKeystroke");
	proxy[11] = GetProcAddress(originalDLL, "XInputGetState");
	proxy[12] = GetProcAddress(originalDLL, "XInputSetState");

	return true;
}

// DllMain
extern "C" __declspec(naked) void __cdecl XInput_DllMain() {
	__asm
	{
		jmp proxy[0 * 4];
	}
}

// entry
extern "C" __declspec(naked) void __cdecl XInput_entry() {
	__asm
	{
		jmp proxy[1 * 4];
	}
}

// Ordinal_100
extern "C" __declspec(naked) void __cdecl XInput_Ordinal_100() {
	__asm
	{
		jmp proxy[2 * 4];
	}
}

// Ordinal_101
extern "C" __declspec(naked) void __cdecl XInput_Ordinal_101() {
	__asm
	{
		jmp proxy[3 * 4];
	}
}

// Ordinal_102
extern "C" __declspec(naked) void __cdecl XInput_Ordinal_102() {
	__asm
	{
		jmp proxy[4 * 4];
	}
}

// Ordinal_103
extern "C" __declspec(naked) void __cdecl XInput_Ordinal_103() {
	__asm
	{
		jmp proxy[5 * 4];
	}
}

// XInputEnable
extern "C" __declspec(naked) void __cdecl XInput_XInputEnable() {
	__asm
	{
		jmp proxy[6 * 4];
	}
}

// XInputGetBatteryInformation
extern "C" __declspec(naked) void __cdecl XInput_XInputGetBatteryInformation() {
	__asm
	{
		jmp proxy[7 * 4];
	}
}

// XInputGetCapabilities
extern "C" __declspec(naked) void __cdecl XInput_XInputGetCapabilities() {
	__asm
	{
		jmp proxy[8 * 4];
	}
}

// XInputGetDSoundAudioDeviceGuids
extern "C" __declspec(naked) void __cdecl XInput_XInputGetDSoundAudioDeviceGuids() {
	__asm
	{
		jmp proxy[9 * 4];
	}
}

// XInputGetKeystroke
extern "C" __declspec(naked) void __cdecl XInput_XInputGetKeystroke() {
	__asm
	{
		jmp proxy[10 * 4];
	}
}

// XInputGetState
extern "C" __declspec(naked) void __cdecl XInput_XInputGetState() {
	__asm
	{
		jmp proxy[11 * 4];
	}
}

// XInputSetState
extern "C" __declspec(naked) void __cdecl XInput_XInputSetState() {
	__asm
	{
		jmp proxy[12 * 4];
	}
}

