#include "../stdafx.h"
#include "BugPrevention.hpp"
#include "../MemUtil.hpp"

namespace BugPrevention {

	/// <summary>
	/// When Rocksmith opens with a Oculus / Meta headset connected to the user computer, it can cause a crash.
	/// This is due to Rocksmith saying it owns memory that it doesn't have access to.
	/// In this fix, we jump over the interior of the for-loop (marked as a while with a break case) that writes to invalid memory.
	/// </summary>
	void PreventOculusCrash() {
		MemUtil::PatchAdr(Offsets::ptr_OculusCrashJmp, "\xE9\x19\x02\x00\x00\x90", 6);

		LOG_INFO("(BUG PREVENTION) Prevented Oculus Crash" << std::endl);
	}

	/// <summary>
	/// When the user enters a song with a buggy tone, every tone after it will not work.
	/// This mod prevents that by changing a conditional jump to a jump that always happens.
	/// So when the user encounters a broken tone, all they need to do is change the tone and the tones should start working again.
	/// </summary>
	void PreventStuckTone() {
		MemUtil::PatchAdr(Offsets::ptr_StuckToneJmp, "\xEB", 1);

		LOG_INFO("(BUG PREVENTION) Prevented Tone Bug" << std::endl);
	}

	/// <summary>
	/// When a user has a faulty PnP (Plug-n-Play) device connected Rocksmith can crash.
	/// It crashes with a memory access violation. The following code skips over the while loop that may eventually crash.
	/// </summary>
	void PreventPnPCrash() {
		MemUtil::PatchAdr(Offsets::ptr_PnpJmp_1, "\xE9\x19\x02\x00\x00\x90", 6);
		MemUtil::PatchAdr(Offsets::ptr_PnpJmp_2, "\x90\x90\x90\x90\x90\x90", 6);

		LOG_INFO("(BUG PREVENTION) Prevented PnP Crash" << std::endl);
	}

	/// <summary>
	/// Ubisoft lets you put almost any character in your Uplay password.
	/// However, Rocksmith does not allow some characters.
	/// This means that the user will have to change their password if they are using an invalid character, and they want to use leaderboards.
	/// Some of those invalid characters are as follows: " \ / and =
	/// This mod prevents the checks for those characters to allow the user to have more complex passwords.
	/// </summary>
	void AllowComplexPasswords() {
		MemUtil::PatchAdr(Offsets::ptr_Password_LimitCharacters, "\x90\x90", 2);
		MemUtil::PatchAdr(Offsets::ptr_Password_LimitCharacters_Clipboard, "\x90\x90", 2);

		LOG_INFO("(BUG PREVENTION) Allowed Complex Uplay Passwords" << std::endl);
	}

	void __declspec(naked) advancedDisplayCrashHook()
	{
		__asm {

			cmp ECX, 0 // ECX == NULL?

			je prevAdvancedDisplayCrash // If ECX == NULL, then we need to jump to prevAdvancedDisplayCrash

			mov DL, BYTE PTR DS : [ECX + 0x4]	// The code we are overwriting to place this hook
			push EDI						// The code we are overwriting to place this hook
			MOV EDI, DWORD PTR DS : [ESI + 0xC] // The code we are overwriting to place this hook

			pushad

			lea ecx, Offsets::ptr_AdvancedDisplayCrashJmpBck
			call VersioningStruct<uintptr_t>::GetValue
			mov Offsets::runtimeVersionStructValue, eax

			popad

			jmp Offsets::runtimeVersionStructValue

			prevAdvancedDisplayCrash :
			ret							// ECX is NULL, so we need to leave this function or we will crash.
		}
	}

	/// <summary>
	/// When a user enters their Advanced Display settings, there is a tendency for Rocksmith 2014 to crash.
	/// This mod tries to prevent that by exiting the function if ECX (the memory address it is reading from) is NULL.
	/// </summary>
	void PreventAdvancedDisplayCrash() {
		MemUtil::PlaceHook(Offsets::ptr_AdvancedDisplayCrash, advancedDisplayCrashHook, 7);

		FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::ptr_AdvancedDisplayCrash.Get(), 7);

		LOG_INFO("(BUG PREVENTION) Prevented Advanced Display Crash" << std::endl);
	}


	/// <summary>
	/// In extremely rare cases, the user may have an audio in device have a driver issue.
	/// This causes Rocksmith to crash when it reads all of their audio input devices.
	/// This just patches out those checks, so it won't crash when EBX is a nullptr.
	/// </summary>
	void PreventPortAudioInDeviceCrash() {
		// Overwrite some code that doesn't do null checks with NOP.
		// Be very careful in this code. If you overwrite the next instruction, then you end up breaking audio input.
		// NB: JZ has been replaced by JL, now it's 10 bytes in total
		MemUtil::PatchAdr(Offsets::ptr_PortAudioInCrash, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10);

		LOG_INFO("(BUG PREVENTION) Prevented Port Audio In Device Crash" << std::endl);
	}

	/// <summary>
	/// Prevention for crash caused by certain audio devices (like Voicemeter virtual cables).
	/// </summary>
	void PreventExtraAudioDevicesCrash() {
		MemUtil::PatchAdr(Offsets::ptr_AdditionalAudioDevicesCrash, "\x90\x90\x90\x90\x90\x31\xC9\x31\xDB", 9);

		LOG_INFO("(BUG PREVENTION) Prevented Additional Audio Devices Crash" << std::endl);
	}

	/// <summary>
	/// The sign-in notification can look up "Cancel" in an input-action tree whose root
	/// is a negative, invalid pointer. GetRoot checks only for zero before reading that node.
	/// Treat negative roots like empty trees using the function's existing return path.
	/// Original report: https://discord.com/channels/238233332511539200/305406306821472257/1552076343632728065
	/// </summary>
	void PreventInvalidInputTreeRootCrash() {
		const BYTE jumpIfLessOrEqual = 0x8E; // TEST EBX,EBX; JLE follows the original empty-tree path.
		if (!MemUtil::PatchAdr(Offsets::ptr_InvalidInputTreeRootBranch, &jumpIfLessOrEqual, sizeof(jumpIfLessOrEqual))) {
			LOG_ERROR("(BUG PREVENTION) Failed Invalid Input Tree Root fix" << std::endl);
			return;
		}

		LOG_INFO("(BUG PREVENTION) Prevented Invalid Input Tree Root Crash" << std::endl);
	}

	/// <summary>
	/// Clamps the sample count to the buffer's real capacity before it is stored.
	/// </summary>
	void __declspec(naked) calibrationSampleCountClampHook() {
		__asm {
			mov edx, dword ptr [esp + 0x10]		// The code we are overwriting to place this hook
			cmp edx, 100						// Capacity of the ring buffer, per player
			jbe keepCalibrationSampleCount
			mov edx, 100
		keepCalibrationSampleCount:
			mov dword ptr [ebx + 0x788], edx	// The code we are overwriting to place this hook

			pushad

			lea ecx, Offsets::ptr_calibrationSampleCountClampJmpBck
			call VersioningStruct<uintptr_t>::GetValue
			mov Offsets::runtimeVersionStructValue, eax

			popad

			jmp Offsets::runtimeVersionStructValue
		}
	}

	/// <summary>
	/// The input calibration screen sizes its volume averaging buffer to the current framerate (1.0 / delta time),
	/// but the buffer is a fixed 100 floats per player. Above ~100 FPS the sampler writes past the end of it, and
	/// the mean is then taken over more floats than the array holds, reading neighbouring members as if they were
	/// samples. The mean never settles in the acceptance window, so the meter never fills and calibration cannot
	/// be completed - which is why players on high refresh rate displays have to cap their framerate first.
	/// Clamping that count to the real capacity fixes both the writes and the reads. Nothing at or below 100 FPS
	/// changes; above it the averaging window just covers less time.
	/// </summary>
	void FixCalibrationSampleCount() {
		MemUtil::PlaceHook(Offsets::ptr_calibrationSampleCountClamp, calibrationSampleCountClampHook, 10);

		FlushInstructionCache(GetCurrentProcess(), (void*)Offsets::ptr_calibrationSampleCountClamp.Get(), 10);

		LOG_INFO("(BUG PREVENTION) Fixed Calibration At High Framerates" << std::endl);
	}

	/// <summary>
	/// Fixes crash when modifying functions in Rocksmith.
	/// </summary>
	void FixModifyingFunctions() {
		uintptr_t forceSuccessLSBOffset = Offsets::ptr_ModdedPtrCrashFix.Get() + 0x3; // LSB of the MOV is what we are replacing
		byte forceFailedLSB = MemUtil::ReadValue<byte>(forceSuccessLSBOffset + 0x14, true); // We are replacing it with ForceFailed LSB, which is 0x14 away
		MemUtil::PatchAdr(forceSuccessLSBOffset, (LPVOID)&forceFailedLSB, 1, true);
	}
}