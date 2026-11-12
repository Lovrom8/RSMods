#include "../stdafx.h"
#include "EnumerationCache.hpp"
#include "CacheStore.hpp"

#include <atomic>
#include <mutex>
#include <new>
#include <unordered_map>
#include <unordered_set>

namespace {
	constexpr unsigned int maxMissLogs = 10;

	std::once_flag loadOnce;
	std::wstring cacheFile;
	std::string identity;
	// Populated by Load, then immutable.
	std::unordered_map<std::string, EnumerationPackage::Record> loaded;
	bool rewrite = false;

	std::mutex freshMutex;
	std::mutex cacheWriteMutex;
	std::unordered_map<std::string, EnumerationPackage::Record> fresh;
	unsigned int freshSinceSave = 0;
	std::vector<std::string> lastQueued;
	bool refsDirty = false;
	uint64_t cacheRevision = 0;
	uint64_t mapperRefsGeneration = 0;
	bool mapperRefsScanEnded = false;
	bool mapperRefsPreparationComplete = false;
	bool cacheWriteQueued = false;

	std::atomic<unsigned int> hits = 0;
	std::atomic<unsigned int> missNew = 0;
	std::atomic<unsigned int> missChanged = 0;
	std::atomic<unsigned int> missContent = 0;
	std::atomic<unsigned int> missLogs = 0;
	std::atomic<long long> hitTicks = 0;
	std::atomic<long long> missTicks = 0;
	std::atomic<unsigned long long> hitBuiltUs = 0;

	double TicksToMs(long long ticks) {
		LARGE_INTEGER frequency{};
		QueryPerformanceFrequency(&frequency);
		return frequency.QuadPart ? ticks * 1000.0 / frequency.QuadPart : 0.0;
	}

	std::string Took(long long start) {
		(void)start;
		return {};
	}

	std::string Utf8(const std::wstring& text) {
		if (text.empty()) return {};
		const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
		std::string out(static_cast<size_t>(size > 0 ? size : 0), '\0');
		if (size > 0)
			WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), out.data(), size, nullptr, nullptr);
		return out;
	}

	// Cache identity depends on the executable and schema, not DLL rebuilds.
	std::string Identity(const std::wstring& exe) {
		std::ostringstream out;
		WIN32_FILE_ATTRIBUTE_DATA data{};
		if (GetFileAttributesExW(exe.c_str(), GetFileExInfoStandard, &data))
			out << "exe " << ((static_cast<unsigned long long>(data.nFileSizeHigh) << 32) | data.nFileSizeLow) << ","
				<< ((static_cast<unsigned long long>(data.ftLastWriteTime.dwHighDateTime) << 32) | data.ftLastWriteTime.dwLowDateTime);
		else
			out << "exe unknown";
		const auto* base = reinterpret_cast<const unsigned char*>(GetModuleHandleW(nullptr));
		const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
		if (base && dos->e_magic == IMAGE_DOS_SIGNATURE) {
			const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
			if (nt->Signature == IMAGE_NT_SIGNATURE)
				out << std::hex << ",pe " << nt->FileHeader.TimeDateStamp << "," << nt->OptionalHeader.CheckSum << std::dec;
		}
		out << "; RSMods enumeration cache v1.2";
		return out.str();
	}

	void LoadOnce() {
		const long long start = EnumerationPackage::Clock();
		CacheStore& store = RSModsCache::Shared();
		wchar_t exe[MAX_PATH * 2]{};
		const DWORD length = GetModuleFileNameW(nullptr, exe, MAX_PATH * 2);
		if (!store.Ready() || length == 0 || length >= MAX_PATH * 2) {
			LOG_WARNING("(ENUMERATION) Cache off: could not find the game folder" << std::endl);
			return;
		}
		cacheFile = store.Path();
		identity = Identity(std::wstring(exe, length));

		std::vector<unsigned char> bytes;
		if (!store.Take(Framework::CacheFile::kEnumeration, bytes)) {
			rewrite = true;
			LOG_INFO("(ENUMERATION) Cache miss: no enumeration records in " << Utf8(cacheFile) << " yet, every package takes the full preparation" << std::endl);
			return;
		}
		std::vector<EnumerationPackage::Record> records;
		std::string reason;
		if (!EnumerationCache::Deserialize(bytes, identity, records, reason)) {
			rewrite = true;
			LOG_INFO("(ENUMERATION) Cache invalidated: " << reason << "; all records dropped" << std::endl);
			return;
		}
		loaded.reserve(records.size());
		for (auto& record : records) {
			std::string path(EnumerationCache::PathOf(record.key));
			loaded[std::move(path)] = std::move(record);
		}
		LOG_INFO("(ENUMERATION) Cache loaded: " << loaded.size() << " records, " << bytes.size() / 1024 << " KB" << Took(start) << std::endl);
	}

	struct CacheWriteRequest { uint64_t generation; };

	void QueueCacheSaveLocked(uint64_t generation);

	DWORD WINAPI WriteCacheWorker(void* context) {
		std::unique_ptr<CacheWriteRequest> request(static_cast<CacheWriteRequest*>(context));
		std::lock_guard<std::mutex> writer(cacheWriteMutex);
		try {
			std::wstring target;
			std::string cacheIdentity;
			std::vector<EnumerationPackage::Record> records;
			size_t dropped = 0;
			size_t withRefs = 0;
			uint64_t revision = 0;
			const long long start = EnumerationPackage::Clock();
			{
				std::lock_guard<std::mutex> lock(freshMutex);
				if (request->generation != mapperRefsGeneration || !mapperRefsScanEnded || !mapperRefsPreparationComplete)
					return 0;
				target = cacheFile;
				cacheIdentity = identity;
				records.reserve(lastQueued.size());
				std::unordered_set<std::string_view> seen;
				std::unordered_set<std::string_view> currentFiles;
				currentFiles.reserve(lastQueued.size());
				for (const auto& path : lastQueued) {
					currentFiles.insert(EnumerationCache::PathOf(path));
					if (!seen.insert(path).second) continue;
					const EnumerationPackage::Record* record = nullptr;
					if (const auto made = fresh.find(path); made != fresh.end()) record = &made->second;
					else if (const auto old = loaded.find(path); old != loaded.end()) record = &old->second;
					if (!record) continue;
					records.push_back(*record);
					if (!record->mapperRefs.empty()) ++withRefs;
				}
				for (const auto& old : loaded)
					if (currentFiles.find(old.first) == currentFiles.end()) ++dropped;
				const bool changed = rewrite || freshSinceSave != 0 || dropped != 0 || refsDirty;
				revision = cacheRevision;
				if (!changed) {
					cacheWriteQueued = false;
					LOG_INFO("(ENUMERATION) Cache unchanged (" << records.size() << " records), not rewritten" << std::endl);
					return 0;
				}
			}

			std::vector<const EnumerationPackage::Record*> keep;
			keep.reserve(records.size());
			for (const auto& record : records) keep.push_back(&record);
			const auto bytes = EnumerationCache::Serialize(cacheIdentity, keep);
			DWORD error = 0;
			const uint64_t generation = request->generation;
			// Refuse the rename once a newer preparation generation has started.
			const bool saved = RSModsCache::Shared().Save(Framework::CacheFile::kEnumeration, bytes, [generation](auto&& replace) {
				std::lock_guard<std::mutex> lock(freshMutex);
				return generation == mapperRefsGeneration && replace();
			}, error);
			bool retry = false;
			bool currentGeneration = false;
			{
				std::lock_guard<std::mutex> lock(freshMutex);
				currentGeneration = request->generation == mapperRefsGeneration;
				if (currentGeneration) {
					cacheWriteQueued = false;
					if (saved && revision == cacheRevision) {
						rewrite = false;
						freshSinceSave = 0;
						refsDirty = false;
					}
					else if (saved && mapperRefsScanEnded && mapperRefsPreparationComplete) {
						retry = true;
					}
				}
			}
			if (!saved) {
				if (currentGeneration)
					LOG_WARNING("(ENUMERATION) Cache save failed: " << Utf8(target) << " (error " << error << ")" << std::endl);
			}
			else {
				LOG_INFO("(ENUMERATION) Cache saved: " << records.size() << " records (" << dropped << " for removed files dropped), "
					<< withRefs << " with mapper references, " << bytes.size() / 1024 << " KB" << Took(start) << std::endl);
			}
			if (retry) {
				std::lock_guard<std::mutex> lock(freshMutex);
				QueueCacheSaveLocked(request->generation);
			}
		}
		catch (...) {
			std::lock_guard<std::mutex> lock(freshMutex);
			if (request->generation == mapperRefsGeneration) cacheWriteQueued = false;
			LOG_ERROR("(ENUMERATION) Cache save worker failed" << std::endl);
		}
		return 0;
	}

	void QueueCacheSaveLocked(uint64_t generation) {
		if (cacheFile.empty() || generation != mapperRefsGeneration || !mapperRefsScanEnded
			|| !mapperRefsPreparationComplete || cacheWriteQueued) return;
		auto* request = new (std::nothrow) CacheWriteRequest{ generation };
		if (!request) {
			LOG_WARNING("(ENUMERATION) Cache save worker could not be queued: out of memory" << std::endl);
			return;
		}
		cacheWriteQueued = true;
		if (!QueueUserWorkItem(&WriteCacheWorker, request, WT_EXECUTEDEFAULT)) {
			const DWORD error = GetLastError();
			cacheWriteQueued = false;
			delete request;
			LOG_WARNING("(ENUMERATION) Cache save worker could not be queued (error " << error << ")" << std::endl);
		}
	}
}

size_t EnumerationCache::LoadedCount() { return loaded.size(); }

std::vector<std::vector<unsigned char>> EnumerationCache::AppIds() {
	std::vector<std::vector<unsigned char>> ids;
	for (const auto& entry : loaded)
		if (std::find(ids.begin(), ids.end(), entry.second.appid) == ids.end())
			ids.push_back(entry.second.appid);
	return ids;
}

void EnumerationCache::Load() {
	std::call_once(loadOnce, LoadOnce);
}

const EnumerationPackage::Record* EnumerationCache::Find(const std::string& key, Miss& miss) {
	if (cacheFile.empty()) {
		miss = Miss::New;
		return nullptr;
	}
	const auto found = loaded.find(std::string(PathOf(key)));
	if (found == loaded.end()) {
		miss = Miss::New;
		return nullptr;
	}
	if (found->second.key != key) {
		miss = Miss::Changed;
		return nullptr;
	}
	miss = Miss::None;
	return &found->second;
}

void EnumerationCache::NoteHit(const EnumerationPackage::Record& record, long long ticks) {
	++hits;
	hitTicks += ticks;
	hitBuiltUs += record.costUs;
}

void EnumerationCache::NoteMiss(const std::string& key, Miss miss) {
	const char* what = "new file";
	switch (miss) {
	case Miss::Changed: ++missChanged; what = "size or write time changed"; break;
	case Miss::Content: ++missContent; what = "index bytes changed"; break;
	default: ++missNew; break;
	}
	// A missing cache logs a summary instead of every package miss.
	if (cacheFile.empty() || (miss == Miss::New && loaded.empty())) return;
	const unsigned int count = ++missLogs;
	if (count <= maxMissLogs)
		LOG_INFO("(ENUMERATION) Cache miss (" << what << "): " << key
			<< (count == maxMissLogs ? " (further misses are only counted)" : "") << std::endl);
}

void EnumerationCache::Remember(EnumerationPackage::Record record, long long ticks) {
	if (cacheFile.empty()) return;
	missTicks += ticks;
	std::string path(PathOf(record.key));
	std::lock_guard<std::mutex> lock(freshMutex);
	const auto existing = fresh.find(path);
	if (record.mapperRefs.empty() && existing != fresh.end()
		&& existing->second.key == record.key && existing->second.indexHash == record.indexHash)
		record.mapperRefs = std::move(existing->second.mapperRefs);
	fresh[std::move(path)] = std::move(record);
	++freshSinceSave;
	++cacheRevision;
	QueueCacheSaveLocked(mapperRefsGeneration);
}

void EnumerationCache::Complete(const std::vector<std::string>& queuedPaths, uint64_t generation) {
	if (cacheFile.empty()) return;
	const unsigned int hitCount = hits.load();
	const unsigned int missCount = missNew.load() + missChanged.load() + missContent.load();
	std::ostringstream summary;
	summary << "(ENUMERATION) Cache: " << hitCount << " hits, " << missCount << " misses (" << missNew.load() << " new, "
		<< missChanged.load() << " changed, " << missContent.load() << " index changed)";
	LOG_INFO(summary.str() << std::endl);

	std::lock_guard<std::mutex> lock(freshMutex);
	if (generation != mapperRefsGeneration) {
		LOG_INFO("(ENUMERATION) Cache save skipped for superseded preparation generation " << generation << std::endl);
		return;
	}
	lastQueued = queuedPaths;
	mapperRefsPreparationComplete = true;
	QueueCacheSaveLocked(generation);
}

const std::vector<unsigned char>* EnumerationCache::FindMapperRefs(const std::string& key, uint64_t indexHash) {
	if (cacheFile.empty()) return nullptr;
	const auto found = loaded.find(std::string(PathOf(key)));
	if (found == loaded.end() || found->second.key != key || found->second.indexHash != indexHash
		|| found->second.mapperRefs.empty()) return nullptr;
	return &found->second.mapperRefs;
}

void EnumerationCache::StoreMapperRefs(const std::string& key, uint64_t indexHash, std::vector<unsigned char> refs) {
	if (cacheFile.empty() || refs.empty() || refs.size() > kMaxMapperRefs) return;
	const std::string path(PathOf(key));
	std::lock_guard<std::mutex> lock(freshMutex);
	auto made = fresh.find(path);
	if (made == fresh.end()) {
		const auto old = loaded.find(path);
		if (old == loaded.end() || old->second.key != key || old->second.indexHash != indexHash) return;
		made = fresh.emplace(path, old->second).first;
	}
	else if (made->second.key != key || made->second.indexHash != indexHash) {
		return;
	}
	made->second.mapperRefs = std::move(refs);
	refsDirty = true;
	++cacheRevision;
	QueueCacheSaveLocked(mapperRefsGeneration);
}

uint64_t EnumerationCache::BeginMapperRefsScan() {
	std::lock_guard<std::mutex> lock(freshMutex);
	++mapperRefsGeneration;
	if (mapperRefsGeneration == 0) ++mapperRefsGeneration;
	mapperRefsScanEnded = false;
	mapperRefsPreparationComplete = false;
	cacheWriteQueued = false;
	return mapperRefsGeneration;
}

void EnumerationCache::MapperRefsScanEnded(uint64_t generation) {
	if (cacheFile.empty()) return;
	std::lock_guard<std::mutex> lock(freshMutex);
	if (generation != mapperRefsGeneration) return;
	mapperRefsScanEnded = true;
	QueueCacheSaveLocked(generation);
}
