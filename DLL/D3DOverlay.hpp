#pragma once

#include <d3d9.h>

namespace GameOverlay {
	void RenderOverlay(IDirect3DDevice9* device);
	void OnLostDevice();
	void OnResetDevice();
}
