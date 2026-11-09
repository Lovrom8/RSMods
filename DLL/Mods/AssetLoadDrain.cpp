#include "../stdafx.h"
#include "AssetLoadDrain.hpp"

namespace {
	uintptr_t loaderStepFunc = 0;
	unsigned int maxLoadsPerLoaderTick = 64;
	bool installed = false;
	bool scanActive = false;
	bool bootActive = false;
	// Written on the game thread; read on the loader thread.
	volatile unsigned int loopActive = 0;
	volatile unsigned int extraLoads = 0;

	extern "C" void __cdecl AssetLoadNoteLoop(unsigned int extra) {
		extraLoads += extra;
	}

	// stdcall(this); preserve EBX/ESI/EDI and return the final AL progress flag.
	void __declspec(naked) LoaderStepLoop() {
		__asm {
			push ebp
			mov ebp, esp
			push esi
			push edi
			mov esi, dword ptr [ebp + 8]
			xor edi, edi
		again:
			push esi
			call dword ptr [loaderStepFunc]
			test al, al
			je done
			cmp dword ptr [loopActive], 0
			je done
			inc edi
			cmp edi, dword ptr [maxLoadsPerLoaderTick]
			jb again
		done:
			push eax
			push edi
			call AssetLoadNoteLoop
			add esp, 4
			pop eax
			pop edi
			pop esi
			pop ebp
			ret 4
		}
	}
}

namespace {
	void UpdateActive() {
		const bool active = scanActive || bootActive;
		if (installed && loopActive != (active ? 1u : 0u))
			LOG_INFO("(ASSET LOADS) Batching " << (active ? "on" : "off") << ", extra loads so far " << extraLoads << std::endl);
		loopActive = active ? 1 : 0;
	}
}

void AssetLoadDrain::SetActive(bool active) {
	scanActive = active;
	UpdateActive();
}

void AssetLoadDrain::SetBootActive(bool active) {
	bootActive = active;
	UpdateActive();
}

void AssetLoadDrain::Install() {
	if (installed)
		return;

	const uintptr_t callSite = Offsets::ptr_assetLoaderStepCall;
	loaderStepFunc = Offsets::func_assetLoaderStep;
	if (!callSite || !loaderStepFunc) {
		LOG_WARNING("(ASSET LOADS) Not supported on this game version" << std::endl);
		return;
	}
	if (MemUtil::IsBadReadPtr(reinterpret_cast<void*>(callSite)) || *reinterpret_cast<const unsigned char*>(callSite) != 0xE8) {
		LOG_WARNING("(ASSET LOADS) Loader call site is not a CALL, not installed" << std::endl);
		return;
	}
	const int32_t originalRel = *reinterpret_cast<const int32_t*>(callSite + 1);
	if (callSite + 5 + originalRel != loaderStepFunc) {
		LOG_WARNING("(ASSET LOADS) Loader call site targets another function, not installed" << std::endl);
		return;
	}
	const int32_t rel = static_cast<int32_t>(reinterpret_cast<uintptr_t>(&LoaderStepLoop) - (callSite + 5));
	if (!MemUtil::PatchAdr(reinterpret_cast<LPVOID>(callSite + 1), &rel, sizeof(rel))) {
		LOG_ERROR("(ASSET LOADS) Call site patch failed, not installed" << std::endl);
		return;
	}
	installed = true;
	LOG_INFO("(ASSET LOADS) Background loader takes up to " << maxLoadsPerLoaderTick << " loads per tick while the DLC scan runs" << std::endl);
}
