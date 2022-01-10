#include "Monitor.hpp"

namespace Wwise::Monitor {

	AKRESULT PostCode(ErrorCode in_eError, ErrorLevel in_eErrorLevel) {
		tMonitor_PostCode func = (tMonitor_PostCode)Wwise::Exports::func_Wwise_Monitor_PostCode;
		return func(in_eError, in_eErrorLevel);
	}
}