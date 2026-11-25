#include "../stdafx.h"
#include "SongSourceAuthorGroupsMod.hpp"

#include "../EngineString.hpp"
#include "../MemUtil.hpp"
#include "EnumerationPackage.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;


// Groups custom songs by package author inside the stock SOURCE sort, with a native section heading
// per author. Built on the CHARTER proof of concept, whose comparator and two display branches worked
// on screen; only the install differs. The proof of concept added a thirteenth registry record, which
// meant relocating the registry and four jump tables, and that relocation collided with SongDatesMod.
// This points the stock SOURCE record (index 10) at the same three functions instead: three checked
// pointer writes, no relocation and no new sort id, so an unpatched boot simply shows the stock sort.
//
// A worker thread reads every song package off the song list's threads and publishes a finished song
// key -> group map, which the comparator and the two display branches only read. Custom songs group by
// the author in their toolkit.version; official songs by the package they came from (RS:R DLC, RS:R,
// RS1 DLC, RS1). Each heading carries the group's song count from the scan, not the filtered list.

namespace {
	constexpr char kEnabled[] = "AddAuthorSort";

	// Where our two display branches rejoin the game's own code.
	uintptr_t dividerWrote = 0;
	uintptr_t dividerSame = 0;
	uintptr_t rowTextBack = 0;

	// The game's string routines, so every string we hand back is built by its own allocator.
	uintptr_t constructFromCStr = 0;
	uintptr_t assignRange = 0;
	uintptr_t assignCopy = 0;
	uintptr_t destroyString = 0;

	// --- Official song keys ---

	struct OfficialDate {
		uint32_t keyHash;
		uint16_t year;
		uint8_t month;
		uint8_t day;
	};

	constexpr OfficialDate kOfficialDates[] = {
#include "SongDatesOfficial.inc"
	};

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

	bool IsOfficial(std::string_view key) {
		const uint32_t hash = Crc32(key);
		const auto end = std::end(kOfficialDates);
		const auto it = std::lower_bound(std::begin(kOfficialDates), end, hash,
			[](const OfficialDate& entry, uint32_t wanted) { return entry.keyHash < wanted; });
		return it != end && it->keyHash == hash;
	}

	std::string Lower(std::string_view text) {
		std::string lower(text);
		std::transform(lower.begin(), lower.end(), lower.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return lower;
	}

	// --- The finished map, published once and then only read ---

	// Groups are listed in this order: every author alphabetically, then the Toolkit's default author,
	// custom songs without one, and the official sources from newest to oldest.
	enum class Rank : uint8_t {
		Author,
		DefaultAuthor,
		NoAuthor,
		RemasteredDlc,
		Remastered,
		Rs1Dlc,
		Rs1,
		Other,
	};

	constexpr char kDefaultAuthor[] = "Custom Song Creator";

	struct Group {
		std::string label;
		Rank rank = Rank::Other;
		size_t songs = 0;
		// "label (songs)", built once the scan has counted everything.
		std::string heading;
	};

	struct AuthorMap {
		// Owned here so a song's pointer stays put while the map grows.
		std::vector<std::unique_ptr<Group>> groups;
		std::unordered_map<std::string, Group*> groupByLabel;
		std::unordered_map<std::string, Group*> groupBySong;

		Group* GroupFor(const std::string& label, Rank rank) {
			const auto found = groupByLabel.find(label);
			if (found != groupByLabel.end())
				return found->second;
			groups.push_back(std::make_unique<Group>());
			Group* group = groups.back().get();
			group->label = label;
			group->rank = rank;
			groupByLabel.emplace(label, group);
			return group;
		}
	};

	// Songs the scan never saw (or every song, before it finishes) fall here, with no count.
	const Group kOther{ "Other", Rank::Other, 0, "Other" };

	// Never replaced after publication, so the comparator reads it without a lock and the pointers it
	// hands out stay good for as long as the game runs.
	std::atomic<const AuthorMap*> authors{ nullptr };
	std::atomic<bool> scanning{ false };

	// --- Reading a package's author ---

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

	// The Toolkit has written this key as "PackageAuthor", "Package Author" and "Author", so the
	// comparison keeps letters only.
	std::string KeyLetters(std::string_view text) {
		std::string letters;
		for (const unsigned char c : text) {
			if (std::isalpha(c))
				letters.push_back(static_cast<char>(std::tolower(c)));
		}
		return letters;
	}

	std::string Trim(std::string_view text) {
		while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
			text.remove_prefix(1);
		while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r'))
			text.remove_suffix(1);
		return std::string(text);
	}

	// toolkit.version is a handful of "<key>: <value>" or "<key>=<value>" lines. PackageAuthor wins
	// and a bare Author is the fallback.
	std::string AuthorFromToolkitVersion(std::string_view text) {
		std::string fallback;
		while (!text.empty()) {
			const size_t newline = text.find('\n');
			const std::string_view line = text.substr(0, newline);
			const size_t separator = line.find_first_of(":=");
			if (separator != std::string_view::npos) {
				const std::string key = KeyLetters(line.substr(0, separator));
				const std::string value = Trim(line.substr(separator + 1));
				if (!value.empty()) {
					if (key == "packageauthor")
						return value;
					if (key == "author" && fallback.empty())
						fallback = value;
				}
			}
			if (newline == std::string_view::npos) break;
			text.remove_prefix(newline + 1);
		}
		return fallback;
	}

	// Parse keeps the names blob in entry order, so a name's position in it is its entry index.
	bool FindEntryIndex(const EnumerationPackage::Package& package, std::string_view wanted, uint32_t& index) {
		if (package.names.empty())
			return false;
		std::string_view names(reinterpret_cast<const char*>(package.names.data()), package.names.size() - 1);
		uint32_t at = 1;
		while (!names.empty()) {
			const size_t newline = names.find('\n');
			if (names.substr(0, newline) == wanted) {
				index = at;
				return true;
			}
			++at;
			if (newline == std::string_view::npos) break;
			names.remove_prefix(newline + 1);
		}
		return false;
	}

	// Which official source a package is, by its file name. Anything else in the dlc folder is official
	// DLC when its song keys are in the official list, and custom otherwise.
	enum class PackageKind { BaseGame, Rs1Disc, Rs1Dlc, Dlc };

	PackageKind KindOf(const std::wstring& path) {
		const std::wstring name = path.substr(path.find_last_of(L'\\') + 1);
		if (_wcsicmp(name.c_str(), L"songs.psarc") == 0) return PackageKind::BaseGame;
		if (_wcsicmp(name.c_str(), L"rs1compatibilitydisc_p.psarc") == 0) return PackageKind::Rs1Disc;
		if (_wcsicmp(name.c_str(), L"rs1compatibilitydlc_p.psarc") == 0) return PackageKind::Rs1Dlc;
		return PackageKind::Dlc;
	}

	void ReadPackage(const std::wstring& path, EnumerationPackage::Inflate inflate, AuthorMap& map) {
		EnumerationPackage::Package package;
		unsigned char header[32]{};
		if (!EnumerationPackage::Open(path, package) || !EnumerationPackage::ReadIndex(package, header)
			|| !EnumerationPackage::Parse(package, header, inflate))
			return;

		const PackageKind kind = KindOf(path);
		Group* official = nullptr;
		if (kind == PackageKind::BaseGame) official = map.GroupFor("RS:R", Rank::Remastered);
		else if (kind == PackageKind::Rs1Disc) official = map.GroupFor("RS1", Rank::Rs1);
		else if (kind == PackageKind::Rs1Dlc) official = map.GroupFor("RS1 DLC", Rank::Rs1Dlc);

		// Only the Toolkit writes toolkit.version, so a dlc package without one is official even when its
		// song key is missing from the official list (Wolfmother's "Woman" is).
		Group* custom = nullptr;
		const auto CustomGroup = [&]() {
			if (custom)
				return custom;
			const uint32_t tocSize = EnumerationPackage::Big32(header + 12);
			const uint32_t entries = EnumerationPackage::Big32(header + 20);
			uint32_t index = 0;
			if (!FindEntryIndex(package, "toolkit.version", index)) {
				custom = map.GroupFor("RS:R DLC", Rank::RemasteredDlc);
				return custom;
			}
			std::vector<unsigned char> content;
			std::string author;
			if (EnumerationPackage::ReadEntry(package, index, tocSize, entries, inflate, content) && !content.empty())
				author = AuthorFromToolkitVersion(
					std::string_view(reinterpret_cast<const char*>(content.data()), content.size()));
			if (author.empty())
				custom = map.GroupFor("No author", Rank::NoAuthor);
			else
				custom = map.GroupFor(author,
					_stricmp(author.c_str(), kDefaultAuthor) == 0 ? Rank::DefaultAuthor : Rank::Author);
			return custom;
		};

		std::string_view key;
		for (const EnumerationPackage::Hash& hash : package.hashes) {
			if (!SongKeyOf(hash.name, key)) continue;
			std::string lower = Lower(key);
			// One entry per song, however many arrangement manifests it has.
			if (map.groupBySong.count(lower)) continue;
			Group* group = official;
			if (!group)
				group = IsOfficial(lower) ? map.GroupFor("RS:R DLC", Rank::RemasteredDlc) : CustomGroup();
			++group->songs;
			map.groupBySong.emplace(std::move(lower), group);
		}
	}

	void CollectPackages(const std::wstring& folder, std::vector<std::wstring>& found) {
		WIN32_FIND_DATAW data{};
		const HANDLE search = FindFirstFileW((folder + L"\\*").c_str(), &data);
		if (search == INVALID_HANDLE_VALUE) return;
		do {
			const std::wstring name = data.cFileName;
			if (name == L"." || name == L"..") continue;
			const std::wstring path = folder + L"\\" + name;
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				CollectPackages(path, found);
				continue;
			}
			if (name.size() > 6 && _wcsicmp(name.c_str() + name.size() - 6, L".psarc") == 0)
				found.push_back(path);
		} while (FindNextFileW(search, &data));
		FindClose(search);
	}

	std::wstring GameFolder() {
		wchar_t executable[MAX_PATH]{};
		if (!GetModuleFileNameW(nullptr, executable, MAX_PATH))
			return {};
		const std::wstring path = executable;
		const size_t slash = path.find_last_of(L"\\/");
		if (slash == std::wstring::npos)
			return {};
		return path.substr(0, slash);
	}

	// Off the song list's threads: every package is opened, inflated and parsed here, and the finished
	// map is published in one store.
	void ScanAuthors(EnumerationPackage::Inflate inflate) {
		std::vector<std::wstring> packages;
		const std::wstring folder = GameFolder();
		if (!folder.empty()) {
			// The base game first, so its songs are never claimed by a package that repeats a key.
			packages.push_back(folder + L"\\songs.psarc");
			CollectPackages(folder + L"\\dlc", packages);
		}

		auto map = std::make_unique<AuthorMap>();
		for (const std::wstring& path : packages)
			ReadPackage(path, inflate, *map);

		size_t authored = 0;
		for (const auto& group : map->groups) {
			group->heading = group->label + " (" + std::to_string(group->songs) + ")";
			if (group->rank == Rank::Author || group->rank == Rank::DefaultAuthor)
				authored += group->songs;
		}

		const size_t songs = map->groupBySong.size();
		const size_t groups = map->groups.size();
		const size_t read = packages.size();
		authors.store(map.release(), std::memory_order_release);
		scanning.store(false, std::memory_order_release);
		LOG_INFO("(SOURCE AUTHORS) Read " << read << " packages: " << songs << " songs in " << groups
			<< " groups, " << authored << " with an author" << std::endl);
	}

	// --- Reading a song entry (runs on the song list's threads) ---

	// The entry's strings are the game's own. One guarded copy, because the comparator sees every entry
	// the list holds and a torn or freed one must not take the game down.
	bool ReadEntryString(uintptr_t field, char* out, size_t capacity) {
		EngineString header{};
		if (!MemUtil::TryRead(field, header))
			return false;
		const char* start = header.endOfStorage == reinterpret_cast<char*>(field + offsetof(EngineString, finish))
			? reinterpret_cast<const char*>(field) : header.data;
		const char* finish = header.finish;
		if (!start || !finish || finish < start)
			return false;
		const size_t length = static_cast<size_t>(finish - start);
		if (length == 0 || length >= capacity)
			return false;
		__try {
			memcpy(out, start, length);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
		out[length] = '\0';
		return true;
	}

	const Group* GroupOf(uintptr_t entry) {
		const AuthorMap* map = authors.load(std::memory_order_acquire);
		if (!map)
			return &kOther; // The scan hasn't finished yet.
		char key[160]{};
		if (!ReadEntryString(entry, key, sizeof(key)))
			return &kOther;
		const auto found = map->groupBySong.find(Lower(key));
		return found == map->groupBySong.end() ? &kOther : found->second;
	}

	// --- The SOURCE sort's comparator ---

	// The registry's comparators are plain cdecl "does a come before b" predicates. Groups order by rank
	// and then by name; a tie falls back to the entry's sort name at +0x18, the field the stock title and
	// artist comparators use, so the order is total and two songs by one author keep a stable place.
	bool __cdecl AuthorLess(uintptr_t a, uintptr_t b) {
		const Group* groupA = GroupOf(a);
		const Group* groupB = GroupOf(b);
		if (groupA->rank != groupB->rank)
			return groupA->rank < groupB->rank;
		const int byAuthor = _stricmp(groupA->label.c_str(), groupB->label.c_str());
		if (byAuthor != 0)
			return byAuthor < 0;
		char nameA[256]{}, nameB[256]{};
		if (!ReadEntryString(a + 0x18, nameA, sizeof(nameA))
			|| !ReadEntryString(b + 0x18, nameB, sizeof(nameB)))
			return false;
		return _stricmp(nameA, nameB) < 0;
	}

	// Writes the author into the heading the song list draws above a group, building the game's own
	// string first so its allocator owns the text, exactly as the "by last played" heading does.
	int __cdecl WriteDivider(uintptr_t song, uintptr_t previous, uintptr_t destination) {
		const Group* mine = GroupOf(song);
		if (mine == GroupOf(previous))
			return 0; // Same group, so it continues and no heading is drawn.

		EngineString scratch{};
		void* slot = &scratch;
		const char* text = mine->heading.c_str();
		const uintptr_t construct = constructFromCStr, copy = assignCopy, destroy = destroyString;
		__asm {
			mov edx, text
			mov esi, slot
			call construct
			mov eax, slot
			mov esi, destination
			call copy
			mov ecx, slot
			call destroy
		}
		return 1;
	}

	// Writes the author into the per-row section text the list view data carries.
	void __cdecl WriteRowText(uintptr_t song, uintptr_t destination) {
		const char* text = GroupOf(song)->heading.c_str();
		const char* finish = text + strlen(text);
		const uintptr_t assign = assignRange;
		// assignRange takes its two arguments on the stack and cleans them itself.
		__asm {
			push finish
			push text
			mov eax, destination
			call assign
		}
	}

	// --- The two display branches ---

	// Reached through the divider jump table's index 10 entry, inside IsDividerNecessary's frame: ESI is the
	// song whose heading is drawn, EAX the one before it and EDI the heading string. Only EBP and the
	// stack matter to the epilogues we rejoin, so the scratch registers are ours to use.
	__declspec(naked) void DividerBranch() {
		__asm {
			push edi
			push eax
			push esi
			call WriteDivider
			add esp, 12
			test eax, eax
			jz same
			jmp dword ptr [dividerWrote]
		same:
			jmp dword ptr [dividerSame]
		}
	}

	// Reached through the row text jump table's index 10 entry: ECX is the song entry and the section string
	// sits at ESP+0x11C, so the destination is taken before anything is pushed.
	__declspec(naked) void RowTextBranch() {
		__asm {
			lea eax, [esp + 0x11C]
			push eax
			push ecx
			call WriteRowText
			add esp, 8
			jmp dword ptr [rowTextBack]
		}
	}

	// --- Install: the three SOURCE (index 10) slots, and the game code the branches call and rejoin ---

	// The comparator in SOURCE's registry record and the index 10 entries of the divider and row text
	// jump tables, each with the stock pointer it must hold before anything is written.
	constexpr uintptr_t kComparatorSlot = 0x011995F8;
	constexpr uintptr_t kDividerSlot = 0x00659394;
	constexpr uintptr_t kRowTextSlot = 0x00654C00;
	constexpr unsigned char kStockComparator[] = { 0x40, 0x60, 0x57, 0x00 };
	constexpr unsigned char kStockDivider[] = { 0xDF, 0x91, 0x65, 0x00 };
	constexpr unsigned char kStockRowText[] = { 0xAE, 0x41, 0x65, 0x00 };

	// Checked but never written, so a moved epilogue or string routine fails the install instead of
	// sending the game to the wrong instruction.
	constexpr unsigned char kConstructBytes[] = { 0x8D, 0x46, 0x10, 0x89, 0x30, 0x89, 0x46, 0x14 };
	constexpr unsigned char kAssignRangeBytes[] = { 0x55, 0x8B, 0xEC, 0x51, 0x53, 0x8B, 0x5D, 0x0C };
	constexpr unsigned char kAssignCopyBytes[] = { 0x3B, 0xC6, 0x74, 0x16, 0x8B, 0x50, 0x10 };
	constexpr unsigned char kDestroyBytes[] = { 0x8B, 0x41, 0x14, 0x8D, 0x51, 0x10, 0x3B, 0xC2 };
	constexpr unsigned char kDividerWroteBytes[] = { 0xB0, 0x01, 0x5E, 0x8B, 0x4D, 0xFC };
	constexpr unsigned char kDividerSameBytes[] = { 0x8B, 0x4D, 0xFC, 0x33, 0xCD, 0x32, 0xC0, 0x5E };
	constexpr unsigned char kRowTextBackBytes[] = { 0x8D, 0x94, 0x24, 0x34, 0x01, 0x00, 0x00 };

	// Kept for the life of the process: the patch set rolls back from them.
	unsigned char comparatorBytes[sizeof(void*)]{};
	unsigned char dividerBytes[sizeof(void*)]{};
	unsigned char rowTextBytes[sizeof(void*)]{};

	MemUtil::PreparedPatchSet patches;

	void StorePointer(unsigned char (&bytes)[sizeof(void*)], uintptr_t value) {
		for (size_t i = 0; i < sizeof(void*); ++i)
			bytes[i] = static_cast<unsigned char>(value >> (8 * i));
	}
}

SettingDefs SongSourceAuthorGroupsMod::Settings() const {
	return {
		SettingDef::Toggle(kEnabled, "Group Custom Songs by Author in SOURCE")
			.Default("on")
			.Hint("Groups custom songs by their package author inside the SOURCE sort, with a heading per author.\n"
				"The name comes from each custom package's toolkit.version; official songs are grouped by "
				"source (RS:R DLC, RS:R, RS1 DLC, RS1).\n"
				"Packages are read as the game starts, so turning this on takes effect after restarting the game.")
			.Heading(Framework::SettingHeading::SongsAndProfiles),
	};
}

bool SongSourceAuthorGroupsMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(kEnabled);
}

void SongSourceAuthorGroupsMod::OnEnabled(ModContext&) {
	if (installed)
		return;
	if (!GameVersion::IsSeptember2022()) {
		LOG_INFO("(SOURCE AUTHORS) Only the September 2022 build is mapped; author groups are off" << std::endl);
		return;
	}

	// Resolve and check everything we call, rejoin and overwrite. Nothing is written until all of it matches.
	const auto site = [](const char* name, uintptr_t address, std::span<const unsigned char> bytes) {
		return GameVersion::CheckedSite("SOURCE AUTHORS", { name, address, 0 },
			bytes.data(), nullptr, bytes.size());
	};
	constructFromCStr = site("string from c-str", 0x004079A0, kConstructBytes);
	assignRange = site("string assign range", 0x00407840, kAssignRangeBytes);
	assignCopy = site("heading string assign", 0x00407580, kAssignCopyBytes);
	destroyString = site("string destroy", 0x0085C810, kDestroyBytes);
	dividerWrote = site("heading written", 0x00659344, kDividerWroteBytes);
	dividerSame = site("heading not needed", 0x00659357, kDividerSameBytes);
	rowTextBack = site("row text done", 0x00653DD3, kRowTextBackBytes);
	const uintptr_t comparatorSlot = site("SOURCE comparator", kComparatorSlot, kStockComparator);
	const uintptr_t dividerSlot = site("SOURCE divider branch", kDividerSlot, kStockDivider);
	const uintptr_t rowTextSlot = site("SOURCE row text branch", kRowTextSlot, kStockRowText);
	if (!constructFromCStr || !assignRange || !assignCopy || !destroyString || !dividerWrote || !dividerSame
		|| !rowTextBack || !comparatorSlot || !dividerSlot || !rowTextSlot) {
		LOG_WARNING("(SOURCE AUTHORS) Game code didn't match; author groups are off" << std::endl);
		return;
	}

	StorePointer(comparatorBytes, reinterpret_cast<uintptr_t>(&AuthorLess));
	StorePointer(dividerBytes, reinterpret_cast<uintptr_t>(&DividerBranch));
	StorePointer(rowTextBytes, reinterpret_cast<uintptr_t>(&RowTextBranch));
	const MemUtil::PreparedPatchRequest requests[] = {
		{ reinterpret_cast<LPVOID>(comparatorSlot), MemUtil::PreparedPatchKind::PointerStore,
			kStockComparator, 0, 0, false, comparatorBytes, sizeof(void*), 0 },
		{ reinterpret_cast<LPVOID>(dividerSlot), MemUtil::PreparedPatchKind::PointerStore,
			kStockDivider, 0, 0, false, dividerBytes, sizeof(void*), 0 },
		{ reinterpret_cast<LPVOID>(rowTextSlot), MemUtil::PreparedPatchKind::PointerStore,
			kStockRowText, 0, 0, false, rowTextBytes, sizeof(void*), 0 },
	};
	if (!patches.Prepare(requests)) {
		patches.Report("SOURCE AUTHORS");
		patches.Reset();
		LOG_WARNING("(SOURCE AUTHORS) SOURCE sort couldn't be patched; author groups are off" << std::endl);
		return;
	}
	const MemUtil::PatchResult result = patches.Commit();
	if (result != MemUtil::PatchResult::Installed) {
		patches.Report("SOURCE AUTHORS");
		// A partial rollback leaves our pointers live, so the scan still has to run for them.
		if (result != MemUtil::PatchResult::RollbackIncomplete) {
			patches.Reset();
			LOG_WARNING("(SOURCE AUTHORS) SOURCE sort couldn't be patched; author groups are off" << std::endl);
			return;
		}
	}
	installed = true;
	LOG_INFO("(SOURCE AUTHORS) Author groups enabled in the SOURCE sort" << std::endl);

	// Opening and inflating every package never happens on a game thread.
	if (Offsets::func_enumInflate.Get() && !scanning.exchange(true))
		std::thread(ScanAuthors,
			reinterpret_cast<EnumerationPackage::Inflate>(Offsets::func_enumInflate.Get())).detach();
}

void SongSourceAuthorGroupsMod::OnDisabled(ModContext&) {
	if (!installed)
		return;
	if (patches.Rollback() != MemUtil::PatchResult::Original) {
		patches.Report("SOURCE AUTHORS");
		return;
	}
	patches.Reset();
	installed = false;
	LOG_INFO("(SOURCE AUTHORS) Author groups removed from the SOURCE sort" << std::endl);
}

static Framework::ModRegistrar<SongSourceAuthorGroupsMod> _songSourceAuthorGroupsModReg;
