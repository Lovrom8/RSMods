#include "CrashPrevention.hpp"

namespace CrashPrevention {

	/// <summary>
	/// When Rocksmith opens with a Oculus / Meta headset connected to the user computer, it can cause a crash.
	/// This is due to Rocksmith saying it owns memory that it doesn't have access to.
	/// In this fix, we jump over the interior of the for-loop (marked as a while with a break case) that writes to invalid memory.
	/// </summary>
	void PreventOculusCrash() {
		MemUtil::PatchAdr((LPVOID)Offsets::ptr_OculusCrashJmp, "\xE9\x19\x02\x00\x00\x90", 6);
		std::cout << "(CRASH PREVENTION) Prevented Oculus Crash" << std::endl;
	}
}