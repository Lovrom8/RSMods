#pragma once

#include <string>

struct ImFont;

// ImGui bakes glyphs into an atlas at fixed (face, size) pairs, so a user-selected font face has to
// be resolved to actual font bytes and baked. This resolves the face through GDI (same matching
// ID3DXFont used) and caches the result by (face, size).
namespace ImGuiHud {
	// Baked ImFont* for (face, pixelHeight), baked on first use. nullptr if GDI has no bytes for the
	// face; caller falls back to the default font. A new bake flips the atlas-dirty flag.
	ImFont* AcquireFont(const std::string& face, int pixelHeight);

	// True once if a new face was baked since the last call: caller must rebuild the atlas and
	// invalidate the backend font texture before ImGui::NewFrame().
	bool ConsumeAtlasDirty();
}
