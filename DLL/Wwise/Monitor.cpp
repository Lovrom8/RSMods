#include "Monitor.hpp"

// Most accurate documentation is available here: https://www.audiokinetic.com/library/2015.1.9_5624/?source=SDK&id=namespace_a_k_1_1_monitor.html

namespace Wwise::Monitor {

	/// <summary>
	/// Post a monitoring message or error code. This will be displayed in the Wwise capture log.
	/// </summary>
	/// <param name="in_eError"> - Message or error code to be displayed</param>
	/// <param name="in_eErrorLevel"> - Specifies whether it should be displayed as a message or an error</param>
	/// <returns>AK_Success if successful, AK_Fail if there was a problem posting the message. In optimized mode, this function returns AK_NotCompatible.</returns>
	AKRESULT PostCode(ErrorCode in_eError, ErrorLevel in_eErrorLevel) {
		tMonitor_PostCode func = (tMonitor_PostCode)Wwise::Exports::func_Wwise_Monitor_PostCode;
		return func(in_eError, in_eErrorLevel);
	}
}