#include "../stdafx.h"
#include "UltrawideMod.hpp"
#include "UltrawideRRDim.hpp"
#include "../D3D/UltrawideShaders.hpp"

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;
namespace Setting = Settings::Setting;

namespace {
	// Makes the game build its camera frustums at the display aspect instead of the 16:9 it
	// assumes.
	constexpr BYTE aspectLoadSignature[] = {
		0xD9, 0x44, 0x24, 0x1C,
		0xDD, 0x05, 0x00, 0x00, 0x00, 0x00,
		0xD8, 0xC9,
		0xD9, 0x5C, 0x24, 0x1C
	};
	constexpr char aspectLoadMask[] = "xxxxxx????xxxxxx";
	constexpr size_t aspectLoadOperandOffset = 6;
	constexpr double sixteenByNine = 16.0 / 9.0;

	double displayAspect = sixteenByNine;

	// Back to stock 16:9: no draw confinement, stock camera, HUD over the whole display.
	void PublishInactive() {
		D3DHooks::ultrawideActive.store(false);
		displayAspect = sixteenByNine;
		Framework::Hud().SetLayoutAspect(0.0f);
	}
}

SettingDefs UltrawideMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::Ultrawide, "Ultrawide", "Ultrawide")
			.Hint("Ultrawide (21:9 / 32:9) display support. The interface stays 16:9.")
	};
}

void UltrawideMod::SetDisplayAspect(double aspect) {
	if (aspect > 0.0)
		displayAspect = aspect;
}

bool UltrawideMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::Ultrawide);
}

void UltrawideMod::OnInitialize(ModContext& c) {
	c.Draw().RegisterFrame([](IDirect3DDevice9* device) {
		D3DHooks::UpdateUltrawideState(device);

		// A disable can land while this runs. OnDisabled clears the setting flag before the state, so
		// re-checking the flag after publishing means whichever side finishes last leaves it off.
		std::atomic_thread_fence(std::memory_order_seq_cst);
		if (!D3DHooks::ultrawideSettingOn.load()) {
			PublishInactive();
			return;
		}

		Framework::Hud().SetLayoutAspect(D3DHooks::ultrawideActive.load(std::memory_order_relaxed)
			? static_cast<float>(sixteenByNine) : 0.0f);
	});

	// The game recreates its shaders, textures and render targets across a reset, and these caches are
	// keyed by raw pointer. Runs whether or not the mod is enabled.
	c.Draw().RegisterDeviceReset([](IDirect3DDevice9*) {
		D3DHooks::ultrawideBackBufferValid = false;
		D3DHooks::ultrawideRenderTargetTextures.clear();
		UltrawideShaders::Forget();
		UltrawideShaders::TextureStages::Forget();
	});
}

void UltrawideMod::OnEnabled(ModContext&) {
	D3DHooks::ultrawideSettingOn = true;
	SetPatched(true);
	// Installed once and left in place; the gate below makes it inert when the correction
	// is not active, so toggling the setting never rewrites game code.
	UltrawideRRDim::Install();
}

void UltrawideMod::OnDisabled(ModContext&) {
	// The frame callback stops once disabled, so undo what it published here (flag first, see OnInitialize).
	D3DHooks::ultrawideSettingOn = false;
	std::atomic_thread_fence(std::memory_order_seq_cst);
	PublishInactive();
	SetPatched(false);
	UltrawideRRDim::SetActive(false);
}

void UltrawideMod::OnTick(ModContext&) {
	// ultrawideActive is decided per frame on the render thread from the setting and the
	// backbuffer aspect, so it is the one flag that already means "corrections apply".
	UltrawideRRDim::SetActive(D3DHooks::ultrawideActive.load(std::memory_order_relaxed));
}

void UltrawideMod::OnShutdown(ModContext&) {
	UltrawideRRDim::Uninstall();
}

void UltrawideMod::SetPatched(bool enable) {
	if (enable == patched)
		return;

	if (!located) {
		const uintptr_t match = MemUtil::FindPattern<uintptr_t>(MemUtil::GetTextSectionAddress(), MemUtil::GetTextSectionLength(),
			const_cast<PBYTE>(aspectLoadSignature), aspectLoadMask);

		if (match) {
			const uintptr_t candidate = match + aspectLoadOperandOffset;
			const uint32_t constantAddress = *reinterpret_cast<const uint32_t*>(candidate);

			if (!MemUtil::IsBadReadPtr(reinterpret_cast<void*>(constantAddress))) {
				const double constant = *reinterpret_cast<const double*>(constantAddress);
				if (constant >= sixteenByNine - 0.01 && constant <= sixteenByNine + 0.01) {
					operandAddress = candidate;
					originalOperand = constantAddress;
					located = true; // Only latch on success, so a scan run before the text section is ready can retry.
				}
			}
		}
	}

	if (!operandAddress)
		return;

	const uint32_t operand = enable
		? static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&displayAspect))
		: originalOperand;

	if (MemUtil::PatchAdr(reinterpret_cast<LPVOID>(operandAddress), &operand, sizeof(operand)))
		patched = enable;
}

static Framework::ModRegistrar<UltrawideMod> _ultrawideReg;
