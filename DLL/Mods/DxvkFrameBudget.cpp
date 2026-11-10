#include "../stdafx.h"
#include "DxvkFrameBudget.hpp"
#include "UltrafastLoadMod.hpp"

#include <string>

namespace {
	// PreInit stores this double in the service manager at +0xB0.
	constexpr Framework::GameVersion::Site kBudgetLoadSite{ "boot frame budget", 0x005A43CF, 0 };
	constexpr unsigned char kBudgetLoadBytes[] = { 0xDD, 0x05, 0x28, 0x44, 0x22, 0x01, 0x8B, 0x46, 0x04, 0xDD, 0x98, 0xB0, 0x00, 0x00, 0x00 };
	const double kDxvkBudgetMs = 0.0;
	constexpr Framework::GameVersion::Site kServiceGlobalSite{ "service manager", 0x0135FBFC, 0 };
	uintptr_t kServiceGlobal = 0;
	constexpr uintptr_t kBudgetOffset = 0xB0;
	bool g_enforce = false;
	bool g_corrected = false;

	// The manager may not exist until boot services start.
	bool ClearBudget(double& value) {
		__try {
			const uintptr_t service = *reinterpret_cast<const uintptr_t*>(kServiceGlobal);
			if (!service)
				return false;
			const uintptr_t manager = *reinterpret_cast<const uintptr_t*>(service + 4);
			if (!manager)
				return false;
			volatile double* slot = reinterpret_cast<volatile double*>(manager + kBudgetOffset);
			value = *slot;
			if (value > 0.0)
				*slot = 0.0;
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	std::wstring DirectoryOf(const std::wstring& path) {
		const size_t slash = path.find_last_of(L"\\/");
		return slash == std::wstring::npos ? std::wstring() : path.substr(0, slash + 1);
	}

	std::wstring ModulePath(HMODULE module) {
		std::wstring path(MAX_PATH, L'\0');
		for (;;) {
			const DWORD length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
			if (length == 0)
				return {};
			if (length < path.size()) {
				path.resize(length);
				return path;
			}
			path.resize(path.size() * 2);
		}
	}
	struct FileView {
		const unsigned char* data = nullptr;
		size_t size = 0;

		bool Has(size_t offset, size_t length) const { return offset <= size && length <= size - offset; }
		template <typename T> const T* At(size_t offset) const {
			return Has(offset, sizeof(T)) ? reinterpret_cast<const T*>(data + offset) : nullptr;
		}
	};
	bool RvaToOffset(const FileView& view, const IMAGE_SECTION_HEADER* sections, WORD count, DWORD rva, size_t& offset) {
		for (WORD i = 0; i < count; ++i) {
			const DWORD start = sections[i].VirtualAddress;
			const DWORD span = sections[i].SizeOfRawData;
			if (rva >= start && rva - start < span) {
				offset = static_cast<size_t>(sections[i].PointerToRawData) + (rva - start);
				return view.Has(offset, 0);
			}
		}
		return false;
	}
	constexpr WORD kAnyId = 0xFFFF;
	bool FindResourceEntry(const FileView& view, size_t resourceBase, size_t resourceSize, size_t nodeOffset, WORD id,
		DWORD& target, bool& isDirectory) {
		if (nodeOffset > resourceSize)
			return false;
		const auto* node = view.At<IMAGE_RESOURCE_DIRECTORY>(resourceBase + nodeOffset);
		if (!node)
			return false;
		const size_t entries = static_cast<size_t>(node->NumberOfNamedEntries) + node->NumberOfIdEntries;
		const size_t first = resourceBase + nodeOffset + sizeof(IMAGE_RESOURCE_DIRECTORY);
		for (size_t i = 0; i < entries; ++i) {
			const auto* entry = view.At<IMAGE_RESOURCE_DIRECTORY_ENTRY>(first + i * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));
			if (!entry)
				return false;
			if (entry->NameIsString)
				continue;
			if (id != kAnyId && entry->Id != id)
				continue;
			target = entry->OffsetToData;
			isDirectory = (target & IMAGE_RESOURCE_DATA_IS_DIRECTORY) != 0;
			target &= ~static_cast<DWORD>(IMAGE_RESOURCE_DATA_IS_DIRECTORY);
			return true;
		}
		return false;
	}
	struct VersionBlock {
		size_t valueLength = 0;
		std::wstring key;
		size_t valueOffset = 0;
		size_t childOffset = 0;
		size_t end = 0;
	};

	size_t Align4(size_t value) { return (value + 3) & ~static_cast<size_t>(3); }

	bool ReadBlock(const FileView& view, size_t offset, size_t limit, VersionBlock& block) {
		if (limit > view.size || offset > limit || limit - offset < 6)
			return false;
		const auto* header = view.At<WORD>(offset);
		if (!header)
			return false;
		const size_t length = header[0];
		block.valueLength = header[1];
		if (length < 6 || length > limit - offset)
			return false;
		block.end = offset + length;
		size_t cursor = offset + 6;
		block.key.clear();
		for (;;) {
			if (cursor + 2 > block.end)
				return false;
			const auto* ch = view.At<wchar_t>(cursor);
			if (!ch)
				return false;
			cursor += 2;
			if (*ch == 0)
				break;
			if (block.key.size() > 64)
				return false;
			block.key.push_back(*ch);
		}
		block.valueOffset = Align4(cursor);
		block.childOffset = block.valueOffset;
		return block.valueOffset <= block.end;
	}
	bool VersionBlobHasDxvkProductName(const FileView& view, size_t start, size_t end) {
		VersionBlock root;
		if (!ReadBlock(view, start, end, root) || root.key != L"VS_VERSION_INFO")
			return false;
		size_t cursor = Align4(root.valueOffset + root.valueLength);
		while (cursor < root.end) {
			VersionBlock info;
			if (!ReadBlock(view, cursor, root.end, info))
				return false;
			if (info.key == L"StringFileInfo") {
				size_t tableCursor = info.childOffset;
				while (tableCursor < info.end) {
					VersionBlock table;
					if (!ReadBlock(view, tableCursor, info.end, table))
						return false;
					size_t stringCursor = table.childOffset;
					while (stringCursor < table.end) {
						VersionBlock entry;
						if (!ReadBlock(view, stringCursor, table.end, entry))
							return false;
						if (entry.key == L"ProductName" && entry.valueLength <= 64 && view.Has(entry.valueOffset, entry.valueLength * 2)) {
							std::wstring value(reinterpret_cast<const wchar_t*>(view.data + entry.valueOffset), entry.valueLength);
							while (!value.empty() && value.back() == 0)
								value.pop_back();
							if (value == L"DXVK")
								return true;
						}
						stringCursor = Align4(entry.end);
					}
					tableCursor = Align4(table.end);
				}
			}
			cursor = Align4(info.end);
		}
		return false;
	}

	bool ViewProductNameIsDxvk(const FileView& view) {
		const auto* dos = view.At<IMAGE_DOS_HEADER>(0);
		if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0)
			return false;
		const size_t ntOffset = static_cast<size_t>(dos->e_lfanew);
		const auto* signature = view.At<DWORD>(ntOffset);
		const auto* fileHeader = view.At<IMAGE_FILE_HEADER>(ntOffset + 4);
		if (!signature || *signature != IMAGE_NT_SIGNATURE || !fileHeader)
			return false;
		const size_t optionalOffset = ntOffset + 4 + sizeof(IMAGE_FILE_HEADER);
		const auto* magic = view.At<WORD>(optionalOffset);
		if (!magic)
			return false;

		DWORD directoryRva = 0;
		DWORD directorySize = 0;
		if (*magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
			const auto* optional = view.At<IMAGE_OPTIONAL_HEADER32>(optionalOffset);
			if (!optional || optional->NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_RESOURCE)
				return false;
			directoryRva = optional->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
			directorySize = optional->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
		}
		else if (*magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
			const auto* optional = view.At<IMAGE_OPTIONAL_HEADER64>(optionalOffset);
			if (!optional || optional->NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_RESOURCE)
				return false;
			directoryRva = optional->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
			directorySize = optional->DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size;
		}
		else
			return false;
		if (directoryRva == 0 || directorySize == 0)
			return false;

		const size_t sectionsOffset = optionalOffset + fileHeader->SizeOfOptionalHeader;
		const WORD sectionCount = fileHeader->NumberOfSections;
		if (sectionCount > 96 || !view.Has(sectionsOffset, sectionCount * sizeof(IMAGE_SECTION_HEADER)))
			return false;
		const auto* sections = view.At<IMAGE_SECTION_HEADER>(sectionsOffset);

		size_t resourceBase = 0;
		if (!sections || !RvaToOffset(view, sections, sectionCount, directoryRva, resourceBase) || !view.Has(resourceBase, directorySize))
			return false;
		DWORD target = 0;
		bool isDirectory = false;
		if (!FindResourceEntry(view, resourceBase, directorySize, 0, static_cast<WORD>(reinterpret_cast<uintptr_t>(RT_VERSION)), target, isDirectory) || !isDirectory)
			return false;
		if (!FindResourceEntry(view, resourceBase, directorySize, target, kAnyId, target, isDirectory) || !isDirectory)
			return false;
		if (!FindResourceEntry(view, resourceBase, directorySize, target, kAnyId, target, isDirectory) || isDirectory)
			return false;
		if (target > directorySize)
			return false;

		const auto* data = view.At<IMAGE_RESOURCE_DATA_ENTRY>(resourceBase + target);
		size_t blobOffset = 0;
		if (!data || data->Size == 0 || !RvaToOffset(view, sections, sectionCount, data->OffsetToData, blobOffset))
			return false;
		if (!view.Has(blobOffset, data->Size))
			return false;
		return VersionBlobHasDxvkProductName(view, blobOffset, blobOffset + data->Size);
	}

	bool MappedFileProductNameIsDxvk(HANDLE file, size_t size) {
		HANDLE mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
		if (!mapping)
			return false;
		bool result = false;
		if (void* base = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0)) {
			FileView view;
			view.data = static_cast<const unsigned char*>(base);
			view.size = size;
			__try {
				result = ViewProductNameIsDxvk(view);
			}
			__except (GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
				result = false;
			}
			UnmapViewOfFile(base);
		}
		CloseHandle(mapping);
		return result;
	}
	// Read the resource directly because installation runs under the loader lock.
	bool ProductNameIsDxvk(const std::wstring& path) {
		HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE)
			return false;
		bool result = false;
		LARGE_INTEGER fileSize{};
		if (GetFileSizeEx(file, &fileSize) && fileSize.QuadPart > 0 && fileSize.QuadPart <= 256LL * 1024 * 1024)
			result = MappedFileProductNameIsDxvk(file, static_cast<size_t>(fileSize.QuadPart));
		CloseHandle(file);
		return result;
	}
	bool GameUsesDxvk() {
		const std::wstring gameFolder = DirectoryOf(ModulePath(nullptr));
		if (gameFolder.empty())
			return false;

		std::wstring d3d9Path = gameFolder + L"d3d9.dll";
		if (const HMODULE loaded = GetModuleHandleW(L"d3d9.dll")) {
			d3d9Path = ModulePath(loaded);
			if (_wcsicmp(DirectoryOf(d3d9Path).c_str(), gameFolder.c_str()) != 0)
				return false;
		}
		else if (GetFileAttributesW(d3d9Path.c_str()) == INVALID_FILE_ATTRIBUTES)
			return false;

		return ProductNameIsDxvk(d3d9Path);
	}
}

void DxvkFrameBudget::Install() {
	if (!UltrafastLoad::Enabled())
		return;
	if (!GameVersion::IsSeptember2022()) {
		LOG_INFO("(DXVK) Boot frame budget disabled: unsupported executable " << Framework::GameVersion::BuildName(GameVersion::Current()) << std::endl);
		return;
	}
	if (!GameUsesDxvk())
		return;
	const uintptr_t budgetLoad = GameVersion::CheckedSite("DXVK", kBudgetLoadSite,
		kBudgetLoadBytes, nullptr, sizeof(kBudgetLoadBytes));
	if (!budgetLoad)
		return;
	kServiceGlobal = GameVersion::Resolve(kServiceGlobalSite);
	if (!kServiceGlobal)
		return;
	const uintptr_t address = reinterpret_cast<uintptr_t>(&kDxvkBudgetMs);
	if (!MemUtil::PatchAdr(reinterpret_cast<LPVOID>(budgetLoad + 2), &address, sizeof(address))) {
		LOG_ERROR("(DXVK) Frame budget patch failed; budget left at 30 ms" << std::endl);
		return;
	}
	g_enforce = true;
	LOG_INFO("(DXVK) d3d9.dll in the game folder is DXVK; boot frame budget changed from 30 ms to 0 ms" << std::endl);
}

void DxvkFrameBudget::Enforce() {
	if (!g_enforce)
		return;
	double value = 0.0;
	if (!ClearBudget(value) || !(value > 0.0))
		return;
	if (!g_corrected) {
		g_corrected = true;
		LOG_WARNING("(DXVK) Frame budget at service manager +0xB0 was " << value << " ms after the boot patch; forced to 0 ms (logged once)" << std::endl);
	}
}
