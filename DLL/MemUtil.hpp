#pragma once

#include "Offsets.hpp"
#include "Framework/BootPatchBundlePolicy.hpp"
#include "winternl.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#endif

namespace MemUtil {
	// Initialization-thread writes remain staged until every member validates.
	void BeginBootPatchBundle(Framework::BootPatch::Bundle bundle);
	bool BootPatchMember(Framework::BootPatch::Member member);
	void FailBootPatchBundle();
	bool CommitBootPatchBundle();
	bool BootPatchBundleEnabled(Framework::BootPatch::Bundle bundle);
	bool BootPatchBundleStaging(Framework::BootPatch::Bundle bundle);
	bool BootPatchStaged(uintptr_t address);
	bool bCompare(const BYTE* pData, const byte* bMask, const char* szMask);

	enum class PatchResult { Installed, Original, RollbackIncomplete };
	enum class PatchByteState { Replacement, Original, Unknown };

	// Installed means replacement bytes were verified and flushed, even if protection restoration
	// degraded. Original additionally requires verified restoration; every other state is incomplete.
	struct PatchOutcome {
		PatchByteState bytes = PatchByteState::Original;
		bool instructionCacheFlushed = true;
		bool protectionRestored = true;
		bool everPublished = false;
	};

	constexpr PatchResult ToPatchResult(PatchOutcome outcome) noexcept {
		if (outcome.bytes == PatchByteState::Replacement && outcome.instructionCacheFlushed)
			return PatchResult::Installed;
		if (outcome.bytes == PatchByteState::Original && outcome.instructionCacheFlushed
			&& outcome.protectionRestored)
			return PatchResult::Original;
		return PatchResult::RollbackIncomplete;
	}

	namespace Detail {
		constexpr bool CanReplacePreparedRecord(PatchResult result, bool hasOwnedGateway) noexcept {
			return result == PatchResult::Original && !hasOwnedGateway;
		}

		struct PatchTransactionSite {
			uintptr_t address = 0;
			std::span<const unsigned char> original{};
			std::span<const unsigned char> replacement{};
			bool pointerStore = false;
			bool hasNativeEntry = false;
			bool everPublished = false;
		};

		struct PatchTransactionRegion {
			uintptr_t base = 0;
			size_t size = 0;
			uint32_t protection = 0;
			uintptr_t queryBase = 0;
			size_t querySize = 0;
			bool opened = false;
		};

		struct PatchTransactionOperations {
			void* context = nullptr;
			bool (*read)(void*, uintptr_t, unsigned char*, size_t) noexcept = nullptr;
			bool (*write)(void*, uintptr_t, const unsigned char*, size_t, bool) noexcept = nullptr;
			bool (*protect)(void*, uintptr_t, size_t, uint32_t) noexcept = nullptr;
			bool (*flush)(void*, uintptr_t, size_t) noexcept = nullptr;
		};

		inline bool CaptureOriginalBytes(uintptr_t address, std::span<const unsigned char> expected,
			std::span<unsigned char> original, const PatchTransactionOperations& operations) noexcept {
			if (!address || original.empty() || (!expected.empty() && expected.size() != original.size())
				|| !operations.read)
				return false;
			if (!operations.read(operations.context, address, original.data(), original.size()))
				return false;
			return expected.empty() || memcmp(original.data(), expected.data(), original.size()) == 0;
		}

		inline bool MatchesPatchBytes(const PatchTransactionSite& site,
			std::span<const unsigned char> expected, const PatchTransactionOperations& operations) noexcept {
			std::array<unsigned char, 64> actual{};
			for (size_t offset = 0; offset < expected.size();) {
				const size_t remaining = expected.size() - offset;
				const size_t count = remaining < actual.size() ? remaining : actual.size();
				if (!operations.read || !operations.read(operations.context, site.address + offset,
					actual.data(), count) || memcmp(actual.data(), expected.data() + offset, count) != 0)
					return false;
				offset += count;
			}
			return true;
		}

		inline PatchOutcome RunPatchTransaction(std::span<PatchTransactionSite> sites,
			std::span<PatchTransactionRegion> regions, const PatchTransactionOperations& operations,
			bool rollback, uint32_t writableProtection, uintptr_t* failureAddress = nullptr) noexcept {
			PatchOutcome outcome{};
			if (failureAddress)
				*failureAddress = 0;
			if (sites.empty()) {
				outcome.bytes = PatchByteState::Replacement;
				return outcome;
			}

			auto inspect = [&]() noexcept {
				bool allOriginal = true;
				bool allReplacement = true;
				for (const PatchTransactionSite& site : sites) {
					allOriginal = allOriginal && MatchesPatchBytes(site, site.original, operations);
					allReplacement = allReplacement && MatchesPatchBytes(site, site.replacement, operations);
				}
				if (allReplacement)
					outcome.bytes = PatchByteState::Replacement;
				else if (allOriginal)
					outcome.bytes = PatchByteState::Original;
				else
					outcome.bytes = PatchByteState::Unknown;
				outcome.everPublished = false;
				for (const PatchTransactionSite& site : sites)
					outcome.everPublished = outcome.everPublished || site.everPublished;
			};
			auto restoreRegions = [&]() noexcept {
				bool restored = true;
				for (size_t i = regions.size(); i > 0; --i) {
					PatchTransactionRegion& region = regions[i - 1];
					if (!region.opened)
						continue;
					if (!operations.protect || (!operations.protect(operations.context, region.base,
						region.size, region.protection) && !operations.protect(operations.context,
						region.base, region.size, region.protection))) {
						restored = false;
						if (failureAddress && !*failureAddress)
							*failureAddress = region.base;
					}
					region.opened = false;
				}
				return restored;
			};

			for (PatchTransactionRegion& region : regions) {
				if (!operations.protect || !operations.protect(operations.context, region.base,
					region.size, writableProtection)) {
					if (failureAddress)
						*failureAddress = region.base;
					outcome.protectionRestored = restoreRegions();
					inspect();
					return outcome;
				}
				region.opened = true;
			}
			if (!rollback) {
				for (const PatchTransactionSite& site : sites) {
					if (!MatchesPatchBytes(site, site.original, operations)) {
						if (failureAddress)
							*failureAddress = site.address;
						outcome.protectionRestored = restoreRegions();
						inspect();
						return outcome;
					}
				}
			}

			size_t attempted = 0;
			bool failed = false;
			bool cacheFlushed = true;
			if (rollback) {
				for (const PatchTransactionSite& site : sites) {
					if (!MatchesPatchBytes(site, site.original, operations)
						&& !MatchesPatchBytes(site, site.replacement, operations)) {
						if (failureAddress)
							*failureAddress = site.address;
						failed = true;
						break;
					}
				}
				if (!failed) {
					for (size_t i = sites.size(); i > 0; --i) {
						PatchTransactionSite& site = sites[i - 1];
						const bool original = MatchesPatchBytes(site, site.original, operations);
						if (!original) {
							if (!operations.write || !operations.write(operations.context, site.address,
								site.original.data(), site.original.size(), site.pointerStore)) {
								if (failureAddress)
									*failureAddress = site.address;
								failed = true;
							}
						}
						if (!operations.flush || !operations.flush(operations.context, site.address,
							site.original.size())) {
							if (failureAddress && !*failureAddress)
								*failureAddress = site.address;
							cacheFlushed = false;
						}
					}
				}
			}
			else {
				for (; attempted < sites.size(); ++attempted) {
					PatchTransactionSite& site = sites[attempted];
					if (site.hasNativeEntry)
						site.everPublished = true;
					if (!operations.write || !operations.write(operations.context, site.address,
						site.replacement.data(), site.replacement.size(), site.pointerStore)) {
						if (failureAddress)
							*failureAddress = site.address;
						failed = true;
						++attempted;
						break;
					}
					if (!operations.flush || !operations.flush(operations.context, site.address,
						site.replacement.size())) {
						if (failureAddress)
							*failureAddress = site.address;
						cacheFlushed = false;
						failed = true;
						++attempted;
						break;
					}
					if (!MatchesPatchBytes(site, site.replacement, operations)) {
						if (failureAddress)
							*failureAddress = site.address;
						failed = true;
						++attempted;
						break;
					}
				}
				if (failed) {
					bool rollbackFlushed = true;
					for (size_t i = attempted; i > 0; --i) {
						PatchTransactionSite& site = sites[i - 1];
						if (!operations.write || !operations.write(operations.context, site.address,
							site.original.data(), site.original.size(), site.pointerStore)) {
							if (failureAddress && !*failureAddress)
								*failureAddress = site.address;
							rollbackFlushed = false;
						}
						if (!operations.flush || !operations.flush(operations.context, site.address,
							site.original.size())) {
							if (failureAddress && !*failureAddress)
								*failureAddress = site.address;
							rollbackFlushed = false;
						}
					}
					cacheFlushed = rollbackFlushed;
				}
			}

			outcome.protectionRestored = restoreRegions();
			outcome.instructionCacheFlushed = cacheFlushed;
			inspect();
			return outcome;
		}
	}

	PatchResult WritePatch(LPVOID address, LPCVOID changeToMake, size_t len);
	PatchResult WritePointerPatch(LPVOID address, LPCVOID value);
	enum class PreparedPatchKind { Jump, Call, Bytes, PointerStore };
	struct PreparedPatchRequest {
		LPVOID address = nullptr;
		PreparedPatchKind kind = PreparedPatchKind::Jump;
		std::span<const unsigned char> expectedBytes{};
		uintptr_t expectedTarget = 0;
		uintptr_t replacementTarget = 0;
		bool captureNativeEntry = false;
		std::span<const unsigned char> replacementBytes{};
		size_t length = 0;
		// A gateway over bytes not known in advance (no expectedBytes, no length) spans up to the
		// first instruction boundary at or after this length.
		size_t minimumLength = 0;
	};

	// Preparation owns all patch and region metadata used while code is being rewritten.
	class PreparedPatchSet {
	public:
		static constexpr size_t MaxSites = 64;
		static constexpr size_t MaxRegions = MaxSites * 2;

		PreparedPatchSet() = default;
		~PreparedPatchSet();
		PreparedPatchSet(const PreparedPatchSet&) = delete;
		PreparedPatchSet& operator=(const PreparedPatchSet&) = delete;

		bool Prepare(std::span<const PreparedPatchRequest> requests);
		bool Append(const PreparedPatchRequest& request);
		void Reset() noexcept;
		// Commit and Rollback write with other threads running unless the owner asked for exclusion:
		// then they stop every other thread around a write that is not only pointer stores, unless no
		// other thread exists at that moment; a refused exclusion leaves the bytes untouched. Pausing
		// every thread is opt-in because the protected executable tolerates it poorly during loading.
		void RequireThreadExclusion() noexcept { threadExclusion_ = true; }
		PatchResult Commit() noexcept;
		PatchResult Rollback() noexcept;
		PatchOutcome Outcome() const noexcept { return outcome_; }
		uintptr_t FailureAddress() const noexcept { return failureAddress_; }
		void Report(const char* group) const;
		bool ExclusionFailed() const noexcept { return failure_ == Failure::ExecutionSyncFailed; }
		// Returning an owned entry publishes its address, so the allocation is retained.
		void* NativeEntry(size_t site) const noexcept;
		uintptr_t SiteAddress(size_t site) const noexcept;
		size_t SiteLength(size_t site) const noexcept;
		size_t Size() const noexcept { return sites_.size(); }

	private:
		friend PBYTE TrampHook(PBYTE src, PBYTE dst, unsigned int len, PatchResult* result,
			uintptr_t* original);
		enum class Failure { None, InvalidRequest, BytesMismatch, ReadFailed, QueryFailed, GatewayFailed,
			CommitFailed, ExecutionSyncFailed };
		struct Site {
			Detail::PatchTransactionSite transaction{};
			std::vector<unsigned char> original;
			std::vector<unsigned char> replacement;
			void* nativeEntry = nullptr;
			bool ownsNativeEntry = false;
			mutable bool nativeEntryExposed = false;
		};

		void ReleaseNativeEntries() noexcept;
		PatchResult RunTransaction(bool rollback) noexcept;

		std::vector<Site> sites_;
		std::vector<Detail::PatchTransactionRegion> regions_;
		PatchOutcome outcome_{};
		PatchResult result_ = PatchResult::Original;
		Failure failure_ = Failure::None;
		uintptr_t failureAddress_ = 0;
		DWORD failureCode_ = ERROR_SUCCESS;
		const char* exclusionReason_ = nullptr;
		DWORD exclusionThread_ = 0;
		unsigned exclusionPass_ = 0;
		bool prepared_ = false;
		// A refused Prepare or Append keeps Commit and Rollback from writing until Reset or Prepare.
		bool preparationFailed_ = false;
		bool operationAttempted_ = false;
		bool useSyscallProtection_ = false;
		bool threadExclusion_ = false;
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
	NTSTATUS HookedQueryVirtualMemory(LPVOID address, PMEMORY_BASIC_INFORMATION memoryBuffer, SIZE_T dwLength);

	uint32_t GetTextSectionAddress();
	uint32_t GetTextSectionLength();
	void CheckMemoryProtection(void* address);
};

namespace MemUtil::Detail {
	// Offset of the first match that starts at or after from and fits in size bytes, or size when
	// there is none. size is the whole byte range; mask 'x' marks the bytes that must match.
	inline size_t FindPatternOffset(const unsigned char* data, size_t size, size_t from,
		const unsigned char* pattern, const char* mask) noexcept {
		if (!data || !pattern || !mask)
			return size;
		const size_t patternLength = std::string_view(mask).size();
		if (patternLength == 0 || size < patternLength)
			return size;
		for (size_t offset = from; offset <= size - patternLength; ++offset) {
			size_t matched = 0;
			while (matched < patternLength
				&& (mask[matched] != 'x' || data[offset + matched] == pattern[matched]))
				++matched;
			if (matched == patternLength)
				return offset;
		}
		return size;
	}

	// Offset of the only match in the whole range, or size when there is none or more than one.
	inline size_t FindUniquePatternOffset(const unsigned char* data, size_t size,
		const unsigned char* pattern, const char* mask) noexcept {
		const size_t first = FindPatternOffset(data, size, 0, pattern, mask);
		if (first == size)
			return size;
		return FindPatternOffset(data, size, first + 1, pattern, mask) == size ? first : size;
	}
}

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
