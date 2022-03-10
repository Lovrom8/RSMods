#include "DisableControllers.hpp"

LPVOID* DirectInputPointer = NULL;

/// <summary>
/// Disable XInput controlers (Xbox Controllers)
/// </summary>
void XInput() {
	// 0x01c5970e function to remove XInput1_3 devices.
	*(HMODULE*)0x0135de18 = 0x0; // XInput Library (Tell the game XInput doesn't need to be loaded anymore)
	*(FARPROC*)0x0135de28 = 0x0; // XInput Enable (to false)
}

/// <summary>
/// Hook DirectInput controllers. Currently not working
/// </summary>
void _declspec(naked) hook_DirectInput() {
	__asm {
		mov DirectInputPointer, esi
		push 0x800
		jmp[Offsets::hookBackAddr_DirectInput8]
	}
}

/// <summary>
/// Disable DirectInput controllers. Currently not working
/// </summary>
void DirectInput() {
	MemUtil::PlaceHook((void*)Offsets::hookAddr_DirectInput8, hook_DirectInput, 5);
}

/// <summary>
/// Public function to turn off XInput and DirectInput Controllers.
/// </summary>
void DisableControllers::DisableControllers() {
	_LOG_INIT;

	_LOG_HEAD << "Disabling Controllers..." << LOG.endl();

	XInput();
	DirectInput();

	_LOG_HEAD << "DI8-Pointer: " << DirectInputPointer << LOG.endl();
}