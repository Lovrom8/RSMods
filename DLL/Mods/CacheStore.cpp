#include "../stdafx.h"
#include "CacheStore.hpp"

#include <sstream>

namespace {
	std::once_flag loadOnce;
	CacheStore store;

	std::string Utf8(const std::wstring& text) {
		if (text.empty()) return {};
		const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
		std::string out(static_cast<size_t>(size > 0 ? size : 0), '\0');
		if (size > 0)
			WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), out.data(), size, nullptr, nullptr);
		return out;
	}

	// Bind the file to the game folder and executable so a moved or patched install starts clean.
	std::string Identity(const std::wstring& folder, const std::wstring& exe) {
		std::ostringstream out;
		out << "root " << Utf8(folder);
		WIN32_FILE_ATTRIBUTE_DATA data{};
		if (GetFileAttributesExW(exe.c_str(), GetFileExInfoStandard, &data))
			out << "; exe " << ((static_cast<unsigned long long>(data.nFileSizeHigh) << 32) | data.nFileSizeLow) << ","
				<< ((static_cast<unsigned long long>(data.ftLastWriteTime.dwHighDateTime) << 32) | data.ftLastWriteTime.dwLowDateTime);
		else
			out << "; exe unknown";
		const auto* base = reinterpret_cast<const unsigned char*>(GetModuleHandleW(nullptr));
		const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
		if (base && dos->e_magic == IMAGE_DOS_SIGNATURE) {
			const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
			if (nt->Signature == IMAGE_NT_SIGNATURE)
				out << std::hex << "; pe " << nt->FileHeader.TimeDateStamp << "," << nt->OptionalHeader.SizeOfImage << std::dec;
		}
		return out.str();
	}

	const char* Describe(Framework::CacheFile::Status status) {
		switch (status) {
		case Framework::CacheFile::Status::Ok: return "ok";
		case Framework::CacheFile::Status::OtherFormat: return "different format";
		case Framework::CacheFile::Status::OtherIdentity: return "built for another game install";
		default: return "damaged header";
		}
	}

	void LoadOnce() {
		wchar_t exe[MAX_PATH * 2]{};
		const DWORD length = GetModuleFileNameW(nullptr, exe, MAX_PATH * 2);
		if (length == 0 || length >= MAX_PATH * 2) {
			LOG_WARNING("(CACHE) Cache off: could not find the game folder" << std::endl);
			return;
		}
		const std::wstring path(exe, length);
		const std::wstring folder = path.substr(0, path.find_last_of(L"\\/") + 1);
		const auto result = store.Load(folder, Identity(folder, path), {
			{ Framework::CacheFile::kEnumeration, 1 },
			{ Framework::CacheFile::kVfs, 1 },
		});
		const std::string file = Utf8(store.Path());
		if (result.migrated)
			LOG_INFO("(CACHE) Moved the old cache files into " << file << std::endl);
		else if (result.migrateError)
			LOG_WARNING("(CACHE) Could not write " << file << " (error " << result.migrateError << "); old cache files kept" << std::endl);
		else if (result.readError && result.readError != ERROR_FILE_NOT_FOUND)
			LOG_WARNING("(CACHE) Could not read " << file << " (error " << result.readError << ")" << std::endl);
		else if (result.parsed.status != Framework::CacheFile::Status::Ok)
			LOG_INFO("(CACHE) " << file << " dropped: " << Describe(result.parsed.status) << std::endl);
		for (const uint32_t id : result.parsed.dropped)
			LOG_INFO("(CACHE) Section " << id << " dropped: damaged or a different version" << std::endl);
		if (result.parsed.truncated)
			LOG_INFO("(CACHE) " << file << " ends early; later sections dropped" << std::endl);
	}
}

CacheStore& RSModsCache::Shared() {
	std::call_once(loadOnce, LoadOnce);
	return store;
}
