#pragma once

#include <windows.h>

#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "../Framework/CacheFile.hpp"

// Persist every cache section in one file next to the game, written by one writer at a time.
class CacheStore {
public:
	struct Legacy {
		uint32_t id;
		const wchar_t* name;
	};

	struct LoadResult {
		Framework::CacheFile::Parsed parsed;
		DWORD readError = 0;
		bool migrated = false;
		DWORD migrateError = 0;
	};

	static constexpr const wchar_t* kName = L"RSMods.cache";
	static constexpr Legacy kLegacy[] = {
		{ Framework::CacheFile::kEnumeration, L"RSMods_enumeration.cache" },
		{ Framework::CacheFile::kVfs, L"RSMods_vfs.cache" },
	};

	// The folder ends with a path separator.
	LoadResult Load(const std::wstring& folder, const std::string& identity, std::map<uint32_t, uint32_t> versions) {
		std::lock_guard<std::mutex> writer(writeMutex);
		folder_ = folder;
		identity_ = identity;
		versions_ = std::move(versions);
		LoadResult result;
		std::vector<unsigned char> bytes;
		if (ReadAll(Path(), bytes, result.readError)) {
			result.parsed = Framework::CacheFile::Parse(bytes, identity_, versions_);
			sections_ = result.parsed.sections;
			DeleteLegacy();
			return result;
		}
		if (result.readError != ERROR_FILE_NOT_FOUND) return result;

		// Carry the old per-feature files over; their payloads are the section formats.
		Framework::CacheFile::Sections carried;
		for (const auto& legacy : kLegacy) {
			std::vector<unsigned char> old;
			DWORD ignored = 0;
			const auto version = versions_.find(legacy.id);
			if (version != versions_.end() && ReadAll(folder_ + legacy.name, old, ignored))
				carried[legacy.id] = { version->second, std::move(old) };
		}
		sections_ = carried;
		result.parsed.status = Framework::CacheFile::Status::Ok;
		result.parsed.sections = carried;
		if (carried.empty()) {
			DeleteLegacy();
			return result;
		}
		if (WriteFileAtomically(Framework::CacheFile::Build(identity_, carried), result.migrateError)) {
			result.migrated = true;
			DeleteLegacy();
		}
		return result;
	}

	std::wstring Path() const { return folder_ + kName; }
	bool Ready() const { return !folder_.empty(); }

	bool Take(uint32_t id, std::vector<unsigned char>& payload) const {
		std::lock_guard<std::mutex> writer(writeMutex);
		const auto found = sections_.find(id);
		if (found == sections_.end()) return false;
		payload = found->second.payload;
		return true;
	}

	// Replace one section and rewrite the file. The gate receives the final rename and may refuse it.
	template <typename Gate>
	bool Save(uint32_t id, std::vector<unsigned char> payload, Gate&& gate, DWORD& error, size_t* fileBytes = nullptr) {
		std::lock_guard<std::mutex> writer(writeMutex);
		const auto version = versions_.find(id);
		if (folder_.empty() || version == versions_.end()) {
			error = ERROR_INVALID_FUNCTION;
			return false;
		}
		auto next = sections_;
		next[id] = { version->second, std::move(payload) };
		const auto bytes = Framework::CacheFile::Build(identity_, next);
		if (fileBytes) *fileBytes = bytes.size();
		if (!WriteTemp(bytes, error)) return false;
		const bool moved = gate([&] { return Replace(error); });
		if (!moved) {
			if (!error) error = ERROR_CANCELLED;
			DeleteFileW(TempPath().c_str());
			return false;
		}
		sections_ = std::move(next);
		DeleteLegacy();
		return true;
	}

	bool Save(uint32_t id, std::vector<unsigned char> payload, DWORD& error, size_t* fileBytes = nullptr) {
		return Save(id, std::move(payload), [](auto&& replace) { return replace(); }, error, fileBytes);
	}

private:
	std::wstring TempPath() const { return Path() + L".tmp"; }

	static bool ReadAll(const std::wstring& path, std::vector<unsigned char>& bytes, DWORD& error) {
		const HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
		if (file == INVALID_HANDLE_VALUE) {
			error = GetLastError();
			return false;
		}
		LARGE_INTEGER size{};
		bool ok = GetFileSizeEx(file, &size) != FALSE;
		if (ok && size.QuadPart > static_cast<LONGLONG>(Framework::CacheFile::kMaxFile)) {
			SetLastError(ERROR_FILE_TOO_LARGE);
			ok = false;
		}
		if (ok) {
			bytes.resize(static_cast<size_t>(size.QuadPart));
			DWORD read = 0;
			ok = bytes.empty() || (ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) && read == bytes.size());
		}
		error = ok ? 0 : GetLastError();
		CloseHandle(file);
		return ok;
	}

	bool WriteTemp(const std::vector<unsigned char>& bytes, DWORD& error) const {
		const std::wstring temp = TempPath();
		const HANDLE file = CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE) {
			error = GetLastError();
			return false;
		}
		DWORD written = 0;
		bool ok = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size()
			&& FlushFileBuffers(file);
		error = ok ? 0 : GetLastError();
		CloseHandle(file);
		if (!ok) DeleteFileW(temp.c_str());
		return ok;
	}

	// Retry briefly when a scanner or another process holds the target open.
	bool Replace(DWORD& error) const {
		for (int attempt = 0; attempt < 5; ++attempt) {
			if (MoveFileExW(TempPath().c_str(), Path().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
				error = 0;
				return true;
			}
			error = GetLastError();
			if (error != ERROR_ACCESS_DENIED && error != ERROR_SHARING_VIOLATION && error != ERROR_LOCK_VIOLATION) break;
			Sleep(20u << attempt);
		}
		return false;
	}

	bool WriteFileAtomically(const std::vector<unsigned char>& bytes, DWORD& error) const {
		if (!WriteTemp(bytes, error)) return false;
		if (Replace(error)) return true;
		DeleteFileW(TempPath().c_str());
		return false;
	}

	// Call only after the combined file is on disk.
	void DeleteLegacy() const {
		for (const auto& legacy : kLegacy) {
			const std::wstring path = folder_ + legacy.name;
			DeleteFileW(path.c_str());
			DeleteFileW((path + L".tmp").c_str());
		}
	}

	mutable std::mutex writeMutex;
	std::wstring folder_;
	std::string identity_;
	std::map<uint32_t, uint32_t> versions_;
	Framework::CacheFile::Sections sections_;
};

namespace RSModsCache {
	// Load the combined file on first use and migrate the old per-feature files.
	CacheStore& Shared();
}
