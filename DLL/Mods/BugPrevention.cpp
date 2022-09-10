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

	void __declspec(naked) advancedDisplayCrashHook()
	{
		__asm {

			cmp ECX, 0 // ECX == NULL?

			je prevAdvancedDisplayCrash // If ECX == NULL, then we need to jump to prevAdvancedDisplayCrash


			mov DL, BYTE PTR DS : [ECX + 0x4]	// The code we are overwriting to place this hook
			push EDI						// The code we are overwriting to place this hook
			MOV EDI, DWORD PTR DS : [ESI + 0xC] // The code we are overwriting to place this hook

			jmp Offsets::ptr_AdvancedDisplayCrashJmpBck // Jump back into the original code.

			prevAdvancedDisplayCrash :
			ret							// ECX is NULL, so we need to leave this function or we will crash.
		}
	}

	/// <summary>
	/// When a user enters their Advanced Display settings, there is a tendency for Rocksmith 2014 to crash.
	/// This mod tries to prevent that by exiting the function if ECX (the memory address it is reading from) is NULL.
	/// </summary>
	void PreventAdvancedDisplayCrash() {
		_LOG_INIT;

		MemUtil::PlaceHook((void*)Offsets::ptr_AdvancedDisplayCrash, advancedDisplayCrashHook, 7);

		FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::ptr_AdvancedDisplayCrash, 7);

		_LOG("(BUG PREVENTION) Prevented Advanced Display Crash" << std::endl);
	}


	/// <summary>
	/// In extremely rare cases, the user may have an audio in device have a driver issue.
	/// This causes Rocksmith to crash when it reads all of their audio input devices.
	/// This just patches out those checks, so it won't crash when EBX is a nullptr.
	/// </summary>
	void PreventPortAudioInDeviceCrash() {
		_LOG_INIT;

		// Overwrite some code that doesn't do null checks with NOP.
		// Be very careful in this code. If you overwrite the next instruction, then you end up breaking audio input.
		// NB: JZ has been replaced by JL, now it's 10 bytes in total
		MemUtil::PatchAdr((LPVOID)Offsets::ptr_PortAudioInCrash, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10);

		_LOG("(BUG PREVENTION) Prevented Port Audio In Device Crash" << std::endl);
	}
}