// Memoize normalized-name MD5 in a thread-local stand-in node and suppress its release.
	struct LocateNode {
		const void* magic;
		unsigned char pad[8];
		unsigned char md5[16];
		unsigned int refs;
	};
	static_assert(offsetof(LocateNode, md5) == 0xC && sizeof(LocateNode) == 0x20, "intern node layout");
	const int g_locateMagic = 0;
	constexpr size_t kLocateMax = 512;
	thread_local LocateNode g_locateNode{ &g_locateMagic, {}, {}, 1 };
	thread_local char g_locateName[kLocateMax];
	thread_local size_t g_locateSize = ~size_t(0);
	uintptr_t g_locateIntern = 0, g_locateRelease = 0;
	std::atomic<unsigned int> g_locateHits{ 0 }, g_locateMisses{ 0 };

	LocateNode* __cdecl LocateIntern(const char* name) {
		const size_t size = strlen(name);
		if (size < kLocateMax && size == g_locateSize && memcmp(name, g_locateName, size) == 0) {
			return &g_locateNode;
		}
		CallHash(g_locateNode.md5, name, static_cast<int>(size));
		if (size < kLocateMax) {
			memcpy(g_locateName, name, size);
			g_locateSize = size;
		}
		else g_locateSize = ~size_t(0);
		return &g_locateNode;
	}

	// 0x8A44A9: ECX = -1 for strlen; name on stack, caller pops.
	void __declspec(naked) LocateInternHook() {
		__asm {
			cmp ecx, -1
			jne native
			push [esp + 4]
			call LocateIntern
			add esp, 4
			ret
		native:
			jmp dword ptr [g_locateIntern]
		}
	}

	// 0x8A44EA: EDI = node; real nodes cannot point to the static data buffer.
	void __declspec(naked) LocateReleaseHook() {
		__asm {
			cmp dword ptr [edi], offset g_locateMagic
			jne native
			ret
		native:
			jmp dword ptr [g_locateRelease]
		}
	}

	bool AddLocatePatches(std::vector<Patch>& patches) {
		// Between hooks only node MD5 is read; the native find computes the result.
		static const unsigned char beforeIntern[] = { 0x57, 0x83, 0xC9, 0xFF };
		static const unsigned char readMd5[] = { 0x8B, 0x4F, 0x10, 0x8B, 0x47, 0x0C, 0x8B, 0x57, 0x14, 0x83, 0xC4, 0x08,
			0x89, 0x45, 0xEC, 0x8B, 0x47, 0x18 };
		static const unsigned char afterRelease[] = { 0x8B, 0xC6, 0x8D, 0x65, 0xDC };
		static const unsigned char internHead[] = { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x83, 0xEC, 0x34 };
		static const unsigned char internHash[] = { 0x57, 0x8B, 0xD3, 0x8D, 0x4C, 0x24, 0x30, 0xE8 };
		// Shared spans: LocateTOCEntry +0x65..+0xAF, interner +0x31.
		const uintptr_t locate = Offsets::func_psarcLocate, intern = Offsets::enumStageIntern;
		if (!locate || !intern || !Offsets::func_enumInternRelease || !Offsets::func_enumNameHash) return false;
		if (!MemUtil::MatchesBytes(locate + 0x65, beforeIntern) || !MemUtil::MatchesBytes(locate + 0x70, readMd5)
			|| !MemUtil::MatchesBytes(locate + 0xAF, afterRelease) || !MemUtil::MatchesBytes(intern, internHead)
			|| !MemUtil::MatchesBytes(intern + 0x31, internHash) || hashFunc != Offsets::func_enumNameHash)
			return false;
		g_locateIntern = intern;
		g_locateRelease = Offsets::func_enumInternRelease;
		patches.push_back(Branch(locate + 0x69, g_locateIntern, &LocateInternHook, 0xE8));
		patches.push_back(Branch(locate + 0xAA, g_locateRelease, &LocateReleaseHook, 0xE8));
		return true;
	}
