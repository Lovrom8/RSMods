#include "stdafx.h"
#include "ImGuiFontManager.hpp"

#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {
	struct Key {
		std::string face; // normalized: trimmed + lower-case
		int size;
		bool operator==(const Key& o) const { return size == o.size && face == o.face; }
	};

	struct KeyHash {
		size_t operator()(const Key& k) const {
			size_t h = std::hash<std::string>()(k.face);
			h ^= std::hash<int>()(k.size) + 0x9e3779b9 + (h << 6) + (h >> 2);
			return h;
		}
	};

	std::string NormalizeFace(std::string s) {
		auto not_space = [](int c) { return !std::isspace(c); };
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
		s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c) { return char(std::tolower(c)); });
		return s;
	}

	using Bytes = std::vector<char>;

	// Everything below reads the OpenType/TTC on-disk format, which is big-endian. Field offsets and
	// record sizes come straight from the spec (learn.microsoft.com/typography/opentype/spec). Every
	// read is bounds-checked, so a truncated or malformed file just fails to match rather than
	// reading past the buffer.
	bool ReadU16(const Bytes& d, size_t off, uint16_t& v) {
		if (off + 2 > d.size()) return false;
		v = uint16_t((uint8_t(d[off]) << 8) | uint8_t(d[off + 1]));
		return true;
	}
	bool ReadU32(const Bytes& d, size_t off, uint32_t& v) {
		if (off + 4 > d.size()) return false;
		v = (uint32_t(uint8_t(d[off])) << 24) | (uint32_t(uint8_t(d[off + 1])) << 16)
			| (uint32_t(uint8_t(d[off + 2])) << 8) | uint32_t(uint8_t(d[off + 3]));
		return true;
	}
	bool TagEquals(const Bytes& d, size_t off, const char* tag) {
		return off + 4 <= d.size() && d[off] == tag[0] && d[off + 1] == tag[1]
			&& d[off + 2] == tag[2] && d[off + 3] == tag[3];
	}

	// One 12-byte entry of the `name` table.
	struct NameRecord {
		uint16_t platformId, encodingId, languageId, nameId, length, stringOffset;
	};
	bool ReadNameRecord(const Bytes& d, size_t off, NameRecord& r) {
		return ReadU16(d, off + 0, r.platformId) && ReadU16(d, off + 2, r.encodingId)
			&& ReadU16(d, off + 4, r.languageId) && ReadU16(d, off + 6, r.nameId)
			&& ReadU16(d, off + 8, r.length) && ReadU16(d, off + 10, r.stringOffset);
	}

	std::string DecodeName(const Bytes& d, size_t at, uint16_t len, bool utf16be) {
		if (at + len > d.size()) return {};
		std::string s;
		if (utf16be)
			for (uint16_t j = 0; j + 1 < len; j += 2) s.push_back(d[at + j + 1]); // low byte is enough for Latin
		else
			s.assign(&d[at], &d[at] + len);
		return s;
	}

	// Family name (nameID 1) from a `name` table, preferring the Windows record. Good enough for the
	// Latin family names GDI reports; a localized-only name just won't match and falls back to font 0.
	std::string ReadFamilyName(const Bytes& d, size_t nameTable) {
		constexpr uint16_t kFamilyName = 1, kWindowsPlatform = 3;
		constexpr size_t kHeaderSize = 6, kRecordSize = 12; // name table header, then records
		constexpr size_t kCountField = 2, kStorageField = 4; // fields within the header

		uint16_t count = 0, storageOffset = 0;
		if (!ReadU16(d, nameTable + kCountField, count) || !ReadU16(d, nameTable + kStorageField, storageOffset))
			return {};
		const size_t storage = nameTable + storageOffset;

		std::string anyMatch, windowsMatch;
		for (uint16_t i = 0; i < count; ++i) {
			NameRecord n;
			if (!ReadNameRecord(d, nameTable + kHeaderSize + size_t(i) * kRecordSize, n)) break;
			if (n.nameId != kFamilyName || n.length == 0) continue;

			std::string name = DecodeName(d, storage + n.stringOffset, n.length, n.platformId == kWindowsPlatform);
			if (name.empty()) continue;
			if (n.platformId == kWindowsPlatform && windowsMatch.empty()) windowsMatch = name;
			if (anyMatch.empty()) anyMatch = name;
		}
		return !windowsMatch.empty() ? windowsMatch : anyMatch;
	}

	// A TTC packs several fonts behind one 'ttcf' header, each with its own table directory. Return the
	// sub-font whose family matches the GDI-selected face; 0 for a plain TTF, no match, or a parse error
	// - all of which safely mean "use the first font".
	int ResolveTtcFontIndex(const Bytes& data, const std::string& selectedFaceNorm) {
		constexpr size_t kNumFontsField = 8, kOffsetTable = 12;   // within the TTC header
		constexpr size_t kNumTablesField = 4, kTableRecords = 12; // within a font's table directory
		constexpr size_t kTableRecordSize = 16, kTableOffsetField = 8; // within one table record

		if (!TagEquals(data, 0, "ttcf")) return 0;
		uint32_t numFonts = 0;
		if (!ReadU32(data, kNumFontsField, numFonts) || numFonts == 0) return 0;

		for (uint32_t i = 0; i < numFonts; ++i) {
			uint32_t fontDir = 0;
			if (!ReadU32(data, kOffsetTable + size_t(i) * 4, fontDir)) break;
			uint16_t numTables = 0;
			if (!ReadU16(data, fontDir + kNumTablesField, numTables)) continue;

			for (uint16_t t = 0; t < numTables; ++t) {
				const size_t record = fontDir + kTableRecords + size_t(t) * kTableRecordSize;
				if (!TagEquals(data, record, "name")) continue;
				uint32_t nameTable = 0;
				if (ReadU32(data, record + kTableOffsetField, nameTable)
					&& NormalizeFace(ReadFamilyName(data, nameTable)) == selectedFaceNorm)
					return int(i);
				break; // this font's name table, matched or not
			}
		}
		return 0;
	}

	// Raw font-file bytes GDI resolves for `face`. The HFONT height is irrelevant - it only selects the
	// typeface; ImGui sizes the glyphs. GetFontData(table 0) returns the whole file, incl. 'ttcf'
	// collections; fontNo is the sub-font ImGui should pick. False if GDI has no byte data for the face.
	bool LoadFaceBytes(const std::string& face, std::vector<char>& out, int& fontNo) {
		fontNo = 0;
		HFONT hFont = CreateFontA(0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, face.c_str());
		if (!hFont) return false;

		HDC hdc = CreateCompatibleDC(nullptr);
		if (!hdc) { DeleteObject(hFont); return false; }
		HGDIOBJ old = SelectObject(hdc, hFont);

		// The face GDI actually selected (post-substitution) is what the bytes hold - match the TTC on it.
		char selected[LF_FACESIZE] = {};
		GetTextFaceA(hdc, LF_FACESIZE, selected);

		bool ok = false;
		DWORD size = GetFontData(hdc, 0, 0, nullptr, 0); // query byte count
		if (size != GDI_ERROR && size > 0) {
			out.resize(size);
			ok = GetFontData(hdc, 0, 0, out.data(), size) == size;
		}

		SelectObject(hdc, old);
		DeleteDC(hdc);
		DeleteObject(hFont);

		if (ok) fontNo = ResolveTtcFontIndex(out, NormalizeFace(selected));
		return ok;
	}

	struct Manager {
		std::unordered_map<Key, ImFont*, KeyHash> fonts;
		// Atlas doesn't own the font bytes (FontDataOwnedByAtlas=false), and Build() re-reads them on
		// every rebuild, so keep them alive for the atlas lifetime.
		std::vector<std::unique_ptr<std::vector<char>>> buffers;
		bool dirty = false;
	};

	Manager& Mgr() {
		static Manager m;
		return m;
	}
}

ImFont* ImGuiHud::AcquireFont(const std::string& face, int pixelHeight) {
	if (pixelHeight <= 0) pixelHeight = 1;
	Key key{ NormalizeFace(face), pixelHeight };

	Manager& m = Mgr();
	if (auto it = m.fonts.find(key); it != m.fonts.end())
		return it->second; // hit (possibly a cached nullptr miss)

	auto bytes = std::make_unique<std::vector<char>>();
	int fontNo = 0;
	if (!LoadFaceBytes(face, *bytes, fontNo)) {
		m.fonts.emplace(key, nullptr); // cache the miss so we don't hammer GDI every frame
		return nullptr;
	}

	ImFontConfig cfg;
	cfg.FontDataOwnedByAtlas = false; // we own the bytes (see Manager::buffers)
	cfg.FontNo = fontNo;              // sub-font index for TTC collections
	ImFont* font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
		bytes->data(), static_cast<int>(bytes->size()),
		static_cast<float>(pixelHeight), &cfg);

	m.buffers.push_back(std::move(bytes));
	m.fonts.emplace(key, font);
	m.dirty = true;
	return font;
}

bool ImGuiHud::ConsumeAtlasDirty() {
	bool d = Mgr().dirty;
	Mgr().dirty = false;
	return d;
}
