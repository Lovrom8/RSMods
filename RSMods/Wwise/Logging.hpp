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

	void hook_log_SetBusEffect();
	void Setup_log_SetBusEffect();

	void hook_log_CloneBusEffect();
	void Setup_log_CloneBusEffect();

	int RackId = 0x99bc574c;
}
#endif