#include "../stdafx.h"
#include "JsonNumberHash.hpp"
#include "../MemUtil.hpp"

namespace JsonNumberHash {
	namespace {
		bool installed = false;

		/// <summary>
		/// Replaces the game's hash, which truncates the double to an integer and so puts every value in [n, n+1) in one bucket.
		/// Equal doubles still hash equal: -0.0 is folded into 0.0 because they compare equal.
		/// </summary>
		uint32_t __cdecl HashNumber(const double* value) {
			double number = *value;
			if (number == 0.0)
				number = 0.0;

			uint64_t bits;
			memcpy(&bits, &number, sizeof(bits));
			bits ^= bits >> 33; // MurmurHash3 finalizer
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
				push eax						// const double*
				call HashNumber
				add esp, 4
				pop edx
				pop ecx
				ret								// Hash in EAX, same as the game's
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

	void Install() {
		if (installed)
			return;
		installed = true;

		CRITICAL_SECTION* lock = *reinterpret_cast<CRITICAL_SECTION**>(Offsets::ptr_jsonNumberTableLock.Get());
		uint8_t* table = *reinterpret_cast<uint8_t**>(Offsets::ptr_jsonNumberTable.Get());

		// Numbers already in the table sit in the buckets of the old hash. Swap the hash and rebuild the table without letting anyone in between.
		if (lock)
			EnterCriticalSection(lock);

		const bool hooked = MemUtil::PlaceHook(Offsets::func_jsonNumberHash, numberHashHook, 6);
		if (hooked) {
			FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::func_jsonNumberHash.Get(), 6);

			const uint32_t size = table ? *reinterpret_cast<uint32_t*>(table + 0x8) : 0;
			const uint32_t buckets = table ? *reinterpret_cast<uint32_t*>(table + 0x24) : 0;
			if (size != 0 && buckets != 0) {
				// The rehash grows the buckets 8x and reinserts every number with the current hash, once the load factor is over the max.
				// Force exactly one: half the current load triggers it, and the reinserts (which check again) see 1/8 of it.
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
	}
}
