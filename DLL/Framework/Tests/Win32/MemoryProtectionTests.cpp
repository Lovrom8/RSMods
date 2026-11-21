#ifdef _WIN32
#include "../../../stdafx.h"
#include <cassert>
#include <new>
#include <stdexcept>
#include "../../../SettingEdits.hpp"

namespace {
	std::atomic<bool> g_countingAllocations{ false };
	std::atomic<unsigned int> g_allocationCount{ 0 };
}

void* operator new(std::size_t size) {
	if (g_countingAllocations.load(std::memory_order_relaxed))
		g_allocationCount.fetch_add(1, std::memory_order_relaxed);
	if (void* allocation = std::malloc(size))
		return allocation;
	throw std::bad_alloc();
}

void* operator new[](std::size_t size) {
	if (g_countingAllocations.load(std::memory_order_relaxed))
		g_allocationCount.fetch_add(1, std::memory_order_relaxed);
	if (void* allocation = std::malloc(size))
		return allocation;
	throw std::bad_alloc();
}

void operator delete(void* allocation) noexcept { std::free(allocation); }
void operator delete[](void* allocation) noexcept { std::free(allocation); }
void operator delete(void* allocation, std::size_t) noexcept { std::free(allocation); }
void operator delete[](void* allocation, std::size_t) noexcept { std::free(allocation); }

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
	bool g_unresolveSyscalls = false;
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
	unsigned int g_openCalls = 0;
	unsigned int g_restoreCalls = 0;
	unsigned int g_queryCalls = 0;
	unsigned int g_failCall = 0;
	unsigned int g_failFrom = 0;
	bool g_failFlush = false;
	void* g_failFlushAddress = nullptr;
	unsigned int g_flushCalls = 0;
	unsigned int g_failFlushCall = 0;
	unsigned int g_failFlushFrom = 0;
	void* g_armRestoreFailureAddress = nullptr;
	void* g_failRestoreAddress = nullptr;
	bool g_slowCall = false;
	bool g_failQuery = false;
	int __cdecl Replacement() { return 7; }
	int __cdecl TestCallOriginal() { return 42; }
	std::vector<BYTE> MakeTestPe32() {
		std::vector<BYTE> image(0x400);
		IMAGE_DOS_HEADER dos{};
		dos.e_magic = IMAGE_DOS_SIGNATURE;
		dos.e_lfanew = 0x80;
		memcpy(image.data(), &dos, sizeof(dos));
		const DWORD signature = IMAGE_NT_SIGNATURE;
		memcpy(image.data() + dos.e_lfanew, &signature, sizeof(signature));
		IMAGE_FILE_HEADER fileHeader{};
		fileHeader.Machine = IMAGE_FILE_MACHINE_I386;
		fileHeader.NumberOfSections = 1;
		fileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32);
		memcpy(image.data() + dos.e_lfanew + sizeof(signature), &fileHeader, sizeof(fileHeader));
		IMAGE_OPTIONAL_HEADER32 optional{};
		optional.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
		optional.SizeOfImage = 0x2000;
		optional.SizeOfHeaders = 0x200;
		optional.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
		optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] = { 0x1000, sizeof(IMAGE_EXPORT_DIRECTORY) };
		const size_t optionalOffset = dos.e_lfanew + sizeof(signature) + sizeof(fileHeader);
		memcpy(image.data() + optionalOffset, &optional, sizeof(optional));
		IMAGE_SECTION_HEADER section{};
		section.Misc.VirtualSize = 0x200;
		section.VirtualAddress = 0x1000;
		section.SizeOfRawData = 0x200;
		section.PointerToRawData = 0x200;
		const size_t sectionOffset = optionalOffset + sizeof(optional);
		memcpy(image.data() + sectionOffset, &section, sizeof(section));
		IMAGE_EXPORT_DIRECTORY exports{};
		exports.NumberOfFunctions = 1;
		exports.NumberOfNames = 1;
		exports.AddressOfNames = 0x1028;
		exports.AddressOfNameOrdinals = 0x102C;
		exports.AddressOfFunctions = 0x102E;
		memcpy(image.data() + 0x200, &exports, sizeof(exports));
		const DWORD nameRva = 0x1040;
		const WORD ordinal = 0;
		const DWORD functionRva = 0x1038;
		memcpy(image.data() + 0x228, &nameRva, sizeof(nameRva));
		memcpy(image.data() + 0x22C, &ordinal, sizeof(ordinal));
		memcpy(image.data() + 0x22E, &functionRva, sizeof(functionRva));
		image[0x238] = 0xB8;
		const uint32_t number = 0x12345678;
		memcpy(image.data() + 0x239, &number, sizeof(number));
		memcpy(image.data() + 0x240, "NtProtectVirtualMemory", sizeof("NtProtectVirtualMemory"));
		return image;
	}

	FARPROC WINAPI TestGetProcAddress(HMODULE module, LPCSTR name) {
		if (g_unresolveSyscalls
			&& (strcmp(name, "NtProtectVirtualMemory") == 0 || strcmp(name, "NtQueryVirtualMemory") == 0))
			return nullptr;
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
	UINT WINAPI TestGetSystemDirectoryA(LPSTR buffer, UINT size) {
		if (g_unresolveSyscalls) {
			SetLastError(ERROR_PATH_NOT_FOUND);
			return 0;
		}
		return GetSystemDirectoryA(buffer, size);
	}
	SIZE_T WINAPI TestVirtualQuery(LPCVOID address, PMEMORY_BASIC_INFORMATION info, SIZE_T size) {
		++g_queryCalls;
		if (g_failQuery) {
			SetLastError(ERROR_ACCESS_DENIED);
			return 0;
		}
		return VirtualQuery(address, info, size);
	}
	BOOL WINAPI TestVirtualProtect(LPVOID address, SIZE_T size, DWORD protection, PDWORD old) {
		++g_protectCalls;
		if (protection == PAGE_EXECUTE_READWRITE)
			++g_openCalls;
		else
			++g_restoreCalls;
		if (g_slowCall) {
			g_slowCall = false;
			Sleep(60);
		}
		if ((address == g_failRestoreAddress && protection == PAGE_READWRITE)
			|| g_protectCalls == g_failCall || (g_failFrom && g_protectCalls >= g_failFrom)) {
			SetLastError(ERROR_ACCESS_DENIED);
			return FALSE;
		}
		return VirtualProtect(address, size, protection, old);
	}
	BOOL WINAPI TestFlushInstructionCache(HANDLE process, LPCVOID address, SIZE_T size) {
		++g_flushCalls;
		if ((g_failFlush && (!g_failFlushAddress || address == g_failFlushAddress))
			|| g_flushCalls == g_failFlushCall || (g_failFlushFrom && g_flushCalls >= g_failFlushFrom))
			return FALSE;
		if (address == g_armRestoreFailureAddress) {
			g_failRestoreAddress = g_armRestoreFailureAddress;
			g_armRestoreFailureAddress = nullptr;
		}
		return FlushInstructionCache(process, address, size);
	}
	DWORD ProtectionAt(const void* address) {
		MEMORY_BASIC_INFORMATION info{};
		assert(VirtualQuery(address, &info, sizeof(info)));
		return info.Protect;
	}
}

#define GetProcAddress TestGetProcAddress
#define GetSystemDirectoryA TestGetSystemDirectoryA
#define RegOpenKeyExW TestRegOpenKeyExW
#define RegCloseKey TestRegCloseKey
#define VirtualProtect TestVirtualProtect
#define VirtualQuery TestVirtualQuery
#define FlushInstructionCache TestFlushInstructionCache
#define CSimpleIniA TestIni
#include "../../../MemUtil.cpp"
#undef CSimpleIniA
#undef FlushInstructionCache
#undef VirtualQuery
#undef VirtualProtect
#undef RegCloseKey
#undef RegOpenKeyExW
#undef GetProcAddress
#undef GetSystemDirectoryA

extern "C" int __cdecl RunRealProtectionCases() {
	MemoryHostPolicy().useWineApis = false;
	auto* page = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	assert(page);
	const unsigned char original = 0x31;
	const unsigned char replacement = 0x72;
	const unsigned int protectionCallsBeforeTransactions = g_protectCalls;
	for (unsigned int repeat = 0; repeat < 4; ++repeat) {
		page[0] = original;
		assert(MemUtil::WritePatch(page, &replacement, 1) == MemUtil::PatchResult::Installed);
		assert(page[0] == replacement && ProtectionAt(page) == PAGE_READWRITE);

		page[16] = original;
		MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::UltrafastCore);
		assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcStorePrefetch));
		assert(MemUtil::PatchAdr(page + 16, &replacement, 1));
		assert(MemUtil::CommitBootPatchBundle());
		assert(page[16] == replacement && ProtectionAt(page + 16) == PAGE_READWRITE);

		page[32] = original;
		MemUtil::PreparedPatchRequest request{};
		request.address = page + 32;
		request.kind = MemUtil::PreparedPatchKind::Bytes;
		request.replacementBytes = std::span<const unsigned char>(&replacement, 1);
		MemUtil::PreparedPatchSet prepared;
		assert(prepared.Prepare(std::span<const MemUtil::PreparedPatchRequest>(&request, 1)));
		assert(prepared.Commit() == MemUtil::PatchResult::Installed);
		assert(page[32] == replacement && ProtectionAt(page + 32) == PAGE_READWRITE);
	}
	assert(g_protectCalls == protectionCallsBeforeTransactions);

	const unsigned int protectionCallsBeforeFailure = g_protectCalls;

	page[48] = original;
	MemUtil::PreparedPatchRequest failingRequest{};
	failingRequest.address = page + 48;
	failingRequest.kind = MemUtil::PreparedPatchKind::Bytes;
	failingRequest.replacementBytes = std::span<const unsigned char>(&replacement, 1);
	MemUtil::PreparedPatchSet failing;
	assert(failing.Prepare(std::span<const MemUtil::PreparedPatchRequest>(&failingRequest, 1)));
	assert(VirtualFree(page, 0, MEM_RELEASE));
	assert(failing.Commit() == MemUtil::PatchResult::RollbackIncomplete);
	assert(failing.Outcome().bytes == MemUtil::PatchByteState::Unknown);
	assert(g_protectCalls == protectionCallsBeforeFailure);
	return 1;
}

extern "C" __declspec(naked) int __cdecl CheckProtectionAbi() {
	__asm {
		push ebp
		mov ebp, esp
		push ebx
		push esi
		push edi
		sub esp, 8
		mov dword ptr [ebp - 16], 8
		mov dword ptr [ebp - 20], esp
		mov ebx, 13579BDFh
		mov esi, 2468ACE0h
		mov edi, 0BADF00Dh
	check_again:
		call RunRealProtectionCases
		cmp esp, dword ptr [ebp - 20]
		jne check_failed
		test eax, eax
		jz check_failed
		cmp ebx, 13579BDFh
		jne check_failed
		cmp esi, 2468ACE0h
		jne check_failed
		cmp edi, 0BADF00Dh
		jne check_failed
		dec dword ptr [ebp - 16]
		jnz check_again
		mov eax, 1
		jmp check_done
	check_failed:
		xor eax, eax
	check_done:
		lea esp, [ebp - 12]
		pop edi
		pop esi
		pop ebx
		pop ebp
		ret
	}
}

int main(int argc, char** argv) {
	const std::vector<BYTE> validPe = MakeTestPe32();
	const auto syscallNumber = MemUtil::Detail::ExtractSyscallFromPE(validPe, "NtProtectVirtualMemory");
	assert(syscallNumber && *syscallNumber == 0x12345678);
	std::vector<BYTE> truncatedPe = validPe;
	truncatedPe.resize(sizeof(IMAGE_DOS_HEADER) - 1);
	assert(!MemUtil::Detail::ExtractSyscallFromPE(truncatedPe, "NtProtectVirtualMemory"));
	std::vector<BYTE> malformedPe = validPe;
	IMAGE_DOS_HEADER malformedDos{};
	memcpy(&malformedDos, malformedPe.data(), sizeof(malformedDos));
	malformedDos.e_lfanew = static_cast<LONG>(0x7fffffff);
	memcpy(malformedPe.data(), &malformedDos, sizeof(malformedDos));
	assert(!MemUtil::Detail::ExtractSyscallFromPE(malformedPe, "NtProtectVirtualMemory"));
	std::vector<BYTE> nonX86Pe = validPe;
	const WORD amd64 = IMAGE_FILE_MACHINE_AMD64;
	memcpy(nonX86Pe.data() + 0x84, &amd64, sizeof(amd64));
	assert(!MemUtil::Detail::ExtractSyscallFromPE(nonX86Pe, "NtProtectVirtualMemory"));
	std::vector<BYTE> truncatedCodePe = validPe;
	const DWORD shortCodeRva = 0x11FE;
	memcpy(truncatedCodePe.data() + 0x22E, &shortCodeRva, sizeof(shortCodeRva));
	assert(!MemUtil::Detail::ExtractSyscallFromPE(truncatedCodePe, "NtProtectVirtualMemory"));
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
			g_failQuery = true;
			MEMORY_BASIC_INFORMATION info{};
			assert(NT_SUCCESS(MemUtil::HookedQueryVirtualMemory(memory, &info, sizeof(info))));
			assert(info.Protect == PAGE_READWRITE);
			assert(VirtualFree(memory, 0, MEM_RELEASE));
			assert(CheckProtectionAbi());
			puts("Windows transaction protection ABI PASS");
			puts("Windows syscall first PASS");
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
	const bool configuredWineApis = MemoryHostPolicy().useWineApis;
	MemoryHostPolicy().useWineApis = false;
	g_unresolveSyscalls = true;
	void* fallbackMemory = VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	assert(fallbackMemory);
	const unsigned char fallbackReplacement = 0x72;
	static_cast<unsigned char*>(fallbackMemory)[0] = 0x31;
	assert(MemUtil::WritePatch(fallbackMemory, &fallbackReplacement, 1) == MemUtil::PatchResult::Installed);
	assert(static_cast<unsigned char*>(fallbackMemory)[0] == fallbackReplacement);
	assert(ProtectionAt(fallbackMemory) == PAGE_READWRITE && g_protectCalls == 2);
	const unsigned int queriesBeforeFallback = g_queryCalls;
	MEMORY_BASIC_INFORMATION fallbackInfo{};
	assert(NT_SUCCESS(MemUtil::HookedQueryVirtualMemory(fallbackMemory, &fallbackInfo, sizeof(fallbackInfo))));
	assert(fallbackInfo.Protect == PAGE_READWRITE && g_queryCalls == queriesBeforeFallback + 1);
	assert(VirtualFree(fallbackMemory, 0, MEM_RELEASE));
	g_unresolveSyscalls = false;
	MemoryHostPolicy().useWineApis = configuredWineApis;
	g_protectCalls = g_openCalls = g_restoreCalls = g_queryCalls = 0;
	MemUtil::LogMemoryProtectionPolicy();
	auto* patternPages = static_cast<unsigned char*>(VirtualAlloc(nullptr, 8192,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	assert(patternPages);
	const unsigned char signature[] = { 0xAA, 0xBB, 0xCC, 0xDD };
	DWORD fallbackOldProtection = 0;
	memcpy(patternPages + 64, signature, sizeof(signature));
	patternPages[4094] = signature[0];
	patternPages[4095] = signature[1];
	assert(VirtualProtect(patternPages + 4096, 4096, PAGE_NOACCESS, &fallbackOldProtection));
	assert(MemUtil::FindPattern<uint32_t>(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(patternPages + 4094)),
		2, const_cast<PBYTE>(signature), "xxxx") == 0);
	assert(MemUtil::FindPattern<uint32_t>(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(patternPages)),
		68, const_cast<PBYTE>(signature), "xxxx")
		== static_cast<uint32_t>(reinterpret_cast<uintptr_t>(patternPages + 64)));
	assert(VirtualFree(patternPages, 0, MEM_RELEASE));

	const bool savedHostPolicy = MemoryHostPolicy().useWineApis;
	MemoryHostPolicy().useWineApis = true;
	auto* preparedPage = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	assert(preparedPage);
	unsigned char originalA[] = { 0xE8, 0, 0, 0, 0 };
	const unsigned char originalB[] = { 6, 7, 8, 9, 10 };
	const uintptr_t originalCallTarget = reinterpret_cast<uintptr_t>(&TestCallOriginal);
	const uintptr_t firstPreparedSite = reinterpret_cast<uintptr_t>(preparedPage + 16);
	const uint32_t originalCallRelative = static_cast<uint32_t>(originalCallTarget - (firstPreparedSite + 5));
	memcpy(originalA + 1, &originalCallRelative, sizeof(originalCallRelative));
	memcpy(preparedPage + 16, originalA, sizeof(originalA));
	memcpy(preparedPage + 48, originalB, sizeof(originalB));
	const std::array<MemUtil::PreparedPatchRequest, 2> preparedRequests = {{
		{ preparedPage + 16, MemUtil::PreparedPatchKind::Call,
			{}, originalCallTarget,
			reinterpret_cast<uintptr_t>(&Replacement), false },
		{ preparedPage + 48, MemUtil::PreparedPatchKind::Jump,
			std::span<const unsigned char>(originalB, sizeof(originalB)), 0,
			reinterpret_cast<uintptr_t>(&Replacement), false }
	}};
	const unsigned char mismatch[] = { 0xCC, 7, 8, 9, 10 };
	MemUtil::PreparedPatchRequest mismatchRequest = preparedRequests[1];
	mismatchRequest.expectedBytes = std::span<const unsigned char>(mismatch, sizeof(mismatch));
	MemUtil::PreparedPatchSet mismatchSet;
	const unsigned int mismatchOpens = g_openCalls;
	assert(!mismatchSet.Prepare(std::span<const MemUtil::PreparedPatchRequest>(&mismatchRequest, 1)));
	assert(g_openCalls == mismatchOpens && memcmp(preparedPage + 16, originalA, sizeof(originalA)) == 0);

	MemUtil::PreparedPatchSet preparedSet;
	const unsigned int queriesBeforePrepared = g_queryCalls;
	assert(preparedSet.Prepare(preparedRequests));
	assert(g_queryCalls - queriesBeforePrepared == 2);
	const unsigned int opensBeforePrepared = g_openCalls;
	const unsigned int restoresBeforePrepared = g_restoreCalls;
	g_allocationCount = 0;
	g_countingAllocations = true;
	const MemUtil::PatchResult preparedInstall = preparedSet.Commit();
	g_countingAllocations = false;
	assert(preparedInstall == MemUtil::PatchResult::Installed);
	assert(g_allocationCount == 0);
	assert(g_openCalls - opensBeforePrepared == 1 && g_restoreCalls - restoresBeforePrepared == 1);
	uint32_t installedCallRelative = 0;
	memcpy(&installedCallRelative, preparedPage + 17, sizeof(installedCallRelative));
	assert(preparedPage[16] == 0xE8 && firstPreparedSite + 5 + installedCallRelative
		== reinterpret_cast<uintptr_t>(&Replacement));
	assert(preparedPage[48] == 0xE9);
	assert(!preparedSet.Prepare(std::span<const MemUtil::PreparedPatchRequest>(&mismatchRequest, 1)));
	assert(preparedSet.Size() == preparedRequests.size());
	assert(preparedSet.SiteAddress(0) == firstPreparedSite);
	assert(preparedSet.Outcome().bytes == MemUtil::PatchByteState::Replacement);
	g_allocationCount = 0;
	g_countingAllocations = true;
	const MemUtil::PatchResult preparedRollback = preparedSet.Rollback();
	g_countingAllocations = false;
	assert(preparedRollback == MemUtil::PatchResult::Original && g_allocationCount == 0);
	assert(memcmp(preparedPage + 16, originalA, sizeof(originalA)) == 0);
	assert(memcmp(preparedPage + 48, originalB, sizeof(originalB)) == 0);

	assert(preparedSet.Prepare(std::span<const MemUtil::PreparedPatchRequest>(preparedRequests.data(), 1)));
	g_failCall = g_protectCalls + 1;
	g_allocationCount = 0;
	g_countingAllocations = true;
	const MemUtil::PatchResult openFailure = preparedSet.Commit();
	g_countingAllocations = false;
	assert(openFailure == MemUtil::PatchResult::Original && g_allocationCount == 0);
	g_failCall = 0;
	assert(memcmp(preparedPage + 16, originalA, sizeof(originalA)) == 0);

	assert(preparedSet.Prepare(std::span<const MemUtil::PreparedPatchRequest>(preparedRequests.data(), 1)));
	g_failFlushCall = g_flushCalls + 1;
	g_allocationCount = 0;
	g_countingAllocations = true;
	const MemUtil::PatchResult flushRollback = preparedSet.Commit();
	g_countingAllocations = false;
	assert(flushRollback == MemUtil::PatchResult::Original && g_allocationCount == 0);
	g_failFlushCall = 0;
	assert(memcmp(preparedPage + 16, originalA, sizeof(originalA)) == 0);

	assert(preparedSet.Prepare(std::span<const MemUtil::PreparedPatchRequest>(preparedRequests.data(), 1)));
	g_failFlushFrom = g_flushCalls + 1;
	g_allocationCount = 0;
	g_countingAllocations = true;
	const MemUtil::PatchResult incompleteRollback = preparedSet.Commit();
	g_countingAllocations = false;
	assert(incompleteRollback == MemUtil::PatchResult::RollbackIncomplete && g_allocationCount == 0);
	g_failFlushFrom = 0;
	assert(memcmp(preparedPage + 16, originalA, sizeof(originalA)) == 0);
	assert(VirtualFree(preparedPage, 0, MEM_RELEASE));
	MemoryHostPolicy().useWineApis = savedHostPolicy;
	g_protectCalls = g_openCalls = g_restoreCalls = g_queryCalls = 0;
	g_failCall = g_failFrom = 0;
	g_failFlushCall = g_failFlushFrom = 0;

	auto* page = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	assert(page);
	page[0] = 1;
	page[1] = 2;
	const unsigned char first = 3, second = 4;
	const unsigned int protectionsBeforeAllocationFailure = g_protectCalls;
	try {
		MemUtil::WritePatch(page, &first, static_cast<size_t>(-1));
		assert(false);
	}
	catch (const std::length_error&) {}
	assert(g_protectCalls == protectionsBeforeAllocationFailure);
	assert(ProtectionAt(page) == PAGE_READWRITE && page[0] == 1);

	MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::UltrafastCore);
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcStorePrefetch));
	assert(MemUtil::PatchAdr(page, &first, 1));
	assert(page[0] == 1);
	MemUtil::FailBootPatchBundle();
	assert(!MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcOwnership));
	assert(!MemUtil::CommitBootPatchBundle());
	assert(page[0] == 1 && g_protectCalls == 0);

	MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::UltrafastCore);
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcStorePrefetch));
	assert(MemUtil::PatchAdr(page, &first, 1));
	assert(!MemUtil::PatchAdr(page, &second, 1));
	assert(!MemUtil::CommitBootPatchBundle());
	assert(page[0] == 1 && g_protectCalls == 0);

	auto* overflowPage = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	assert(overflowPage);
	MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::UltrafastCore);
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcStorePrefetch));
	for (size_t i = 0; i < MemUtil::PreparedPatchSet::MaxSites; ++i)
		assert(MemUtil::PatchAdr(overflowPage + i, &first, 1));
	assert(!MemUtil::PatchAdr(overflowPage + MemUtil::PreparedPatchSet::MaxSites, &first, 1));
	assert(!MemUtil::CommitBootPatchBundle());
	assert(!MemUtil::BootPatchBundleEnabled(Framework::BootPatch::Bundle::UltrafastCore));
	assert(overflowPage[0] == 0 && overflowPage[MemUtil::PreparedPatchSet::MaxSites - 1] == 0);
	assert(VirtualFree(overflowPage, 0, MEM_RELEASE));

	// Shared members query once each and open and restore their common region once.
	const unsigned int queriesBeforeBundle = g_queryCalls;
	const unsigned int opensBeforeBundle = g_openCalls;
	const unsigned int restoresBeforeBundle = g_restoreCalls;
	MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::UltrafastCore);
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcStorePrefetch));
	assert(MemUtil::PatchAdr(page, &first, 1));
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcOwnership));
	assert(MemUtil::PatchAdr(page + 1, &second, 1));
	assert(MemUtil::CommitBootPatchBundle());
	assert(g_queryCalls - queriesBeforeBundle == 2);
	assert(g_openCalls - opensBeforeBundle == 1);
	assert(g_restoreCalls - restoresBeforeBundle == 1);
	page[0] = 1;
	page[1] = 2;
	void* pointerSlot = nullptr;
	void* pointerValue = reinterpret_cast<void*>(&Replacement);
	assert(MemUtil::SetStaticValue(reinterpret_cast<uintptr_t>(&pointerSlot), pointerValue, 0));
	assert(pointerSlot == pointerValue);

	page[2] = 1;
	MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::UltrafastCore);
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcStorePrefetch));
	assert(MemUtil::PatchAdr(page + 2, &first, 1));
	g_armRestoreFailureAddress = page + 2;
	assert(!MemUtil::CommitBootPatchBundle());
	assert(!MemUtil::BootPatchBundleEnabled(Framework::BootPatch::Bundle::UltrafastCore));
	assert(page[2] == first && ProtectionAt(page + 2) == PAGE_EXECUTE_READWRITE);
	g_failRestoreAddress = nullptr;
	DWORD restoredProtection = 0;
	assert(VirtualProtect(page, 4096, PAGE_READWRITE, &restoredProtection));
	page[2] = 1;

	// Fail each open and the final restore; a single restore failure is retried.
	for (unsigned int failure = 1; failure <= 2; ++failure) {
		g_protectCalls = 0;
		g_failCall = failure;
		MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::UltrafastCore);
		assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcStorePrefetch));
		assert(MemUtil::PatchAdr(page, &first, 1));
		assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcOwnership));
		assert(MemUtil::PatchAdr(page + 1, &second, 1));
		const bool enabled = MemUtil::CommitBootPatchBundle();
		assert(enabled == MemUtil::BootPatchBundleEnabled(Framework::BootPatch::Bundle::UltrafastCore));
		assert(enabled == (failure == 2));
		if (enabled) {
			assert(page[0] == first && page[1] == second);
			assert(ProtectionAt(page) == PAGE_READWRITE);
		}
		else {
			assert(page[0] == 1 && page[1] == 2);
		}
		page[0] = 1;
		page[1] = 2;
	}
	g_failCall = 0;

	g_protectCalls = 0;
	g_failCall = 0;
	MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::UltrafastCore);
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcStorePrefetch));
	assert(MemUtil::PatchAdr(page, &first, 1));
	g_slowCall = true;
	assert(MemUtil::CommitBootPatchBundle());
	assert(page[0] == first && MemUtil::BootPatchBundleEnabled(Framework::BootPatch::Bundle::UltrafastCore));

	MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::SignIn);
	assert(MemUtil::BootPatchBundleStaging(Framework::BootPatch::Bundle::SignIn));
	assert(!MemUtil::BootPatchBundleStaging(Framework::BootPatch::Bundle::ProfileAutoLoad));
	assert(!MemUtil::BootPatchBundleEnabled(Framework::BootPatch::Bundle::SignIn));
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::SourceSignIn));
	assert(MemUtil::CommitBootPatchBundle());
	assert(!MemUtil::BootPatchBundleStaging(Framework::BootPatch::Bundle::SignIn));
	MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::ProfileAutoLoad);
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::ProfileAutoLoad));
	MemUtil::FailBootPatchBundle();
	assert(!MemUtil::CommitBootPatchBundle());
	assert(MemUtil::BootPatchBundleEnabled(Framework::BootPatch::Bundle::SignIn));
	assert(!MemUtil::BootPatchBundleEnabled(Framework::BootPatch::Bundle::ProfileAutoLoad));

	MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::UltrafastCore);
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcStorePrefetch));
	assert(MemUtil::PatchAdr(page, &second, 1));
	page[0] = 9;
	assert(!MemUtil::CommitBootPatchBundle());
	assert(page[0] == 9);

	// Trampolines are executable before publication, while the source stays untouched.
	g_failCall = 0;
	const unsigned char code[] = { 0xB8, 42, 0, 0, 0, 0xC3 };
	memcpy(page + 64, code, sizeof(code));
	DWORD previous = 0;
	MEMORY_BASIC_INFORMATION gatewayInfo{};
	assert(VirtualProtect(page, 4096, PAGE_EXECUTE_READWRITE, &previous));
	using Function = int(__cdecl*)();
	const auto original = reinterpret_cast<Function>(page + 64);
	MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::UltrafastCore);
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcStorePrefetch));
	PBYTE gateway = MemUtil::TrampHook(page + 64, reinterpret_cast<PBYTE>(&Replacement), 5);
	assert(gateway && original() == 42);
	assert(reinterpret_cast<Function>(gateway)() == 42);
	assert(MemUtil::CommitBootPatchBundle());
	assert(original() == 7 && reinterpret_cast<Function>(gateway)() == 42);
	assert(MemUtil::PatchAdr(page + 64, code, sizeof(code)));
	assert(VirtualQuery(gateway, &gatewayInfo, sizeof(gatewayInfo)));
	assert(gatewayInfo.State == MEM_COMMIT && reinterpret_cast<Function>(gateway)() == 42);

	// A failed bundle keeps a staged gateway when its source write was attempted.
	memcpy(page + 96, code, sizeof(code));
	MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::UltrafastCore);
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcStorePrefetch));
	PBYTE retainedGateway = MemUtil::TrampHook(page + 96,
		reinterpret_cast<PBYTE>(&Replacement), 5);
	assert(retainedGateway);
	g_failFlush = true;
	g_failFlushAddress = page + 96;
	assert(!MemUtil::CommitBootPatchBundle());
	g_failFlush = false;
	g_failFlushAddress = nullptr;
	assert(VirtualQuery(retainedGateway, &gatewayInfo, sizeof(gatewayInfo)));
	assert(gatewayInfo.State == MEM_COMMIT && reinterpret_cast<Function>(retainedGateway)() == 42);
	assert(memcmp(page + 96, code, sizeof(code)) == 0);

	// A bundle rejected before its first source write frees its staged gateway.
	memcpy(page + 112, code, sizeof(code));
	MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::UltrafastCore);
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcStorePrefetch));
	PBYTE unpublishedGateway = MemUtil::TrampHook(page + 112,
		reinterpret_cast<PBYTE>(&Replacement), 5);
	assert(unpublishedGateway);
	page[112] = 0xCC;
	assert(!MemUtil::CommitBootPatchBundle());
	assert(VirtualQuery(unpublishedGateway, &gatewayInfo, sizeof(gatewayInfo)));
	assert(gatewayInfo.State == MEM_COMMIT && reinterpret_cast<Function>(unpublishedGateway)() == 42);
	memcpy(page + 112, code, sizeof(code));

	// The shared builder relocates relative calls and refuses partial or internal branch spans.
	const uintptr_t relativeCallSite = reinterpret_cast<uintptr_t>(page + 224);
	unsigned char relativeCall[] = { 0xE8, 0, 0, 0, 0, 0xC3 };
	const uint32_t relativeCallOffset = static_cast<uint32_t>(
		reinterpret_cast<uintptr_t>(&TestCallOriginal) - (relativeCallSite + 5));
	memcpy(relativeCall + 1, &relativeCallOffset, sizeof(relativeCallOffset));
	memcpy(page + 224, relativeCall, sizeof(relativeCall));
	MemUtil::PreparedPatchRequest relativeRequest{};
	relativeRequest.address = page + 224;
	relativeRequest.kind = MemUtil::PreparedPatchKind::Jump;
	relativeRequest.expectedBytes = std::span<const unsigned char>(relativeCall, 5);
	relativeRequest.replacementTarget = reinterpret_cast<uintptr_t>(&Replacement);
	relativeRequest.captureNativeEntry = true;
	MemUtil::PreparedPatchSet relativeSet;
	assert(relativeSet.Prepare(std::span<const MemUtil::PreparedPatchRequest>(&relativeRequest, 1)));
	PBYTE relocatedCall = static_cast<PBYTE>(relativeSet.NativeEntry(0));
	assert(relocatedCall && reinterpret_cast<Function>(relocatedCall)() == 42);
	assert(relativeSet.Commit() == MemUtil::PatchResult::Installed);
	assert(reinterpret_cast<Function>(page + 224)() == 7);
	assert(relativeSet.Rollback() == MemUtil::PatchResult::Original);
	assert(reinterpret_cast<Function>(page + 224)() == 42);
	assert(reinterpret_cast<Function>(relocatedCall)() == 42);

	unsigned char partialCall[] = { 0xE8, 0, 0, 0, 0, 0xB8, 1, 2, 3, 4 };
	const uintptr_t partialCallSite = reinterpret_cast<uintptr_t>(page + 256);
	const uint32_t partialCallOffset = static_cast<uint32_t>(
		reinterpret_cast<uintptr_t>(&TestCallOriginal) - (partialCallSite + 5));
	memcpy(partialCall + 1, &partialCallOffset, sizeof(partialCallOffset));
	memcpy(page + 256, partialCall, sizeof(partialCall));
	MemUtil::PreparedPatchRequest partialRequest = relativeRequest;
	partialRequest.address = page + 256;
	partialRequest.expectedBytes = std::span<const unsigned char>(partialCall, 6);
	MemUtil::PreparedPatchSet partialSet;
	assert(!partialSet.Prepare(std::span<const MemUtil::PreparedPatchRequest>(&partialRequest, 1)));

	unsigned char internalBranch[] = { 0xEB, 0x00, 0x90, 0x90, 0x90 };
	memcpy(page + 288, internalBranch, sizeof(internalBranch));
	MemUtil::PreparedPatchRequest internalRequest = relativeRequest;
	internalRequest.address = page + 288;
	internalRequest.expectedBytes = std::span<const unsigned char>(internalBranch, sizeof(internalBranch));
	MemUtil::PreparedPatchSet internalSet;
	assert(!internalSet.Prepare(std::span<const MemUtil::PreparedPatchRequest>(&internalRequest, 1)));

	// A patch across two pages restores each page's own protection.
	auto* pair = static_cast<unsigned char*>(VirtualAlloc(nullptr, 8192,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	assert(pair);
	memset(pair, 0xCC, 8192);
	const uint64_t originalValue = 0x1122334455667788ULL;
	const uint64_t replacementValue = 0x8877665544332211ULL;
	memcpy(pair + 4092, &originalValue, sizeof(originalValue));
	assert(VirtualProtect(pair + 4096, 4096, PAGE_EXECUTE_READ, &previous));
	const unsigned char spanning[] = { 1, 2, 3, 4 };
	assert(MemUtil::WritePatch(pair + 4094, spanning, sizeof(spanning)) == MemUtil::PatchResult::Installed);
	assert(memcmp(pair + 4094, spanning, sizeof(spanning)) == 0);
	assert(ProtectionAt(pair) == PAGE_READWRITE && ProtectionAt(pair + 4096) == PAGE_EXECUTE_READ);
	assert(MemUtil::SetStaticValue(reinterpret_cast<uintptr_t>(pair + 4092), replacementValue, 1));
	assert(memcmp(pair + 4092, &replacementValue, sizeof(replacementValue)) == 0);
	assert(ProtectionAt(pair) == PAGE_READWRITE && ProtectionAt(pair + 4096) == PAGE_EXECUTE_READ);
	const uint64_t failedRestoreValue = 0xA1B2C3D4E5F60718ULL;
	g_failRestoreAddress = pair + 4092;
	assert(MemUtil::SetStaticValue(reinterpret_cast<uintptr_t>(pair + 4092), failedRestoreValue, 1));
	g_failRestoreAddress = nullptr;
	assert(memcmp(pair + 4092, &failedRestoreValue, sizeof(failedRestoreValue)) == 0);
	assert(ProtectionAt(pair) == PAGE_EXECUTE_READWRITE && ProtectionAt(pair + 4096) == PAGE_EXECUTE_READ);
	assert(VirtualProtect(pair, 4096, PAGE_READWRITE, &previous));
	assert(MemUtil::WritePatch(pair + 4094, spanning, sizeof(spanning)) == MemUtil::PatchResult::Installed);

	// Opening the second page fails: the first page is restored and the bytes stay original.
	g_failCall = g_protectCalls + 2;
	const unsigned char other[] = { 5, 6, 7, 8 };
	assert(MemUtil::WritePatch(pair + 4094, other, sizeof(other)) == MemUtil::PatchResult::Original);
	assert(memcmp(pair + 4094, spanning, sizeof(spanning)) == 0);
	assert(ProtectionAt(pair) == PAGE_READWRITE && ProtectionAt(pair + 4096) == PAGE_EXECUTE_READ);

	// A failed rollback flush keeps the result incomplete even when bytes match.
	g_failCall = 0;
	g_failFlush = true;
	assert(MemUtil::WritePatch(pair + 4094, other, sizeof(other)) == MemUtil::PatchResult::RollbackIncomplete);
	g_failFlush = false;
	assert(memcmp(pair + 4094, spanning, sizeof(spanning)) == 0);
	assert(ProtectionAt(pair) == PAGE_READWRITE && ProtectionAt(pair + 4096) == PAGE_EXECUTE_READ);
	assert(VirtualFree(pair, 0, MEM_RELEASE));

	// A later member failure rolls earlier bytes back to their pre-install protection snapshots.
	auto* firstPage = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	auto* secondPage = static_cast<unsigned char*>(VirtualAlloc(nullptr, 4096,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	assert(firstPage && secondPage);
	firstPage[0] = 11;
	secondPage[0] = 12;
	assert(VirtualProtect(secondPage, 4096, PAGE_EXECUTE_READ, &previous));
	const unsigned char firstReplacement = 21, secondReplacement = 22;
	MemUtil::BeginBootPatchBundle(Framework::BootPatch::Bundle::UltrafastCore);
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcStorePrefetch));
	assert(MemUtil::PatchAdr(firstPage, &firstReplacement, 1));
	assert(MemUtil::BootPatchMember(Framework::BootPatch::Member::DlcOwnership));
	assert(MemUtil::PatchAdr(secondPage, &secondReplacement, 1));
	g_armRestoreFailureAddress = firstPage;
	g_failFlush = true;
	g_failFlushAddress = secondPage;
	assert(!MemUtil::CommitBootPatchBundle());
	g_failFlush = false;
	g_failFlushAddress = nullptr;
	g_failRestoreAddress = nullptr;
	assert(firstPage[0] == 11 && secondPage[0] == 12);
	assert(ProtectionAt(firstPage) == PAGE_EXECUTE_READWRITE);
	assert(ProtectionAt(secondPage) == PAGE_EXECUTE_READ);
	assert(VirtualFree(firstPage, 0, MEM_RELEASE));
	assert(VirtualFree(secondPage, 0, MEM_RELEASE));

	// A written hook whose protection restore keeps failing stays installed with a live, sealed trampoline.
	memcpy(page + 128, code, sizeof(code));
	const auto standalone = reinterpret_cast<Function>(page + 128);
	assert(ProtectionAt(page) == PAGE_EXECUTE_READWRITE);
	assert(VirtualProtect(page, 4096, PAGE_EXECUTE_READ, &previous));
	g_failFrom = g_protectCalls + 3;
	MemUtil::PatchResult outcome = MemUtil::PatchResult::Original;
	uintptr_t published = 0;
	PBYTE live = MemUtil::TrampHook(page + 128, reinterpret_cast<PBYTE>(&Replacement), 5, &outcome, &published);
	g_failFrom = 0;
	assert(outcome == MemUtil::PatchResult::Installed);
	assert(live && published == reinterpret_cast<uintptr_t>(live));
	assert(page[128] == 0xE9);
	assert(ProtectionAt(page) == PAGE_EXECUTE_READWRITE);
	assert(ProtectionAt(live) == PAGE_EXECUTE_READ);
	assert(standalone() == 7 && reinterpret_cast<Function>(live)() == 42);

	// A hook that never reached the source reports the original state.
	memcpy(page + 192, code, sizeof(code));
	g_failFrom = g_protectCalls + 2;
	PBYTE refused = MemUtil::TrampHook(page + 192, reinterpret_cast<PBYTE>(&Replacement), 5, &outcome);
	g_failFrom = 0;
	assert(!refused && outcome == MemUtil::PatchResult::Original);
	assert(memcmp(page + 192, code, sizeof(code)) == 0);
	assert(reinterpret_cast<Function>(page + 192)() == 42);

	MEMORY_BASIC_INFORMATION info{};
	assert(NT_SUCCESS(MemUtil::HookedQueryVirtualMemory(page, &info, sizeof(info))));
	assert(!NT_SUCCESS(MemUtil::HookedQueryVirtualMemory(page, nullptr, 0)));
	assert(VirtualFree(page, 0, MEM_RELEASE));
	puts("Boot bundle and documented memory paths ALL PASS");
}

#endif
