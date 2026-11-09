#pragma once

#include <algorithm>

namespace EnumerationDrain {
	inline double FrameBudgetMs(bool ready, unsigned int fps, double frameMs, double enumerationMs) {
		if (!ready) return 2.0;
		const double targetMs = 1000.0 / fps;
		const double gameMs = (std::max)(0.0, frameMs - enumerationMs);
		// Keep scans progressing after menu readiness even when frame headroom is exhausted.
		return gameMs > targetMs ? 8.0 : (std::max)(2.0, targetMs - gameMs);
	}
}
