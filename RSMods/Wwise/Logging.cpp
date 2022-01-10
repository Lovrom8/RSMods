#include "Logging.hpp"

#ifdef _WWISE_LOGS
namespace Wwise::Logging {

	void __stdcall log_PostEvent_Name(char* eventName, int gameObject) {
		std::cout << "(Wwise) PostEvent: " << eventName << " on game object 0x" << std::hex << gameObject << std::endl;
	}

	void __declspec(naked) hook_log_PostEvent() {
		__asm {
			push ESI	// Save current state of ESI to the stack.
			push EAX	// Save current state of EAX to the stack.
			push ESP	// Save current state of ESP to the stack.

			push[EBP + 0xC]			// Wwise Object
			push[EBP + 0x8]			// Wwise Event name.
			call log_PostEvent_Name	// Log the event name

			pop ESP	// Get old ESP
			pop EAX	// Get old EAX
			pop ESI // Get old ESI

			mov ESI, EAX  // Assembly we removed to place this hook
			add ESP, 0x20 // Assembly we removed to place this hook
			jmp Offsets::ptr_Wwise_Log_PostEventHookJmpBck  // Jump back to the original code.
		}
	}

	void Setup_log_PostEvent() {
		MemUtil::PlaceHook((void*)Offsets::ptr_Wwise_Log_PostEventHook, hook_log_PostEvent, 5);
	}


	void __stdcall log_SetRTPCValue(char* rtpcName, float rtpcValue, int gameObject) {
		if (!strcmp(rtpcName, "Player_Success") ||
			!strcmp(rtpcName, "Portal_Size") ||
			!strcmp(rtpcName, "LoftAmb_CamPosition") ||
			!strcmp(rtpcName, "AmbRadio_OnOff_Return") ||
			!strcmp(rtpcName, "Average_Song_Difficulty") ||
			!strcmp(rtpcName, "Lowest_Phrase_Difficulty_Level") ||
			!strcmp(rtpcName, "PortalClose_TailLP_Return") ||
			!strcmp(rtpcName, "PortalClose_TailRamping_Return") ||
			!strcmp(rtpcName, "P1_Instrument_Volume") ||
			!strcmp(rtpcName, "GateTime") ||
			!strcmp(rtpcName, "P1_Streak_Hit_Count") ||
			!strcmp(rtpcName, "P1_Streak_Miss") ||
			!strcmp(rtpcName, "P1_Streak_Note_Count") ||
			!strcmp(rtpcName, "P1_Streak_Chord_Count") ||
			!strcmp(rtpcName, "P1_Streak_Phrase_Count") ||
			!strcmp(rtpcName, "P2_Instrument_Volume") ||
			!strcmp(rtpcName, "MusicRamping")
			)
			return; // To prevent spamming of the log. If you need to look at these, remove the if-statement.

		std::cout << "(Wwise) SetRTPCValue: " << rtpcName << " to " << rtpcValue << " on game object 0x" << std::hex << gameObject << std::endl;
	}

	void __declspec(naked) hook_log_SetRTPCValue() {
		__asm {
			push ESP // Save current state of ESP to the stack.
			push EAX // Save current state of EAX to the stack.

			push[EBP + 0x10] // RTPC Object
			push[EBP + 0xC]	// RTPC Value
			push[EBP + 0x8]	// RTPC Name
			call log_SetRTPCValue

			pop EAX // Get old EAX
			pop ESP // Get old ESP

			add ESP, 0x4 // Assembly we removed to place this hook
			test EAX, EAX // Assembly we removed to place this hook
			jmp Offsets::ptr_Wwise_Log_SetRTPCValueHookJmpBck // Jump back to the original code.
		}
	}

	void Setup_log_SetRTPCValue() {
		MemUtil::PlaceHook((void*)Offsets::ptr_Wwise_Log_SetRTPCValueHook, hook_log_SetRTPCValue, 5);
	}

	void __stdcall log_SeekOnEvent(char* eventName, AkGameObjectID gameObject, AkTimeMs position, bool seekToNearestMarker) {
		std::cout << "(Wwise) SeekOnEvent: " << eventName << " on object 0x" << std::hex << gameObject << " at time " << (float)(position / 1000) << " seconds. Seek to nearest marker: " << std::boolalpha << seekToNearestMarker << std::endl;
	}

	void __declspec(naked) hook_log_SeekOnEvent() {
		__asm {
			push ESP // Save current state of ESP to the stack
			push EAX // Save current state of EAX to the stack

			push[EBP + 0x14] // Seek To Nearest Marker
			push[EBP + 0x10] // Position (in ms)
			push[EBP + 0xC]	// Game Object
			push[EBP + 0x8]	// Event Name
			call log_SeekOnEvent

			pop EAX // Get old EAX
			pop ESP // Get old ESP

			mov ECX, dword ptr[EBP + 0x14] // Assembly we removed to place this hook
			mov EDX, dword ptr[EBP + 0x10] // Assembly we removed to place this hook
			jmp Offsets::ptr_Wwise_Log_SeekOnEventHookJmpBck // Jump back to the original code
		}
	}

	void Setup_log_SeekOnEvent() {
		MemUtil::PlaceHook((void*)Offsets::ptr_Wwise_Log_SeekOnEventHook, hook_log_SeekOnEvent, 6);
	}
}
#endif