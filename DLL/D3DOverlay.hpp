#pragma once

namespace GameOverlay {
	// Two phases around Menu::RenderImGuiMenu's ImGui frame. The atlas can't be touched between
	// NewFrame and Render, so font baking is split out from drawing.
	void PrepareImGuiHud(); // before NewFrame: snapshot HUD, bake/rebuild fonts off-frame
	void DrawImGuiHud();    // inside frame: emit text into the background draw list
}
