#include "../stdafx.h"
#include "JsonNumberHash.hpp"
#include "../MemUtil.hpp"

namespace JsonNumberHash {
	namespace {
		bool installed = false;

		// Hash fractional bits to avoid integer-truncation collisions; fold -0.0 into 0.0 for equality.
		uint32_t __cdecl HashNumber(const double* value) {
			double number = *value;
			if (number == 0.0)
				number = 0.0;

			uint64_t bits;
			memcpy(&bits, &number, sizeof(bits));
			// MurmurHash3 finalizer.
			bits ^= bits >> 33;
			bits *= 0xFF51AFD7ED558CCDull;
			bits ^= bits >> 33;
			bits *= 0xC4CEB9FE1A85EC53ull;
			bits ^= bits >> 33;
			return static_cast<uint32_t>(bits);
		}

		void __declspec(naked) numberHashHook() {
			__asm {
				push ecx
				push edx
				push eax
				call HashNumber
				add esp, 4
				pop edx
				pop ecx
				ret
			}
		}

		void RehashNumberTable(void* table) {
			const uintptr_t rehash = Offsets::func_jsonNumberTableRehash.Get();
			__asm {
				push edi
				mov edi, table
				call rehash
				pop edi
			}
		}
	}

	bool Install() {
		static bool hookedOnce = false;
		if (installed)
			return hookedOnce;
		installed = true;

		constexpr unsigned char expected[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x10 };
		if (!MemUtil::MatchesBytes(Offsets::func_jsonNumberHash.Get(), expected)) {
			LOG_ERROR("(JSON) JSON number hash site did not match the reviewed bytes, leaving the game's hash" << std::endl);
			return false;
		}

		CRITICAL_SECTION* lock = *reinterpret_cast<CRITICAL_SECTION**>(Offsets::ptr_jsonNumberTableLock.Get());
		uint8_t* table = *reinterpret_cast<uint8_t**>(Offsets::ptr_jsonNumberTable.Get());

		// Replace the hash and rehash existing entries atomically under the table lock.
		if (lock)
			EnterCriticalSection(lock);

		const bool hooked = MemUtil::PlaceHook(Offsets::func_jsonNumberHash, numberHashHook, 6);
		if (hooked) {
			FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::func_jsonNumberHash.Get(), 6);

			const uint32_t size = table ? *reinterpret_cast<uint32_t*>(table + 0x8) : 0;
			const uint32_t buckets = table ? *reinterpret_cast<uint32_t*>(table + 0x24) : 0;
			if (size != 0 && buckets != 0) {
				// Halve the current load threshold to trigger one rehash; eightfold growth prevents recursion.
				float& maxLoadFactor = *reinterpret_cast<float*>(table + 0x28);
				const float savedMaxLoadFactor = maxLoadFactor;
				maxLoadFactor = 0.5f * static_cast<float>(size) / static_cast<float>(buckets);
				RehashNumberTable(table);
				maxLoadFactor = savedMaxLoadFactor;
			}
		}

		if (lock)
			LeaveCriticalSection(lock);

		if (hooked)
			LOG_INFO("(JSON) Replaced the JSON number hash" << std::endl);
		else
			LOG_ERROR("(JSON) Failed to replace the JSON number hash" << std::endl);
		hookedOnce = hooked;
		return hooked;
	}
}
