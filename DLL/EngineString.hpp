#pragma once

#include "MemUtil.hpp"
#include <string>

/// <summary>
/// The game's string layout. Strings of 15 chars or less live in the 16 byte inline buffer,
/// and endOfStorage then points at finish (the end of that buffer).
/// </summary>
struct EngineString {
	char* data;
	char inlineRest[12];
	char* finish;
	char* endOfStorage;
};
static_assert(sizeof(EngineString) == 0x18, "EngineString must match the game's layout");

namespace EngineStrings {
	/// <summary>
	/// Where the text of a live game string starts.
	/// </summary>
	inline char* Data(EngineString* str) {
		return str->endOfStorage == reinterpret_cast<char*>(&str->finish) ? reinterpret_cast<char*>(str) : str->data;
	}

	/// <summary>
	/// Copy a game string we don't own (it may be freed or garbage) without crashing. Returns fallback if it can't be read.
	/// </summary>
	inline std::string SafeRead(uintptr_t address, size_t maxLength = 256, const char* fallback = "?") {
		EngineString str{};
		if (!MemUtil::TryRead(address, str))
			return fallback;

		const uintptr_t start = str.endOfStorage == reinterpret_cast<char*>(address + offsetof(EngineString, finish))
			? address : reinterpret_cast<uintptr_t>(str.data);
		const uintptr_t end = reinterpret_cast<uintptr_t>(str.finish);
		if (!start || end < start || end - start > maxLength)
			return fallback;

		std::string text(end - start, '\0');
		for (size_t i = 0; i < text.size(); i++) {
			if (!MemUtil::TryRead(start + i, text[i]))
				return fallback;
		}
		return text;
	}
}
