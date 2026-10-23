#include "../stdafx.h"
#include "Enumeration.hpp"
#include <climits>

void SaveSteamServicePointer(uint32_t eax) {
	if (Enumeration::rsSteamServiceFlagsPtr == nullptr) {
		printf("Found DLC service pointer at: %X\n", eax + 4);
		Enumeration::rsSteamServiceFlagsPtr = (uint32_t*)(eax + 4);
	}
}

uint32_t hookBackAddr;
uint32_t pushedPrologueValue;

void __declspec(naked) Hook_EnumerationService() {
	__asm {
		push ebp
		mov ebp, esp
		push dword ptr[pushedPrologueValue]

		pushad

		mov eax, esi
		push eax
		call SaveSteamServicePointer
		add esp, 4

		popad

		jmp dword ptr[hookBackAddr]
	}
}

void Enumeration::HookEnumerationService() {
	uint32_t BaseTextAddress = MemUtil::GetTextSectionAddress();

	const char* sig = "\x55\x8B\xEC\x6A\x00\x68\x00\x00\x00\x00\x64\xA1\x00\x00\x00\x00\x50\x83\xEC\x00\xA1\x00\x00\x00\x00\x33\xC5\x89\x45\x00\x53\x57\x50\x8D\x45\x00\x64\xA3\x00\x00\x00\x00\x33\xDB\x38\x5E\x00\x0F\x84";
	const char* mask = "xxxx?x????xx????xxx?x????xxxx?xxxxx?xx????xxxx?xx";

	uint32_t hookAddr = MemUtil::FindPattern<uint32_t>(BaseTextAddress, (size_t)MemUtil::GetTextSectionLength(), (uint8_t*)sig, mask);
	short len = 0x5;

	if (hookAddr == NULL) {
		LOG_ERROR("Failed to find Steam enumeration location!");
		return;
	}

	hookBackAddr = hookAddr + len;
	pushedPrologueValue = (uint32_t)(int32_t)*(int8_t*)(hookAddr + 4); // The imm8 of `push imm8`, at offset 4 of the signature.

	if (MemUtil::PlaceHook((void*)hookAddr, Hook_EnumerationService, len)) {
		LOG_INFO("Hooked Steam enumeration function successfully!" << std::endl);
	}
	else {
		LOG_ERROR("Failed to hook function!" << std::endl);
	}
}

/// <summary>
/// Trigger Enumeration
/// </summary>
void Enumeration::ForceEnumeration() {
	// Get memory address for Enumeration flag

	// Normally it would go via a regular pointer, but... this is simpler to find
	// uintptr_t rsSteamServiceFlagsPtr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_enumerateService, Offsets::ptr_enumerateServiceOffsets);

	if (rsSteamServiceFlagsPtr)
	{
		// Set Enumeration flags to 1.
		*(BYTE*)rsSteamServiceFlagsPtr = 1;
		*(BYTE*)(rsSteamServiceFlagsPtr + 1) = 1;
	}
}

/// <summary>
/// Get amount of files in a path. Recursive.
/// </summary>
/// <param name="path"> - Path</param>
/// <returns>Amount of files</returns>
int Enumeration::GetFileCount(std::filesystem::path path) {
	int count = 0;
	try {
		std::error_code error;
		const auto options = std::filesystem::directory_options::skip_permission_denied;
		std::filesystem::recursive_directory_iterator it(path, options, error), end;
		for (; !error && it != end; it.increment(error)) {
			if (count < INT_MAX)
				++count;
		}
	}
	catch (...) {
		LOG_ERROR("(ENUMERATION) Failed to count DLC files; using the count collected so far" << std::endl);
	}
	return count;
}

/// <summary>
/// Get amount of files in DLC folder.
/// </summary>
/// <returns>Amount of files in the Rocksmith2014/dlc folder. Recursive.</returns>
int Enumeration::GetCurrentDLCCount() {
	try {
		std::error_code error;
		std::filesystem::path dlcFolderPath = std::filesystem::current_path(error);
		if (error)
			return 0;
		dlcFolderPath /= "dlc";

		if (!std::filesystem::exists(dlcFolderPath, error) || error)
			return 0;

		return GetFileCount(dlcFolderPath);
	}
	catch (...) {
		LOG_ERROR("(ENUMERATION) Failed to resolve the DLC folder" << std::endl);
		return 0;
	}
}


