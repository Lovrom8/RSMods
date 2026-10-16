#pragma once

#include <atomic>
#include <d3d9.h>
#include <windows.h>

namespace Menu
{
	inline bool ImGuiInit = false;
	inline std::atomic<bool> menuEnabled = false; // Do we show the user the ImGUI settings menu? Toggled on MainThread, read by the window and render threads.

	void Init(IDirect3DDevice9* pDevice, LONG_PTR WndProc);
	void RenderImGuiMenu();
	bool IsOverlayCall();
}
