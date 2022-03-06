#include "BugPrevention.hpp"

namespace BugPrevention {

	/// <summary>
	/// When Rocksmith opens with a Oculus / Meta headset connected to the user computer, it can cause a crash.
	/// This is due to Rocksmith saying it owns memory that it doesn't have access to.
	/// In this fix, we jump over the interior of the for-loop (marked as a while with a break case) that writes to invalid memory.
	/// </summary>
	void PreventOculusCrash() {
		_LOG_INIT;

		MemUtil::PatchAdr((LPVOID)Offsets::ptr_OculusCrashJmp, "\xE9\x19\x02\x00\x00\x90", 6);
		_LOG("(BUG PREVENTION) Prevented Oculus Crash" << std::endl);
	}

	/// <summary>
	/// When the user enters a song with a buggy tone, every tone after it will not work.
	/// This mod prevents that by changing a conditional jump to a jump that always happens.
	/// So when the user encounters a broken tone, all they need to do is change the tone and the tones should start working again.
	/// </summary>
	void PreventStuckTone() {
		_LOG_INIT;

		MemUtil::PatchAdr((LPVOID)Offsets::ptr_StuckToneJmp, "\xEB", 1);
		_LOG("(BUG PREVENTION) Prevented Tone Bug" << std::endl);
	}

	/// <summary>
	/// When a user has a faulty PnP (Plug-n-Play) device connected Rocksmith can crash.
	/// It crashes with a memory access violation. The following code skips over the while loop that may eventually crash.
	/// </summary>
	void PreventPnPCrash() {
		_LOG_INIT;

		MemUtil::PatchAdr((LPVOID)Offsets::ptr_PnpJmp_1, "\xE9\x19\x02\x00\x00\x90", 6);
		MemUtil::PatchAdr((LPVOID)Offsets::ptr_PnpJmp_2, "\x90\x90\x90\x90\x90\x90", 6);
		_LOG("(BUG PREVENTION) Prevented PnP Crash" << std::endl);
	}


	/// <summary>
	/// Ubisoft lets you put almost any character in your Uplay password.
	/// However, Rocksmith does not allow some characters.
	/// This means that the user will have to change their password if they are using an invalid character, and they want to use leaderboards.
	/// Some of those invalid characters are as follows: " \ / and =
	/// This mod prevents the checks for those characters to allow the user to have more complex passwords.
	/// </summary>
	void AllowComplexPasswords() {
		_LOG_INIT;

		MemUtil::PatchAdr((LPVOID)Offsets::ptr_Password_LimitCharacters, "\x90\x90", 2);
		MemUtil::PatchAdr((LPVOID)Offsets::ptr_Password_LimitCharacters_Clipboard, "\x90\x90", 2);

		_LOG("(BUG PREVENTION) Allowed Complex Uplay Passwords" << std::endl);
	}
}