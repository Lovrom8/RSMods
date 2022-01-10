#pragma once

#include "Windows.h"
#include <iostream>

#include "Types.hpp"

#include "../Offsets.hpp"
#include "../MemUtil.hpp"

#ifdef _WWISE_LOGS
namespace Wwise::Logging {
	void hook_log_PostEvent();
	void Setup_log_PostEvent();

	void hook_log_SetRTPCValue();
	void Setup_log_SetRTPCValue();

	void hook_log_SeekOnEvent();
	void Setup_log_SeekOnEvent();
}
#endif