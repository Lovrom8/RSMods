#include "../stdafx.h"
#include "DisableControllers.hpp"

LPVOID* DirectInputPointer = NULL;

/// <summary>
/// Disable XInput controlers (Xbox Controllers)
/// </summary>
void XInput() {
	MemUtil::SetStaticValue(Offsets::xinputModule.Get(), 0, sizeof(int));
	MemUtil::SetStaticValue(Offsets::xinputEnable.Get(), 0, sizeof(int));
}

/// <summary>
/// Hook DirectInput controllers. Currently not working
/// </summary>
void _declspec(naked) hook_DirectInput() {
	__asm {
		mov DirectInputPointer, esi
		push 0x800

		push offset Offsets::hookBackAddr_DirectInput8
		jmp MemUtil::JumpToVersioned
	}
}

/// <summary>
/// Disable DirectInput controllers. Currently not working
/// </summary>
void DirectInput() {
	MemUtil::PlaceHook(Offsets::hookAddr_DirectInput8, hook_DirectInput, 5);
}

/// <summary>
/// Public function to turn off XInput and DirectInput Controllers.
/// </summary>
void DisableControllers::DisableControllers() {
	LOG_INFO("Disabling Controllers..." << std::endl);

	XInput();
	DirectInput();

	LOG_INFO("DI8-Pointer: " << DirectInputPointer << std::endl);
}