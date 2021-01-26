#include "LaunchOnExternalMonitor.hpp"

namespace LaunchOnExternalMonitor {

	/// <summary>
	/// Move Rocksmith to a seperate monitor on boot.
	/// </summary>
	/// <param name="startX"> - top LEFT of the screen</param>
	/// <param name="startY"> - TOP left of the screen</param>
	void SendRocksmithToScreen(int startX, int startY) {
		HWND hWnd = FindWindow(NULL, L"Rocksmith 2014");

		if (!hWnd || hWnd == 0) {
			Sleep(500);
			SendRocksmithToScreen(startX, startY);
		}
		else {
			RECT windowSize;
			if (GetWindowRect(hWnd, &windowSize)) {
				SetWindowPos(hWnd, HWND_TOPMOST, startX, startY, windowSize.right - windowSize.left, windowSize.bottom - windowSize.top, SWP_SHOWWINDOW);
			}
		}
	}
}
