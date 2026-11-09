	struct PackageVector {
		uintptr_t object = 0, begin = 0, end = 0, capacity = 0;
		size_t Count() const { return (end - begin) / sizeof(uintptr_t); }
	};
	bool ReadPackageVector(PackageVector& out) {
		__try {
			out.object = *reinterpret_cast<const uintptr_t*>(g_packages);
			if (!out.object) return false;
			const auto* vector = reinterpret_cast<const uintptr_t*>(out.object);
			out.begin = vector[0]; out.end = vector[1]; out.capacity = vector[2];
			return out.end >= out.begin && out.capacity >= out.end
				&& (out.end - out.begin) % 4 == 0 && out.Count() <= 100000;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
	}
	bool ReadPackageName(uintptr_t record, char* out, size_t& length) {
		__try {
			const uintptr_t string = record + 0x20;
			const uintptr_t data = *reinterpret_cast<const uintptr_t*>(string + 0x14) == string + 0x10
				? string : *reinterpret_cast<const uintptr_t*>(string);
			const uintptr_t end = *reinterpret_cast<const uintptr_t*>(string + 0x10);
			if (!data || end < data || end - data > 4096) return false;
			length = end - data;
			memcpy(out, reinterpret_cast<const void*>(data), length);
			out[length] = 0;
			// Match native strlen input comparison against the full stored length.
			return memchr(out, 0, length) == nullptr;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
	}
	bool ReadRecordPointer(uintptr_t slot, uintptr_t& record) {
		__try { record = *reinterpret_cast<const uintptr_t*>(slot); return record != 0; }
		__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
	}
	bool ReadQuery(const char* name, char* out) {
		__try {
			for (size_t i = 0; i <= 4096; ++i) {
				out[i] = name[i];
				if (!out[i]) return true;
			}
			return false;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) { return false; }
	}
	std::unordered_set<std::string> g_packageNames;
	PackageVector g_indexVector;
	bool g_indexValid = false;
	std::atomic<DWORD> g_indexThread{ 0 };
	std::atomic<bool> g_indexDisabled{ false };
	std::atomic<unsigned int> g_indexHits{ 0 }, g_indexMisses{ 0 }, g_indexFallbacks{ 0 };
	bool IndexThread() {
		const DWORD thread = GetCurrentThreadId();
		DWORD unset = 0;
		g_indexThread.compare_exchange_strong(unset, thread);
		if (thread != g_indexThread.load()) g_indexDisabled.store(true, std::memory_order_relaxed);
		return !g_indexDisabled.load(std::memory_order_relaxed);
	}
	bool SameVector(const PackageVector& a, const PackageVector& b) {
		return a.object == b.object && a.begin == b.begin && a.end == b.end
			&& a.capacity == b.capacity;
	}
	bool SeedIndex(const PackageVector& vector) {
		g_indexValid = false;
		g_packageNames.clear();
		char name[4097];
		for (uintptr_t slot = vector.begin; slot < vector.end; slot += 4) {
			uintptr_t record = 0;
			size_t length = 0;
			if (!ReadRecordPointer(slot, record) || !ReadPackageName(record, name, length)
				|| !g_packageNames.emplace(name, length).second) return false;
		}
		PackageVector after;
		if (!ReadPackageVector(after) || !SameVector(vector, after)) return false;
		g_indexVector = after;
		g_indexValid = true;
		return true;
	}
	int __cdecl IndexedDuplicate(const char* name) noexcept {
		try {
			PackageVector vector;
			char query[4097];
			if (IndexThread() && ReadQuery(name, query) && ReadPackageVector(vector)
				&& ((g_indexValid && SameVector(vector, g_indexVector)) || SeedIndex(vector))) {
				if (g_packageNames.find(query) != g_packageNames.end()) {
					++g_indexHits;
					return 1;
				}
				++g_indexMisses;
				return 2;
			}
		}
		catch (...) { g_indexValid = false; }
		++g_indexFallbacks;
		return 0;
	}
	void __cdecl IndexMutation(uintptr_t record, bool inserted) noexcept {
		if (!IndexThread() || !g_indexValid) return;
		try {
			PackageVector vector;
			char name[4097];
			size_t length = 0;
			const size_t expected = inserted ? g_packageNames.size() + 1 : g_packageNames.size() - 1;
			if (!ReadPackageVector(vector) || vector.object != g_indexVector.object
				|| vector.Count() != expected || !ReadPackageName(record, name, length)) {
				g_indexValid = false;
				return;
			}
			const std::string key(name, length);
			const bool changed = inserted ? g_packageNames.insert(key).second : g_packageNames.erase(key) == 1;
			g_indexValid = changed;
			g_indexVector = vector;
		}
		catch (...) { g_indexValid = false; }
	}
	void __cdecl InvalidateIndex() noexcept {
		if (IndexThread()) g_indexValid = false;
	}
#ifndef ENUMERATION_PREPARATION_TEST
	uintptr_t g_insertDone = 0, g_insertDlc = 0, g_removeDone = 0;
	uintptr_t g_teardownDone = 0, g_indexCreate = 0, g_indexHit = 0;
	void __declspec(naked) IndexedDuplicateHook() {
		__asm { pushfd }
		__asm { pushad }
		__asm { push edx }
		__asm { call IndexedDuplicate }
		__asm { add esp, 4 }
		__asm { mov [esp + 28], eax }
		__asm { popad }
		__asm { popfd }
		__asm { test eax, eax }
		__asm { jz native }
		__asm { cmp eax, 1 }
		__asm { mov eax, [g_packages] }
		__asm { mov eax, [eax] }
		__asm { push edi }
		__asm { mov edi, ecx }
		__asm { mov ecx, [eax] }
		__asm { mov esi, edx }
		__asm { mov [ebp - 30h], edi }
		__asm { mov [ebp - 2Ch], esi }
		__asm { mov [ebp - 24h], ecx }
		__asm { je hit }
		__asm { jmp dword ptr [g_indexCreate] }
	 hit:
		__asm { jmp dword ptr [g_indexHit] }
	 native:
		__asm { mov eax, [g_packages] }
		__asm { mov eax, [eax] }
		__asm { jmp dword ptr [g_duplicateBody] }
	}
	void __declspec(naked) IndexInsertHook() {
		__asm { pushfd }
		__asm { pushad }
		__asm { push 1 }
		__asm { push esi }
		__asm { call IndexMutation }
		__asm { add esp, 8 }
		__asm { popad }
		__asm { popfd }
		__asm { test bl, bl }
		__asm { jne nonDlc }
		__asm { jmp dword ptr [g_insertDlc] }
	 nonDlc:
		__asm { jmp dword ptr [g_insertDone] }
	}
	void __declspec(naked) IndexRemoveHook() {
		__asm { add dword ptr [esi + 4], -4 }
		__asm { pushfd }
		__asm { pushad }
		__asm { push 0 }
		__asm { push ebx }
		__asm { call IndexMutation }
		__asm { add esp, 8 }
		__asm { popad }
		__asm { popfd }
		__asm { test ebx, ebx }
		__asm { jmp dword ptr [g_removeDone] }
	}
	void __declspec(naked) IndexTeardownHook() {
		__asm { pushfd }
		__asm { pushad }
		__asm { call InvalidateIndex }
		__asm { popad }
		__asm { popfd }
		__asm { mov eax, [g_packages] }
		__asm { mov [eax], ebx }
		__asm { jmp dword ptr [g_teardownDone] }
	}
#endif
