#pragma once

#include <d3d9.h>
#include <windows.h>

namespace Menu
{
	inline bool ImGuiInit = false;
	inline bool menuEnabled = false; // Do we show the user the ImGUI settings menu?

	void Init(IDirect3DDevice9* pDevice, LONG_PTR WndProc);
	void RenderImGuiMenu();
	bool IsOverlayCall();
}
