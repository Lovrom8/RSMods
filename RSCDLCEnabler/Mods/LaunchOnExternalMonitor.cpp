#include "LaunchOnExternalMonitor.hpp"

namespace LaunchOnExternalMonitor {
	void SendRocksmithToScreen(int startX) {
		HWND hWnd = FindWindow(NULL, L"Rocksmith 2014");

		if (!hWnd || hWnd == 0) {
			Sleep(500);
			SendRocksmithToScreen(startX);
		}
		else {
			RECT windowSize;
			if (GetWindowRect(hWnd, &windowSize)) {
				SetWindowPos(hWnd, HWND_TOPMOST, startX, 0, windowSize.right - windowSize.left, windowSize.bottom - windowSize.top, SWP_SHOWWINDOW);
			}
		}
	}
}
