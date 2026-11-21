#pragma once

#include "LnpSites.hpp"
#include "MemUtil.hpp"
#include <initializer_list>

namespace LnpSites {
	inline uintptr_t Base() { return reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr)); }
	inline uintptr_t At(const Span& span) { return Base() + span.rva; }
	inline uintptr_t At(uint32_t rva) { return Base() + rva; }

	// True when the live bytes at the span equal its text, absolute operands relocated to this load address.
	inline bool Matches(const Span& span) {
		std::vector<unsigned char> expected;
		return Expand(span.bytes, static_cast<uint32_t>(Base()), expected) && MemUtil::MatchesBytes(At(span), expected);
	}

	inline bool MatchesAll(std::initializer_list<Span> spans) {
		for (const Span& span : spans)
			if (!Matches(span))
				return false;
		return true;
	}
}
