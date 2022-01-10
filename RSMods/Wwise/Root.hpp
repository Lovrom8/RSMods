#pragma once

#include "Types.hpp"
#include "Exports.hpp"

namespace Wwise {
	bool IsRestoreSinkRequested(void);
	bool IsUsingDummySink(void);
}

typedef bool(__cdecl* tIsRestoreSinkRequested)(void);
typedef bool(__cdecl* tIsUsingDummySink)(void);