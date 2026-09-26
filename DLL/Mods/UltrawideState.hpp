#pragma once

#include <atomic>
#include <unordered_map>
#include <utility>

// Render-side state of the ultrawide correction, shared by UltrawideMod and its draw-path helpers.
namespace UltrawideState {
	inline std::atomic<bool> active = false;   // The setting is on and the backbuffer is wider than 16:9: corrections apply.

	// True while a Guitarcade minigame is being played or paused. Evaluated once
	// per frame in UltrawideMod's frame callback because the underlying GameState query is
	// a string comparison and the draw path reads this per draw.
	inline std::atomic<bool> inGuitarcade = false;
	inline std::atomic<bool> inVideoPlayer = false; // The video player composites at its own aspect.
	inline std::atomic<bool> settingOn = false; // Published by UltrawideMod (mod thread) from the Ultrawide setting. The source of `active`.
	inline float clipXScale = 1.0f; // (16/9) / the display aspect: how far the interface has to be squeezed back in.
	inline std::unordered_map<void*, std::pair<UINT, UINT>> renderTargetTextures; // Render thread only: every texture that has been a render target, with its size. Scene-aspect ones mark post-processing draws; off-aspect ones (text strips) mark 16:9 UI.
	inline bool backBufferValid = false;
	inline UINT backBufferWidth = 0;
	inline UINT backBufferHeight = 0;
	inline bool renderTargetIsScene = false;
	inline UINT renderTargetWidth = 0;
	inline UINT renderTargetHeight = 0;
}
