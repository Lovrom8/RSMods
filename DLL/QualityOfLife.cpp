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
}