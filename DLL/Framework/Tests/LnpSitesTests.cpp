// Every Learn & Play span must parse, and absolute operands must relocate with the load address.
#include "../../LnpSites.hpp"

#include <cstdio>
#include <initializer_list>

using namespace LnpSites;

static int failures = 0;

static void Check(bool condition, const char* what) {
	if (!condition) {
		std::printf("FAIL: %s\n", what);
		++failures;
	}
}

int main() {
	std::vector<unsigned char> out;

	Check(Expand("55 8B EC", 0x1D0000, out) && out == std::vector<unsigned char>{ 0x55, 0x8B, 0xEC }, "plain bytes");
	Check(Expand("A1 @E5C240 33", 0x1D0000, out)
		&& out == std::vector<unsigned char>{ 0xA1, 0x40, 0xC2, 0x02, 0x01, 0x33 }, "operand at the dump base");
	Check(Expand("@E5C240", 0x00A20000, out)
		&& out == std::vector<unsigned char>{ 0x40, 0xC2, 0x87, 0x01 }, "operand follows the load address");
	Check(!Expand("5", 0, out), "half byte rejected");
	Check(!Expand("5G", 0, out), "bad digit rejected");
	Check(!Expand("555", 0, out), "run-on byte rejected");
	Check(!Expand("@", 0, out), "empty operand rejected");
	Check(!Expand("@123456789", 0, out), "long operand rejected");
	Check(!Expand("", 0, out), "empty span rejected");

	const Span all[] = {
		kUplayDialogCancel, kUplayDialogIdRead, kUplayDialogTickSlot, kUplayDialogTransitionSlot, kUplayDialogCancelSlot,
		kUplayDialogTick, kUplayTransitionBegin, kUplayListTickSlot, kUplayListTransitionSlot, kUplayListCancelSlot,
		kUplayListTick, kUplayConnectSkip, kUplayProfileServiceSlot, kUplayProfileServiceGetter, kUplayHideDialog,
		kUplayDialogManagerHide, kUplayWin32Skip, kRdvMetadataEntry, kRdvMetadataNoop, kDlcStorePrefetch,
		kDlcInstalledCheck, kFrameBudgetLoad, kFrameBudgetThirty, kBootLoftLight, kSetViewFrustum, kGfxOpenEx, kGfxTell, kGfxLength, kGfxRead, kGfxSeek,
		kVfsTable, kEngineStringDestroy, kMapperAddReferenceCall, kMapperAddReference, kProfileIdCheck,
		kRrDimCall, kRrDimTarget, kRrDimReturn, kPitchReference, kPitchRoute,
	};
	for (const Span& span : all) {
		const bool parsed = Expand(span.bytes, 0x1D0000, out);
		Check(parsed, span.bytes);
	}

	// The table's eight pointers are the natives, in order.
	Check(Expand(kVfsTable.bytes, 0, out) && out.size() == 32, "VFS table size");
	for (int i = 0; i < 8 && out.size() == 32; ++i) {
		const uint32_t value = out[i * 4] | out[i * 4 + 1] << 8 | out[i * 4 + 2] << 16 | static_cast<uint32_t>(out[i * 4 + 3]) << 24;
		Check(value == kVfsNatives[i], "VFS table entry is its native");
	}

	// Call sites whose rel32 the mods follow land on the mapped targets.
	auto target = [](const Span& span, size_t at) {
		std::vector<unsigned char> b;
		Expand(span.bytes, 0, b);
		const int32_t rel = static_cast<int32_t>(b[at + 1] | b[at + 2] << 8 | b[at + 3] << 16 | static_cast<uint32_t>(b[at + 4]) << 24);
		return span.rva + static_cast<uint32_t>(at) + 5 + static_cast<uint32_t>(rel);
	};
	Check(target(kMapperAddReferenceCall, 0) == kMapperAddReference.rva, "AddReference call target");
	Check(target(kUplayHideDialog, 17) == kUplayDialogManagerHide.rva, "hideDialog call target");
	Check(target(kRrDimCall, 0) == kRrDimTarget.rva, "RR Dim call target");
	Check(target(kPitchRoute, 0x52) == kPitchReference.rva, "pitch route call target");
	Check(Expand(kPitchRoute.bytes, 0, out) && out.size() >= 0x40, "pitch route span");
	for (size_t at : { size_t(0x2C), size_t(0x3C) }) {
		if (out.size() < at + 4) { Check(false, "pitch manager operand present"); continue; }
		const uint32_t manager = out[at] | out[at + 1] << 8 | out[at + 2] << 16
			| static_cast<uint32_t>(out[at + 3]) << 24;
		Check(manager == kPitchManagerGlobal, "pitch manager operand");
	}

	if (failures) {
		std::printf("%d LnpSites check(s) failed\n", failures);
		return 1;
	}
	std::printf("LnpSites: all checks passed\n");
	return 0;
}
