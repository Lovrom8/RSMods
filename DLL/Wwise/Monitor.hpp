#pragma once

#include "Types.hpp"
#include "Exports.hpp"

namespace Wwise::Monitor {
	AKRESULT PostCode(ErrorCode in_eError, ErrorLevel in_eErrorLevel);
}

typedef AKRESULT(__cdecl* tMonitor_PostCode)(ErrorCode in_eError, ErrorLevel in_eErrorLevel);