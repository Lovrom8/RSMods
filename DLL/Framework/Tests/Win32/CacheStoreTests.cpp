#include "../../../Mods/CacheStore.hpp"

#include <iostream>

namespace {
	int failures = 0;

	void Check(bool condition, const char* what) {
		if (!condition) {
			++failures;
			std::cerr << "FAIL: " << what << "\n";
		}
	}

	using Framework::CacheFile::kEnumeration;
	using Framework::CacheFile::kVfs;

	const std::map<uint32_t, uint32_t> versions = { { kEnumeration, 1 }, { kVfs, 1 } };

	bool Exists(const std::wstring& path) {
		return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
	}

	void Put(const std::wstring& path, const std::vector<unsigned char>& bytes) {
		const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		DWORD written = 0;
		WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr);
		CloseHandle(file);
	}

	void Clear(const std::wstring& folder) {
		for (const wchar_t* name : { L"RSMods.cache", L"RSMods.cache.tmp", L"RSMods_enumeration.cache",
			L"RSMods_enumeration.cache.tmp", L"RSMods_vfs.cache", L"RSMods_vfs.cache.tmp" })
			DeleteFileW((folder + name).c_str());
	}
}

int main() {
	wchar_t temp[MAX_PATH]{};
	GetTempPathW(MAX_PATH, temp);
	const std::wstring folder = std::wstring(temp) + L"RSModsCacheStoreTests" + std::to_wstring(GetCurrentProcessId()) + L"\\";
	CreateDirectoryW(folder.c_str(), nullptr);
	Clear(folder);
	const std::vector<unsigned char> oldEnumeration = { 1, 2, 3 };
	const std::vector<unsigned char> oldVfs = { 4, 5 };
	std::vector<unsigned char> payload;

	{
		Put(folder + L"RSMods_enumeration.cache", oldEnumeration);
		Put(folder + L"RSMods_vfs.cache", oldVfs);
		Put(folder + L"RSMods_vfs.cache.tmp", { 0 });
		CacheStore store;
		const auto result = store.Load(folder, "game A", versions);
		Check(result.migrated, "old files migrated");
		Check(Exists(folder + L"RSMods.cache"), "combined file written");
		Check(!Exists(folder + L"RSMods_enumeration.cache") && !Exists(folder + L"RSMods_vfs.cache"), "old files deleted after migration");
		Check(!Exists(folder + L"RSMods_vfs.cache.tmp"), "old temporary file deleted");
		Check(store.Take(kEnumeration, payload) && payload == oldEnumeration, "enumeration carried over");
		Check(store.Take(kVfs, payload) && payload == oldVfs, "vfs carried over");
	}
	{
		CacheStore store;
		store.Load(folder, "game A", versions);
		Check(store.Take(kEnumeration, payload) && payload == oldEnumeration, "migrated file reloads");
		DWORD error = 0;
		Check(store.Save(kVfs, { 6, 6, 6 }, error), "section saved");
		Check(!Exists(folder + L"RSMods.cache.tmp"), "no temporary file left after save");
		Check(!store.Save(kVfs, { 7 }, [](auto&&) { return false; }, error) && error == ERROR_CANCELLED, "gate refuses rename");
		Check(!Exists(folder + L"RSMods.cache.tmp"), "refused save removes its temporary file");
	}
	{
		CacheStore store;
		store.Load(folder, "game A", versions);
		Check(store.Take(kVfs, payload) && payload == std::vector<unsigned char>{ 6, 6, 6 }, "saved section survives, refused one does not");
		Check(store.Take(kEnumeration, payload) && payload == oldEnumeration, "untouched section kept by another section's save");
	}
	{
		CacheStore store;
		const auto result = store.Load(folder, "game B", versions);
		Check(result.parsed.status == Framework::CacheFile::Status::OtherIdentity, "other install detected");
		Check(!store.Take(kEnumeration, payload) && !store.Take(kVfs, payload), "other install drops every section");
	}
	{
		Clear(folder);
		Put(folder + L"RSMods_vfs.cache", oldVfs);
		CreateDirectoryW((folder + L"RSMods.cache").c_str(), nullptr);
		CacheStore store;
		const auto result = store.Load(folder, "game A", versions);
		Check(!result.migrated, "unreadable combined file is not replaced by migration");
		Check(Exists(folder + L"RSMods_vfs.cache"), "old file kept while the combined file cannot be written");
		RemoveDirectoryW((folder + L"RSMods.cache").c_str());
	}
	{
		Clear(folder);
		Put(folder + L"RSMods_vfs.cache", oldVfs);
		CacheStore store;
		const auto result = store.Load(folder, "game A", versions);
		Check(result.migrated && store.Take(kVfs, payload) && payload == oldVfs && !store.Take(kEnumeration, payload), "one old file migrates alone");
	}
	Clear(folder);
	RemoveDirectoryW(folder.c_str());

	if (failures) {
		std::cerr << failures << " cache store checks failed\n";
		return 1;
	}
	std::cout << "Cache store ALL PASS\n";
	return 0;
}
