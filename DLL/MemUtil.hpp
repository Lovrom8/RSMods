#pragma once

#include "Offsets.hpp"
#include "winternl.h"
#include <array>
#include <atomic>
#include <limits>
#include <span>
#include <string_view>
#include <type_traits>

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#endif

namespace MemUtil {
	// Initialization-thread writes remain staged until every member validates.
	void BeginBootPatchBundle();
	bool BootPatchMember(const char* name);
	void FailBootPatchBundle();
	bool CommitBootPatchBundle();
	bool BootPatchBundleEnabled();
	bool BootPatchBundleStaging();
	bool BootPatchStaged(uintptr_t address);
	bool bCompare(const BYTE* pData, const byte* bMask, const char* szMask);

	// Installed: the replacement is in place, read back. Original: the original bytes and every
	// page's protection are confirmed. RollbackIncomplete: neither could be confirmed, so anything
	// the site may jump to must stay alive.
	enum class PatchResult { Installed, Original, RollbackIncomplete };
	PatchResult WritePatch(LPVOID address, LPCVOID changeToMake, size_t len);
	PatchResult WritePointerPatch(LPVOID address, LPCVOID value);
	enum class PreparedPatchKind { Jump, Call };
	struct PreparedPatchRequest {
		LPVOID address = nullptr;
		PreparedPatchKind kind = PreparedPatchKind::Jump;
		std::span<const unsigned char> expectedBytes{};
		uintptr_t expectedTarget = 0;
		uintptr_t replacementTarget = 0;
		bool captureNativeEntry = false;
	};

	// Preparation owns all patch and region metadata used while code is being rewritten.
	class PreparedPatchSet {
	public:
		static constexpr size_t MaxSites = 8;
		static constexpr size_t MaxPatchBytes = 32;
		static constexpr size_t MaxRegions = MaxSites * 2;

		PreparedPatchSet() = default;
		~PreparedPatchSet();
		PreparedPatchSet(const PreparedPatchSet&) = delete;
		PreparedPatchSet& operator=(const PreparedPatchSet&) = delete;

		bool Prepare(std::span<const PreparedPatchRequest> requests);
		PatchResult Commit() noexcept;
		PatchResult Rollback() noexcept;
		void Report(const char* group) const;
		void* NativeEntry(size_t site) const noexcept;
		size_t Size() const noexcept { return siteCount_; }

	private:
		enum class Failure { None, InvalidRequest, Capacity, BytesMismatch, QueryFailed, GatewayFailed,
			OpenFailed, CommitMismatch, WriteFailed, FlushFailed, ReadbackFailed, RestoreFailed };
		struct Site {
			uintptr_t address = 0;
			size_t length = 0;
			std::array<unsigned char, MaxPatchBytes> original{};
			std::array<unsigned char, MaxPatchBytes> replacement{};
			void* nativeEntry = nullptr;
			bool ownsNativeEntry = false;
		};
		struct Region {
			uintptr_t base = 0;
			SIZE_T size = 0;
			uintptr_t queryBase = 0;
			SIZE_T querySize = 0;
			ULONG protection = 0;
			bool opened = false;
		};

		bool SetRegionProtection(Region& region, ULONG protection) noexcept;
		bool RestoreOpenedRegions() noexcept;
		bool MatchesOriginal(size_t count) const noexcept;
		void ReleaseNativeEntries() noexcept;

		std::array<Site, MaxSites> sites_{};
		std::array<Region, MaxRegions> regions_{};
		size_t siteCount_ = 0;
		size_t regionCount_ = 0;
		PatchResult result_ = PatchResult::Original;
		Failure failure_ = Failure::None;
		uintptr_t failureAddress_ = 0;
		DWORD failureCode_ = ERROR_SUCCESS;
		bool prepared_ = false;
		bool operationAttempted_ = false;
		bool useWineApis_ = true;
		bool protectSyscallResolved_ = false;
	};
	bool PatchAdr(VersioningStruct<uintptr_t>& address, LPCVOID changeToMake, size_t len, bool addBaseHandle = false);
	bool PatchAdr(LPVOID address, LPCVOID changeToMake, size_t len);
	bool PatchAdr(uintptr_t address, LPCVOID changeToMake, size_t len, bool addBaseHandle);
	bool PatchAdr(uintptr_t address, std::string_view data, bool addBaseHandle);
	bool PlaceHook(VersioningStruct<uintptr_t>& hookSpot, void* ourFunct, int len, bool addBaseHandle = false);
	bool PlaceHook(void* hookSpot, void* ourFunct, int len);
	void JumpToVersioned();
	void** GetVTable(void* object);
	bool PatchVTableSlot(void** vtable, size_t slot, void* replacement);

	// Patches vtable slots and keeps the originals, so one hook installed in several vtables (e.g. a COM
	// class's A and W variants) can find the function it replaced for the object it was called on.
	// Patch/RestoreAll from one thread; Original is safe from any thread, including from inside the hook.
	class VTablePatcher {
	public:
		bool Patch(void** vtable, size_t slot, void* replacement);

		// nullptr if this object's vtable slot wasn't patched here.
		[[nodiscard]] void* Original(void* object, size_t slot) const;
		template <typename Fn>
		[[nodiscard]] Fn Original(void* object, size_t slot) const { return reinterpret_cast<Fn>(Original(object, slot)); }

		// Records stay, so a call already inside a hook still resolves its original.
		void RestoreAll();

	private:
		struct Entry {
			void** vtable = nullptr;
			size_t slot = 0;
			void* original = nullptr;
		};
		// Fixed storage: readers on other threads never see a reallocation.
		std::array<Entry, 16> entries{};
		std::atomic<size_t> count{ 0 };
	};
	// Returns the trampoline unless the source is confirmed original; check result to tell
	// an installed hook from an incomplete rollback. original receives the trampoline before
	// the source is written, so a hook entered mid-install never reads a null pointer.
	PBYTE TrampHook(PBYTE src, PBYTE dst, unsigned int len, PatchResult* result = nullptr,
		uintptr_t* original = nullptr);
	bool IsBadReadPtr(const void* pointer);
	template <typename T>
		requires std::is_trivially_copyable_v<T>
	bool TryRead(uintptr_t address, T& value);
	bool MatchesBytes(uintptr_t address, std::span<const unsigned char> expected);
	uintptr_t FindDMAAddy(uintptr_t ptr, std::span<const unsigned int> offsets, bool safe = false);
	uintptr_t ReadPtr(uintptr_t adr);
	template <typename T>
		requires std::is_trivially_copyable_v<T>
	bool SetStaticValue(uintptr_t staticValue, T data, unsigned int lengthOfData);

	template <typename T>
	T FindPattern(uint32_t address, size_t size, PBYTE pattern, const char* mask);

	template <typename T>
		requires std::is_trivially_copyable_v<T>
	T ReadValue(uintptr_t adr, bool addBaseHandle);

	const char* DetectedHost();
	bool IsRunningOnWine();
	void LogMemoryProtectionPolicy();
	NTSTATUS HookedVirtualProtect(LPVOID address, SIZE_T len, ULONG newProtection, ULONG& oldProtection);
	NTSTATUS HookedQueryVirtualMemory(LPVOID address, PMEMORY_BASIC_INFORMATION memoryBuffer, SIZE_T dwLength);

	uint32_t GetTextSectionAddress();
	uint32_t GetTextSectionLength();
	void CheckMemoryProtection(void* address);
};

template <typename T> requires std::is_trivially_copyable_v<T>
bool MemUtil::TryRead(uintptr_t address, T& value) {
	if (!address)
		return false;

	__try {
		value = *reinterpret_cast<const T*>(address);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

template <typename T>
/// <summary>
/// Scans memory chunk for a pattern.
/// </summary>
/// <typeparam name="T"> - Type of value to return</typeparam>
/// <param name="address"> - Address to start the search at.</param>
/// <param name="size"> - Size of search.</param>
/// <param name="pattern"> - Pattern to look for.</param>
/// <param name="mask"> - Mask of what bytes we know (notated with an "x") and what bytes we don't (notated with a "?").</param>
/// <returns>Value if found or NULL if not.</returns>
T MemUtil::FindPattern(uint32_t address, size_t size, PBYTE pattern, const char* mask) {
	if (!address || !pattern || !mask || size == 0)
		return T{};
	const size_t patternLength = std::string_view(mask).size();
	if (size < patternLength || static_cast<uint64_t>(size)
		> (std::numeric_limits<uint64_t>::max)() - address)
		return T{};
	const uint64_t rangeEnd = static_cast<uint64_t>(address) + size;
	if (rangeEnd > static_cast<uint64_t>((std::numeric_limits<uint32_t>::max)()) + 1)
		return T{};
	if (patternLength == 0)
		return T(address);
	const size_t lastStart = size - patternLength;
	for (size_t offset = 0; offset <= lastStart; ++offset) {
		const uint32_t candidate = static_cast<uint32_t>(static_cast<uint64_t>(address) + offset);
		if (bCompare(reinterpret_cast<PBYTE>(candidate), pattern, mask)) {
			return T(candidate);
		}
	}

	return T{};
}

template <typename T> requires std::is_trivially_copyable_v<T>
/// <summary>
/// Sets a static value in the executable, utilizing VirtualProtect.
/// </summary>
/// <typeparam name="T"> - Type of the data.</typeparam>
/// <param name="staticValue"> - Address of the static value.</param>
/// <param name="data"> - Data we should change the staticValue to.</param>
/// <param name="lengthOfData"> - Retained for callers; the write span is sizeof(T).</param>
/// <returns>Successfully able to set the value.</returns>
bool MemUtil::SetStaticValue(uintptr_t staticValue, T data, unsigned int) {
	if constexpr (std::is_pointer_v<T> && sizeof(T) == sizeof(void*))
		return WritePointerPatch(reinterpret_cast<LPVOID>(staticValue), &data) == PatchResult::Installed;
	else
		return WritePatch(reinterpret_cast<LPVOID>(staticValue), &data, sizeof(T)) == PatchResult::Installed;
}

template <typename T> requires std::is_trivially_copyable_v<T>
T MemUtil::ReadValue(uintptr_t address, bool addBaseHandle) {
	if (address == 0) {
		return T{};
	}

	const uintptr_t addr = address + (addBaseHandle ? Offsets::baseHandle : 0);
	return *reinterpret_cast<T*>(addr);
}
