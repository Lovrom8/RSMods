// Borrow the mounted list under the backend lock, preserving native iteration order.
// Restore the temporary sentinel in [esp+0x58] and EAX before native destruction.
	constexpr uint32_t kZeroCopyMinimum = 64;
	uintptr_t g_zeroCopyNative = 0;
	bool g_zeroCopyOn = false;
	thread_local uintptr_t t_zeroCopyDest = 0, t_zeroCopyOwn = 0;

	// PSARC opens cannot resolve inside mounted packages; omit large mounted-package lists.
	bool g_psarcListSkip = false;
	std::atomic<unsigned int> g_psarcListsSkipped{ 0 };

	bool IsPsarcPath(const char* path) {
		if (!path || MemUtil::IsBadReadPtr(const_cast<char*>(path)))
			return false;
		const size_t n = strnlen(path, 1024);
		return n > 6 && _stricmp(path + n - 6, ".psarc") == 0;
	}

	// Filter indexed PSARC misses before the walk; retain other file systems and native order.
	// Thread-local nodes contain next, prev and fs at +8.
	bool g_ownerList = false;
	std::atomic<unsigned int> g_ownerLists{ 0 }, g_ownerKept{ 0 }, g_ownerDropped{ 0 };
	struct ListNode { uintptr_t next, prev; void* fs; };
	thread_local ListNode t_ownerSentinel{};
	thread_local std::vector<ListNode> t_ownerNodes;

	// Requires the backend lock; rebuild the cached list shape when mounts or the index change.
	struct ListShape {
		uintptr_t source = 0;
		uint32_t size = 0;
		size_t indexed = 0;
		unsigned int forgets = 0;
		std::unordered_map<void*, uint32_t> position;
		std::vector<std::pair<uint32_t, void*>> always;
	};
	ListShape g_shapes[4];
	unsigned int g_shapeNext = 0;
	std::atomic<unsigned int> g_shapeBuilds{ 0 };

	// Requires shared g_candidateLock.
	ListShape* ShapeFor(uintptr_t source, uint32_t size) {
		const size_t indexed = g_candidateIndexed.size();
		const unsigned int forgets = g_candidateForgets.load(std::memory_order_relaxed);
		ListShape* shape = nullptr;
		for (ListShape& candidate : g_shapes)
			if (candidate.source == source)
				shape = &candidate;
		if (shape && shape->size == size && shape->indexed == indexed && shape->forgets == forgets)
			return shape;
		if (!shape)
			shape = &g_shapes[g_shapeNext++ % _countof(g_shapes)];
		shape->source = source;
		shape->size = size;
		shape->indexed = indexed;
		shape->forgets = forgets;
		shape->position.clear();
		shape->always.clear();
		const uintptr_t head = *reinterpret_cast<const uintptr_t*>(source + 4);
		uint32_t at = 0;
		for (uintptr_t node = *reinterpret_cast<const uintptr_t*>(head); node != head && at <= size;
			node = *reinterpret_cast<const uintptr_t*>(node), ++at) {
			void* fs = *reinterpret_cast<void* const*>(node + 8);
			shape->position.emplace(fs, at);
			if (!(fs && *static_cast<uintptr_t*>(fs) == kPsarcVtable && g_candidateIndexed.count(fs)))
				shape->always.emplace_back(at, fs);
		}
		g_shapeBuilds.fetch_add(1, std::memory_order_relaxed);
		if (at != size) {
			shape->source = 0;
			return nullptr;
		}
		return shape;
	}

	bool BuildOwnerList(uintptr_t source, uint32_t size, const char* subpath) {
		if (!subpath || MemUtil::IsBadReadPtr(const_cast<char*>(subpath)))
			return false;
		const size_t length = strnlen(subpath, kCandidateNameMax);
		if (!length || length >= kCandidateNameMax)
			return false;
		std::vector<std::pair<uint32_t, void*>> kept;
		AcquireSRWLockShared(&g_candidateLock);
		ListShape* shape = ShapeFor(source, size);
		if (!shape) {
			ReleaseSRWLockShared(&g_candidateLock);
			return false;
		}
		kept = shape->always;
		for (int flag = 0; flag < 2; ++flag) {
			char name[kCandidateNameMax];
			memcpy(name, subpath, length + 1);
			CallNormalise(name, flag);
			CandidateMd5 md5;
			CallHash(&md5, name, static_cast<int>(strlen(name)));
			const auto found = g_candidateByMd5.find(md5);
			if (found == g_candidateByMd5.end())
				continue;
			for (void* fs : found->second) {
				const auto toc = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(fs) + 0x58);
				const int fsFlag = toc ? (*reinterpret_cast<unsigned char*>(toc + 0x1C) & 1) : 0;
				const auto at = shape->position.find(fs);
				if (fsFlag == flag && at != shape->position.end())
					kept.emplace_back(at->second, fs);
			}
		}
		ReleaseSRWLockShared(&g_candidateLock);
		if (kept.size() >= size)
			return false;
		// Preserve native list order.
		std::sort(kept.begin(), kept.end());
		t_ownerNodes.resize(kept.size());
		const uintptr_t sentinel = reinterpret_cast<uintptr_t>(&t_ownerSentinel);
		for (size_t i = 0; i < kept.size(); ++i) {
			t_ownerNodes[i].fs = kept[i].second;
			t_ownerNodes[i].next = i + 1 < kept.size() ? reinterpret_cast<uintptr_t>(&t_ownerNodes[i + 1]) : sentinel;
			t_ownerNodes[i].prev = i ? reinterpret_cast<uintptr_t>(&t_ownerNodes[i - 1]) : sentinel;
		}
		t_ownerSentinel.next = kept.empty() ? sentinel : reinterpret_cast<uintptr_t>(&t_ownerNodes.front());
		t_ownerSentinel.prev = kept.empty() ? sentinel : reinterpret_cast<uintptr_t>(&t_ownerNodes.back());
		g_ownerLists.fetch_add(1, std::memory_order_relaxed);
		g_ownerKept.fetch_add(static_cast<unsigned int>(kept.size()), std::memory_order_relaxed);
		g_ownerDropped.fetch_add(size - static_cast<unsigned int>(kept.size()), std::memory_order_relaxed);
		return true;
	}

	int __cdecl ZeroCopyBegin(uintptr_t source, uintptr_t dest, const char* path, const char* subpath) {
		if (t_zeroCopyDest)
			return 0;
		const uint32_t size = *reinterpret_cast<const uint32_t*>(source + 8);
		const uintptr_t sourceHead = *reinterpret_cast<const uintptr_t*>(source + 4);
		if (!g_zeroCopyOn || size < kZeroCopyMinimum || !sourceHead) {
			return 0;
		}
		if (g_psarcListSkip && IsPsarcPath(path)) {
			// Retain the empty sentinel so native destruction owns only its original list.
			g_psarcListsSkipped.fetch_add(1, std::memory_order_relaxed);
			return 1;
		}
		t_zeroCopyDest = dest;
		t_zeroCopyOwn = *reinterpret_cast<const uintptr_t*>(dest + 4);
		if (g_ownerList && BuildOwnerList(source, size, subpath)) {
			*reinterpret_cast<uintptr_t*>(dest + 4) = reinterpret_cast<uintptr_t>(&t_ownerSentinel);
			return 1;
		}
		*reinterpret_cast<uintptr_t*>(dest + 4) = sourceHead;
		return 1;
	}

	// Zero denotes a native copy; otherwise restore the temporary sentinel.
	uintptr_t __cdecl ZeroCopyEnd() {
		if (!t_zeroCopyDest)
			return 0;
		const uintptr_t own = t_zeroCopyOwn;
		*reinterpret_cast<uintptr_t*>(t_zeroCopyDest + 4) = own;
		t_zeroCopyDest = 0;
		t_zeroCopyOwn = 0;
		return own;
	}

	// 0x89DB30: ECX = source container, [esp+4] = temporary.
	void __declspec(naked) ZeroCopyHook() {
		__asm {
			// Caller ESP is current ESP +8; its subpath local is at caller ESP +0x14.
			push ecx
			push dword ptr [esp + 4 + 8 + 14h]
			push dword ptr [ebp + 8]
			push [esp + 0x10]
			push ecx
			call ZeroCopyBegin
			add esp, 0x10
			pop ecx
			test eax, eax
			jnz taken
			jmp dword ptr [g_zeroCopyNative]
		taken:
			ret 4
		}
	}

	// 0x89DB8D replaces mov ecx,[esp+0x64] and cmp ecx,ebx; preserve comparison flags.
	void __declspec(naked) ZeroCopyEndHook() {
		__asm {
			push eax
			push ecx
			push edx
			call ZeroCopyEnd
			pop edx
			pop ecx
			test eax, eax
			jz notTaken
			add esp, 4
			jmp tail
		notTaken:
			pop eax
		tail:
			mov ecx, [esp + 4 + 64h]
			cmp ecx, ebx
			ret
		}
	}

	bool AddZeroCopyPatches(std::vector<Patch>& patches) {
		static const unsigned char copyCall[] = { 0x8D, 0x44, 0x24, 0x54, 0x50, 0x8D, 0x4E, 0x30 };
		static const unsigned char afterLoop[] = { 0x8B, 0x4C, 0x24, 0x64, 0x3B, 0xCB, 0x74, 0x0E };
		static const unsigned char destroy[] = { 0x8B, 0x30, 0x89, 0x00, 0x8B, 0x44, 0x24, 0x58 };
		static const unsigned char copyHead[] = { 0x55, 0x8B, 0xEC, 0x6A, 0xFF };
		// Shared patch span: Backend Open +0xCC..+0x15F.
		const uintptr_t open = Offsets::func_vfsBackendOpen, copy = Offsets::func_vfsCopyContainer;
		kPsarcVtable = Offsets::ptr_psarcVtable;
		if (!open || !copy || !kPsarcVtable) return false;
		if (!MemUtil::MatchesBytes(open + 0xCC, copyCall) || !MemUtil::MatchesBytes(open + 0x13D, afterLoop)
			|| !MemUtil::MatchesBytes(open + 0x15F, destroy) || !MemUtil::MatchesBytes(copy, copyHead))
			return false;
		g_zeroCopyOn = true;
		g_psarcListSkip = true;
		g_ownerList = true;
		g_zeroCopyNative = copy;
		patches.push_back(Branch(open + 0xE0, copy, &ZeroCopyHook, 0xE8));
		patches.push_back(Branch(open + 0x13D, 0, &ZeroCopyEndHook, 0xE8, { 0x8B, 0x4C, 0x24, 0x64, 0x3B, 0xCB }));
		return true;
	}

	void LogZeroCopy() {
	}
