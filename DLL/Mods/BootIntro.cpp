#include "../stdafx.h"
#include "BootIntro.hpp"

#include <atomic>
#include <span>

namespace {
	// Redirect intro state 0 to state 4 cleanup without constructing intro assets.
	// RSBootService is in EBX; +0x60 holds its state.
	uintptr_t introStateTableEntry0 = 0;
	uintptr_t introStateFourCleanup = 0;
	// Expected cleanup sequence: unspawn +0x5C if present, then set state +0x60 to 5.
	unsigned char introStateFourCleanupExpected[] = {
		0x8B, 0x43, 0x5C, 0x85, 0xC0, 0x74, 0x0C, 0x8B, 0x8E, 0x38, 0x01, 0x00, 0x00, 0x51, 0xE8, 0, 0, 0, 0,
		0xC7, 0x43, 0x60, 0x05, 0x00, 0x00, 0x00
	};
	constexpr size_t introCleanupCallOperand = 15;

	// Defer root-stage integration until boot phase +0x48 reaches 3 to avoid a startup stall.
	// IntegrateRootStage takes its manager in ESI and returns with plain ret.
	uintptr_t integrateRootStage = 0;
	constexpr unsigned char integrateRootStageExpected[] = { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x83, 0xEC, 0x50 };
	constexpr uintptr_t off_bootPhase = 0x48;
	constexpr int bootPhaseIntroDone = 3;

	// Keep the splash at GRRenderContext+0xCC active until ReleaseSplash clears flag bit 0.
	uintptr_t splashSite = 0;
	constexpr unsigned char splashExpected[] = {
		0x8B, 0x88, 0xCC, 0x00, 0x00, 0x00, 0x85, 0xC9, 0x74, 0x09, 0xBA, 0xFE, 0xFF, 0x00, 0x00, 0x66, 0x21, 0x51, 0x08,
		0xC6, 0x80, 0x0C, 0x02, 0x00, 0x00, 0x01
	};
	uintptr_t splashExpectedStart = 0;
	uintptr_t preLoadClickVtable = 0;
	constexpr uintptr_t off_clickFlags = 0x08;
	constexpr ULONGLONG splashMaxMs = 25000;
	volatile uintptr_t bootSplashClick = 0;
	ULONGLONG bootSplashPatchedAt = 0;
	uintptr_t splashResume = 0;
	// Published by the mod thread when boot reaches a screen beyond the title.
	std::atomic<bool> pastTitle{ false };

	void __declspec(naked) BootSplashThunk() {
		__asm {
			mov dword ptr [bootSplashClick], ecx
			jmp dword ptr [splashResume]
		}
	}

	uintptr_t integrateRootStageOriginal = 0;
	volatile uintptr_t introBootService = 0;
	bool introRemoved = false;
	bool rootStageReleased = false;

	extern "C" void __cdecl NoteIntroSkipped(uintptr_t service) {
		introBootService = service;
	}

	void __declspec(naked) IntroStateZeroThunk() {
		__asm {
			pushad
			push ebx
			call NoteIntroSkipped
			add esp, 4
			popad
			jmp dword ptr [introStateFourCleanup]
		}
	}

	extern "C" bool __cdecl HoldRootStage() {
		if (!introRemoved || rootStageReleased)
			return false;
		const uintptr_t service = introBootService;
		if (!service)
			return true;
		const int phase = *reinterpret_cast<const int*>(service + off_bootPhase);
		if (phase < bootPhaseIntroDone)
			return true;
		rootStageReleased = true;
		LOG_INFO("(INTRO) The title's root stage is released at boot phase " << phase << std::endl);
		return false;
	}

	void __declspec(naked) IntegrateRootStageHook() {
		__asm {
			pushfd
			pushad
			call HoldRootStage
			test al, al
			jnz hold
			popad
			popfd
			jmp dword ptr [integrateRootStageOriginal]
		hold:
			popad
			popfd
			ret
		}
	}
}

void BootIntro::ReleaseSplash() {
	const uintptr_t click = bootSplashClick;
	if (!click)
		return;
	bootSplashClick = 0;
	if (MemUtil::IsBadReadPtr(reinterpret_cast<void*>(click)) || *reinterpret_cast<const uintptr_t*>(click) != preLoadClickVtable)
		return;
	*reinterpret_cast<unsigned short*>(click + off_clickFlags) &= 0xFFFE;
	LOG_INFO("(INTRO) Boot splash released" << std::endl);
}

void BootIntro::NoteMenu(const std::string& menu) {
	// SelectionListDialog is part of title sign-in, so it must retain the boot splash.
	if (!menu.empty() && menu != "pre_enter_prompt" && menu != "TitleScreen" && menu != "SelectionListDialog")
		pastTitle.store(true, std::memory_order_relaxed);
}

void BootIntro::OnFrame() {
	if (!bootSplashClick)
		return;
	if (pastTitle.load(std::memory_order_relaxed) || GetTickCount64() - bootSplashPatchedAt > splashMaxMs)
		ReleaseSplash();
}

void BootIntro::Remove() {
	if (!GameVersion::IsSeptember2022() && !GameVersion::IsLearnAndPlay()) {
		LOG_WARNING("(INTRO) Intro left in: the executable is not a supported build" << std::endl);
		return;
	}
	introStateTableEntry0 = Offsets::startupIntroTableEntry;
	introStateFourCleanup = Offsets::startupIntroCleanup;
	integrateRootStage = Offsets::startupIntegrateRootStage;
	splashSite = Offsets::startupSplashSite;
	splashResume = Offsets::startupSplashResume;
	splashExpectedStart = Offsets::startupSplashCheck;
	preLoadClickVtable = Offsets::startupPreLoadClickVtable;
	const uintptr_t introStateZero = Offsets::startupIntroStateZero;
	const unsigned char* const introStateZeroBytes = reinterpret_cast<const unsigned char*>(&introStateZero);
	const std::span<const unsigned char> introStateTableEntry0Expected{ introStateZeroBytes, sizeof(introStateZero) };
	const uintptr_t unspawnRel = Offsets::startupUnspawnEntity
		- (introStateFourCleanup + introCleanupCallOperand + sizeof(uint32_t));
	memcpy(introStateFourCleanupExpected + introCleanupCallOperand, &unspawnRel, sizeof(uint32_t));
	if (!introStateTableEntry0 || !introStateFourCleanup || !integrateRootStage || !splashSite || !splashResume
		|| !splashExpectedStart || !preLoadClickVtable || !introStateZero || !Offsets::startupUnspawnEntity
		|| !MemUtil::MatchesBytes(introStateTableEntry0, introStateTableEntry0Expected)
		|| !MemUtil::MatchesBytes(introStateFourCleanup, introStateFourCleanupExpected)
		|| !MemUtil::MatchesBytes(integrateRootStage, integrateRootStageExpected)
		|| !MemUtil::MatchesBytes(splashExpectedStart, std::span<const unsigned char>(splashExpected))) {
		LOG_WARNING("(INTRO) Intro state table or root stage bytes mismatch; no changes made" << std::endl);
		return;
	}
	integrateRootStageOriginal = reinterpret_cast<uintptr_t>(MemUtil::TrampHook(reinterpret_cast<PBYTE>(integrateRootStage),
		reinterpret_cast<PBYTE>(&IntegrateRootStageHook), sizeof(integrateRootStageExpected)));
	if (!integrateRootStageOriginal) {
		LOG_ERROR("(INTRO) Root stage hook failed; intro left in" << std::endl);
		return;
	}
	const int32_t splashRelative = static_cast<int32_t>(reinterpret_cast<uintptr_t>(&BootSplashThunk) - (splashSite + 5));
	unsigned char splashPatch[9] = { 0xE9, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90 };
	memcpy(splashPatch + 1, &splashRelative, sizeof(splashRelative));
	bootSplashPatchedAt = GetTickCount64();
	if (!MemUtil::PatchAdr(reinterpret_cast<LPVOID>(splashSite), splashPatch, sizeof(splashPatch)))
		LOG_WARNING("(INTRO) Boot splash patch failed; the screen goes black until the title draws" << std::endl);
	introRemoved = true;
	const uintptr_t thunk = reinterpret_cast<uintptr_t>(&IntroStateZeroThunk);
	if (!MemUtil::PatchAdr(reinterpret_cast<LPVOID>(introStateTableEntry0), &thunk, sizeof(thunk))) {
		introRemoved = false;
		LOG_ERROR("(INTRO) Intro state table patch failed; intro left in" << std::endl);
		return;
	}
	LOG_INFO("(INTRO) The intro is never built; the title's root stage waits for boot phase " << bootPhaseIntroDone << std::endl);
}
