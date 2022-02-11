#include "LaunchOnExternalMonitor.hpp"

namespace LaunchOnExternalMonitor {

	/// <summary>
	/// Move Rocksmith to a seperate monitor on boot.
	/// </summary>
	/// <param name="startX"> - top LEFT of the screen</param>
	/// <param name="startY"> - TOP left of the screen</param>
	void SendRocksmithToScreen(int startX, int startY) {

		// Get HWND of Rocksmith.
		HWND hWnd = FindWindow(NULL, L"Rocksmith 2014");

		while (!hWnd) {
			Sleep(500);
			HWND hWnd = FindWindow(NULL, L"Rocksmith 2014");
		}
		
		// Set the windows top left corner to StartX and StartY.
		RECT windowSize;
		if (hWnd) {
			if (GetWindowRect(hWnd, &windowSize)) {
				SetWindowPos(hWnd, HWND_TOPMOST, startX, startY, windowSize.right - windowSize.left, windowSize.bottom - windowSize.top, SWP_SHOWWINDOW);
			}
		}
	}
}
