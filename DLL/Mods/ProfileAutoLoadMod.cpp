#include "../stdafx.h"
#include "ProfileAutoLoadMod.hpp"
#include "../GameState.hpp"
#include "../SettingEdits.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>

using Framework::SettingDefs;
using Framework::SettingDef;
namespace Setting = Settings::Setting;

namespace {
	// Selector call ABI: ECX = profile service, player index on stack, ret 4.
	uintptr_t selectorCall = 0;
	uintptr_t startSelector = 0;

	// Profile service +0xBC points to entries at +0/+4, stride 0x38.
	// Entry +0 profile id, +0x28 LastModified, double Unix seconds.
	constexpr uintptr_t off_profileManager = 0xBC;
	constexpr uintptr_t profileEntrySize = 0x38;
	constexpr unsigned int maxProfiles = 64;
	constexpr uintptr_t off_entryLastModified = 0x28;
	// PlayerName uses string end +0x10 and inline sentinel +0x14.
	// Name matching is disabled on Learn & Play because its layout is unsupported.
	constexpr uintptr_t off_entryName = 0x10;
	constexpr size_t maxNameLength = 64;

	// Connect slot +0x5C selects a local profile: (player, const id*), ret 8.
	uintptr_t connectSingleton = 0;
	uintptr_t connectVtable = 0;
	uintptr_t connectSelectSlot = 0;
	uintptr_t connectSelect = 0;

	struct ProfileId { uint32_t part[4]; };
	using SelectFn = uintptr_t(__thiscall*)(void* connect, unsigned int player, const ProfileId* id);
	using StartSelectorFn = void(__thiscall*)(void* service, unsigned int player);

	std::string ToHex(const ProfileId& id) {
		char text[33]{};
		snprintf(text, sizeof(text), "%08x%08x%08x%08x", id.part[0], id.part[1], id.part[2], id.part[3]);
		return text;
	}

	template <typename T>
	bool Read(uintptr_t address, T& value) {
		if (!address || MemUtil::IsBadReadPtr(reinterpret_cast<void*>(address)))
			return false;
		value = *reinterpret_cast<const T*>(address);
		return true;
	}

	bool ReadName(uintptr_t entry, std::string& name) {
		const uintptr_t self = entry + off_entryName;
		uintptr_t heap = 0, finish = 0, storageEnd = 0;
		if (!Read(self, heap) || !Read(self + 0x10, finish) || !Read(self + 0x14, storageEnd))
			return false;
		const bool inlineText = storageEnd == self + 0x10;
		const uintptr_t start = inlineText ? self : heap;
		if (finish < start || finish - start > maxNameLength || (inlineText && finish > self + 0x10)
			|| (!inlineText && storageEnd < finish))
			return false;
		const size_t length = finish - start;
		name.assign(length, '\0');
		for (size_t i = 0; i < length; ++i) {
			unsigned char c = 0;
			if (!Read(start + i, c) || c < 0x20 || c == 0x7F)
				return false;
			name[i] = static_cast<char>(c);
		}
		return true;
	}

	bool SameName(const std::string& a, const std::string& b) {
		if (a.size() != b.size())
			return false;
		for (size_t i = 0; i < a.size(); ++i)
			if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
				return false;
		return true;
	}

	// Queue and dispatch selection on the game thread, one frame apart.
	bool pending = false;
	uintptr_t pendingService = 0;
	unsigned int pendingPlayer = 0;
	ProfileId pendingId{};

	// Game-thread selector hook: 1 suppresses the selector, 0 keeps native selection.
	extern "C" int __cdecl ChooseProfile(uintptr_t service, unsigned int player) {
		// Autoload only player 1 at boot; profile changes and multiplayer retain the selector.
		if (player != 0 || GameState::BootComplete || !Settings::IsOn(Setting::ProfileAutoLoad))
			return 0;
		uintptr_t manager = 0, begin = 0, end = 0;
		if (!Read(service + off_profileManager, manager) || !Read(manager, begin) || !Read(manager + 4, end)
			|| end < begin || (end - begin) % profileEntrySize != 0 || (end - begin) / profileEntrySize > maxProfiles)
			return 0;
		const unsigned int count = static_cast<unsigned int>((end - begin) / profileEntrySize);
		if (count == 0) {
			LOG_INFO("(PROFILE) No local profiles: the profile selector opens" << std::endl);
			return 0;
		}

		const char* rule = nullptr;
		const std::string wanted = Settings::ReturnSettingValue(Setting::ProfileToLoad);
		if (!wanted.empty() && GameVersion::IsLearnAndPlay())
			LOG_WARNING("(PROFILE) Name matching disabled on Learn & Play: layout unconfirmed" << std::endl);
		if (!wanted.empty() && GameVersion::IsSeptember2022()) {
			bool namesReadable = true;
			for (unsigned int i = 0; i < count && !rule && namesReadable; ++i) {
				std::string name;
				ProfileId id{};
				namesReadable = ReadName(begin + i * profileEntrySize, name);
				if (namesReadable && SameName(name, wanted) && Read(begin + i * profileEntrySize, id)) {
					pendingId = id;
					rule = "Profile to Load";
				}
			}
			if (!namesReadable)
				LOG_WARNING("(PROFILE) The local profile names could not be read, so Profile to Load is not used this time" << std::endl);
			else if (!rule)
				LOG_WARNING("(PROFILE) No local profile is named \"" << wanted << "\"; loading the usual profile instead" << std::endl);
		}

		const std::string remembered = Settings::ReturnSettingValue(Setting::AutoProfile);
		for (unsigned int i = 0; i < count && !rule && !remembered.empty(); ++i) {
			ProfileId id{};
			if (Read(begin + i * profileEntrySize, id) && ToHex(id) == remembered) {
				pendingId = id;
				rule = "remembered";
			}
		}
		if (!rule && count == 1) {
			ProfileId id{};
			if (Read(begin, id)) {
				pendingId = id;
				rule = "only one";
			}
		}
		if (!rule && count > 1) {
			double best = 0.0;
			for (unsigned int i = 0; i < count; ++i) {
				ProfileId id{};
				double modified = 0.0;
				if (!Read(begin + i * profileEntrySize, id) || !Read(begin + i * profileEntrySize + off_entryLastModified, modified)
					|| !std::isfinite(modified) || modified <= 0.0 || modified <= best)
					continue;
				best = modified;
				pendingId = id;
				rule = "most recently used";
			}
		}
		if (!rule) {
			LOG_INFO("(PROFILE) " << count << " local profiles, none usable: the profile selector opens" << std::endl);
			return 0;
		}

		pendingService = service;
		pendingPlayer = player;
		pending = true;
		std::string chosenName;
		for (unsigned int i = 0; i < count && GameVersion::IsSeptember2022(); ++i) {
			ProfileId id{};
			if (Read(begin + i * profileEntrySize, id) && ToHex(id) == ToHex(pendingId))
				ReadName(begin + i * profileEntrySize, chosenName);
		}
		LOG_INFO("(PROFILE) Selected name=" << chosenName << " id=" << ToHex(pendingId) << std::endl);
		LOG_INFO("(PROFILE) Profile chosen (" << rule << "), the profile selector is skipped" << std::endl);
		return 1;
	}

	uintptr_t originalStartSelector = 0;

	// After pushfd/pushad: ECX +0x18, return address +0x24, player argument +0x28.
	__declspec(naked) void SelectorThunk() {
		__asm {
			pushfd
			pushad
			mov eax, [esp + 0x18]
			mov ecx, [esp + 0x28]
			push ecx
			push eax
			call ChooseProfile
			add esp, 8
			mov [esp + 0x1C], eax
			popad
			popfd
			test eax, eax
			jnz suppress
			jmp dword ptr [originalStartSelector]
		suppress:
			ret 4
		}
	}

	bool TrySelect(void* connect) {
		__try {
			reinterpret_cast<SelectFn>(connectSelect)(connect, pendingPlayer, &pendingId);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	bool TryStockSelector() {
		__try {
			reinterpret_cast<StartSelectorFn>(startSelector)(reinterpret_cast<void*>(pendingService), pendingPlayer);
			return true;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return false;
		}
	}

	void Remember(const ProfileId& id) {
		const std::string hex = ToHex(id);
		if (Settings::ReturnSettingValue(Setting::AutoProfile) != hex)
			SettingEdits::Apply({ Setting::AutoProfile, "Mod Settings", "AutoProfile", hex, "Profile loaded automatically", false });
	}

	// Remember player 1 only; multiplayer must not replace the startup profile.
	uintptr_t __fastcall SelectHook(void* connect, void*, unsigned int player, const ProfileId* id) {
		ProfileId copy{};
		if (player == 0 && Read(reinterpret_cast<uintptr_t>(id), copy) && Settings::ReturnSettingValue(Setting::AutoProfile) != ToHex(copy)) {
			Remember(copy);
			LOG_INFO("(PROFILE) Remembered the selected profile for later launches" << std::endl);
		}
		return reinterpret_cast<SelectFn>(connectSelect)(connect, player, id);
	}

	bool installed = false;
}

SettingDefs ProfileAutoLoadMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::ProfileAutoLoad, "ProfileAutoLoad", "Load Profile Automatically", "Mod Settings", "Mod Settings", "on")
			.MigratesFrom("Toggle Switches", "ForceProfileLoad")
			.Hint("Loads your profile at startup without the profile screen.\n"
				"Turn it off if you see problems with your profile at startup.")
			.Heading(Framework::SettingHeading::SongsAndProfiles),
		SettingDef::String(Setting::ProfileToLoad, "Profile to Load")
			.Hint("The name of the profile to load. Leave it empty to load the profile you picked last.")
			.WithVisibleWhen(Setting::ProfileAutoLoad),
		SettingDef::String(Setting::AutoProfile, "Profile Picked Last")
			.Hint("Remembered for you when you pick a profile. Clear it and the most recently used profile loads instead.")
			.Ini("Mod Settings", "AutoProfile")
			.WithVisibleWhen(Setting::ProfileAutoLoad),
	};
}

void ProfileAutoLoadMod::Install() {
	if (!GameVersion::IsSeptember2022() && !GameVersion::IsLearnAndPlay()) {
		LOG_WARNING("(PROFILE) Automatic profile load not installed: the executable is not a supported build" << std::endl);
		return;
	}
	selectorCall = Offsets::startupSelectorCall;
	startSelector = Offsets::startupStartSelector;
	originalStartSelector = startSelector;
	connectSingleton = Offsets::startupConnectSingleton;
	connectVtable = Offsets::startupConnectVtable;
	connectSelectSlot = connectVtable + 0x5C;
	connectSelect = Offsets::startupConnectSelect;
	unsigned char opcode = 0;
	int32_t rel = 0;
	uintptr_t select = 0;
	if (!Read(selectorCall, opcode) || opcode != 0xE8 || !Read(selectorCall + 1, rel) || selectorCall + 5 + rel != startSelector
		|| !Read(connectSelectSlot, select) || select != connectSelect) {
		LOG_WARNING("(PROFILE) Automatic profile load not installed: the selector call or the connect service did not match" << std::endl);
		return;
	}
	const int32_t thunkRel = static_cast<int32_t>(reinterpret_cast<uintptr_t>(&SelectorThunk) - (selectorCall + 5));
	MemUtil::PatchAdr(reinterpret_cast<LPVOID>(selectorCall + 1), &thunkRel, sizeof(thunkRel));
	const uintptr_t hook = reinterpret_cast<uintptr_t>(&SelectHook);
	MemUtil::PatchAdr(reinterpret_cast<LPVOID>(connectSelectSlot), &hook, sizeof(hook));
	installed = true;
	LOG_INFO("(PROFILE) Automatic profile load installed" << std::endl);
}

void ProfileAutoLoadMod::OnFrame() {
	if (!installed || !pending)
		return;
	pending = false;

	uintptr_t connect = 0, vtable = 0;
	if (Read(connectSingleton, connect) && Read(connect, vtable) && vtable == connectVtable && TrySelect(reinterpret_cast<void*>(connect))) {
		LOG_INFO("(PROFILE) Profile selected natively" << std::endl);
		Remember(pendingId);
		return;
	}
	LOG_WARNING("(PROFILE) Selecting the profile failed; opening the profile selector" << std::endl);
	if (!TryStockSelector())
		LOG_ERROR("(PROFILE) The profile selector couldn't be opened either" << std::endl);
}

static Framework::ModRegistrar<ProfileAutoLoadMod> _profileAutoLoadReg;
