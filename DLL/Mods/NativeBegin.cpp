#include "../stdafx.h"
#include "NativeBegin.hpp"
#include "BootIntro.hpp"

namespace {
	// Title Tick is slot +0x18; wait for input-ready state 0x12 before Begin.
	// State 0x11 is loaded but hidden.
	uintptr_t titleVtable = 0;
	uintptr_t titleTickSlot = 0;
	uintptr_t titleTick = 0;
	constexpr uintptr_t off_inputEnabled = 0x0C;
	constexpr uintptr_t off_state = 0x10C;
	constexpr int stateLoadedHidden = 0x11;
	constexpr int stateInputReady = 0x12;

	// Connect +0x7C takes player and input-user indices; keyboard uses input user 0.
	// Native profile-service checks may refuse Begin.
	uintptr_t connectSingleton = 0;
	uintptr_t connectVtable = 0;
	uintptr_t connectBeginSlot = 0;
	uintptr_t connectBegin = 0;

	using TickFn = int(__thiscall*)(void* title);
	using BeginFn = bool(__thiscall*)(void* connect, unsigned int player, unsigned int inputUser);

	bool accepted = false;
	// Retry a refused Begin only after a new readiness edge.
	bool wasReady = false;
	// Shared by title Tick and autosave thunk on the game thread.
	volatile unsigned char skipSignIn = 0;

	template <typename T>
	bool Read(uintptr_t address, T& value) {
		if (!address || MemUtil::IsBadReadPtr(reinterpret_cast<void*>(address)))
			return false;
		value = *reinterpret_cast<const T*>(address);
		return true;
	}

	bool SignInBypassOn() {
		return Settings::ReturnSettingValue(Settings::Setting::UplayLogin) != "ask";
	}

	void TryBegin(uintptr_t title) {
		uintptr_t vtable = 0;
		int state = 0;
		unsigned char input = 0;
		const bool ready = Read(title, vtable) && vtable == titleVtable
			&& Read(title + off_state, state) && state == stateInputReady
			&& Read(title + off_inputEnabled, input) && input != 0;
		const bool edge = ready && !wasReady;
		wasReady = ready;
		if (!edge)
			return;

		uintptr_t connect = 0, connectTable = 0, begin = 0;
		if (!Read(connectSingleton, connect) || !Read(connect, connectTable) || connectTable != connectVtable
			|| !Read(connectBeginSlot, begin) || begin != connectBegin) {
			LOG_WARNING("(NATIVE BEGIN) Connect service not as expected, leaving the title to the player" << std::endl);
			accepted = true;
			return;
		}

		accepted = reinterpret_cast<BeginFn>(begin)(reinterpret_cast<void*>(connect), 0, 0);
		if (accepted)
			LOG_INFO("(NATIVE BEGIN) Sign-in started from the title" << std::endl);
		else
			LOG_INFO("(NATIVE BEGIN) Sign-in refused, waiting for the title to be ready again" << std::endl);
	}

	// Skip the warning after flushing pending profile writes through the native save path.
	// SaveProfile takes the manager on stack, ret 4; SignInFlowComplete has no arguments.
	uintptr_t armWarning = 0;
	constexpr unsigned char armWarningBytes[] = { 0xC6, 0x43, 0x18, 0x01, 0xEB, 0x57 };
	uintptr_t armWarningJumpTarget = 0;
	uintptr_t gameService = 0;
	constexpr uintptr_t off_saveManager = 0x30;
	constexpr uintptr_t off_flushGate = 0x0C;
	uintptr_t saveProfile = 0;
	constexpr unsigned char saveProfileBytes[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x08, 0x8B, 0x45, 0x08 };

	using SaveProfileFn = void(__stdcall*)(void* manager);

	uintptr_t signInFlowCompleteCall = 0;
	uintptr_t handlerTailJump = 0;
	uintptr_t getInputMode = 0;
	uintptr_t setInputMode = 0;
	uintptr_t defaultInputMode = 0;
	uintptr_t profileContinuation = 0;
	bool inputModeOnStack = false;

	unsigned char PrimaryInputMode() {
		unsigned char mode = 4;
		if (inputModeOnStack)
			mode = reinterpret_cast<unsigned char(__cdecl*)(unsigned int)>(getInputMode)(0);
		else {
			__asm {
				xor eax, eax
				call dword ptr [getInputMode]
				mov mode, al
			}
		}
		return mode;
	}

	void __cdecl CompleteProfileSignIn() {
		const unsigned char mode = PrimaryInputMode();
		// Sign-in is complete here. Use this profile, never a transient cached mode.
		if (mode < 4) {
			__asm {
				xor eax, eax
				mov cl, mode
				call dword ptr [setInputMode]
			}
		}
		else {
			reinterpret_cast<bool(__cdecl*)()>(defaultInputMode)();
			// Mode 4 is never a valid input; only an unfinished profile may keep it until setup picks one.
			if (PrimaryInputMode() >= 4)
				LOG_ERROR("(NATIVE BEGIN) Input mode still unknown after the stock default; first-run input selection has to set it" << std::endl);
		}
		LOG_INFO("(NATIVE BEGIN) Loaded profile input mode=" << static_cast<unsigned int>(mode)
			<< "; continuing through stock first-run/menu selection" << std::endl);
		// The title always requests MainMenu. The picker checks FinishedFE and opens setup.
		reinterpret_cast<void(__cdecl*)()>(profileContinuation)();
	}


	void SaveBeforeMenu() {
		uintptr_t service = 0, manager = 0;
		if (Read(gameService, service) && Read(service + off_saveManager, manager) && manager) {
			*reinterpret_cast<unsigned char*>(manager + off_flushGate) = 1;
			reinterpret_cast<SaveProfileFn>(saveProfile)(reinterpret_cast<void*>(manager));
		}
		LOG_INFO("(NATIVE BEGIN) Autosave warning skipped, profile continuation requested" << std::endl);
	}

	__declspec(naked) void SignInCompleteThunk() {
		__asm {
			cmp byte ptr [skipSignIn], 0
			je native
			call SaveBeforeMenu
			call dword ptr [signInFlowCompleteCall]
			jmp dword ptr [handlerTailJump]
		native:
			mov byte ptr [ebx + 0x18], 1
			jmp dword ptr [armWarningJumpTarget]
		}
	}

	int __fastcall TitleTick(void* title, void*) {
		if (!accepted) {
			skipSignIn = SignInBypassOn() ? 1 : 0;
			TryBegin(reinterpret_cast<uintptr_t>(title));
		}
		const int result = reinterpret_cast<TickFn>(titleTick)(title);
		// Release the splash only after the title leaves its loaded-hidden state.
		int shownState = 0;
		if (wasReady && Read(reinterpret_cast<uintptr_t>(title) + off_state, shownState) && shownState > stateLoadedHidden)
			BootIntro::ReleaseSplash();
		if (!accepted)
			TryBegin(reinterpret_cast<uintptr_t>(title));
		return result;
	}
}

void NativeBegin::Install() {
	if (!GameVersion::IsSeptember2022() && !GameVersion::IsLearnAndPlay()) {
		LOG_WARNING("(NATIVE BEGIN) Not installed: the executable is not a supported build" << std::endl);
		return;
	}
	titleVtable = Offsets::startupTitleVtable;
	titleTickSlot = titleVtable + 0x18;
	titleTick = Offsets::startupTitleTick;
	connectSingleton = Offsets::startupConnectSingleton;
	connectVtable = Offsets::startupConnectVtable;
	connectBeginSlot = connectVtable + 0x7C;
	connectBegin = Offsets::startupConnectBegin;
	armWarning = Offsets::startupArmWarning;
	gameService = Offsets::startupGameService;
	saveProfile = Offsets::startupSaveProfile;
	signInFlowCompleteCall = Offsets::startupSignInFlowComplete;
	handlerTailJump = Offsets::startupHandlerTail;
	getInputMode = Offsets::startupGetInputMode;
	setInputMode = Offsets::startupSetInputMode;
	defaultInputMode = Offsets::startupDefaultInputMode;
	profileContinuation = Offsets::startupProfileContinuation;
	inputModeOnStack = GameVersion::IsLearnAndPlay();
	constexpr unsigned char completeBytes[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x20 };
	const unsigned char getBytes[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC,
		static_cast<unsigned char>(inputModeOnStack ? 0x08 : 0x0C) };
	constexpr unsigned char setBytes[] = { 0x51, 0x56, 0x8B, 0xF0, 0x32, 0xC0, 0x57, 0x80, 0xF9, 0x04 };
	constexpr unsigned char defaultBytes[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x08, 0x53, 0x57 };
	constexpr unsigned char continuationBytes[] = { 0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x74 };
	if (!getInputMode || !setInputMode || !defaultInputMode || !profileContinuation
		|| !MemUtil::MatchesBytes(signInFlowCompleteCall, completeBytes)
		|| !MemUtil::MatchesBytes(getInputMode, getBytes)
		|| !MemUtil::MatchesBytes(setInputMode, setBytes)
		|| !MemUtil::MatchesBytes(defaultInputMode, defaultBytes)
		|| !MemUtil::MatchesBytes(profileContinuation, continuationBytes)) {
		MemUtil::FailBootPatchBundle();
		LOG_WARNING("(NATIVE BEGIN) Boot bypass off: profile input completion sites unavailable or mismatched" << std::endl);
		return;
	}
	uintptr_t tick = 0;
	if (!Read(titleTickSlot, tick) || tick != titleTick || !MemUtil::MatchesBytes(connectBeginSlot, std::span<const unsigned char>(
		reinterpret_cast<const unsigned char*>(&connectBegin), sizeof(connectBegin)))) {
		MemUtil::FailBootPatchBundle();
		LOG_WARNING("(NATIVE BEGIN) Not installed: the title's Tick or the connect service did not match" << std::endl);
		return;
	}
	const uintptr_t hook = reinterpret_cast<uintptr_t>(&TitleTick);
	LOG_INFO("(NATIVE BEGIN) Input completion original at 0x" << std::hex << signInFlowCompleteCall
		<< std::dec << ": 55 8B EC 83 EC 20" << std::endl);
	MemUtil::PlaceHook(reinterpret_cast<void*>(signInFlowCompleteCall),
		reinterpret_cast<void*>(&CompleteProfileSignIn), sizeof(completeBytes));
	MemUtil::PatchAdr(reinterpret_cast<LPVOID>(titleTickSlot), &hook, sizeof(hook));
	// Resolve the short jump relative to the end of the six patched bytes.
	armWarningJumpTarget = armWarning + sizeof(armWarningBytes) + armWarningBytes[5];
	if (MemUtil::MatchesBytes(armWarning, armWarningBytes) && MemUtil::MatchesBytes(saveProfile, saveProfileBytes))
		MemUtil::PlaceHook(reinterpret_cast<void*>(armWarning), reinterpret_cast<void*>(&SignInCompleteThunk), sizeof(armWarningBytes));
	else {
		MemUtil::FailBootPatchBundle();
		LOG_WARNING("(NATIVE BEGIN) Autosave warning left as is: its site doesn't match" << std::endl);
	}
	LOG_INFO("(NATIVE BEGIN) Installed" << std::endl);
}
