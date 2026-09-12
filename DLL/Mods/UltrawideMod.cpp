#include "../stdafx.h"
#include "UltrawideMod.hpp"

using Framework::ModContext;
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
}

void UltrawideMod::SetDisplayAspect(double aspect) {
	if (aspect > 0.0)
		displayAspect = aspect;
}

bool UltrawideMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::Ultrawide);
}

void UltrawideMod::OnEnabled(ModContext&) {
	D3DHooks::ultrawideSettingOn = true;
	SetPatched(true);
}

void UltrawideMod::OnDisabled(ModContext&) {
	D3DHooks::ultrawideSettingOn = false;
	SetPatched(false);
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
