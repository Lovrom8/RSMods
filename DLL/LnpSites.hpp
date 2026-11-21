#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// Sites are RVAs. @RVA expands to a relocated 32-bit module pointer.
namespace LnpSites {
	struct Span {
		uint32_t rva;
		const char* bytes;
	};

	inline int HexDigit(char c) {
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'A' && c <= 'F') return c - 'A' + 10;
		if (c >= 'a' && c <= 'f') return c - 'a' + 10;
		return -1;
	}

	// Expands a span's text for a module loaded at base. False on malformed text.
	inline bool Expand(const char* text, uint32_t base, std::vector<unsigned char>& out) {
		out.clear();
		const char* p = text;
		while (*p) {
			if (*p == ' ') { ++p; continue; }
			if (*p == '@') {
				uint32_t rva = 0;
				int digits = 0;
				for (++p; *p && *p != ' '; ++p, ++digits) {
					const int d = HexDigit(*p);
					if (d < 0 || digits >= 8) return false;
					rva = rva * 16 + static_cast<uint32_t>(d);
				}
				if (digits == 0) return false;
				const uint32_t value = base + rva;
				for (int i = 0; i < 4; ++i)
					out.push_back(static_cast<unsigned char>(value >> (8 * i)));
				continue;
			}
			const int hi = HexDigit(p[0]);
			const int lo = hi < 0 ? -1 : HexDigit(p[1]);
			if (lo < 0 || (p[2] != ' ' && p[2] != '\0')) return false;
			out.push_back(static_cast<unsigned char>(hi * 16 + lo));
			p += 2;
		}
		return !out.empty();
	}

	// Uplay dialog skips. Dialog and list vtables: Tick +0x18, transitionBegin +0x6C, cancel +0x110.
	inline constexpr Span kUplayDialogCancel = { 0x2A9C20, "55 8B EC 83 E4 F8 83 EC 08 56 8B 35 @F6062C" };
	inline constexpr Span kUplayDialogIdRead = { 0x2A9C35, "8D B9 C0 02 00 00" };
	inline constexpr Span kUplayDialogTickSlot = { 0xDC4798, "@34DCF0" };
	inline constexpr Span kUplayDialogTransitionSlot = { 0xDC47EC, "@2A9150" };
	inline constexpr Span kUplayDialogCancelSlot = { 0xDC4890, "@2A9C20" };
	inline constexpr Span kUplayDialogTick = { 0x34DCF0, "55 8B EC 51 53 56 57 8B F1 E8" };
	inline constexpr Span kUplayTransitionBegin = { 0x2A9150, "55 8B EC 83 EC 08 8D 45 F8 50 81 C1 C0 02 00 00" };
	inline constexpr Span kUplayListTickSlot = { 0xDBD820, "@1DC090" };
	inline constexpr Span kUplayListTransitionSlot = { 0xDBD874, "@2A9150" };
	inline constexpr Span kUplayListCancelSlot = { 0xDBD918, "@2A9C20" };
	inline constexpr Span kUplayListTick = { 0x1DC090, "51 56 57 8B F1 E8" };
	inline constexpr Span kUplayConnectSkip = { 0x4B83C0, "8B 01 8B 90 00 02 00 00 FF D2" };
	// The connect service's +0x200 slot: the getter that returns its profile service at +0xC.
	inline constexpr Span kUplayProfileServiceSlot = { 0xDDD5DC, "@0010D0" };
	inline constexpr Span kUplayProfileServiceGetter = { 0x0010D0, "8B 41 0C C3" };
	inline constexpr Span kUplayHideDialog = { 0x1A48F0, "55 8B EC 8B 45 08 8B 0D @F6062C 8B 51 10 50 52 E8 DA 9A 03 00 5D C2 04 00" };
	inline constexpr Span kUplayDialogManagerHide = { 0x1DE3E0, "55 8B EC 83 E4 F8 83 EC 24 A1 @E5C240 33 C4" };
	inline constexpr Span kUplayWin32Skip = { 0x4CABE0, "C7 81 CC 00 00 00 04 00 00 00 83 3D @F75E90" };

	// RDV metadata guard: DownloadLatestMetadata's entry and its no-download branch.
	inline constexpr Span kRdvMetadataEntry = { 0x4C4090, "51 83 7E 04 03" };
	inline constexpr Span kRdvMetadataNoop = { 0x4C40EC, "C6 44 37 0A 01 59 C3" };

	// RSSteamDownloadableContent::Init: the store prefetch call and the installed check.
	inline constexpr Span kDlcStorePrefetch = { 0x4DB03A, "50 56 E8 1F 07 00 00" };
	inline constexpr Span kDlcInstalledCheck = { 0x4DAFC4, "50 8B 42 1C FF D0 88 44 24 5D 8B 46 1C C6 44 24 5F 01" };

	// RSBootService::OnPreInit's frame budget load, and the 30.0 it loads.
	inline constexpr Span kFrameBudgetLoad = { 0x1A598F, "DD 05 @E25CC0 8B 46 04 DD 98 B0 00 00 00" };
	inline constexpr Span kFrameBudgetThirty = { 0xE25CC0, "00 00 00 00 00 00 3E 40" };

	// The loft's first light set during registration, and NiCamera::SetViewFrustum.
	inline constexpr Span kBootLoftLight = { 0x375A74, "83 7F 14 00 75 0C 6A 00 6A 00 53 33 C0 E8 5A 00 00 00" };
	inline constexpr Span kSetViewFrustum = { 0x5D7F20, "55 8B EC 51 D9 42 10" };

	// NiGFxFile OpenEx, through its stack cookie operand.
	inline constexpr Span kGfxOpenEx = { 0x82F7B0, "55 8B EC 83 E4 F8 83 EC 1C A1 @E5C240 33 C4 89 44 24 18" };

	inline constexpr Span kGfxTell = { 0x82F990, "8B 49 08 8B 01 8B 50 0C FF D2 33 D2 C3" };
	inline constexpr Span kGfxLength = { 0x82F9A0, "8B 49 08 8B 01 8B 50 24 FF E2" };
	inline constexpr Span kGfxRead = { 0x82F9C0, "55 8B EC 56 8B F1 83 7E 08 00 74 1A" };
	inline constexpr Span kGfxSeek = { 0x82FA70, "55 8B EC 83 79 08 00 75 07 83 C8 FF 5D C2 08 00 8B 45 0C 8B 49 08 56 8B 31 50 8B 45 08 99 52 8B 56 20 50 FF D2 0F B6 C0 5E 5D C2 08 00" };

	// GRVFS native file callbacks: Open, Exists, Close, Read, Size, Tell, Seek, Modification.
	inline constexpr uint32_t kVfsNatives[8] = { 0x9A440, 0x9A470, 0x9A490, 0x9A4B0, 0x9A4D0, 0x9A4E0, 0x9A4F0, 0x9A540 };
	inline constexpr Span kVfsTable = { 0xEEF6BC, "@9A440 @9A470 @9A490 @9A4B0 @9A4D0 @9A4E0 @9A4F0 @9A540" };

	// The enumeration banner's engine string destructor, and the mapper's AddReference call.
	inline constexpr Span kEngineStringDestroy = { 0x555910, "8B 41 14 8D 51 10 3B C2 74 3E 56 8B 31 85 F6 74 36" };
	inline constexpr Span kMapperAddReferenceCall = { 0x9E995, "E8 B6 F8 FF FF" };
	inline constexpr Span kMapperAddReference = { 0x9E250, "55 8B EC 81 EC B0 04 00 00" };

	// The profile platform-ID check's branch, with its lead-in.
	inline constexpr Span kProfileIdCheck = { 0x3CF14C, "80 7C 24 0F 00 74 2F" };
	// Loft post-effects call and its target, including the callee's stack cleanup.
	inline constexpr Span kRrDimCall = { 0x375B5A, "E8 01 18 00 00" };
	inline constexpr Span kRrDimTarget = { 0x377360, "55 8B EC 53 8B 5D 08 83 7B 0C 00 56 57 8B F0" };
	inline constexpr Span kRrDimReturn = { 0x377609, "C2 08 00" };

	// Pitch reference setter and the route dispatcher through its call to the setter.
	inline constexpr uint32_t kPitchManagerGlobal = 0xF6057C;
	inline constexpr Span kPitchReference = { 0xDD930, "55 8B EC 83 E4 F8 83 EC 10 83 7D 08 00 53 57 74 31 DB 45 08 83 EC 08 DC 35 @E25D10 D9 5C 24 14 D9 44 24 14 D9 5C 24 04 D9 05 @D99904 D9 1C 24 E8 C9 7D 00 00 DC 0D @E25D08 83 C4 08 EB 06 D9 05 @E25D00 33 C0 D9 9E 5C 13 00 00" };
	inline constexpr Span kPitchRoute = { 0xEA020, "55 8B EC 56 83 F9 05 74 4E 33 C0 EB 03 8D 49 00 39 0C 85 @F587B0 74 09 40 83 F8 0A 7C F1 5E 5D C3 83 F8 FF 74 30 85 C9 75 0A A1 @F6057C 8B 40 10 EB 0E 83 F9 02 75 1D 8B 0D @F6057C 8B 41 14 85 C0 74 10 8B 70 04 85 F6 74 09 8B 55 08 52 E8 B9 38 FF FF 5E 5D C3" };
}
