// Restore cached mapper references through native insertion primitives and lock order.
// Unsafe or mismatched records fall back to native derivation per entry.
#ifndef ENUMERATION_PREPARATION_TEST
namespace MapperRestore {
	enum class Mode { off, native, check, restore };
	Mode mode = Mode::off;
	bool armed = false;
	const char* armedReason = "not requested";
	// A failed read-back disables restoration for the rest of the process.
	bool disabledForRun = false;
	bool digestEnabled = false;
	bool verifyReadback = false;
	bool targetConfigured = false;
	bool hookPrepared = false;
	std::atomic<DWORD> sessionThread{ 0 };
	uintptr_t g_native = 0;

	constexpr uintptr_t kMapperGlobal = 0x138AB70;
	constexpr uintptr_t kLockEnterSlot = 0x10B707C, kLockLeaveSlot = 0x10B7080, kThreadIdSlot = 0x10B7078;
	constexpr uintptr_t kInsertUnique = 0x8FB6F0, kConstructRange = 0x4079D0, kPushDir = 0x417620;
	constexpr uintptr_t kRefCtor = 0x49BF70, kCopyRef = 0x7164C0, kFreeRef = 0x4859E0, kGrow = 0x49E8C0;
	// MRF2 magic.
	constexpr uint32_t kMagic = 0x3246524D;
	constexpr uint8_t kUnsafe = 0, kRegistered = 1, kDuplicate = 2;
	uintptr_t g_fnInsert = kInsertUnique, g_fnConstruct = kConstructRange, g_fnPushDir = kPushDir;
	uintptr_t g_fnRefCtor = kRefCtor, g_fnCopyRef = kCopyRef, g_fnFreeRef = kFreeRef, g_fnGrow = kGrow;

	struct Entry {
		uint8_t flags = kUnsafe;
		uint64_t pathHash = 0;
		uint32_t llid = 0, nameKey = 0, dirKey = 0;
		std::string dir, fname, ext;
	};

	struct Stats {
		unsigned int packages = 0, packagesRecorded = 0, packagesRestored = 0, packagesStale = 0, packagesFaulted = 0;
		unsigned int entries = 0, restored = 0, nativeCalls = 0, duplicates = 0, declined = 0, unsafe = 0;
		unsigned int recordMismatch = 0, checkOk = 0, checkMismatch = 0, pathMismatch = 0, readbackFail = 0, unobserved = 0;
		unsigned int observedNew = 0, observedDup = 0;
		uint64_t digest = 0;
		long long restoreTicks = 0, nativeTicks = 0, overheadTicks = 0;
		unsigned int mismatchLogs = 0;
	} stats;
	enum Prim { pLock, pUnlock, pConstruct, pPushDir, pRefCtor, pCopyRef, pFreeRef, pGrow, pInsName, pInsLlid, pInsDir, pReadback, pObserve, pDupFind, pDestroy };
	long long Counter() {
		LARGE_INTEGER now{};
		QueryPerformanceCounter(&now);
		return now.QuadPart;
	}

	struct PrimTimer {
		explicit PrimTimer(int) {}
	};

	struct Session {
		bool open = false, haveSaved = false, recording = false, sawFirst = false, faulted = false;
		std::string key;
		std::vector<Entry> saved, recorded;
		size_t index = 0;
		uint32_t expected = 0;
		uint64_t indexHash = 0;
		DWORD ownerThread = 0;
	} session;

	void Put32(std::vector<unsigned char>& out, uint32_t v) { EnumerationCache::Format::Put32(out, v); }
	void Put64(std::vector<unsigned char>& out, uint64_t v) { EnumerationCache::Format::Put64(out, v); }
	void PutStr(std::vector<unsigned char>& out, const std::string& s) {
		const uint16_t n = static_cast<uint16_t>(s.size());
		out.insert(out.end(), reinterpret_cast<const unsigned char*>(&n), reinterpret_cast<const unsigned char*>(&n) + 2);
		out.insert(out.end(), s.begin(), s.end());
	}
	std::vector<unsigned char> Pack(const std::vector<Entry>& entries) {
		std::vector<unsigned char> out;
		Put32(out, kMagic);
		Put32(out, static_cast<uint32_t>(entries.size()));
		for (const auto& e : entries) {
			out.push_back(e.flags);
			Put64(out, e.pathHash);
			if (e.flags == kUnsafe) continue;
			Put32(out, e.llid);
			Put32(out, e.nameKey);
			Put32(out, e.dirKey);
			if (e.flags == kRegistered) {
				PutStr(out, e.dir);
				PutStr(out, e.fname);
				PutStr(out, e.ext);
			}
		}
		return out;
	}
	bool GetStr(EnumerationCache::Format::Reader& in, std::string& s) {
		if (in.size - in.at < 2) return false;
		uint16_t n = 0;
		std::memcpy(&n, in.data + in.at, 2);
		in.at += 2;
		if (in.size - in.at < n || n > 1100) return false;
		s.assign(reinterpret_cast<const char*>(in.data + in.at), n);
		in.at += n;
		return true;
	}
	bool Unpack(const std::vector<unsigned char>& blob, std::vector<Entry>& entries) {
		entries.clear();
		EnumerationCache::Format::Reader in{ blob.data(), blob.size() };
		uint32_t magic = 0, count = 0;
		if (!in.Get32(magic) || magic != kMagic || !in.Get32(count) || count > 200000) return false;
		entries.resize(count);
		for (auto& e : entries) {
			if (in.size - in.at < 1) return false;
			e.flags = in.data[in.at++];
			if (e.flags > kDuplicate || !in.Get64(e.pathHash)) return false;
			if (e.flags == kUnsafe) continue;
			if (!in.Get32(e.llid) || !in.Get32(e.nameKey) || !in.Get32(e.dirKey)) return false;
			if (e.flags == kRegistered && (!GetStr(in, e.dir) || !GetStr(in, e.fname) || !GetStr(in, e.ext))) return false;
		}
		return in.at == blob.size();
	}

	uint64_t PathHash(const char* path, size_t size) {
		return EnumerationPackage::Fnv64(reinterpret_cast<const unsigned char*>(path), size);
	}

	// Unsupported path shapes are saved as unsafe and always use native derivation.
	bool Derive(const char* prefix, const char* path, size_t n, Entry& e) {
		if (std::strcmp(prefix, "data") != 0 || n == 0 || n > 900) return false;
		for (size_t i = 0; i < n; ++i) {
			const unsigned char c = static_cast<unsigned char>(path[i]);
			if (c < 0x20 || c >= 0x7F || c == '\\' || c == ':') return false;
		}
		const std::string formatted = std::string(prefix) + "/" + path;
		const size_t slash = formatted.rfind('/');
		e.dir = formatted.substr(0, slash + 1);
		const std::string base = formatted.substr(slash + 1);
		if (base.empty()) return false;
		const size_t dot = base.rfind('.');
		e.fname = dot == std::string::npos ? base : base.substr(0, dot);
		e.ext = dot == std::string::npos ? std::string() : base.substr(dot);
		const auto* b = reinterpret_cast<const unsigned char*>(base.data());
		const auto* d = reinterpret_cast<const unsigned char*>(e.dir.data());
		return EnumerationMapperHash::Ascii(b, base.size(), e.nameKey)
			&& EnumerationMapperHash::Ascii(d, e.dir.size(), e.dirKey)
			&& EnumerationLlid::Compute(path, n, e.llid);
	}

	uintptr_t Mapper() { return *reinterpret_cast<const uintptr_t*>(kMapperGlobal); }
	uint32_t Read32(uintptr_t at) { return *reinterpret_cast<const uint32_t*>(at); }

	// STLport tree header: root +4, left +8, right +0xC; node key +0x10, value +0x14.
	// Use native lower_bound-then-equal lookup semantics.
	bool TreeFind(uintptr_t tree, uint32_t key, uint32_t* value) {
		uintptr_t node = Read32(tree + 4), best = tree;
		while (node) {
			if (Read32(node + 0x10) < key) node = Read32(node + 0xC);
			else { best = node; node = Read32(node + 8); }
		}
		if (best == tree || key < Read32(best + 0x10)) return false;
		if (value) *value = Read32(best + 0x14);
		return true;
	}

	bool StrView(uintptr_t at, std::string_view& out) {
		const uintptr_t finish = Read32(at + 0x10), storage = Read32(at + 0x14);
		const uintptr_t data = storage == at + 0x10 ? at : Read32(at);
		if (!data || finish < data || finish - data > 4096) return false;
		out = std::string_view(reinterpret_cast<const char*>(data), finish - data);
		return true;
	}

	struct Live { std::string_view dir, fname, ext; uint32_t index = 0, dirIndex = 0; };
	bool ReadLive(uintptr_t m, uint32_t nameKey, Live& live) {
		if (!TreeFind(m + 0x90, nameKey, &live.index)) return false;
		const uintptr_t begin = Read32(m + 0xC0), end = Read32(m + 0xC4);
		if (end < begin || live.index >= (end - begin) / 0x34) return false;
		const uintptr_t element = begin + static_cast<uintptr_t>(live.index) * 0x34;
		live.dirIndex = Read32(element);
		const uintptr_t dirBegin = Read32(m + 0xCC), dirEnd = Read32(m + 0xD0);
		if (dirEnd < dirBegin || live.dirIndex >= (dirEnd - dirBegin) / 0x18) return false;
		return StrView(element + 4, live.fname) && StrView(element + 0x1C, live.ext)
			&& StrView(dirBegin + static_cast<uintptr_t>(live.dirIndex) * 0x18, live.dir);
	}

	// existedBefore records the name-key lookup before the native call.
	bool VerifyLive(uintptr_t m, const Entry& e, bool existedBefore) {
		if (e.flags == kUnsafe) return true;
		if (e.flags == kDuplicate) return existedBefore;
		if (existedBefore) return false;
		Live live;
		uint32_t value = 0;
		return ReadLive(m, e.nameKey, live) && live.fname == e.fname && live.ext == e.ext && live.dir == e.dir
			&& TreeFind(m + 0x78, e.llid, &value) && value == live.index
			&& TreeFind(m + 0xA8, e.dirKey, &value) && value == live.dirIndex;
	}

	void Observe(uintptr_t m, const Entry& e, bool existedBefore) {
		Live live;
		if (e.flags == kUnsafe || !ReadLive(m, e.nameKey, live)) { ++stats.unobserved; return; }
		std::string text;
		text.reserve(live.dir.size() + live.fname.size() + live.ext.size() + 4);
		text.push_back(existedBefore ? 'D' : 'N');
		text.append(live.fname).push_back('\0');
		text.append(live.ext).push_back('\0');
		text.append(live.dir);
		stats.digest += EnumerationPackage::Fnv64(reinterpret_cast<const unsigned char*>(text.data()), text.size());
		++(existedBefore ? stats.observedDup : stats.observedNew);
	}

	// MSVC preserves EBX/ESI/EDI when referenced by inline assembly.
	using CsFn = void(__stdcall*)(void*);
	using TidFn = DWORD(__stdcall*)();
	void Lock(uintptr_t lock) {
		PrimTimer timer(pLock);
		(*reinterpret_cast<const CsFn*>(kLockEnterSlot))(reinterpret_cast<void*>(lock));
		const DWORD id = (*reinterpret_cast<const TidFn*>(kThreadIdSlot))();
		++*reinterpret_cast<int*>(lock + 0x1C);
		*reinterpret_cast<DWORD*>(lock + 0x18) = id;
	}
	void Unlock(uintptr_t lock) {
		PrimTimer timer(pUnlock);
		if (--*reinterpret_cast<int*>(lock + 0x1C) == 0) *reinterpret_cast<DWORD*>(lock + 0x18) = 0;
		(*reinterpret_cast<const CsFn*>(kLockLeaveSlot))(reinterpret_cast<void*>(lock));
	}
	void InsertUnique(uintptr_t tree, uint32_t key, uint32_t value, int prim) {
		PrimTimer timer(prim);
		uint32_t pair[2] = { key, value };
		uint32_t out[4] = {};
		void* pairPtr = pair;
		void* outPtr = out;
		const uintptr_t fn = g_fnInsert;
		__asm {
			push pairPtr
			push outPtr
			push tree
			call fn
		}
	}
	void InitSbo(void* s) {
		auto* p = static_cast<unsigned char*>(s);
		*reinterpret_cast<uintptr_t*>(p + 0x10) = reinterpret_cast<uintptr_t>(p);
		*reinterpret_cast<uintptr_t*>(p + 0x14) = reinterpret_cast<uintptr_t>(p + 0x10);
	}
	void ConstructString(void* s, const char* begin, const char* end) {
		PrimTimer timer(pConstruct);
		InitSbo(s);
		const uintptr_t fn = g_fnConstruct;
		__asm {
			push end
			push begin
			mov eax, s
			call fn
		}
	}
	void DestroyString(void* s) {
		PrimTimer timer(pDestroy);
		using Destroy = void(__thiscall*)(void*);
		reinterpret_cast<Destroy>(Offsets::func_engineStringDestroy.Get())(s);
	}
	void PushDir(uintptr_t vector, void* s) {
		PrimTimer timer(pPushDir);
		const uintptr_t fn = g_fnPushDir;
		__asm {
			mov ecx, s
			mov edi, vector
			call fn
		}
	}
	void RefCtor(void* dest, uint32_t dirIndex, const char* fname, const char* ext) {
		PrimTimer timer(pRefCtor);
		const uintptr_t fn = g_fnRefCtor;
		__asm {
			push ext
			mov edi, dest
			mov eax, dirIndex
			mov ecx, fname
			call fn
		}
	}
	void CopyRef(void* dest, void* src) {
		PrimTimer timer(pCopyRef);
		const uintptr_t fn = g_fnCopyRef;
		__asm {
			mov eax, src
			mov esi, dest
			call fn
		}
	}
	void FreeRef(void* element) {
		PrimTimer timer(pFreeRef);
		const uintptr_t fn = g_fnFreeRef;
		__asm {
			mov eax, element
			call fn
		}
	}
	void GrowRefs(uintptr_t vector, uintptr_t position, void* element) {
		PrimTimer timer(pGrow);
		const uintptr_t fn = g_fnGrow;
		__asm {
			push element
			push position
			push vector
			call fn
		}
	}
	void CallNative(const char* prefix, const char* path, uintptr_t mapper) {
		const uintptr_t fn = g_native;
		__asm {
			push mapper
			mov edx, path
			mov ecx, prefix
			call fn
		}
	}

	// Insert only after the name-key lookup misses.
	void InsertRegistered(uintptr_t m, const Entry& e) {
		Lock(m + 0x38);
		uint32_t dirIndex = 0;
		if (!TreeFind(m + 0xA8, e.dirKey, &dirIndex)) {
			dirIndex = (Read32(m + 0xD0) - Read32(m + 0xCC)) / 0x18;
			alignas(4) unsigned char text[0x18];
			ConstructString(text, e.dir.data(), e.dir.data() + e.dir.size());
			PushDir(m + 0xCC, text);
			DestroyString(text);
			InsertUnique(m + 0xA8, e.dirKey, dirIndex, pInsDir);
		}
		Unlock(m + 0x38);
		Lock(m + 0x18);
		alignas(4) unsigned char reference[0x34];
		RefCtor(reference, dirIndex, e.fname.c_str(), e.ext.c_str());
		const uint32_t index = (Read32(m + 0xC4) - Read32(m + 0xC0)) / 0x34;
		const uintptr_t vector = m + 0xC0, end = Read32(vector + 4);
		if (end == Read32(vector + 8)) {
			GrowRefs(vector, end, reference);
		}
		else {
			if (end) CopyRef(reinterpret_cast<void*>(end), reference);
			*reinterpret_cast<uint32_t*>(vector + 4) = static_cast<uint32_t>(end + 0x34);
		}
		InsertUnique(m + 0x90, e.nameKey, index, pInsName);
		Lock(m + 0x58);
		InsertUnique(m + 0x78, e.llid, index, pInsLlid);
		Unlock(m + 0x58);
		FreeRef(reference);
		Unlock(m + 0x18);
	}

	void LogMismatch(const char* what, const char* path) {
		if (stats.mismatchLogs++ < 8)
			LOG_WARNING("(RESTORE STEP1) " << what << ": " << path << std::endl);
	}

	// Return 1 if handled, 0 to run the native call.
	int __cdecl Handle(const char* prefix, const char* path, uintptr_t mapper, uintptr_t vector) {
		if (GetCurrentThreadId() != sessionThread.load(std::memory_order_acquire)) return 0;
		if (!session.open || !mapper || !vector || disabledForRun || !prefix || !path) return 0;
		if (!session.sawFirst) {
			session.sawFirst = true;
			const uintptr_t begin = Read32(vector), end = Read32(vector + 4);
			session.expected = end >= begin ? static_cast<uint32_t>((end - begin) / 0x1C) : 0;
			if (session.haveSaved && session.saved.size() != session.expected) {
				session.saved.clear();
				session.haveSaved = false;
				session.recording = mode != Mode::native;
				++stats.packagesStale;
			}
		}
		const long long t0 = Counter();
		++stats.entries;
		const Entry* saved = session.haveSaved && session.index < session.saved.size() ? &session.saved[session.index] : nullptr;
		++session.index;
		const size_t pathSize = std::strlen(path);
		const uint64_t pathHash = PathHash(path, pathSize);
		const bool savedPathMatches = saved && saved->pathHash == pathHash && std::strcmp(prefix, "data") == 0;

		if (mode == Mode::restore && saved && saved->flags != kUnsafe) {
			if (!savedPathMatches) {
				++stats.pathMismatch;
			}
			else {
				bool exists;
				{
					PrimTimer timer(pDupFind);
					exists = TreeFind(mapper + 0x90, saved->nameKey, nullptr);
				}
				if (exists || saved->flags == kRegistered) {
					if (exists) {
						++stats.duplicates;
					}
					else {
						InsertRegistered(mapper, *saved);
						if (verifyReadback) {
							PrimTimer timer(pReadback);
							if (!VerifyLive(mapper, *saved, false)) {
								++stats.readbackFail;
								disabledForRun = true;
								LOG_ERROR("(RESTORE STEP1) read-back after a restored entry disagrees with the saved record: " << path
									<< "; native for the rest of this run" << std::endl);
							}
						}
					}
					if (digestEnabled) {
						PrimTimer timer(pObserve);
						Observe(mapper, *saved, exists);
					}
					++stats.restored;
					stats.restoreTicks += Counter() - t0;
					return 1;
				}
				++stats.declined;
			}
		}

		const bool observe = mode == Mode::check || digestEnabled;
		const bool derive = mode == Mode::check || session.recording
			|| (mode == Mode::native && digestEnabled) || (mode == Mode::restore && digestEnabled);
		Entry derived;
		const bool derivedOk = derive && Derive(prefix, path, pathSize, derived);
		derived.pathHash = pathHash;
		const bool existedBefore = derivedOk && TreeFind(mapper + 0x90, derived.nameKey, nullptr);
		const uint32_t elementsBefore = derivedOk
			? (Read32(mapper + 0xC4) - Read32(mapper + 0xC0)) / 0x34 : 0;

		const long long nativeStart = Counter();
		CallNative(prefix, path, mapper);
		const long long nativeEnd = Counter();
		++stats.nativeCalls;
		Entry entry = std::move(derived);
		entry.pathHash = pathHash;
		entry.flags = kUnsafe;
		if (derivedOk) {
			Live live;
			if (existedBefore) {
				const uint32_t elementsAfter = (Read32(mapper + 0xC4) - Read32(mapper + 0xC0)) / 0x34;
				if (elementsAfter == elementsBefore) entry.flags = kDuplicate;
			}
			else if (ReadLive(mapper, entry.nameKey, live)) {
				entry.flags = kRegistered;
			}
			if (entry.flags == kUnsafe || !VerifyLive(mapper, entry, existedBefore)) {
				entry.flags = kUnsafe;
				++stats.recordMismatch;
				LogMismatch("derived entry disagrees with the native result", path);
			}
		}
		if (entry.flags == kUnsafe) ++stats.unsafe;
		if (entry.flags == kDuplicate) ++stats.duplicates;
		if (observe) {
			PrimTimer timer(pObserve);
			Observe(mapper, entry, existedBefore);
		}
		if (mode == Mode::check && saved) {
			if (!savedPathMatches) {
				++stats.pathMismatch;
			}
			else if (saved->flags != kUnsafe && (saved->flags != entry.flags
				|| !VerifyLive(mapper, *saved, existedBefore))) {
				++stats.checkMismatch;
				LogMismatch("saved record disagrees with the native result", path);
			}
			else {
				++stats.checkOk;
			}
		}
		if (session.recording && mode != Mode::native) session.recorded.push_back(std::move(entry));
		stats.nativeTicks += nativeEnd - nativeStart;
		stats.overheadTicks += (Counter() - t0) - (nativeEnd - nativeStart);
		return 1;
	}
	void __declspec(naked) Hook() {
		__asm {
			push ecx
			push edx
			push edi
			push dword ptr [esp + 16]
			push edx
			push ecx
			call Handle
			add esp, 16
			pop edx
			pop ecx
			test eax, eax
			jnz handled
			jmp dword ptr [g_native]
		handled:
			ret 4
		}
	}

	void NoteBegin(const EnumerationPreparation::Prepared* prepared) {
		session = Session();
		sessionThread.store(0, std::memory_order_release);
		if (mode == Mode::off || !armed || !prepared || prepared->snapshot.empty()) return;
		session.open = true;
		session.key = prepared->snapshot;
		session.indexHash = prepared->package.indexHash;
		++stats.packages;
		if (const auto* blob = EnumerationCache::FindMapperRefs(session.key, session.indexHash))
			session.haveSaved = Unpack(*blob, session.saved);
		if (session.haveSaved) ++stats.packagesRestored;
		session.recording = !session.haveSaved && mode != Mode::native;
		sessionThread.store(GetCurrentThreadId(), std::memory_order_release);
	}

	void NoteEnd() {
		if (!session.open) return;
		if (session.faulted) {
			++stats.packagesFaulted;
		}
		else if (session.recording && session.expected > 0 && session.recorded.size() == session.expected) {
			EnumerationCache::StoreMapperRefs(session.key, session.indexHash, Pack(session.recorded));
			++stats.packagesRecorded;
		}
		session = Session();
		sessionThread.store(0, std::memory_order_release);
	}

	uintptr_t SiteTarget(uintptr_t site) {
		const unsigned char callBytes[] = { 0xE8, 0x56, 0xF9, 0xFF, 0xFF };
		if (!site || !MemUtil::MatchesBytes(site, callBytes)) return 0;
		int32_t relative = 0;
		std::memcpy(&relative, reinterpret_cast<const void*>(site + 1), sizeof(relative));
		return site + 5 + relative;
	}

	// Arm hooks only after every write in the patch transaction succeeds.
	uintptr_t Target(uintptr_t native) {
		if (targetConfigured) return hookPrepared ? reinterpret_cast<uintptr_t>(&Hook) : native;
		targetConfigured = true;
		g_native = native;
		mode = Mode::restore;
		digestEnabled = false;
		verifyReadback = false;
		if (!native) { armedReason = "AddReference call-site byte guard failed"; return native; }
		const unsigned char insertBytes[] = { 0x55, 0x8B, 0xEC, 0x8B, 0x4D, 0x08, 0x53, 0x8B, 0x5D, 0x0C };
		const unsigned char constructBytes[] = { 0x55, 0x8B, 0xEC, 0x8B, 0x4D, 0x08, 0x53, 0x56, 0x57, 0x8B, 0x7D, 0x0C };
		const unsigned char pushDirBytes[] = { 0x8B, 0x47, 0x04, 0x3B, 0x47, 0x08, 0x74, 0x26, 0x85, 0xC0 };
		const unsigned char refCtorBytes[] = { 0x55, 0x8B, 0xEC, 0x53, 0x8B, 0x5D, 0x08, 0x89, 0x07 };
		const unsigned char copyBytes[] = { 0x53, 0x57, 0x8B, 0xF8, 0x8B, 0x07, 0x89, 0x06 };
		const unsigned char freeBytes[] = { 0x56, 0x57, 0x8B, 0xF8, 0x8B, 0x47, 0x30 };
		const unsigned char growBytes[] = { 0x55, 0x8B, 0xEC, 0x51, 0x53, 0x8B, 0x5D, 0x08, 0x8B, 0x4B, 0x04 };
		const unsigned char nativeBytes[] = { 0x55, 0x8B, 0xEC, 0x81, 0xEC, 0xB0, 0x04, 0x00, 0x00 };
		if (!MemUtil::MatchesBytes(kInsertUnique, insertBytes) || !MemUtil::MatchesBytes(kConstructRange, constructBytes)
			|| !MemUtil::MatchesBytes(kPushDir, pushDirBytes) || !MemUtil::MatchesBytes(kRefCtor, refCtorBytes)
			|| !MemUtil::MatchesBytes(kCopyRef, copyBytes) || !MemUtil::MatchesBytes(kFreeRef, freeBytes)
			|| !MemUtil::MatchesBytes(kGrow, growBytes) || !MemUtil::MatchesBytes(0x49DA60, nativeBytes)
			|| !*reinterpret_cast<const uintptr_t*>(kLockEnterSlot) || !*reinterpret_cast<const uintptr_t*>(kLockLeaveSlot)
			|| !*reinterpret_cast<const uintptr_t*>(kThreadIdSlot) || !Offsets::func_engineStringDestroy) {
			armedReason = "a native helper failed its byte guard";
			return native;
		}
		hookPrepared = true;
		armedReason = "hook pending patch commit";
		return reinterpret_cast<uintptr_t>(&Hook);
	}

	void PatchCommitted() {
		if (!hookPrepared) return;
		armed = true;
		armedReason = "armed; byte-checked patch transaction committed";
	}
	void PatchRejected(const char* reason) {
		armed = false;
		if (hookPrepared) armedReason = reason;
	}

	void NoteInstallFault() { session.faulted = true; }

	void Reset() {
		stats = Stats();
		session = Session();
		sessionThread.store(0, std::memory_order_release);
		disabledForRun = false;
	}

	void Log() {
		if (mode == Mode::off && armedReason == std::string_view("not requested")) return;
		LARGE_INTEGER frequency{};
		QueryPerformanceFrequency(&frequency);
		const auto ms = [&](long long ticks) { return frequency.QuadPart ? ticks * 1000.0 / frequency.QuadPart : 0.0; };
		const char* modeName = mode == Mode::restore ? "1" : (mode == Mode::check ? "check" : (mode == Mode::native ? "native" : "off"));
		LOG_INFO("(RESTORE STEP1) mode=" << modeName << ", " << armedReason << ", packages=" << stats.packages
			<< " (with saved record " << stats.packagesRestored << ", recorded now " << stats.packagesRecorded
			<< ", stale " << stats.packagesStale << ", faulted " << stats.packagesFaulted << "), entries=" << stats.entries
			<< ", restored=" << stats.restored << ", native calls=" << stats.nativeCalls << ", duplicates=" << stats.duplicates
			<< ", declined=" << stats.declined << ", unsafe=" << stats.unsafe << ", record mismatches=" << stats.recordMismatch
			<< ", check ok/mismatch=" << stats.checkOk << '/' << stats.checkMismatch << ", path mismatches=" << stats.pathMismatch
			<< ", read-back failures=" << stats.readbackFail << ", disabled=" << (disabledForRun ? 1 : 0)
			<< "; ms restored=" << ms(stats.restoreTicks) << ", native call=" << ms(stats.nativeTicks)
			<< ", derive+verify+observe=" << ms(stats.overheadTicks)
			<< "; readback=" << (verifyReadback ? 1 : 0)
			<< "; observed new=" << stats.observedNew << ", dup=" << stats.observedDup << ", unobserved=" << stats.unobserved
			<< ", digest=0x" << std::hex << stats.digest << std::dec << std::endl);
	}
}
#endif
