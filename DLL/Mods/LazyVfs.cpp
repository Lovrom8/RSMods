#include "../stdafx.h"
#include "LazyVfs.hpp"

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <sys/stat.h>
#include <fstream>

namespace {
	constexpr uintptr_t table = 0x012EE6BC;
	constexpr uintptr_t natives[8] = { 0x00499C00, 0x00499C30, 0x00499C50, 0x00499C70, 0x00499C90, 0x00499CA0,
		0x00499CB0, 0x00499D00 };

	// Open returns its handle through the out parameter.
	using OpenFn = bool(__cdecl*)(const char* path, int mode, void** out);
	using CloseFn = void(__cdecl*)(void* handle);
	using ReadFn = int(__cdecl*)(void* handle, void* buffer, int size);
	// Forward 64-bit returns so both EAX and EDX survive regardless of native result width.
	using HandleFn = long long(__cdecl*)(void* handle);
	using SeekFn = long long(__cdecl*)(void* handle, int position);
	using ModificationFn = long long(__cdecl*)(void* handle);

	OpenFn NativeOpen() { return reinterpret_cast<OpenFn>(natives[0]); }

	struct Lazy {
		const void* magic;
		std::string path;
		int mode = 0;
		void* real = nullptr;
		int position = 0;
		// Size and modification time validate cached reads.
		uint32_t fileSize = 0;
		int32_t fileTime = 0;
		const struct CachedFile* cached = nullptr;
		std::vector<std::pair<uint32_t, std::string>> recorded;
		std::mutex lock;
	};
	const int lazyMagic = 0;

	bool lazyOn = false;
	using Stat32Fn = int(__cdecl*)(const char* path, struct _stat32* out);
	Stat32Fn crtStat32 = nullptr;
	std::atomic<unsigned int> byRead{ 0 }, bySize{ 0 }, byModification{ 0 };
	std::atomic<unsigned int> lazyOpens{ 0 }, lazyMaterialised{ 0 }, lazyClosedUnopened{ 0 }, lazyFailed{ 0 };

	Lazy* AsLazy(void* handle) {
		auto* lazy = static_cast<Lazy*>(handle);
		return handle && lazy->magic == &lazyMagic ? lazy : nullptr;
	}

	bool IsDlcPsarc(const char* path) {
		if (!path) return false;
		const size_t n = strlen(path);
		if (n <= 6 || _stricmp(path + n - 6, ".psarc") != 0)
			return false;
		std::string lower(path, n);
		for (char& c : lower)
			c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
		return lower.find(R"(\dlc\)") != std::string::npos || lower.find("/dlc/") != std::string::npos;
	}

	// Reuse boot reads by path, size and modification time without opening the archive.
	struct CachedFile {
		uint32_t fileSize = 0;
		int32_t fileTime = 0;
		std::vector<std::pair<uint32_t, std::string>> reads;
	};
	// VFSC magic.
	constexpr uint32_t cacheMagic = 0x43534656;
	constexpr uint32_t cacheFormat = 1;
	constexpr size_t maxRecordedPerFile = 64 * 1024;
	std::mutex cacheLock;
	// Immutable after installation.
	std::unordered_map<std::string, CachedFile> cache;
	std::unordered_map<std::string, CachedFile> fresh;
	// Drop cached files not used by this session when saving.
	std::unordered_set<std::string> seen;
	std::atomic<unsigned int> servedReads{ 0 }, servedBytes{ 0 };
	std::string cachePath;
	std::atomic<bool> cacheLoaded{ false };

	std::string Key(const std::string& path) {
		std::string key = path;
		for (char& c : key)
			c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
		return key;
	}

	bool Get32(std::ifstream& in, uint32_t& value) {
		return static_cast<bool>(in.read(reinterpret_cast<char*>(&value), 4));
	}

	void LoadCache() {
		std::ifstream in(cachePath, std::ios::binary);
		uint32_t magic = 0, format = 0, count = 0;
		if (!Get32(in, magic) || !Get32(in, format) || !Get32(in, count) || magic != cacheMagic || format != cacheFormat
			|| count > 1000000)
			return;
		for (uint32_t i = 0; i < count; ++i) {
			uint32_t pathSize = 0, fileSize = 0, fileTime = 0, reads = 0;
			if (!Get32(in, pathSize) || pathSize > 4096)
				return;
			std::string path(pathSize, ' ');
			if (!in.read(path.data(), pathSize) || !Get32(in, fileSize) || !Get32(in, fileTime) || !Get32(in, reads) || reads > 64)
				return;
			CachedFile file;
			file.fileSize = fileSize;
			file.fileTime = static_cast<int32_t>(fileTime);
			for (uint32_t r = 0; r < reads; ++r) {
				uint32_t offset = 0, size = 0;
				if (!Get32(in, offset) || !Get32(in, size) || size > maxRecordedPerFile)
					return;
				std::string bytes(size, ' ');
				if (!in.read(bytes.data(), size))
					return;
				file.reads.emplace_back(offset, std::move(bytes));
			}
			cache[path] = std::move(file);
		}
	}

	void Put32(std::ofstream& out, uint32_t value) {
		out.write(reinterpret_cast<const char*>(&value), 4);
	}

	void SaveCache() {
		std::unordered_map<std::string, CachedFile> merged;
		{
			std::lock_guard<std::mutex> guard(cacheLock);
			if (fresh.empty() && seen.size() == cache.size())
				return;
			for (const auto& [path, file] : cache)
				if (seen.count(path))
					merged[path] = file;
			for (auto& [path, file] : fresh)
				merged[path] = file;
			fresh.clear();
		}
		const std::string temp = cachePath + ".tmp";
		{
			std::ofstream out(temp, std::ios::binary | std::ios::trunc);
			Put32(out, cacheMagic);
			Put32(out, cacheFormat);
			Put32(out, static_cast<uint32_t>(merged.size()));
			for (const auto& [path, file] : merged) {
				Put32(out, static_cast<uint32_t>(path.size()));
				out.write(path.data(), static_cast<std::streamsize>(path.size()));
				Put32(out, file.fileSize);
				Put32(out, static_cast<uint32_t>(file.fileTime));
				Put32(out, static_cast<uint32_t>(file.reads.size()));
				for (const auto& [offset, bytes] : file.reads) {
					Put32(out, offset);
					Put32(out, static_cast<uint32_t>(bytes.size()));
					out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
				}
			}
			if (!out)
				return;
		}
		MoveFileExA(temp.c_str(), cachePath.c_str(), MOVEFILE_REPLACE_EXISTING);
		LOG_INFO("(VFS) Boot read cache saved: " << merged.size() << " files" << std::endl);
	}

	// Requires lazy->lock; open and position the real file on demand.
	void* Materialise(Lazy* lazy) {
		if (lazy->real)
			return lazy->real;
		void* real = nullptr;
		if (!NativeOpen()(lazy->path.c_str(), lazy->mode, &real) || !real) {
			lazyFailed.fetch_add(1, std::memory_order_relaxed);
			return nullptr;
		}
		if (lazy->position)
			reinterpret_cast<SeekFn>(natives[6])(real, lazy->position);
		lazy->real = real;
		lazyMaterialised.fetch_add(1, std::memory_order_relaxed);
		return real;
	}

	bool __cdecl Open(const char* path, int mode, void** out) {
		struct _stat32 st{};
		// Missing or unreadable files fall back to native Open.
		if (lazyOn && out && IsDlcPsarc(path) && crtStat32 && crtStat32(path, &st) == 0) {
			auto* lazy = new Lazy{ &lazyMagic };
			lazy->path = path;
			lazy->mode = mode;
			lazy->fileSize = static_cast<uint32_t>(st.st_size);
			lazy->fileTime = static_cast<int32_t>(st.st_mtime);
			{
				std::lock_guard<std::mutex> guard(cacheLock);
				auto it = cache.find(Key(lazy->path));
				if (it != cache.end() && it->second.fileSize == lazy->fileSize && it->second.fileTime == lazy->fileTime) {
					lazy->cached = &it->second;
					seen.insert(it->first);
				}
			}
			*out = lazy;
			lazyOpens.fetch_add(1, std::memory_order_relaxed);
			return true;
		}
		return NativeOpen()(path, mode, out);
	}

	void __cdecl Close(void* handle) {
		if (Lazy* lazy = AsLazy(handle)) {
			if (!lazy->recorded.empty() && !lazy->cached) {
				std::lock_guard<std::mutex> guard(cacheLock);
				CachedFile& file = fresh[Key(lazy->path)];
				file.fileSize = lazy->fileSize;
				file.fileTime = lazy->fileTime;
				for (auto& read : lazy->recorded)
					if (file.reads.size() < 16)
						file.reads.push_back(std::move(read));
			}
			if (lazy->real)
				reinterpret_cast<CloseFn>(natives[2])(lazy->real);
			else
				lazyClosedUnopened.fetch_add(1, std::memory_order_relaxed);
			lazy->magic = nullptr;
			delete lazy;
			return;
		}
		reinterpret_cast<CloseFn>(natives[2])(handle);
	}

	int __cdecl Read(void* handle, void* buffer, int size) {
		int got = 0;
		if (Lazy* lazy = AsLazy(handle)) {
			std::lock_guard<std::mutex> guard(lazy->lock);
			if (!lazy->real && lazy->cached && size > 0) {
				for (const auto& [offset, bytes] : lazy->cached->reads) {
					if (offset == static_cast<uint32_t>(lazy->position) && bytes.size() >= static_cast<size_t>(size)) {
						memcpy(buffer, bytes.data(), static_cast<size_t>(size));
						lazy->position += size;
						servedReads.fetch_add(1, std::memory_order_relaxed);
						servedBytes.fetch_add(static_cast<unsigned int>(size), std::memory_order_relaxed);
						return size;
					}
				}
			}
			if (!lazy->real) byRead.fetch_add(1, std::memory_order_relaxed);
			const int at = lazy->position;
			void* real = Materialise(lazy);
			got = real ? reinterpret_cast<ReadFn>(natives[3])(real, buffer, size) : 0;
			if (got > 0) {
				lazy->position += got;
				size_t recordedBytes = 0;
				for (const auto& read : lazy->recorded)
					recordedBytes += read.second.size();
				if (!lazy->cached && recordedBytes + static_cast<size_t>(got) <= maxRecordedPerFile)
					lazy->recorded.emplace_back(static_cast<uint32_t>(at),
						std::string(static_cast<const char*>(buffer), static_cast<size_t>(got)));
			}
		}
		else got = reinterpret_cast<ReadFn>(natives[3])(handle, buffer, size);
		return got;
	}

	long long __cdecl Size(void* handle) {
		if (Lazy* lazy = AsLazy(handle)) {
			std::lock_guard<std::mutex> guard(lazy->lock);
			if (!lazy->real) bySize.fetch_add(1, std::memory_order_relaxed);
			void* real = Materialise(lazy);
			return real ? reinterpret_cast<HandleFn>(natives[4])(real) : 0;
		}
		return reinterpret_cast<HandleFn>(natives[4])(handle);
	}

	long long __cdecl Tell(void* handle) {
		if (Lazy* lazy = AsLazy(handle)) {
			std::lock_guard<std::mutex> guard(lazy->lock);
			return lazy->real ? reinterpret_cast<HandleFn>(natives[5])(lazy->real) : lazy->position;
		}
		return reinterpret_cast<HandleFn>(natives[5])(handle);
	}

	long long __cdecl Seek(void* handle, int position) {
		if (Lazy* lazy = AsLazy(handle)) {
			std::lock_guard<std::mutex> guard(lazy->lock);
			if (lazy->real)
				return reinterpret_cast<SeekFn>(natives[6])(lazy->real, position);
			lazy->position = position;
			return 1;
		}
		return reinterpret_cast<SeekFn>(natives[6])(handle, position);
	}

	// Use msvcr100 _stat32 to match native _fstat modification-time conversion, including DST.
	std::atomic<unsigned int> statServed{ 0 };

	long long __cdecl Modification(void* handle) {
		if (Lazy* lazy = AsLazy(handle)) {
			std::lock_guard<std::mutex> guard(lazy->lock);
			if (!lazy->real) {
				const long long served = static_cast<long long>(lazy->fileTime);
				statServed.fetch_add(1, std::memory_order_relaxed);
				return served;
			}
			return reinterpret_cast<ModificationFn>(natives[7])(lazy->real);
		}
		return reinterpret_cast<ModificationFn>(natives[7])(handle);
	}

	// Cache missing folders only until main-menu readiness so later files remain visible.
	using ExistsFn = int(__cdecl*)(const char* path, int* mode);
	std::atomic<bool> existsOn{ false };
	SRWLOCK foldersLock = SRWLOCK_INIT;
	// Normalized lowercase backslash paths; fully listed folders imply absent unknown children.
	enum class Folder : unsigned char { Missing, There, Listed };
	std::unordered_map<std::string, Folder> folders;
	std::unordered_map<std::string, bool> psarcs;
	std::atomic<unsigned int> existsCalls{ 0 }, existsAnswered{ 0 }, existsKnown{ 0 }, folderQueries{ 0 };

	void Set(const std::string& folder, Folder state) {
		AcquireSRWLockExclusive(&foldersLock);
		folders[folder] = state;
		ReleaseSRWLockExclusive(&foldersLock);
	}

	Folder List(const std::string& folder) {
		folderQueries.fetch_add(1, std::memory_order_relaxed);
		std::vector<std::string> children;
		WIN32_FIND_DATAA data{};
		HANDLE find = FindFirstFileExA((folder + R"(\*)").c_str(), FindExInfoBasic, &data, FindExSearchLimitToDirectories,
			nullptr, FIND_FIRST_EX_LARGE_FETCH);
		if (find == INVALID_HANDLE_VALUE) {
			// Unreadable folders stay unlisted so children still receive individual disk probes.
			Set(folder, Folder::There);
			return Folder::There;
		}
		do {
			if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || strcmp(data.cFileName, ".") == 0 || strcmp(data.cFileName, "..") == 0)
				continue;
			std::string child = folder + R"(\)" + data.cFileName;
			for (char& c : child)
				c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
			children.push_back(std::move(child));
		} while (FindNextFileA(find, &data));
		FindClose(find);
		AcquireSRWLockExclusive(&foldersLock);
		for (auto& child : children)
			folders.emplace(std::move(child), Folder::There);
		folders[folder] = Folder::Listed;
		ReleaseSRWLockExclusive(&foldersLock);
		return Folder::Listed;
	}

	Folder State(const std::string& folder) {
		AcquireSRWLockShared(&foldersLock);
		const auto it = folders.find(folder);
		const bool known = it != folders.end();
		const Folder state = known ? it->second : Folder::Missing;
		ReleaseSRWLockShared(&foldersLock);
		if (known)
			return state == Folder::There ? List(folder) : state;
		const size_t cut = folder.find_last_of('\\');
		if (cut != std::string::npos && cut != 0 && folder[cut - 1] != ':') {
			const Folder parent = State(folder.substr(0, cut));
			if (parent == Folder::Missing) {
				Set(folder, Folder::Missing);
				return Folder::Missing;
			}
			if (parent == Folder::Listed) {
				AcquireSRWLockShared(&foldersLock);
				const bool entered = folders.count(folder) != 0;
				ReleaseSRWLockShared(&foldersLock);
				if (entered)
					return State(folder);
				Set(folder, Folder::Missing);
				return Folder::Missing;
			}
		}
		const DWORD attributes = GetFileAttributesA(folder.c_str());
		if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
			folderQueries.fetch_add(1, std::memory_order_relaxed);
			Set(folder, Folder::Missing);
			return Folder::Missing;
		}
		return List(folder);
	}

	bool Missing(const std::string& folder) {
		return State(folder) == Folder::Missing;
	}

	bool FolderMissing(const char* path) {
		const char* cut = nullptr;
		for (const char* c = path; *c; ++c) {
			if (*c == '%')
				return false;
			if (*c == '\\' || *c == '/')
				cut = c;
		}
		if (!cut || cut == path)
			return false;
		std::string folder(path, cut);
		for (char& c : folder)
			c = c == '/' ? '\\' : static_cast<char>(tolower(static_cast<unsigned char>(c)));
		return Missing(folder);
	}

	int __cdecl Exists(const char* path, int* mode) {
		const auto native = reinterpret_cast<ExistsFn>(natives[1]);
		if (!existsOn.load(std::memory_order_relaxed) || !path)
			return native(path, mode);
		existsCalls.fetch_add(1, std::memory_order_relaxed);
		// Answer read-mode existence probes from one metadata lookup per package per boot.
		if (IsDlcPsarc(path) && (!mode || *mode == 0 || *mode == 3)) {
			std::string key = Key(path);
			for (char& c : key)
				if (c == '/')
					c = '\\';
			AcquireSRWLockShared(&foldersLock);
			const auto it = psarcs.find(key);
			const bool known = it != psarcs.end(), present = known && it->second;
			ReleaseSRWLockShared(&foldersLock);
			bool there = present;
			if (!known) {
				const DWORD attributes = GetFileAttributesA(path);
				there = attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
				AcquireSRWLockExclusive(&foldersLock);
				psarcs.emplace(std::move(key), there);
				ReleaseSRWLockExclusive(&foldersLock);
			}
			else
				existsKnown.fetch_add(1, std::memory_order_relaxed);
			return there ? 1 : 0;
		}
		if (!FolderMissing(path))
			return native(path, mode);
		existsAnswered.fetch_add(1, std::memory_order_relaxed);
		return 0;
	}

	bool Swap(int index, uintptr_t value) {
		const auto slot = reinterpret_cast<uintptr_t*>(table + index * 4);
		return MemUtil::PatchAdr(reinterpret_cast<LPVOID>(slot), &value, sizeof(value));
	}
}

void LazyVfs::Install() {
	// Install only when callback and native targets are mapped for this build.
	const Framework::GameVersion::Site tableSite{ "GRVFS callback table", table, 0 };
	if (!GameVersion::IsSeptember2022())
		return;
	if (!GameVersion::CheckedSite("VFS", tableSite, reinterpret_cast<const std::uint8_t*>(natives), nullptr, sizeof(natives))) {
		MemUtil::FailBootPatchBundle();
		return;
	}
	if (HMODULE crt = GetModuleHandleA("msvcr100.dll"))
		crtStat32 = reinterpret_cast<Stat32Fn>(GetProcAddress(crt, "_stat32"));
	if (!crtStat32) {
		MemUtil::FailBootPatchBundle();
		LOG_WARNING("(VFS) msvcr100 _stat32 not found; lazy psarc opens not installed" << std::endl);
		return;
	}
	char exe[MAX_PATH]{};
	GetModuleFileNameA(nullptr, exe, MAX_PATH);
	cachePath = exe;
	cachePath = cachePath.substr(0, cachePath.find_last_of(R"(\)") + 1) + "RSMods_vfs.cache";
	LoadCache();
	cacheLoaded.store(true, std::memory_order_release);
	LOG_INFO("(VFS) Boot read cache loaded: " << cache.size() << " files" << std::endl);
	const uintptr_t hooks[] = { reinterpret_cast<uintptr_t>(&Open), reinterpret_cast<uintptr_t>(&Exists),
		reinterpret_cast<uintptr_t>(&Close), reinterpret_cast<uintptr_t>(&Read), reinterpret_cast<uintptr_t>(&Size),
		reinterpret_cast<uintptr_t>(&Tell), reinterpret_cast<uintptr_t>(&Seek), reinterpret_cast<uintptr_t>(&Modification) };
	// Publish Open last so lazy handles cannot reach uninstalled wrappers.
	const int order[] = { 1, 2, 3, 4, 5, 6, 7, 0 };
	for (int index : order) {
		if (Swap(index, hooks[index]))
			continue;
		bool restored = true;
		for (int rollback : order)
			if (!Swap(rollback, natives[rollback]))
				restored = false;
		LOG_WARNING("(VFS) Callback patch failed; rollback " << (restored ? "complete" : "failed") << std::endl);
		return;
	}
	lazyOn = true;
	existsOn = true;
	LOG_INFO("(VFS) Exists answered from folder checks until the main menu is ready" << std::endl);
	LOG_INFO("(VFS) Native file callbacks wrapped: lazy DLC psarc opens on" << std::endl);
}

bool LazyVfs::Covers(const wchar_t* path) {
	if (!MemUtil::BootPatchBundleEnabled() || !lazyOn || !path || !cacheLoaded.load(std::memory_order_acquire))
		return false;
	char narrow[MAX_PATH * 2]{};
	if (!WideCharToMultiByte(CP_ACP, 0, path, -1, narrow, sizeof(narrow), nullptr, nullptr))
		return false;
	std::string key = Key(narrow);
	for (char& c : key)
		if (c == '/')
			c = '\\';
	return cache.count(key) != 0;
}

void LazyVfs::Report(const char* when) {
	AcquireSRWLockShared(&foldersLock);
	const size_t folderCount = folders.size();
	ReleaseSRWLockShared(&foldersLock);
	if (lazyOn && strcmp(when, "scan done") == 0)
		SaveCache();
	if (strcmp(when, "main menu ready") == 0)
		existsOn = false;
	LOG_INFO("(VFS) " << when << ": Exists calls " << existsCalls.load() << ", known DLC psarcs " << existsKnown.load() << ", answered from folder checks "
		<< existsAnswered.load() << ", folders known " << folderCount << " (asked the disk " << folderQueries.load() << ")" << std::endl);
	LOG_INFO("(VFS) " << when << ": lazy DLC psarc opens " << lazyOpens.load() << ", opened for real "
		<< lazyMaterialised.load() << ", closed without opening " << lazyClosedUnopened.load() << ", failed "
		<< lazyFailed.load() << " (first need: read " << byRead.load() << ", size " << bySize.load() << ", modification "
		<< byModification.load() << "); reads served from the boot read cache " << servedReads.load() << " ("
		<< servedBytes.load() / 1024 << " KB); modification served by _stat32 " << statServed.load()
		<< (crtStat32 ? "" : " (msvcr100 _stat32 not found)") << std::endl);
}
