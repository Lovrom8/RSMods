#include "../stdafx.h"
#include "LyricsRenderSpeedup.hpp"
#include "../MemUtil.hpp"

/// <summary>
/// Every frame, the lyrics renderer's update (0x0082E2B0 on the September 2022 build) finds the first glyph in each display
/// state (sung, singing, upcoming, next line...) by walking every glyph of every lyric line in the song. A song with a
/// whole discography of lyrics has hundreds of thousands of glyphs, so that walk costs a noticeable slice of every frame.
///
/// Each line has one state, and the game only records a glyph index the first time it sees it, so only the first glyph
/// of each line can ever be recorded. We walk the lines instead of the glyphs and give the game the same results.
/// Every so often we also do the game's full glyph walk and compare; any difference turns this off for the session.
/// </summary>
namespace LyricsRenderSpeedup {

	constexpr int lyricsLines = 0xB8;		// vector of lines
	constexpr int lineSize = 0x28;
	constexpr int lineState = 0x14;			// short
	constexpr int lineGlyphs = 0x1C;		// vector of 4 byte glyphs

	// First glyph index per state, as floats. 1000000 (0x0119A12C / 0x01224570) means none yet.
	constexpr int firstPast = 0x10;			// state 1
	constexpr int firstCurrent = 0x14;		// state 2
	constexpr int firstUpcoming = 0x18;		// state 3
	constexpr int firstNextLine = 0x1C;		// state 4
	constexpr int firstShown = 0x20;		// any state but 0
	constexpr int firstHiddenAfter = 0x24;	// state 0, after the first shown glyph
	constexpr float none = 1000000.0f;
	const double noneDouble = 1000000.0;

	// Frame locals the glyph walk leaves behind (relative to EBP)
	constexpr int frameGlyphCounter = 0x8;
	constexpr int frameLineOffset = -0x10;

	constexpr uint32_t checkEvery = 256;

	struct PushadRegisters {
		uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
	};

	template <typename T>
	T& At(uintptr_t base, int offset) {
		return *reinterpret_cast<T*>(base + offset);
	}

	struct Result {
		float first[6];	// Same order as the game's fields at 0x10..0x24
		uint32_t glyphs;
		bool operator==(const Result& other) const {
			return glyphs == other.glyphs && memcmp(first, other.first, sizeof(first)) == 0;
		}
	};

	bool disabled = false;
	uint32_t calls = 0;

	/// The game's glyph walk (0x0082E6E1), glyph by glyph.
	Result FullWalk(uintptr_t lines, uint32_t lineCount) {
		Result result;
		std::fill(std::begin(result.first), std::end(result.first), none);
		float& past = result.first[0];
		float& current = result.first[1];
		float& upcoming = result.first[2];
		float& nextLine = result.first[3];
		float& shown = result.first[4];
		float& hiddenAfter = result.first[5];
		int glyphIndex = 0;
		for (uint32_t i = 0; i < lineCount; i++) {
			const uintptr_t line = lines + i * lineSize;
			const uintptr_t end = At<uintptr_t>(line, lineGlyphs + 4);
			for (uintptr_t glyph = At<uintptr_t>(line, lineGlyphs); glyph != end; glyph += 4) {
				const float value = static_cast<float>(glyphIndex++);
				const short state = At<short>(line, lineState);
				if (state != 0 && shown == noneDouble)
					shown = value;
				if (state == 2 && current == noneDouble)
					current = value;
				if (state == 1 && past == noneDouble)
					past = value;
				if (state == 3 && upcoming == noneDouble)
					upcoming = value;
				if (state == 4 && nextLine == noneDouble)
					nextLine = value;
				if (state == 0 && !(shown == noneDouble) && hiddenAfter == noneDouble)
					hiddenAfter = value;
			}
		}
		result.glyphs = static_cast<uint32_t>(glyphIndex);
		return result;
	}

	/// Same result, one step per line: a line's glyphs share its state and only a field still at "none" gets set,
	/// so only a line's first glyph can land. (Needs every glyph index below 1000000, checked by the caller.)
	Result LineWalk(uintptr_t lines, uint32_t lineCount) {
		Result result;
		std::fill(std::begin(result.first), std::end(result.first), none);
		bool found[6] = {};
		uint32_t glyphIndex = 0;
		for (uint32_t i = 0; i < lineCount; i++) {
			const uintptr_t line = lines + i * lineSize;
			const uint32_t glyphs = (At<uintptr_t>(line, lineGlyphs + 4) - At<uintptr_t>(line, lineGlyphs)) / 4;
			if (glyphs == 0)
				continue;
			const float value = static_cast<float>(glyphIndex);
			const short state = At<short>(line, lineState);
			const auto take = [&](int field) {
				if (!found[field]) {
					found[field] = true;
					result.first[field] = value;
				}
			};
			if (state == 0) {
				if (found[4])
					take(5);
			}
			else {
				take(4);
				if (state >= 1 && state <= 4)
					take(state - 1);
			}
			glyphIndex += glyphs;
		}
		result.glyphs = glyphIndex;
		return result;
	}

	void __declspec(naked) glyphWalkOriginal() {
		static const double sentinel = 1000000.0; // 0x01224570
		__asm {
			fld qword ptr [sentinel]
			push offset Offsets::ptr_lyricsGlyphWalkJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) glyphWalkDone() {
		__asm {
			push offset Offsets::ptr_lyricsGlyphWalkDone
			jmp MemUtil::JumpToVersioned
		}
	}

	uintptr_t __stdcall OnGlyphWalk(uintptr_t ebp, PushadRegisters* registers) {
		if (disabled)
			return reinterpret_cast<uintptr_t>(glyphWalkOriginal);

		const uintptr_t render = registers->esi;
		const uintptr_t lines = At<uintptr_t>(render, lyricsLines);
		const uint32_t lineCount = (At<uintptr_t>(render, lyricsLines + 4) - lines) / lineSize;

		// The line walk relies on the fields starting at "none" and on no glyph index reaching it.
		for (int field = firstPast; field <= firstHiddenAfter; field += 4) {
			if (At<float>(render, field) != none)
				return reinterpret_cast<uintptr_t>(glyphWalkOriginal);
		}
		uint64_t totalGlyphs = 0;
		for (uint32_t i = 0; i < lineCount; i++) {
			const uintptr_t line = lines + i * lineSize;
			const uintptr_t bytes = At<uintptr_t>(line, lineGlyphs + 4) - At<uintptr_t>(line, lineGlyphs);
			if (bytes % 4 != 0)
				return reinterpret_cast<uintptr_t>(glyphWalkOriginal);
			totalGlyphs += bytes / 4;
		}
		if (totalGlyphs >= 1000000)
			return reinterpret_cast<uintptr_t>(glyphWalkOriginal);

		const Result result = LineWalk(lines, lineCount);
		if (++calls % checkEvery == 0 && !(FullWalk(lines, lineCount) == result)) {
			disabled = true;
			LOG_ERROR("(LYRICS) Our lyrics glyph lookup didn't match the game's. Turned it off for this session." << std::endl);
			return reinterpret_cast<uintptr_t>(glyphWalkOriginal);
		}

		for (int i = 0; i < 6; i++)
			At<float>(render, firstPast + i * 4) = result.first[i];

		// What the game's walk leaves behind
		At<uint32_t>(ebp, frameGlyphCounter) = result.glyphs;
		At<uint32_t>(ebp, frameLineOffset) = lineCount * lineSize;
		registers->ebx = lineCount;
		registers->edi = 0;
		return reinterpret_cast<uintptr_t>(glyphWalkDone);
	}

	void __declspec(naked) glyphWalkHook() {
		__asm {
			push 0
			pushfd
			pushad
			mov eax, esp
			push eax
			push ebp
			call OnGlyphWalk
			mov [esp + 36], eax
			popad
			popfd
			ret
		}
	}

	void Install() {
		MemUtil::PlaceHook(Offsets::ptr_lyricsGlyphWalk, glyphWalkHook, 6);
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(Offsets::ptr_lyricsGlyphWalk.Get()), 6);
		LOG_INFO("(LYRICS) Installed per-frame lyrics speedup" << std::endl);
	}
}
