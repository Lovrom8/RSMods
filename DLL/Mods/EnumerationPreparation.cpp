#ifndef ENUMERATION_PREPARATION_TEST
#include "../stdafx.h"
#endif
#include "EnumerationPreparation.hpp"
#include <atomic>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include "EnumerationOwnership.hpp"
#include "EnumerationMapperHash.hpp"
#ifndef ENUMERATION_PREPARATION_TEST
#include "EnumerationCache.hpp"
#endif

#ifdef ENUMERATION_PREPARATION_TEST
#define ENUMERATION_OWNERSHIP_TEST
#endif
#include "EnumerationOwnership.inc"
#include "EnumerationMapperHash.inc"

namespace {
	using CrtMalloc = void*(__cdecl*)(size_t);
	using CrtFreeFn = void(__cdecl*)(void*);
	CrtMalloc crtMalloc = nullptr;
	CrtFreeFn crtFree = nullptr;
	EnumerationPackage::Inflate inflate = nullptr;
	uintptr_t parserFunc = 0;
	uintptr_t hashFunc = 0;
	uintptr_t openFunc = 0;
	uintptr_t readBody = 0;
	bool installed = false;
	size_t installedPatchSites = 0;
	thread_local EnumerationPreparation::Prepared* active = nullptr;
	thread_local long long openTicks = 0;

	void __declspec(naked) __cdecl CallParser(void**, void**,
		void*, char**) {
		__asm {
			mov ecx, [esp + 4]
			mov edx, [esp + 8]
			push [esp + 16]
			push [esp + 16]
			call dword ptr [parserFunc]
			add esp, 8
			ret
		}
	}

	void StreamRef(void* stream, unsigned int slot) {
		using Ref = void(__fastcall*)(void*, void*);
		const auto table = *static_cast<uintptr_t**>(stream);
		reinterpret_cast<Ref>(table[slot])(stream, nullptr);
	}

	void** __cdecl Parse(void** out, void** toc, void* stream, char** names) {
		if (!active || !active->valid || !stream || stream != active->sourceStream
			|| !active->toc || !active->names) {
			CallParser(out, toc, stream, names);
			return out;
		}
		StreamRef(stream, 1);
		*out = stream;
		active->nativeToc = active->toc.get();
		*toc = active->toc.release();
		*names = reinterpret_cast<char*>(active->names.release());
		return out;
	}

	void __declspec(naked) ParserHook() {
		__asm {
			push [esp + 8]
			push [esp + 8]
			push edx
			push ecx
			call Parse
			add esp, 16
			ret
		}
	}

	void __declspec(naked) __cdecl CallHash(void*, const char*, int) {
		__asm {
			mov ecx, [esp + 4]
			mov edx, [esp + 8]
			push [esp + 12]
			call dword ptr [hashFunc]
			add esp, 4
			ret
		}
	}

	void __cdecl Hash(void* out, const char* text, int size) {
		if (active && active->valid && size >= 0) {
			const std::string_view name(text, static_cast<size_t>(size));
			const auto& hashes = active->package.hashes;
			const auto found = std::lower_bound(hashes.begin(), hashes.end(), name,
				[](const EnumerationPackage::Hash& hash, std::string_view key) {
					return hash.name < key;
				});
			if (found != hashes.end() && found->name == name) {
				memcpy(out, found->digest.data(), found->digest.size());
				return;
			}
		}
		CallHash(out, text, size);
	}

	void __declspec(naked) HashHook() {
		__asm {
			push [esp + 4]
			push edx
			push ecx
			call Hash
			add esp, 12
			ret
		}
	}

	unsigned int __cdecl Read(const void* entry, void* dest, unsigned int size,
		unsigned int offset, const void* toc, void* stream) {
		if (!active || !active->valid || toc != active->nativeToc
			|| entry != static_cast<const unsigned char*>(toc) + 32
				+ active->package.appidIndex * 30)
			return ~0u;
		const auto& data = active->package.appid;
		const size_t available = data.size() - (std::min)(data.size(), size_t(offset));
		const unsigned int count = static_cast<unsigned int>((std::min)(available, size_t(size)));
		if (count) memcpy(dest, data.data() + offset, count);
		// Native reads consume the by-value stream reference even on empty or short reads.
		if (stream) StreamRef(stream, 2);
		return count;
	}

	void __declspec(naked) ReadHook() {
		__asm {
			push ebp
			mov ebp, esp
			push ecx
			push edx
			push [ebp + 24]
			push [ebp + 20]
			push [ebp + 16]
			push [ebp + 12]
			push [ebp + 8]
			push ecx
			call Read
			add esp, 24
			pop edx
			pop ecx
			pop ebp
			cmp eax, -1
			je original
			ret
		original:
			push ebp
			mov ebp, esp
			sub esp, 34h
			jmp dword ptr [readBody]
		}
	}

	bool __declspec(naked) __cdecl CallOpen(const char*, void**) {
		__asm {
			mov ecx, [esp + 4]
			mov edx, [esp + 8]
			call dword ptr [openFunc]
			ret
		}
	}

	bool __cdecl Open(const char* path, void** out) {
		const long long start = EnumerationPackage::Clock();
		const bool result = CallOpen(path, out);
		// Only the DLC install stream may consume these prepared buffers.
		if (active) active->sourceStream = result ? *out : nullptr;
		openTicks += EnumerationPackage::Clock() - start;
		return result;
	}

	void __declspec(naked) OpenHook() {
		__asm {
			push edx
			push ecx
			call Open
			add esp, 8
			ret
		}
	}

	#ifndef ENUMERATION_PREPARATION_TEST
    uintptr_t ownershipDone = 0;
    void __declspec(naked) OwnershipHook() {
        __asm { call EnumerationOwnership::Dispatch }
        __asm { mov byte ptr [esi + 01dh], al }
        __asm { jmp dword ptr [ownershipDone] }
    }
#endif
    uintptr_t g_packages = 0;
    std::atomic<unsigned int> g_cacheOnlyHits = 0;
    // Set continuation targets before installing borrowed-name and duplicate-index hooks.
    uintptr_t g_duplicateBody = 0;
    uintptr_t g_releaseBody = 0, g_releaseDone = 0, g_internLock = 0;
#include "EnumerationBorrowedName.inc"
#if !defined(ENUMERATION_PREPARATION_TEST) || defined(ENUMERATION_HOOK_ABI_TEST)
#include "EnumerationBorrowedHooks.inc"
#endif
#include "EnumerationIndex.inc"

#ifndef ENUMERATION_PREPARATION_TEST
	struct Patch {
		uintptr_t site;
		std::vector<unsigned char> before;
		std::vector<unsigned char> after;
	};

	// Fill absolute and rel32 operands from the version tables before byte validation.
	// Operand placeholders in the expected span are zero.
	struct Operand { size_t at; uintptr_t value; bool relative; };
	Operand Abs(size_t at, uintptr_t value) { return { at, value, false }; }
	Operand Rel(size_t at, uintptr_t target) { return { at, target, true }; }
	std::vector<unsigned char> Span(uintptr_t site, std::vector<unsigned char> bytes, std::initializer_list<Operand> operands) {
		for (const auto& operand : operands) {
			const auto value = static_cast<uint32_t>(operand.relative ? operand.value - (site + operand.at + 4) : operand.value);
			memcpy(bytes.data() + operand.at, &value, sizeof(value));
		}
		return bytes;
	}

	Patch Branch(uintptr_t site, uintptr_t target, void* hook, unsigned char opcode,
		std::vector<unsigned char> prologue = {}) {
		std::vector<unsigned char> before = std::move(prologue);
		if (before.empty()) {
			before.resize(5);
			before[0] = opcode;
			const auto rel = static_cast<int32_t>(target - site - 5);
			memcpy(before.data() + 1, &rel, sizeof(rel));
		}
		std::vector<unsigned char> after(before.size(), 0x90);
		after[0] = opcode;
		const auto rel = static_cast<int32_t>(reinterpret_cast<uintptr_t>(hook) - site - 5);
		memcpy(after.data() + 1, &rel, sizeof(rel));
		return { site, std::move(before), std::move(after) };
	}

	// The VMP name-split shim adds one leading argument to _splitpath; caller pops 0x14.
	using SplitPath = void(__cdecl*)(const char*, char*, char*, char*, char*);
	SplitPath crtSplitPath = nullptr;
	uintptr_t splitShim = 0;
	void __cdecl SplitName(const char* path, char* drive, char* dir, char* fname, char* ext) {
		crtSplitPath(path, drive, dir, fname, ext);
	}
	void __declspec(naked) SplitNameHook() {
		// Discard the leading shim argument while preserving the caller cleanup.
		__asm { pop eax }
		__asm { xchg eax, dword ptr [esp] }
		__asm { jmp SplitName }
	}
#include "EnumerationLlid.inc"
#include "EnumerationMapperRestore.inc"
#include "EnumerationMapperHashChecks.inc"
#include "EnumerationStagePatches.inc"
#include "EnumerationLocateMemo.inc"
#include "EnumerationCandidateIndex.inc"
#include "EnumerationZeroCopyWalk.inc"
#endif
}

void EnumerationPreparation::CrtFree::operator()(unsigned char* data) const {
	crtFree(data);
}

std::unique_ptr<EnumerationPreparation::Prepared> EnumerationPreparation::Prepare(
	const std::wstring& path, std::string snapshot) {
	auto result = std::make_unique<Prepared>();
	result->snapshot = std::move(snapshot);
	auto& package = result->package;
	unsigned char header[32]{};
	const long long start = EnumerationPackage::Clock();
#ifndef ENUMERATION_PREPARATION_TEST
	// Cache-only lookup checks path, size and mtime without a write-denying handle.
	// A same-size rewrite preserving mtime can escape this check.
	if (result->snapshot.find("|missing") == std::string::npos) {
		auto cachedMiss = EnumerationCache::Miss::New;
		if (const auto* cached = EnumerationCache::Find(result->snapshot, cachedMiss);
			cached && EnumerationPackage::RestoreCached(package, *cached)) {
			EnumerationCache::NoteHit(*cached, EnumerationPackage::Clock() - start);
			result->fromCache = true;
			++g_cacheOnlyHits;
			const auto copy = [](const std::vector<unsigned char>& data) {
				Buffer buffer(static_cast<unsigned char*>(crtMalloc(data.size())));
				if (buffer) memcpy(buffer.get(), data.data(), data.size());
				return buffer;
			};
			result->toc = copy(package.toc);
			result->names = copy(package.names);
			result->valid = result->toc && result->names;
			std::vector<unsigned char>().swap(package.toc);
			std::vector<unsigned char>().swap(package.names);
			return result;
		}
		package = EnumerationPackage::Package{};
	}
#endif
	bool ok = EnumerationPackage::Open(path, package);
	if (ok) {
		// Build the snapshot key from the handle held through native installation.
		const auto separator = result->snapshot.find('|');
		result->snapshot = result->snapshot.substr(0, separator) + '|'
			+ std::to_string(package.fileSize) + '|' + std::to_string(package.writeTime);
		ok = EnumerationPackage::ReadIndex(package, header);
	}
#ifndef ENUMERATION_PREPARATION_TEST
	auto miss = EnumerationCache::Miss::New;
	const EnumerationPackage::Record* record = ok ? EnumerationCache::Find(result->snapshot, miss) : nullptr;
	const bool hit = record && EnumerationPackage::Restore(package, header, *record);
	if (record && !hit) miss = EnumerationCache::Miss::Content;
#else
	const bool hit = false;
#endif
	if (ok && !hit)
		ok = EnumerationPackage::Parse(package, header, inflate);
	if (!ok) {
		result->package = EnumerationPackage::Package{};
		return result;
	}
#ifndef ENUMERATION_PREPARATION_TEST
	const long long ticks = EnumerationPackage::Clock() - start;
	result->fromCache = hit;
	if (hit) {
		EnumerationCache::NoteHit(*record, ticks);
	}
	else {
		EnumerationCache::NoteMiss(result->snapshot, miss);
		LARGE_INTEGER frequency{};
		QueryPerformanceFrequency(&frequency);
		const auto costUs = static_cast<uint32_t>(frequency.QuadPart ? ticks * 1000000 / frequency.QuadPart : 0);
		EnumerationCache::Remember(EnumerationPackage::Capture(package, result->snapshot, costUs), ticks);
	}
#else
	(void)start;
#endif
	const auto copy = [](const std::vector<unsigned char>& data) {
		Buffer buffer(static_cast<unsigned char*>(crtMalloc(data.size())));
		if (buffer) memcpy(buffer.get(), data.data(), data.size());
		return buffer;
	};
	const long long copyStart = EnumerationPackage::Clock();
	result->toc = copy(result->package.toc);
	result->names = copy(result->package.names);
	result->valid = result->toc && result->names;
	std::vector<unsigned char>().swap(result->package.toc);
	std::vector<unsigned char>().swap(result->package.names);
	result->package.parseTicks += EnumerationPackage::Clock() - copyStart;
	return result;
}

void EnumerationPreparation::Begin(Prepared* prepared) {
	active = prepared;
#ifndef ENUMERATION_PREPARATION_TEST
	MapperRestore::NoteBegin(prepared);
#endif
	if (active) active->sourceStream = nullptr;
	openTicks = 0;
}

long long EnumerationPreparation::End() {
#ifndef ENUMERATION_PREPARATION_TEST
	MapperRestore::NoteEnd();
#endif
	active = nullptr;
	return openTicks;
}

bool EnumerationPreparation::IsInstalled() { return installed; }
unsigned int EnumerationPreparation::CacheOnlyHits() { return g_cacheOnlyHits.load(); }

#ifndef ENUMERATION_PREPARATION_TEST
bool EnumerationPreparation::Install() {
	if (installed) return true;
	parserFunc = Offsets::func_enumReadIndex;
	hashFunc = Offsets::func_enumNameHash;
	openFunc = Offsets::func_enumOpen;
	const uintptr_t read = Offsets::func_enumReadEntry;
	const uintptr_t decompress = Offsets::func_enumInflate;
	ownershipDone = uintptr_t(Offsets::enumDeep8DC12D) + 5;
	if (!Offsets::enumDeep8DC12D) return false;
	if (!parserFunc || !hashFunc || !openFunc || !read || !decompress) return false;
	readBody = read + 6;
	const HMODULE crt = GetModuleHandleW(L"msvcr100.dll");
	crtMalloc = crt ? reinterpret_cast<CrtMalloc>(GetProcAddress(crt, "malloc")) : nullptr;
	crtFree = crt ? reinterpret_cast<CrtFreeFn>(GetProcAddress(crt, "free")) : nullptr;
	if (!crtMalloc || !crtFree) return false;
	const unsigned char inflateBytes[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x38,
		0x8B, 0x4D, 0x14, 0x8B, 0x45, 0x10, 0x8B, 0x55, 0x08 };
	std::vector<Patch> patches = {
		Branch(Offsets::ptr_enumReadIndexCall, parserFunc, &ParserHook, 0xE8),
		Branch(Offsets::ptr_enumNameHashCall, hashFunc, &HashHook, 0xE8),
		Branch(Offsets::ptr_enumOpenCall, openFunc,
			&OpenHook,
			0xE8),
		Branch(read, 0, &ReadHook, 0xE9, { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x34 })
	};
	// Validate the content/apps chain and native memo path.
    const uintptr_t install = Offsets::func_dlcInstallContentPackage;
    if (!install || !Offsets::ptr_dlcOwnershipService || !Offsets::enumAnchorContentRead || !Offsets::enumAnchorAppsRead
        || !Offsets::func_enumDiagStub) return false;
    for (const auto& anchor : std::vector<std::pair<uintptr_t, std::vector<unsigned char>>>{
        { install + 0x339, Span(install + 0x339, { 0x8B, 0x0D, 0, 0, 0, 0 }, { Abs(2, Offsets::ptr_dlcOwnershipService) }) },
        { Offsets::enumAnchorContentRead, { 0x8B, 0x81, 0xA4, 0, 0, 0 } },
        { Offsets::enumAnchorAppsRead, { 0x8B, 0x4B, 0x14, 0x8B, 0x11, 0x8B, 0x06, 0x8B, 0x52, 0x1C, 0x50 } } }) {
        if (!MemUtil::MatchesBytes(anchor.first, anchor.second)) {
            LOG_WARNING("(ENUMERATION) Ownership anchor mismatch; no preparation patches applied" << std::endl);
            return false;
        }
    }
    if (!CheckMapperHash()) {
        LOG_WARNING("(ENUMERATION) Mapper hash semantic anchor mismatch; no preparation patches applied" << std::endl);
        return false;
    }
    EnumerationMapperHash::native = Offsets::enumMapperHash;
    if (!EnumerationMapperHash::native || !Offsets::enumMapperHashCall0 || !Offsets::enumMapperHashCall1) return false;
    patches.push_back(Branch(Offsets::enumMapperHashCall0, EnumerationMapperHash::native, &EnumerationMapperHash::Hook, 0xE8));
    patches.push_back(Branch(Offsets::enumMapperHashCall1, EnumerationMapperHash::native, &EnumerationMapperHash::Hook, 0xE8));
    if (!AddStagePatches(patches)) return false;
	const uintptr_t mapperSite = Offsets::enumDeep49E105;
	const uintptr_t mapperNative = MapperRestore::SiteTarget(mapperSite);
	const uintptr_t mapperTarget = MapperRestore::Target(mapperNative);
	if (mapperNative && mapperTarget != mapperNative)
		patches.push_back(Branch(mapperSite, mapperNative, &MapperRestore::Hook, 0xE8,
			{ 0xE8, 0x56, 0xF9, 0xFF, 0xFF }));
    const bool llid = EnumerationLlid::AddPatches(patches, true);
    if (!llid)
        LOG_WARNING("(ENUMERATION) Native LLID sites unmapped; asset LLIDs stay native" << std::endl);
    const bool locateMemo = AddLocatePatches(patches);
    if (!locateMemo)
        LOG_WARNING("(ENUMERATION) PSARC locate memo bytes mismatch; backend Open keeps native interning" << std::endl);
    const bool candidateIndex = AddCandidatePatches(patches);
    if (!AddZeroCopyPatches(patches))
        LOG_WARNING("(ENUMERATION) Zero-copy walk bytes mismatch; backend Open keeps copying the mount containers" << std::endl);
    if (!candidateIndex)
        LOG_WARNING("(ENUMERATION) PSARC candidate index bytes mismatch; backend Open asks every mounted psarc" << std::endl);
    // Skip the unused PathGetFileName diagnostic; EH state remains 3 and ESI is reloaded.
    // Jump from InstallContentPackage +0x386 to +0x3C0 in both builds.
    patches.push_back(Branch(install + 0x386, 0, reinterpret_cast<void*>(install + 0x3C0), 0xE9,
            { 0x8B, 0xCF, 0x8D, 0xB5, 0xD4, 0xFC, 0xFF, 0xFF }));
    crtSplitPath = reinterpret_cast<SplitPath>(GetProcAddress(crt, "_splitpath"));
    const uintptr_t splitPush = Offsets::enumNameSplitPush;
    splitShim = Offsets::func_enumNameSplitShim;
    if (!splitPush || !splitShim)
        LOG_INFO("(ENUMERATION) Package name split stays native: no mapping for this game version" << std::endl);
    else if (crtSplitPath
        && MemUtil::MatchesBytes(splitPush, std::vector<unsigned char>{ 0x57, 0x51 }))
        patches.push_back(Branch(splitPush + 2, splitShim, &SplitNameHook, 0xE8));
    patches.push_back(Branch(Offsets::enumDeep8DC12D, 0, &OwnershipHook, 0xE9,
        { 0xFF, 0xD2, 0x88, 0x46, 0x1D }));
	if (!MemUtil::MatchesBytes(decompress, inflateBytes)) {
		LOG_WARNING("(ENUMERATION) Preparation inflate bytes mismatch, no preparation patches applied" << std::endl);
		return false;
	}
	// Validate the full patch group first; restore every attempted site on failure.
	for (const auto& patch : patches) {
		if (!MemUtil::MatchesBytes(patch.site, patch.before)) {
			MapperRestore::PatchRejected("patch transaction byte preflight failed");
			std::ostringstream actual;
			for (size_t i = 0; i < patch.before.size(); ++i)
				actual << std::hex << static_cast<unsigned int>(*reinterpret_cast<const unsigned char*>(patch.site + i)) << ' ';
			LOG_WARNING("(ENUMERATION) Preparation bytes mismatch at 0x" << std::hex
				<< patch.site << std::dec << ", actual: " << actual.str() << "; no preparation patches applied" << std::endl);
			return false;
		}
		std::ostringstream bytes;
		for (unsigned char byte : patch.before) bytes << std::hex << static_cast<unsigned int>(byte) << ' ';
		LOG_INFO("(ENUMERATION) Preparation original 0x" << std::hex << patch.site
			<< std::dec << ": " << bytes.str() << std::endl);
	}
	for (size_t i = 0; i < patches.size(); ++i) {
		const auto& patch = patches[i];
		if (MemUtil::PatchAdr(reinterpret_cast<void*>(patch.site), patch.after.data(), patch.after.size()))
			continue;
		for (size_t restore = 0; restore <= i; ++restore) {
			const auto& original = patches[restore];
			if (!MemUtil::PatchAdr(reinterpret_cast<void*>(original.site), original.before.data(), original.before.size()))
				LOG_ERROR("(ENUMERATION) Preparation rollback failed at 0x" << std::hex
					<< original.site << std::dec << std::endl);
		}
		MapperRestore::PatchRejected("patch transaction write failed and was rolled back");
		return false;
	}
	MapperRestore::PatchCommitted();
	inflate = reinterpret_cast<EnumerationPackage::Inflate>(decompress);
	installed = true;
	installedPatchSites = patches.size();
	LOG_INFO("(ENUMERATION) PSARC locate memo " << (locateMemo ? "on" : "off") << std::endl);
	LOG_INFO("(ENUMERATION) PSARC candidate index " << (candidateIndex ? "on" : "off") << std::endl);
	LOG_INFO("(ENUMERATION) Worker preparation installed: TOC, file list, appid and MD5; DLC transient interning bypass and duplicate index on; scan-time ownership bypass on; native VFS/census writes/registration stay on game thread" << std::endl);
	return true;
}
#endif

#ifndef ENUMERATION_PREPARATION_TEST
void EnumerationPreparation::NoteInstallFault() { MapperRestore::NoteInstallFault(); }
void EnumerationPreparation::ResetMapperRestore() { MapperRestore::Reset(); }
void EnumerationPreparation::LogMapperRestore() { MapperRestore::Log(); }
#endif

#ifndef ENUMERATION_PREPARATION_TEST
void EnumerationPreparation::ResetIndexStats() {
	g_indexHits = g_indexMisses = g_indexFallbacks = 0;
}
void EnumerationPreparation::LogIndexStats() {
	LOG_INFO("(ENUMERATION) Duplicate index: hits " << g_indexHits << ", misses "
		<< g_indexMisses << ", native fallbacks " << g_indexFallbacks
		<< ", entries " << g_packageNames.size() << ", valid " << g_indexValid << std::endl);
}
#endif
