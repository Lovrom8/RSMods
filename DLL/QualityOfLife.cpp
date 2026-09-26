#include "stdafx.h"
#include "QualityOfLife.hpp"

namespace QualityOfLife {
	/// <summary>
	/// Patches Two RTC cables error message.
	/// </summary>
	void PatchTwoRTC()
	{
		char patch[25];
		std::fill_n(patch, 25, static_cast<char>(0x90));
		MemUtil::PatchAdr(Offsets::ptr_twoRTCBypass, patch, sizeof(patch));
	}

	/// <summary>
	/// RS spawns two processes, one of which complains about Steam not being active. 
	/// We find that one and close it.
	/// </summary>
	HANDLE GetMessageBoxProcess()
	{
		HWND hWnd = FindWindowW(L"#32770", L"Error."); // Dialog box class
		if (!hWnd) {
			return nullptr; 
		}

		DWORD dwProcessId = 0;
		GetWindowThreadProcessId(hWnd, &dwProcessId);
		if (dwProcessId == 0) {
			return nullptr;
		}
		
		return OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, dwProcessId);
	}

	/// <summary>
	/// Stops the game from starting two instances
	/// </summary>
	void StopTwoRSInstances() {
		HANDLE handle = GetMessageBoxProcess();

		if (!handle) {
			return;
		}

		std::vector<wchar_t> processNameBuffer(MAX_PATH);
		HMODULE hMod;

		if (DWORD cNeeded; EnumProcessModulesEx(handle, &hMod, sizeof(hMod), &cNeeded, LIST_MODULES_32BIT | LIST_MODULES_64BIT))
		{
			if (GetModuleBaseNameW(handle, hMod, processNameBuffer.data(), processNameBuffer.size()))
			{
				std::wstring processName(processNameBuffer.data());

				if (processName == L"Rocksmith2014.exe") {
					std::string narrowName;
					narrowName.resize(processName.size());
					std::transform(processName.begin(), processName.end(), narrowName.begin(),
						[](wchar_t ch) { return static_cast<char>(ch); });
					LOG_INFO("Found parasitic process '" << narrowName << "'. Terminating." << std::endl);

					if (!TerminateProcess(handle, 0)) {
						LOG_ERROR("Failed to terminate process. Error: " << GetLastError() << std::endl);
					}
				}
				else {
					std::string narrowName;
					narrowName.resize(processName.size());
					std::transform(processName.begin(), processName.end(), narrowName.begin(),
						[](wchar_t ch) { return static_cast<char>(ch); });
					LOG_INFO("Found dialog box, but process '" << narrowName << "' is not the target. Not terminating." << std::endl);
				}
			}
		}

		CloseHandle(handle);
	}

	/// <summary>
	/// Rocksmith ignores any note below MIDI 24 (C1, 32.7 Hz), so a bass low E tuned more than 4 semitones down at A440
	/// can never be detected. This lowers that floor to 18, about as low as the game can pick up a pitch at 48 kHz (~23.4 Hz).
	/// The game reads the floor when note detection starts, so this has to be patched early.
	/// </summary>
	void LowerNoteDetectionFloor() {
		MemUtil::PatchAdr(Offsets::ptr_noteDetectionFloor, "\x12", 1);

		LOG_INFO("Lowered note detection floor" << std::endl);
	}
}