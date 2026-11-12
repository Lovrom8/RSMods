#include "../stdafx.h"
#include "TitleFade.hpp"
#include "TitleFadeBytes.hpp"
#include "UltrafastLoadMod.hpp"

#include <atomic>

namespace {
	uintptr_t kFileOpenEx = 0;
	constexpr unsigned char kFileOpenExBytes[] = {
		0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x83, 0xEC,
		0x1C, 0xA1, 0x40, 0xB2, 0x25, 0x01
	};
	uintptr_t kFileRead = 0;
	constexpr unsigned char kFileReadBytes[] = {
		0x55, 0x8B, 0xEC, 0x56, 0x8B, 0xF1, 0x83, 0x7E, 0x08, 0x00
	};
	constexpr uintptr_t kFileNameOffset = 0x0C;
	constexpr char kTitleFileToken[] = "titlescreen";
	constexpr char kMainMenuFileToken[] = "\\mainmenu.gfx";
	using FileReadFn = int(__thiscall*)(void* file, void* buffer, int length);
	using TellFn = int64_t(__thiscall*)(void*);
	using LengthFn = int(__thiscall*)(void*);
	using SeekFn = int(__thiscall*)(void*, int, int);
	uintptr_t g_fileTell = 0, g_fileLength = 0, g_fileSeek = 0;
	uintptr_t g_fileReadTrampoline = 0;
	std::atomic<unsigned int> g_patchedMask = 0;
	std::atomic<unsigned int> g_mainMenuPatchedMask = 0;
	std::atomic<unsigned int> g_titleOutPatchedMask = 0;
	std::atomic<bool> g_mainMenuDone = false;
	std::atomic<bool> g_loggedMainMenuReload = false;
	std::atomic<bool> g_loggedPositionLost = false;


	unsigned char LowerAscii(unsigned char value) {
		return value >= 'A' && value <= 'Z' ? static_cast<unsigned char>(value + ('a' - 'A')) : value;
	}

	bool NameContains(uintptr_t file, const char* token, size_t tokenLength) {
		const uintptr_t name = file + kFileNameOffset;
		uintptr_t heap = 0, end = 0, storageEnd = 0;
		if (!MemUtil::TryRead(name, heap) || !MemUtil::TryRead(name + 0x10, end)
			|| !MemUtil::TryRead(name + 0x14, storageEnd))
			return false;

		const uintptr_t data = storageEnd == name + 0x10 ? name : heap;
		if (!data || end < data || end - data > 1024)
			return false;

		__try {
			const size_t length = static_cast<size_t>(end - data);
			const auto* path = reinterpret_cast<const unsigned char*>(data);
			for (size_t offset = 0; offset + tokenLength <= length; ++offset) {
				bool match = true;
				for (size_t i = 0; i < tokenLength && match; ++i)
					match = LowerAscii(path[offset + i]) == static_cast<unsigned char>(token[i]);
				if (match)
					return true;
			}
			return false;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	const GfxDurationBytes::Site* MovieSite(uintptr_t file) {
		if (NameContains(file, kTitleFileToken, sizeof(kTitleFileToken) - 1))
			return &TitleFadeBytes::kSite;
		if (NameContains(file, kMainMenuFileToken, sizeof(kMainMenuFileToken) - 1))
			return &MainMenuFadeBytes::kSite;
		return nullptr;
	}

	bool ReadPosition(void* file, const GfxDurationBytes::Site& site, int64_t& position) {
		__try {
			if (reinterpret_cast<LengthFn>(g_fileLength)(file) != static_cast<int>(site.fileSize))
				return false;
			position = reinterpret_cast<TellFn>(g_fileTell)(file);
			return position >= 0 && position <= site.fileSize;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	enum class SourceActionResult { Read, Unavailable, PositionLost };

	SourceActionResult ReadSourceAction(void* file, const GfxDurationBytes::Site& site, int64_t end,
		std::array<unsigned char, GfxDurationBytes::kActionSize>& action) {
		bool read = false, restored = false;
		__try {
			if (reinterpret_cast<TellFn>(g_fileTell)(file) != end)
				return SourceActionResult::PositionLost;
			__try {
				if (reinterpret_cast<SeekFn>(g_fileSeek)(file, static_cast<int>(site.actionOffset), 0) == 1
					&& reinterpret_cast<TellFn>(g_fileTell)(file) == site.actionOffset) {
					read = reinterpret_cast<FileReadFn>(g_fileReadTrampoline)(file,
						action.data(), static_cast<int>(action.size())) == static_cast<int>(action.size());
				}
			}
			__finally {
				restored = reinterpret_cast<SeekFn>(g_fileSeek)(file, static_cast<int>(end), 0) == 1
					&& reinterpret_cast<TellFn>(g_fileTell)(file) == end;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			read = false;
		}
		if (!restored)
			return SourceActionResult::PositionLost;
		return read ? SourceActionResult::Read : SourceActionResult::Unavailable;
	}



	int __fastcall FileRead(void* file, void*, void* buffer, int length) {
		const GfxDurationBytes::Site* site = MovieSite(reinterpret_cast<uintptr_t>(file));
		const bool titleFile = site == &TitleFadeBytes::kSite;
		const bool mainMenuFile = site == &MainMenuFadeBytes::kSite;
		int64_t start = -1;
		const bool positioned = site && ReadPosition(file, *site, start);
		const int result = reinterpret_cast<FileReadFn>(g_fileReadTrampoline)(file, buffer, length);
		if (result <= 0 || result > length)
			return result;

		if (!positioned || start + result > site->fileSize)
			return result;
		if (mainMenuFile && g_mainMenuDone.load()) {
			if (start < site->DurationOffset() + GfxDurationBytes::kDurationSize && start + result > site->DurationOffset()
				&& !g_loggedMainMenuReload.exchange(true))
				LOG_INFO("(MENU FADE) mainmenu.gfx read again; this and later loads keep the stock button fade" << std::endl);
			return result;
		}
		const GfxDurationBytes::Site* sites[2] = { site, titleFile ? &TitleOutBytes::kSite : nullptr };
		for (const GfxDurationBytes::Site* current : sites) {
			if (!current || start >= current->DurationOffset() + GfxDurationBytes::kDurationSize
				|| start + result <= current->DurationOffset())
				continue;
			std::array<unsigned char, GfxDurationBytes::kActionSize> action{};
			const auto source = ReadSourceAction(file, *current, start + result, action);
			if (source == SourceActionResult::PositionLost) {
				if (!g_loggedPositionLost.exchange(true)) {
					LOG_ERROR("(TITLE FADE) Stream position lost while validating source action; expected "
						<< start + result << "; returning native read failure" << std::endl);
				}
				return 0;
			}
			if (source != SourceActionResult::Read
				|| !GfxDurationBytes::PatchFragment(*current, { static_cast<unsigned char*>(buffer), static_cast<size_t>(result) },
					static_cast<uint32_t>(start), action))
				continue;
			const unsigned int bits = GfxDurationBytes::DurationMask(*current,
				static_cast<uint32_t>(start), static_cast<size_t>(result));
			const bool titleOut = current == &TitleOutBytes::kSite;
			std::atomic<unsigned int>& patched = mainMenuFile ? g_mainMenuPatchedMask : titleOut ? g_titleOutPatchedMask : g_patchedMask;
			const unsigned int mask = patched.fetch_or(bits) | bits;
			if (mask != GfxDurationBytes::kDurationFullMask)
				continue;
			patched.store(0);
			if (mainMenuFile) {
				g_mainMenuDone.store(true);
				LOG_INFO("(MENU FADE) Patched the first main menu's button fade, 0.2 s to 0.001 s, at file offset 0x1260C"
					<< " after validating the full source action; later loads stay stock" << std::endl);
			}
			else if (titleOut) {
				LOG_INFO("(TITLE FADE) Patched the title's out tween, 1 s to 0.001 s, at file offset 0x12463"
					<< " after validating the full source action; its onComplete retained" << std::endl);
			}
			else {
				LOG_INFO("(TITLE FADE) Patched duration at file offset 0x12258 after validating the full source action"
					<< "; original onComplete retained" << std::endl);
			}
		}
		return result;
	}
}

void TitleFade::Install() {
	const unsigned char tellBytes[] = { 0x8B, 0x49, 0x08, 0x8B, 0x01, 0x8B, 0x50, 0x0C, 0xFF, 0xD2, 0x33, 0xD2, 0xC3 };
	const unsigned char lengthBytes[] = { 0x8B, 0x49, 0x08, 0x8B, 0x01, 0x8B, 0x50, 0x24, 0xFF, 0xE2 };
	const unsigned char seekBytes[] = {
		0x55, 0x8B, 0xEC, 0x83, 0x79, 0x08, 0, 0x75, 0x07, 0x83, 0xC8, 0xFF, 0x5D, 0xC2, 0x08, 0,
		0x8B, 0x45, 0x0C, 0x8B, 0x49, 0x08, 0x56, 0x8B, 0x31, 0x50, 0x8B, 0x45, 0x08, 0x99, 0x52,
		0x8B, 0x56, 0x20, 0x50, 0xFF, 0xD2, 0x0F, 0xB6, 0xC0, 0x5E, 0x5D, 0xC2, 0x08, 0
	};
	if (!UltrafastLoad::Enabled())
        return;
    if (!GameVersion::IsSeptember2022()) {
        LOG_INFO("(TITLE FADE) Disabled on this build" << std::endl);
        return;
    }
    kFileOpenEx = GameVersion::CheckedSite("TITLE FADE", { "GFX OpenEx", 0x00C30330, 0 },
        kFileOpenExBytes, nullptr, sizeof(kFileOpenExBytes));
    kFileRead = GameVersion::CheckedSite("TITLE FADE", { "GFX Read", 0x00C30530, 0 },
        kFileReadBytes, nullptr, sizeof(kFileReadBytes));
    g_fileTell = GameVersion::CheckedSite("TITLE FADE", { "GFX Tell", 0x00C30500, 0 },
        tellBytes, nullptr, sizeof(tellBytes));
    g_fileLength = GameVersion::CheckedSite("TITLE FADE", { "GFX Length", 0x00C30510, 0 },
        lengthBytes, nullptr, sizeof(lengthBytes));
    g_fileSeek = GameVersion::CheckedSite("TITLE FADE", { "GFX Seek", 0x00C305E0, 0 },
        seekBytes, nullptr, sizeof(seekBytes));
    if (!kFileOpenEx || !kFileRead || !g_fileTell || !g_fileLength || !g_fileSeek) {
        MemUtil::FailBootPatchBundle();
        return;
    }

	g_fileReadTrampoline = reinterpret_cast<uintptr_t>(MemUtil::TrampHook(
		reinterpret_cast<PBYTE>(kFileRead), reinterpret_cast<PBYTE>(&FileRead), sizeof(kFileReadBytes)));
	if (!g_fileReadTrampoline) {
		LOG_WARNING("(TITLE FADE) GFX read hook could not be installed" << std::endl);
		return;
	}
	LOG_INFO("(TITLE FADE) GFX read hook installed; validating source action for chunked reads" << std::endl);
}
