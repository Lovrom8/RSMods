#define NOMINMAX
#include <Windows.h>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <vector>

#include "../../MemUtil.hpp"

namespace {
	using MemUtil::Detail::PatchTransactionOperations;
	using MemUtil::Detail::PatchTransactionRegion;
	using MemUtil::Detail::PatchTransactionSite;

	struct Event {
		char operation;
		uintptr_t address;
		unsigned char value;
		uint32_t protection;
	};

	struct FakeMemory {
		static constexpr uintptr_t Base = 0x1000;
		std::array<unsigned char, 64> bytes{};
		std::vector<Event> events;
		unsigned int readCalls = 0;
		unsigned int writeCalls = 0;
		unsigned int protectCalls = 0;
		unsigned int failWriteCall = 0;
		unsigned int failProtectCall = 0;
		bool failRead = false;

		bool Contains(uintptr_t address, size_t length) const noexcept {
			return address >= Base && address - Base <= bytes.size()
				&& length <= bytes.size() - (address - Base);
		}

		void ResetEvents() {
			events.clear();
			readCalls = 0;
			writeCalls = 0;
			protectCalls = 0;
			failWriteCall = 0;
			failProtectCall = 0;
			failRead = false;
		}
	};

	bool Read(void* context, uintptr_t address, unsigned char* output, size_t length) noexcept {
		auto& memory = *static_cast<FakeMemory*>(context);
		++memory.readCalls;
		if (memory.failRead || !memory.Contains(address, length))
			return false;
		memcpy(output, memory.bytes.data() + (address - FakeMemory::Base), length);
		return true;
	}

	bool Write(void* context, uintptr_t address, const unsigned char* input, size_t length,
		bool) noexcept {
		auto& memory = *static_cast<FakeMemory*>(context);
		++memory.writeCalls;
		const unsigned char value = length ? input[0] : 0;
		memory.events.push_back({ 'W', address, value, 0 });
		if (memory.writeCalls == memory.failWriteCall || !memory.Contains(address, length))
			return false;
		memcpy(memory.bytes.data() + (address - FakeMemory::Base), input, length);
		return true;
	}

	bool Protect(void* context, uintptr_t address, size_t, uint32_t protection) noexcept {
		auto& memory = *static_cast<FakeMemory*>(context);
		++memory.protectCalls;
		memory.events.push_back({ 'P', address, 0, protection });
		return memory.protectCalls != memory.failProtectCall;
	}

	bool Flush(void* context, uintptr_t address, size_t) noexcept {
		auto& memory = *static_cast<FakeMemory*>(context);
		memory.events.push_back({ 'F', address, 0, 0 });
		return true;
	}

	PatchTransactionOperations Operations(FakeMemory& memory) {
		memory.events.reserve(32);
		return { &memory, Read, Write, Protect, Flush };
	}

	std::array<PatchTransactionSite, 2> Sites(bool gateway = false) {
		static constexpr unsigned char originalA[] = { 1, 2 };
		static constexpr unsigned char replacementA[] = { 11, 12 };
		static constexpr unsigned char originalB[] = { 3, 4 };
		static constexpr unsigned char replacementB[] = { 13, 14 };
		return {{
			{ FakeMemory::Base, originalA, replacementA, false, gateway, false },
			{ FakeMemory::Base + 8, originalB, replacementB, false, false, false }
		}};
	}

	std::array<PatchTransactionRegion, 2> Regions() {
		return {{
			{ FakeMemory::Base, 8, 0x04, FakeMemory::Base, 8, false },
			{ FakeMemory::Base + 8, 8, 0x20, FakeMemory::Base + 8, 8, false }
		}};
	}

	void ResetBytes(FakeMemory& memory) {
		memory.bytes.fill(0);
		memory.bytes[0] = 1;
		memory.bytes[1] = 2;
		memory.bytes[8] = 3;
		memory.bytes[9] = 4;
	}
}

int main() {
	using MemUtil::PatchByteState;
	using MemUtil::PatchOutcome;
	using MemUtil::PatchResult;
	using MemUtil::ToPatchResult;
	// Current boot bundles stage at most 24 patch sites.
	static_assert(MemUtil::PreparedPatchSet::MaxSites >= 24);
	assert(!MemUtil::Detail::CanReplacePreparedRecord(PatchResult::Installed, false));
	assert(!MemUtil::Detail::CanReplacePreparedRecord(PatchResult::RollbackIncomplete, false));
	assert(!MemUtil::Detail::CanReplacePreparedRecord(PatchResult::Original, true));
	assert(MemUtil::Detail::CanReplacePreparedRecord(PatchResult::Original, false));

	assert(ToPatchResult({ PatchByteState::Replacement, true, true, false }) == PatchResult::Installed);
	assert(ToPatchResult({ PatchByteState::Replacement, true, false, false }) == PatchResult::Installed);
	assert(ToPatchResult({ PatchByteState::Original, true, true, false }) == PatchResult::Original);
	assert(ToPatchResult({ PatchByteState::Original, true, false, false }) == PatchResult::RollbackIncomplete);
	assert(ToPatchResult({ PatchByteState::Unknown, true, true, false }) == PatchResult::RollbackIncomplete);

	FakeMemory memory{};
	ResetBytes(memory);
	PatchTransactionOperations operations = Operations(memory);
	const unsigned char expected[] = { 1, 2 };
	std::array<unsigned char, 2> captured{};
	assert(MemUtil::Detail::CaptureOriginalBytes(FakeMemory::Base, expected, captured, operations));
	const unsigned char wrong[] = { 9, 2 };
	assert(!MemUtil::Detail::CaptureOriginalBytes(FakeMemory::Base, wrong, captured, operations));
	memory.failRead = true;
	assert(!MemUtil::Detail::CaptureOriginalBytes(FakeMemory::Base, {}, captured, operations));
	memory.failRead = false;
	assert(!MemUtil::Detail::CaptureOriginalBytes(FakeMemory::Base + 128, {}, captured, operations));

	ResetBytes(memory);
	memory.ResetEvents();
	operations = Operations(memory);
	auto sites = Sites(true);
	auto regions = Regions();
	uintptr_t failureAddress = 0;
	PatchOutcome outcome = MemUtil::Detail::RunPatchTransaction(sites, regions, operations,
		false, 0x40, &failureAddress);
	assert(outcome.bytes == PatchByteState::Replacement && outcome.instructionCacheFlushed
		&& outcome.protectionRestored && outcome.everPublished);
	assert(failureAddress == 0);
	assert(memory.events.size() == 8);
	assert(memory.events[0].operation == 'P' && memory.events[0].address == FakeMemory::Base
		&& memory.events[0].protection == 0x40);
	assert(memory.events[1].operation == 'P' && memory.events[1].address == FakeMemory::Base + 8
		&& memory.events[1].protection == 0x40);
	assert(memory.events[2].operation == 'W' && memory.events[2].address == FakeMemory::Base
		&& memory.events[2].value == 11);
	assert(memory.events[4].operation == 'W' && memory.events[4].address == FakeMemory::Base + 8
		&& memory.events[4].value == 13);
	assert(memory.events[6].operation == 'P' && memory.events[6].address == FakeMemory::Base + 8
		&& memory.events[6].protection == 0x20);
	assert(memory.events[7].operation == 'P' && memory.events[7].address == FakeMemory::Base
		&& memory.events[7].protection == 0x04);

	ResetBytes(memory);
	memory.ResetEvents();
	memory.failWriteCall = 2;
	operations = Operations(memory);
	sites = Sites(true);
	regions = Regions();
	outcome = MemUtil::Detail::RunPatchTransaction(sites, regions, operations,
		false, 0x40, &failureAddress);
	assert(outcome.bytes == PatchByteState::Original && outcome.instructionCacheFlushed
		&& outcome.protectionRestored && outcome.everPublished);
	assert(ToPatchResult(outcome) == PatchResult::Original);
	assert(memory.events[2].operation == 'W' && memory.events[2].address == FakeMemory::Base
		&& memory.events[2].value == 11);
	assert(memory.events[4].operation == 'W' && memory.events[4].address == FakeMemory::Base + 8
		&& memory.events[4].value == 13);
	assert(memory.events[5].operation == 'W' && memory.events[5].address == FakeMemory::Base + 8
		&& memory.events[5].value == 3);
	assert(memory.events[7].operation == 'W' && memory.events[7].address == FakeMemory::Base
		&& memory.events[7].value == 1);

	ResetBytes(memory);
	memory.ResetEvents();
	memory.failProtectCall = 2;
	operations = Operations(memory);
	sites = Sites();
	regions = Regions();
	outcome = MemUtil::Detail::RunPatchTransaction(sites, regions, operations,
		false, 0x40, &failureAddress);
	assert(outcome.bytes == PatchByteState::Original && outcome.protectionRestored);
	assert(memory.writeCalls == 0);
	assert(memory.events.size() == 3);
	assert(memory.events[2].operation == 'P' && memory.events[2].address == FakeMemory::Base
		&& memory.events[2].protection == 0x04);

	// Unique pattern search covers the whole range, including a match at its very end.
	{
		using MemUtil::Detail::FindPatternOffset;
		using MemUtil::Detail::FindUniquePatternOffset;
		const unsigned char pattern[] = { 0xAA, 0x00, 0xCC };
		const char mask[] = "x?x";
		std::array<unsigned char, 32> text{};
		text[28] = 0xAA;
		text[29] = 0x11;
		text[30] = 0xCC;
		assert(FindPatternOffset(text.data(), text.size(), 0, pattern, mask) == 28);
		assert(FindUniquePatternOffset(text.data(), text.size(), pattern, mask) == 28);
		text[29] = 0xAA;
		text[31] = 0xCC;
		assert(FindPatternOffset(text.data(), text.size(), 0, pattern, mask) == 28);
		assert(FindPatternOffset(text.data(), text.size(), 29, pattern, mask) == 29);
		assert(FindUniquePatternOffset(text.data(), text.size(), pattern, mask) == text.size());
		text.fill(0);
		text[4] = 0xAA;
		text[6] = 0xCC;
		text[29] = 0xAA;
		text[31] = 0xCC;
		assert(FindUniquePatternOffset(text.data(), text.size(), pattern, mask) == text.size());
		text[29] = 0;
		assert(FindUniquePatternOffset(text.data(), text.size(), pattern, mask) == 4);
		assert(FindUniquePatternOffset(text.data(), 2, pattern, mask) == 2);
		assert(FindPatternOffset(text.data(), text.size(), 31, pattern, mask) == text.size());
	}

	puts("Memory patch transaction core ALL PASS");
}
