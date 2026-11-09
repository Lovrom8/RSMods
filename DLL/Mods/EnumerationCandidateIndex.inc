// Discard impossible PSARC candidates after the native sort to preserve its unstable tie order.
	struct CandidateEntry {
		void* fs;
		const char* subpath;
	};
	struct CandidateMd5 {
		uint32_t w[4];
		bool operator==(const CandidateMd5& other) const { return memcmp(w, other.w, sizeof(w)) == 0; }
	};
	struct CandidateMd5Hash {
		size_t operator()(const CandidateMd5& md5) const { return md5.w[0] ^ md5.w[1] ^ md5.w[2] ^ md5.w[3]; }
	};
	struct CandidateFs {
		uintptr_t head;
		uint32_t size;
	};
	// Initialized by AddCandidatePatches before hooks run.
	uintptr_t kPsarcVtable = 0;
	uintptr_t kPsarcNormalise = 0;
	constexpr size_t kCandidateNameMax = 1024;
	SRWLOCK g_candidateLock = SRWLOCK_INIT;
	std::unordered_map<CandidateMd5, std::vector<void*>, CandidateMd5Hash> g_candidateByMd5;
	std::unordered_map<void*, CandidateFs> g_candidateIndexed;
	uintptr_t g_candidateDestructor = 0;
	std::atomic<unsigned int> g_candidateOpens{ 0 }, g_candidateKept{ 0 }, g_candidateDropped{ 0 },
		g_candidateIndexedFs{ 0 }, g_candidateEntries{ 0 };

	// Match native slash conversion and optional CRT lowercasing.
	void __declspec(naked) __cdecl CallNormalise(char*, int) {
		__asm {
			push ebx
			push esi
			push edi
			mov eax, [esp + 16]
			push [esp + 20]
			call dword ptr [kPsarcNormalise]
			add esp, 4
			pop edi
			pop esi
			pop ebx
			ret
		}
	}

	using PriorityFn = int(__thiscall*)(void*);
	int CandidatePriority(void* fs) {
		const auto table = *static_cast<uintptr_t**>(fs);
		return reinterpret_cast<PriorityFn>(table[3])(fs);
	}

	// TOC +0x74: head +0x78, size +0x7C; nodes contain next, prev, MD5 and FileEntry*.
	void CandidateForgetLocked(void* fs) {
		const auto found = g_candidateIndexed.find(fs);
		if (found == g_candidateIndexed.end()) return;
		const uintptr_t head = found->second.head;
		for (uintptr_t node = *reinterpret_cast<uintptr_t*>(head); node != head; node = *reinterpret_cast<uintptr_t*>(node)) {
			CandidateMd5 md5;
			memcpy(&md5, reinterpret_cast<void*>(node + 8), sizeof(md5));
			const auto bucket = g_candidateByMd5.find(md5);
			if (bucket == g_candidateByMd5.end()) continue;
			auto& owners = bucket->second;
			owners.erase(std::remove(owners.begin(), owners.end(), fs), owners.end());
			if (owners.empty()) g_candidateByMd5.erase(bucket);
		}
		g_candidateIndexed.erase(found);
	}

	// Keep unindexable file systems as native candidates.
	bool CandidateIndexLocked(void* fs) {
		const auto base = reinterpret_cast<uintptr_t>(fs);
		if (!*reinterpret_cast<void**>(base + 0x58)) return false;
		const uintptr_t head = *reinterpret_cast<uintptr_t*>(base + 0x78);
		const uint32_t size = *reinterpret_cast<uint32_t*>(base + 0x7C);
		const auto known = g_candidateIndexed.find(fs);
		if (known != g_candidateIndexed.end()) {
			if (known->second.head == head && known->second.size == size) return true;
			CandidateForgetLocked(fs);
		}
		if (!head) return false;
		uint32_t count = 0;
		for (uintptr_t node = *reinterpret_cast<uintptr_t*>(head); node != head && count <= size; node = *reinterpret_cast<uintptr_t*>(node))
			++count;
		if (count != size) return false;
		for (uintptr_t node = *reinterpret_cast<uintptr_t*>(head); node != head; node = *reinterpret_cast<uintptr_t*>(node)) {
			// An indexed miss is definitive; native Exists must still resolve hits.
			CandidateMd5 md5;
			memcpy(&md5, reinterpret_cast<void*>(node + 8), sizeof(md5));
			g_candidateByMd5[md5].push_back(fs);
		}
		g_candidateIndexed[fs] = { head, size };
		return true;
	}

	std::atomic<unsigned int> g_candidateForgets{ 0 };

	void __cdecl CandidateForget(void* fs) {
		AcquireSRWLockExclusive(&g_candidateLock);
		CandidateForgetLocked(fs);
		g_candidateForgets.fetch_add(1, std::memory_order_relaxed);
		ReleaseSRWLockExclusive(&g_candidateLock);
	}

	// Requires the backend lock; runs after sort and before winner selection.
	void __cdecl CandidateCompact(CandidateEntry* begin, CandidateEntry** endSlot) {
		CandidateEntry* const end = *endSlot;
		if (!begin || end - begin < 2) return;
		struct Query { const char* subpath; int flag; const std::vector<void*>* owners; };
		Query queries[8];
		size_t queryCount = 0;
		const auto isPsarc = [](const CandidateEntry& entry) {
			return entry.fs && entry.subpath && *static_cast<uintptr_t*>(entry.fs) == kPsarcVtable;
		};
		CandidateEntry* out = begin;
		AcquireSRWLockExclusive(&g_candidateLock);
		// Finish indexing before retaining pointers into the map.
		bool indexed[2048];
		const size_t count = static_cast<size_t>(end - begin);
		if (count > _countof(indexed)) {
			ReleaseSRWLockExclusive(&g_candidateLock);
			return;
		}
		for (size_t i = 0; i < count; ++i)
			indexed[i] = isPsarc(begin[i]) && CandidateIndexLocked(begin[i].fs);
		for (size_t i = 0; i < count; ++i) {
			const CandidateEntry entry = begin[i];
			bool keep = true;
			if (indexed[i]) {
				const auto toc = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(entry.fs) + 0x58);
				const int flag = *reinterpret_cast<unsigned char*>(toc + 0x1C) & 1;
				const Query* query = nullptr;
				for (size_t q = 0; q < queryCount && !query; ++q)
					if (queries[q].subpath == entry.subpath && queries[q].flag == flag) query = &queries[q];
				if (!query) {
					const size_t size = strlen(entry.subpath);
					if (size >= kCandidateNameMax || queryCount == _countof(queries)) {
						// Preserve the remaining candidates when a path cannot be indexed.
						for (; i < count; ++i) *out++ = begin[i];
						break;
					}
					char name[kCandidateNameMax];
					memcpy(name, entry.subpath, size + 1);
					CallNormalise(name, flag);
					CandidateMd5 md5;
					CallHash(&md5, name, static_cast<int>(strlen(name)));
					const auto found = g_candidateByMd5.find(md5);
					queries[queryCount] = { entry.subpath, flag, found == g_candidateByMd5.end() ? nullptr : &found->second };
					query = &queries[queryCount++];
				}
				keep = query->owners && std::find(query->owners->begin(), query->owners->end(), entry.fs) != query->owners->end();
			}
			if (keep) *out++ = entry;
		}
		ReleaseSRWLockExclusive(&g_candidateLock);
		*endSlot = out;
	}

	// At 0x89DD56 the vector end is [esp+0x50], then [esp+0x40] after sort arguments pop.
	// Update both locations before the winner loop.
	void __declspec(naked) CandidateHook() {
		__asm {
			pushfd
			pushad
			lea eax, [esp + 36 + 4 + 50h]
			push eax
			push esi
			call CandidateCompact
			add esp, 8
			popad
			popfd
			mov edx, [esp + 4 + 50h]
			xor edi, edi
			ret
		}
	}

	// Slot 0 is the scalar deleting destructor; its TOC list is still valid on entry.
	void __declspec(naked) CandidateDestructorHook() {
		__asm {
			pushfd
			pushad
			push ecx
			call CandidateForget
			add esp, 4
			popad
			popfd
			jmp dword ptr [g_candidateDestructor]
		}
	}

	bool AddCandidatePatches(std::vector<Patch>& patches) {
		static const unsigned char loopEnd[] = { 0x83, 0xC6, 0x08, 0x3B, 0x74, 0x24, 0x40 };
		static const unsigned char pushEntry[] = { 0x8B, 0x4E, 0x08, 0x8B, 0x54, 0x24, 0x14, 0x89, 0x4C, 0x24, 0x2C,
			0x89, 0x54, 0x24, 0x30 };
		static const unsigned char exists[] = { 0x55, 0x8B, 0xEC, 0x51, 0x8B, 0x4D, 0x08, 0xE8, 0x04, 0x04, 0x00, 0x00,
			0xF7, 0xD8, 0x1B, 0xC0, 0xF7, 0xD8 };
		static const unsigned char locate[] = { 0x83, 0x7B, 0x58, 0x00 };
		static const unsigned char normalise[] = { 0x55, 0x8B, 0xEC, 0x56, 0x57, 0x8B, 0xF8 };
		// Shared patch spans: Backend Open +0xF0..+0x382, LocateTOCEntry +0x14..+0x93.
		const uintptr_t open = Offsets::func_vfsBackendOpen, psarcLocate = Offsets::func_psarcLocate;
		const uintptr_t vtable = Offsets::ptr_psarcVtable, normaliseAt = Offsets::func_psarcNormalise;
		if (!open || !psarcLocate || !vtable || !normaliseAt || !Offsets::func_psarcDestructor || !Offsets::func_psarcExists
			|| !Offsets::func_vfsSortPredicate || !Offsets::func_vfsSort || !Offsets::func_psarcTocFind
			|| !Offsets::func_enumNameHash) return false;
		const uintptr_t beforeSortAt = open + 0x2EA, afterSortAt = open + 0x301;
		const auto beforeSort = Span(beforeSortAt, { 0x8B, 0x44, 0x24, 0x40, 0x8B, 0x74, 0x24, 0x3C, 0x8B, 0xC8, 0x2B, 0xCE,
			0x68, 0, 0, 0, 0 }, { Abs(13, Offsets::func_vfsSortPredicate) });
		const auto afterSort = Span(afterSortAt, { 0xE8, 0, 0, 0, 0, 0x8B, 0x54, 0x24, 0x50, 0x33, 0xFF, 0x83, 0xC4, 0x10 },
			{ Rel(1, Offsets::func_vfsSort) });
		const auto locateNormalise = Span(psarcLocate + 0x51, { 0x8B, 0x43, 0x58, 0x8A, 0x48, 0x1C, 0x80, 0xE1, 0x01, 0x0F,
			0xB6, 0xD1, 0x52, 0x8B, 0xC7, 0xE8, 0, 0, 0, 0 }, { Rel(16, normaliseAt) });
		const auto locateFind = Span(psarcLocate + 0x93, { 0x8D, 0x43, 0x74, 0xE8, 0, 0, 0, 0, 0x8B, 0x45, 0xE8,
			0x3B, 0x43, 0x78, 0x74, 0x05, 0x8B, 0x70, 0x18 }, { Rel(4, Offsets::func_psarcTocFind) });
		const auto destructorSlot = Span(vtable, { 0, 0, 0, 0 }, { Abs(0, Offsets::func_psarcDestructor) });
		const auto existsSlot = Span(vtable + 0x18, { 0, 0, 0, 0 }, { Abs(0, Offsets::func_psarcExists) });
		if (!MemUtil::MatchesBytes(beforeSortAt, beforeSort) || !MemUtil::MatchesBytes(afterSortAt, afterSort)
			|| !MemUtil::MatchesBytes(open + 0x382, loopEnd) || !MemUtil::MatchesBytes(open + 0xF0, pushEntry)
			|| !MemUtil::MatchesBytes(Offsets::func_psarcExists, exists) || !MemUtil::MatchesBytes(psarcLocate + 0x14, locate)
			|| !MemUtil::MatchesBytes(psarcLocate + 0x51, locateNormalise) || !MemUtil::MatchesBytes(psarcLocate + 0x93, locateFind)
			|| !MemUtil::MatchesBytes(normaliseAt, normalise) || !MemUtil::MatchesBytes(vtable + 0x18, existsSlot)
			|| hashFunc != Offsets::func_enumNameHash)
			return false;
		kPsarcVtable = vtable;
		kPsarcNormalise = normaliseAt;
		g_candidateDestructor = Offsets::func_psarcDestructor;
		patches.push_back(Branch(open + 0x306, 0, &CandidateHook, 0xE8, { 0x8B, 0x54, 0x24, 0x50, 0x33, 0xFF }));
		const auto hook = reinterpret_cast<uintptr_t>(&CandidateDestructorHook);
		std::vector<unsigned char> slot(sizeof(hook));
		memcpy(slot.data(), &hook, sizeof(hook));
		patches.push_back({ vtable, destructorSlot, std::move(slot) });
		return true;
	}
