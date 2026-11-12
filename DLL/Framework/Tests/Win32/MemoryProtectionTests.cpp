#include "../../../stdafx.h"
#include <cassert>

// Versioned overloads are linked but unused by this memory-only fixture.
namespace Offsets { uintptr_t baseHandle = 0; }
template <> uintptr_t& VersioningStruct<uintptr_t>::Get() {
	static uintptr_t unused = 0;
	return unused;
}
template <> uintptr_t VersioningStruct<uintptr_t>::GetValue() { return Get(); }

namespace {
	const char* g_wineExport = "wine_get_build_id";
	HKEY g_wineRoot = nullptr;
	unsigned int g_protectCalls = 0;
	unsigned int g_failCall = 0;
	bool g_slowCall = false;
	bool g_failQuery = false;

	FARPROC WINAPI TestGetProcAddress(HMODULE module, LPCSTR name) {
		if (strncmp(name, "wine_", 5) == 0)
			return strcmp(name, g_wineExport) == 0 ? reinterpret_cast<FARPROC>(1) : nullptr;
		return GetProcAddress(module, name);
	}
	LSTATUS WINAPI TestRegOpenKeyExW(HKEY root, LPCWSTR, DWORD, REGSAM, PHKEY key) {
		if (root != g_wineRoot)
			return ERROR_FILE_NOT_FOUND;
		*key = root;
		return ERROR_SUCCESS;
	}
	LSTATUS WINAPI TestRegCloseKey(HKEY) { return ERROR_SUCCESS; }
	SIZE_T WINAPI TestVirtualQuery(LPCVOID address, PMEMORY_BASIC_INFORMATION info, SIZE_T size) {
		if (g_failQuery) {
			SetLastError(ERROR_ACCESS_DENIED);
			return 0;
		}
		return VirtualQuery(address, info, size);
	}
	BOOL WINAPI TestVirtualProtect(LPVOID address, SIZE_T size, DWORD protection, PDWORD old) {
		++g_protectCalls;
		if (g_slowCall) {
			g_slowCall = false;
			Sleep(60);
		}
		if (g_protectCalls == g_failCall) {
			SetLastError(ERROR_ACCESS_DENIED);
			return FALSE;
		}
		return VirtualProtect(address, size, protection, old);
	}
}

#define GetProcAddress TestGetProcAddress
#define RegOpenKeyExW TestRegOpenKeyExW
#define RegCloseKey TestRegCloseKey
#define VirtualProtect TestVirtualProtect
#define VirtualQuery TestVirtualQuery
#include "../../../MemUtil.cpp"
#undef VirtualQuery
#undef VirtualProtect
#undef RegCloseKey
#undef RegOpenKeyExW
#undef GetProcAddress

int main(int argc, char** argv) {
	if (argc > 1) {
		g_wineExport = argv[1];
		if (strcmp(argv[1], "HKCU") == 0) g_wineRoot = HKEY_CURRENT_USER;
		if (strcmp(argv[1], "HKLM") == 0) g_wineRoot = HKEY_LOCAL_MACHINE;
		assert(MemUtil::IsRunningOnWine() == (strcmp(argv[1], "native") != 0));
		if (strcmp(argv[1], "native") == 0) {
			void* memory = VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			assert(memory);
			DWORD old = 0;
			assert(NT_SUCCESS(MemUtil::HookedVirtualProtect(memory, 4096, PAGE_READONLY, old)));
			assert(old == PAGE_READWRITE && g_protectCalls == 1);
			g_failCall = 2;
			assert(NT_SUCCESS(MemUtil::HookedVirtualProtect(memory, 4096, PAGE_READWRITE, old)));
			assert(old == PAGE_READONLY);
			g_failQuery = true;
			MEMORY_BASIC_INFORMATION info{};
			assert(NT_SUCCESS(MemUtil::HookedQueryVirtualMemory(memory, &info, sizeof(info))));
			assert(info.Protect == PAGE_READWRITE);
			assert(VirtualFree(memory, 0, MEM_RELEASE));
			puts("Windows syscall fallback PASS");
		}
		puts("Detection PASS");
		return 0;
	}
	assert(MemUtil::IsRunningOnWine());
	auto* page = VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	assert(page);
	DWORD old = 0;
	g_slowCall = true;
	assert(NT_SUCCESS(MemUtil::HookedVirtualProtect(page, 1, PAGE_READONLY, old)));
	assert(old == PAGE_READWRITE && g_protectCalls == 1);
	g_failCall = 2;
	assert(!NT_SUCCESS(MemUtil::HookedVirtualProtect(page, 1, PAGE_READWRITE, old)));
	g_failQuery = true;
	MEMORY_BASIC_INFORMATION info{};
	assert(!NT_SUCCESS(MemUtil::HookedQueryVirtualMemory(page, &info, sizeof(info))));
	assert(VirtualFree(page, 0, MEM_RELEASE));
	puts("Documented memory paths ALL PASS");
}
