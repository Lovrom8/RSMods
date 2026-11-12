#ifdef _WIN32
#include "../../../stdafx.h"
#include <cassert>
#include "../../../SettingEdits.hpp"

namespace SettingEdits {
	unsigned int writes = 0;
	std::string saved;
	void Apply(Edit edit) { ++writes; saved = edit.value; }
}

// Versioned overloads are linked but unused by this memory-only fixture.
namespace Offsets { uintptr_t baseHandle = 0; }
template <> uintptr_t& VersioningStruct<uintptr_t>::Get() {
	static uintptr_t unused = 0;
	return unused;
}
template <> uintptr_t VersioningStruct<uintptr_t>::GetValue() { return Get(); }

namespace {
	const char* g_wineExport = "wine_get_build_id";
	const char* g_hostName = nullptr;
	const char* g_override = "auto";
	const char* g_savedHost = "";
	void __cdecl TestHostVersion(const char** name, const char** release) {
		*name = g_hostName;
		*release = "test";
	}
	struct TestIni {
		void LoadFile(const char*) {}
		const char* GetValue(const char*, const char* key, const char* fallback) {
			(void)fallback;
			return strcmp(key, "HostOverride") == 0 ? g_override : g_savedHost;
		}
	};
	HKEY g_wineRoot = nullptr;
	unsigned int g_protectCalls = 0;
	unsigned int g_failCall = 0;
	bool g_slowCall = false;
	bool g_failQuery = false;
	int __cdecl Replacement() { return 7; }

	FARPROC WINAPI TestGetProcAddress(HMODULE module, LPCSTR name) {
		if (strcmp(name, "wine_get_host_version") == 0 && strcmp(name, g_wineExport) == 0)
			return reinterpret_cast<FARPROC>(&TestHostVersion);
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
#define CSimpleIniA TestIni
#include "../../../MemUtil.cpp"
#undef CSimpleIniA
#undef VirtualQuery
#undef VirtualProtect
#undef RegCloseKey
#undef RegOpenKeyExW
#undef GetProcAddress

int main(int argc, char** argv) {
	if (argc > 1) {
		g_wineExport = argv[1];
		if (argc > 2) g_override = argv[2];
		if (argc > 3) g_hostName = argv[3];
		if (strcmp(argv[1], "HKCU") == 0) g_wineRoot = HKEY_CURRENT_USER;
		if (strcmp(argv[1], "HKLM") == 0) g_wineRoot = HKEY_LOCAL_MACHINE;
		const char* expectedHost = strcmp(argv[1], "native") == 0 ? "Windows" : "Wine";
		if (g_hostName && strcmp(g_hostName, "Linux") == 0) expectedHost = "Wine-Linux";
		if (g_hostName && strcmp(g_hostName, "Darwin") == 0) expectedHost = "Wine-macOS";
		if (argc > 4) g_savedHost = expectedHost;
		assert(MemUtil::IsRunningOnWine() == (strcmp(argv[1], "native") != 0));
		assert(strcmp(MemUtil::DetectedHost(), expectedHost) == 0);
		const bool expectedWineApis = strcmp(g_override, "wine") == 0
			|| strcmp(expectedHost, "Windows") != 0;
		assert(MemoryHostPolicy().useWineApis == expectedWineApis);
		g_override = "changed";
		g_hostName = "changed";
		assert(MemoryHostPolicy().useWineApis == expectedWineApis);
		assert(strcmp(MemUtil::DetectedHost(), expectedHost) == 0);
		MemUtil::LogMemoryProtectionPolicy();
		assert(SettingEdits::writes == (argc > 4 ? 0u : 1u));
		if (argc <= 4) assert(SettingEdits::saved == expectedHost);
		MemUtil::LogMemoryProtectionPolicy();
		assert(SettingEdits::writes == (argc > 4 ? 0u : 1u));
		if (!expectedWineApis) {
			void* memory = VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			assert(memory);
			DWORD old = 0;
			assert(NT_SUCCESS(MemUtil::HookedVirtualProtect(memory, 4096, PAGE_READONLY, old)));
			// Windows goes to the kernel first; the documented call is only the fallback.
			assert(old == PAGE_READWRITE && g_protectCalls == 0);
			assert(NT_SUCCESS(MemUtil::HookedVirtualProtect(memory, 4096, PAGE_READWRITE, old)));
			assert(old == PAGE_READONLY && g_protectCalls == 0);
			g_failQuery = true;
			MEMORY_BASIC_INFORMATION info{};
			assert(NT_SUCCESS(MemUtil::HookedQueryVirtualMemory(memory, &info, sizeof(info))));
			assert(info.Protect == PAGE_READWRITE);
			assert(VirtualFree(memory, 0, MEM_RELEASE));
			puts("Windows syscall first PASS");
		}
		if (expectedWineApis) {
			void* memory = VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			assert(memory);
			DWORD old = 0;
			g_failCall = g_protectCalls + 1;
			assert(!NT_SUCCESS(MemUtil::HookedVirtualProtect(memory, 1, PAGE_READONLY, old)));
			g_failQuery = true;
			MEMORY_BASIC_INFORMATION info{};
			assert(!NT_SUCCESS(MemUtil::HookedQueryVirtualMemory(memory, &info, sizeof(info))));
			assert(VirtualFree(memory, 0, MEM_RELEASE));
		}
		if (expectedWineApis) {
			g_failQuery = true;
			MEMORY_BASIC_INFORMATION info{};
			assert(!NT_SUCCESS(MemUtil::HookedQueryVirtualMemory(nullptr, &info, sizeof(info))));
		}
		puts("Detection PASS");
		return 0;
	}
	assert(MemUtil::IsRunningOnWine());
	MemUtil::LogMemoryProtectionPolicy();
	auto* page = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	assert(page);
	page[0] = 1;
	page[1] = 2;
	const unsigned char first = 3, second = 4;

	MemUtil::BeginBootPatchBundle();
	assert(MemUtil::BootPatchMember("first"));
	assert(MemUtil::PatchAdr(page, &first, 1));
	assert(page[0] == 1);
	MemUtil::FailBootPatchBundle();
	assert(!MemUtil::BootPatchMember("must not run"));
	assert(!MemUtil::CommitBootPatchBundle());
	assert(page[0] == 1 && g_protectCalls == 0);

	MemUtil::BeginBootPatchBundle();
	assert(MemUtil::BootPatchMember("overlap"));
	assert(MemUtil::PatchAdr(page, &first, 1));
	assert(!MemUtil::PatchAdr(page, &second, 1));
	assert(!MemUtil::CommitBootPatchBundle());
	assert(page[0] == 1 && g_protectCalls == 0);

	// Fail each acquire/restore in preflight and commit, including a post-write failure.
	for (unsigned int failure = 1; failure <= 8; ++failure) {
		g_protectCalls = 0;
		g_failCall = failure;
		MemUtil::BeginBootPatchBundle();
		assert(MemUtil::BootPatchMember("first"));
		assert(MemUtil::PatchAdr(page, &first, 1));
		assert(MemUtil::BootPatchMember("second"));
		assert(MemUtil::PatchAdr(page + 1, &second, 1));
		assert(!MemUtil::CommitBootPatchBundle());
		assert(page[0] == 1 && page[1] == 2);
		assert(!MemUtil::BootPatchBundleEnabled());
		MEMORY_BASIC_INFORMATION protection{};
		assert(VirtualQuery(page, &protection, sizeof(protection)));
		assert(protection.Protect == PAGE_READWRITE);
	}

	g_protectCalls = 0;
	g_failCall = 0;
	MemUtil::BeginBootPatchBundle();
	assert(MemUtil::BootPatchMember("success"));
	assert(MemUtil::PatchAdr(page, &first, 1));
	g_slowCall = true;
	assert(MemUtil::CommitBootPatchBundle());
	assert(page[0] == first && MemUtil::BootPatchBundleEnabled());

	MemUtil::BeginBootPatchBundle();
	assert(MemUtil::BootPatchMember("changed site"));
	assert(MemUtil::PatchAdr(page, &second, 1));
	page[0] = 9;
	assert(!MemUtil::CommitBootPatchBundle());
	assert(page[0] == 9);

	// Trampolines are executable before publication, while the source stays untouched.
	g_failCall = 0;
	const unsigned char code[] = { 0xB8, 42, 0, 0, 0, 0xC3 };
	memcpy(page + 64, code, sizeof(code));
	DWORD previous = 0;
	assert(VirtualProtect(page, 4096, PAGE_EXECUTE_READWRITE, &previous));
	using Function = int(__cdecl*)();
	const auto original = reinterpret_cast<Function>(page + 64);
	MemUtil::BeginBootPatchBundle();
	assert(MemUtil::BootPatchMember("trampoline"));
	PBYTE gateway = MemUtil::TrampHook(page + 64, reinterpret_cast<PBYTE>(&Replacement), 5);
	assert(gateway && original() == 42);
	assert(reinterpret_cast<Function>(gateway)() == 42);
	assert(MemUtil::CommitBootPatchBundle());
	assert(original() == 7 && reinterpret_cast<Function>(gateway)() == 42);
	assert(MemUtil::PatchAdr(page + 64, code, sizeof(code)));
	assert(VirtualFree(gateway, 0, MEM_RELEASE));

	DWORD old = 0;
	g_failCall = g_protectCalls + 1;
	assert(!NT_SUCCESS(MemUtil::HookedVirtualProtect(page, 1, PAGE_READONLY, old)));
	MEMORY_BASIC_INFORMATION info{};
	assert(NT_SUCCESS(MemUtil::HookedQueryVirtualMemory(page, &info, sizeof(info))));
	assert(!NT_SUCCESS(MemUtil::HookedQueryVirtualMemory(page, nullptr, 0)));
	assert(VirtualFree(page, 0, MEM_RELEASE));
	puts("Boot bundle and documented memory paths ALL PASS");
}

#endif
