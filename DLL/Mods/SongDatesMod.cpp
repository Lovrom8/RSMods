#include "../stdafx.h"
#include "SongDatesMod.hpp"

#include "../EngineString.hpp"
#include "../MemUtil.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;

// The song lists sort by a "YYYY-MM-DD hh:mm:ss" release date kept in each song entry. When it is empty the
// list sorts the song first under DATE RELEASED and labels it "NEW RELEASE". The game fills it with fixed dates
// for disc songs, and for DLC only with dates it downloaded from Ubisoft's servers, so without that download
// every DLC song ends up "NEW RELEASE".
//
// When the game mounts a DLC package, this mod reads the package's file list (which the game just parsed) for
// the songs in it and dates each one from SongDatesOfficial.inc, falling back to the date the package file was
// last modified. The song lists then fill any empty date from that. Dates the game did find are never changed.
// A song dated within the last N days stays empty, so the game shows it as "NEW RELEASE".

namespace {
	constexpr char kEnabled[] = "SongDatesEnabled";
	constexpr char kNewDays[] = "SongDatesNewDays";
	constexpr int kDefaultNewDays = 14;
	constexpr int kMaxNewDays = 365;

	struct OfficialDate {
		uint32_t keyHash; // CRC32 of the lowercase song key.
		uint16_t year;
		uint8_t month;
		uint8_t day;
	};

	constexpr OfficialDate kOfficialDates[] = {
#include "SongDatesOfficial.inc"
	};

	// Lowercase song key -> last-modified time (UTC FILETIME) of its package, for songs not in the official list.
	std::unordered_map<std::string, uint64_t> fallbackDates;
	std::mutex fallbackDatesLock;
	std::atomic<int> newDays{ kDefaultNewDays };

	// The game's string formatter: this in ECX, caller cleans the stack.
	uintptr_t formatString = 0;
	constexpr char kDateFormat[] = "%04d-%02d-%02d %02d:%02d:%02d";

	std::string Lower(std::string_view text) {
		std::string lower(text);
		std::transform(lower.begin(), lower.end(), lower.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return lower;
	}

	// Standard CRC32 (as zlib), matching the hashes in SongDatesOfficial.inc.
	uint32_t Crc32(std::string_view text) {
		uint32_t hash = 0xFFFFFFFF;
		for (const unsigned char c : text) {
			hash ^= c;
			for (int bit = 0; bit < 8; ++bit)
				hash = (hash >> 1) ^ (0xEDB88320 & (0 - (hash & 1)));
		}
		return ~hash;
	}

	const OfficialDate* FindOfficial(std::string_view key) {
		const uint32_t hash = Crc32(key);
		const auto end = std::end(kOfficialDates);
		const auto it = std::lower_bound(std::begin(kOfficialDates), end, hash,
			[](const OfficialDate& entry, uint32_t wanted) { return entry.keyHash < wanted; });
		return it != end && it->keyHash == hash ? it : nullptr;
	}

	// --- Package mount ---

	// The installer's list of the mounted package's file names, begin/end at [ebp-0x340] / [ebp-0x33C].
	struct FileName {
		union {
			char text[16];       // Used while capacity <= 15.
			const char* pointer; // Used otherwise.
		};
		uint32_t length;
		uint32_t capacity;
		uint32_t reserved;

		std::string_view View() const { return { capacity > 15 ? pointer : text, length }; }
	};
	static_assert(sizeof(FileName) == 0x1C, "FileName must match the game's layout");

	// Each arrangement's manifest is "manifests/<package>/<song key>_<arrangement>.json".
	bool SongKeyOf(std::string_view name, std::string_view& key) {
		constexpr std::string_view folder = "manifests/";
		constexpr std::string_view extension = ".json";
		if (name.size() <= folder.size() + extension.size() || name.substr(0, folder.size()) != folder
			|| name.substr(name.size() - extension.size()) != extension)
			return false;
		const std::string_view file = name.substr(name.rfind('/') + 1);
		const size_t underscore = file.rfind('_');
		if (underscore == std::string_view::npos || underscore == 0) return false;
		key = file.substr(0, underscore);
		return true;
	}

	void __cdecl OnPackageMounted(uintptr_t package, uintptr_t path, uintptr_t frame) {
		if (!package) return; // The mount failed; the game skips this package too.

		const std::string file = EngineStrings::SafeRead(path, MAX_PATH, "");
		WIN32_FILE_ATTRIBUTE_DATA attributes{};
		if (file.empty() || !GetFileAttributesExA(file.c_str(), GetFileExInfoStandard, &attributes))
			return; // No fallback date; official songs still come from the list.
		const uint64_t modified = (uint64_t(attributes.ftLastWriteTime.dwHighDateTime) << 32)
			| attributes.ftLastWriteTime.dwLowDateTime;

		const auto* name = *reinterpret_cast<const FileName* const*>(frame - 0x340);
		const auto* end = *reinterpret_cast<const FileName* const*>(frame - 0x33C);
		std::vector<std::string> keys;
		std::string_view key;
		for (; name < end; ++name) {
			if (!SongKeyOf(name->View(), key)) continue;
			std::string lower = Lower(key);
			// A song's arrangements share its key, so skip repeats and songs the official list dates.
			if (std::find(keys.begin(), keys.end(), lower) == keys.end() && !FindOfficial(lower))
				keys.push_back(std::move(lower));
		}
		if (keys.empty()) return;

		std::lock_guard<std::mutex> lock(fallbackDatesLock);
		for (std::string& songKey : keys)
			fallbackDates.insert_or_assign(std::move(songKey), modified);
	}

	// --- Date fill (runs on the thread building the song list) ---

	// Formats the date into the entry's string with the game's own formatter and allocator.
	void WriteDate(uintptr_t field, const SYSTEMTIME& when) {
		const int year = when.wYear, month = when.wMonth, day = when.wDay;
		const int hour = when.wHour, minute = when.wMinute, second = when.wSecond;
		const char* format = kDateFormat;
		const uintptr_t formatter = formatString;
		__asm {
			push second
			push minute
			push hour
			push day
			push month
			push year
			push format
			mov ecx, field
			call formatter
			add esp, 0x1C
		}
	}

	// The official list ends in 2020 and the new-song window is at most a year, so only fallback dates can be recent.
	bool FindDate(const std::string& key, SYSTEMTIME& when) {
		if (const OfficialDate* official = FindOfficial(key)) {
			when = {};
			when.wYear = official->year;
			when.wMonth = official->month;
			when.wDay = official->day;
			return true;
		}

		uint64_t modified = 0;
		{
			std::lock_guard<std::mutex> lock(fallbackDatesLock);
			const auto found = fallbackDates.find(key);
			if (found == fallbackDates.end()) return false;
			modified = found->second;
		}

		const int days = newDays.load(std::memory_order_relaxed);
		if (days > 0) {
			FILETIME now{};
			GetSystemTimeAsFileTime(&now);
			const uint64_t nowTicks = (uint64_t(now.dwHighDateTime) << 32) | now.dwLowDateTime;
			constexpr uint64_t kTicksPerDay = 24ull * 60 * 60 * 10'000'000;
			if (modified >= nowTicks || nowTicks - modified < uint64_t(days) * kTicksPerDay)
				return false; // Recent: stays empty, so the game shows it as "NEW RELEASE".
		}

		const FILETIME utc{ static_cast<DWORD>(modified), static_cast<DWORD>(modified >> 32) };
		FILETIME local{};
		if (!FileTimeToLocalFileTime(&utc, &local) || !FileTimeToSystemTime(&local, &when))
			return false;
		// The list starts a new date heading whenever the stored text differs, so keep only the day.
		when.wHour = when.wMinute = when.wSecond = when.wMilliseconds = 0;
		return true;
	}

	void __cdecl FillReleaseDate(uintptr_t entry, uint32_t dateOffset) {
		const uintptr_t field = entry + dateOffset;
		EngineString date{};
		if (!MemUtil::TryRead(field, date)) return;
		const char* text = date.endOfStorage == reinterpret_cast<char*>(field + offsetof(EngineString, finish))
			? reinterpret_cast<char*>(field) : date.data;
		if (date.finish != text) return; // The game already has a date.

		const std::string key = Lower(EngineStrings::SafeRead(entry, 128, ""));
		SYSTEMTIME when{};
		if (!key.empty() && FindDate(key, when))
			WriteDate(field, when);
	}

	// --- Hooks: each runs our code, then the instructions it replaced, then continues ---

	// DLC package installer, right after the mount call (both builds): EAX = mounted package or 0, EDI = path
	// string, EBP = installer frame. Replaced: mov ebx, eax; add esp, 0x18.
	__declspec(naked) void MountHook() {
		__asm {
			pushfd
			pushad
			push ebp
			push edi
			push eax
			call OnPackageMounted
			add esp, 12
			popad
			popfd
			mov ebx, eax
			add esp, 0x18
			push offset Offsets::hookBackAddr_SongDatesMount
			jmp MemUtil::JumpToVersioned
		}
	}

	// September 2022 song entry builder: entry in EDI, date at +0x208. Replaced: push edi; call [esp+0x80].
	__declspec(naked) void BuilderHookSeptember() {
		__asm {
			pushfd
			pushad
			push 0x208
			push edi
			call FillReleaseDate
			add esp, 8
			popad
			popfd
			push edi
			call dword ptr [esp + 0x80]
			push offset Offsets::hookBackAddr_SongDatesBuilder
			jmp MemUtil::JumpToVersioned
		}
	}

	// Learn & Play song entry builder: entry in a stack slot, date at +0x208. Replaced: mov esi, [esp+0x44]; push esi.
	__declspec(naked) void BuilderHookLearnAndPlay() {
		__asm {
			mov esi, dword ptr [esp + 0x44]
			pushfd
			pushad
			push 0x208
			push esi
			call FillReleaseDate
			add esp, 8
			popad
			popfd
			push esi
			push offset Offsets::hookBackAddr_SongDatesBuilder
			jmp MemUtil::JumpToVersioned
		}
	}

	// Score Attack song list, both builds: entry in EDI, date at +0x184.
	// Replaced: mov esi, [esp+0x24]; lea eax, [esp+0x1BC].
	__declspec(naked) void ScoreAttackHook() {
		__asm {
			pushfd
			pushad
			push 0x184
			push edi
			call FillReleaseDate
			add esp, 8
			popad
			popfd
			mov esi, dword ptr [esp + 0x24]
			lea eax, [esp + 0x1BC]
			push offset Offsets::hookBackAddr_SongDatesScoreAttack
			jmp MemUtil::JumpToVersioned
		}
	}

	constexpr unsigned char kFormatterBytes[] = { 0x55, 0x8B, 0xEC, 0x8D, 0x45, 0x0C, 0x50, 0x51, 0x8B, 0x4D,
		0x08, 0xE8, 0x30, 0x00, 0x00, 0x00, 0x5D, 0xC3 };
	constexpr unsigned char kMountBytes[] = { 0x8B, 0xD8, 0x83, 0xC4, 0x18 };
	constexpr unsigned char kBuilderSeptemberBytes[] = { 0x57, 0xFF, 0x94, 0x24, 0x80, 0x00, 0x00, 0x00 };
	constexpr unsigned char kBuilderLearnAndPlayBytes[] = { 0x8B, 0x74, 0x24, 0x44, 0x56 };
	constexpr unsigned char kScoreAttackBytes[] = { 0x8B, 0x74, 0x24, 0x24, 0x8D, 0x84, 0x24, 0xBC, 0x01, 0x00, 0x00 };
}

SettingDefs SongDatesMod::Settings() const {
	return {
		SettingDef::Toggle(kEnabled, "Release Dates for Songs")
			.Default("on")
			.Hint("Gives songs a date for the song list's DATE RELEASED sort when the game has none.\n"
				"Official DLC uses its US release date; any other song falls back to the date its file was last modified.\n"
				"Songs dated within the last few days stay at the top as NEW RELEASE.\n"
				"Dates the game already has (from Ubisoft's servers, or for disc songs) are never changed.\n"
				"Songs are dated as the game loads them, so turning this on takes effect after restarting the game.")
			.Heading(Framework::SettingHeading::SongsAndProfiles),
		SettingDef::Numeric(kNewDays, "Days a Song Shows as New")
			.Range(0, kMaxNewDays)
			.Default(std::to_string(kDefaultNewDays))
			.Hint("Songs dated within this many days show as NEW RELEASE at the top of the DATE RELEASED sort. 0 dates every song instead.")
			.WithVisibleWhen(kEnabled),
	};
}

bool SongDatesMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(kEnabled);
}

void SongDatesMod::OnSettingsChanged(ModContext& c) {
	newDays.store(std::clamp(c.Int(kNewDays), 0, kMaxNewDays), std::memory_order_relaxed);
}

void SongDatesMod::OnEnabled(ModContext& c) {
	OnSettingsChanged(c);
	if (PlaceHooks()) {
		LOG_INFO("(SONG DATES) Release date hooks placed" << std::endl);
		return;
	}
	RemoveHooks();
	LOG_WARNING("(SONG DATES) Game code didn't match; release dates are off" << std::endl);
}

void SongDatesMod::OnDisabled(ModContext&) {
	RemoveHooks();
}

bool SongDatesMod::PlaceHooks() {
	if (!GameVersion::IsSeptember2022() && !GameVersion::IsLearnAndPlay())
		return false;
	formatString = Offsets::func_SongDatesFormat;
	const bool september = GameVersion::IsSeptember2022();
	Offsets::hookBackAddr_SongDatesMount.Get() = Offsets::hookAddr_SongDatesMount.Get() + sizeof(kMountBytes);
	Offsets::hookBackAddr_SongDatesBuilder.Get() = Offsets::hookAddr_SongDatesBuilder.Get()
		+ (september ? sizeof(kBuilderSeptemberBytes) : sizeof(kBuilderLearnAndPlayBytes));
	Offsets::hookBackAddr_SongDatesScoreAttack.Get() = Offsets::hookAddr_SongDatesScoreAttack.Get() + sizeof(kScoreAttackBytes);

	// Check all sites before changing any code.
	if (memcmp(reinterpret_cast<void*>(formatString), kFormatterBytes, sizeof(kFormatterBytes))
		|| memcmp(reinterpret_cast<void*>(Offsets::hookAddr_SongDatesMount.Get()), kMountBytes, sizeof(kMountBytes))
		|| memcmp(reinterpret_cast<void*>(Offsets::hookAddr_SongDatesBuilder.Get()),
			september ? kBuilderSeptemberBytes : kBuilderLearnAndPlayBytes,
			september ? sizeof(kBuilderSeptemberBytes) : sizeof(kBuilderLearnAndPlayBytes))
		|| memcmp(reinterpret_cast<void*>(Offsets::hookAddr_SongDatesScoreAttack.Get()), kScoreAttackBytes, sizeof(kScoreAttackBytes)))
		return false;

	return PlaceHook(Offsets::hookAddr_SongDatesMount, MountHook, kMountBytes, sizeof(kMountBytes))
		&& PlaceHook(Offsets::hookAddr_SongDatesBuilder, september ? BuilderHookSeptember : BuilderHookLearnAndPlay,
			september ? kBuilderSeptemberBytes : kBuilderLearnAndPlayBytes,
			september ? sizeof(kBuilderSeptemberBytes) : sizeof(kBuilderLearnAndPlayBytes))
		&& PlaceHook(Offsets::hookAddr_SongDatesScoreAttack, ScoreAttackHook, kScoreAttackBytes, sizeof(kScoreAttackBytes));
}

bool SongDatesMod::PlaceHook(uintptr_t address, void* hook, const unsigned char* bytes, size_t size) {
	if (!MemUtil::PlaceHook(reinterpret_cast<void*>(address), hook, static_cast<int>(size)))
		return false;
	placed.push_back({ address, std::vector<unsigned char>(bytes, bytes + size) });
	return true;
}

void SongDatesMod::RemoveHooks() {
	for (const PlacedHook& hook : placed)
		MemUtil::PatchAdr(reinterpret_cast<LPVOID>(hook.address), hook.original.data(), hook.original.size());
	placed.clear();
}

static Framework::ModRegistrar<SongDatesMod> _songDatesModReg;


