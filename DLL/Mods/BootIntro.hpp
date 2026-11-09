#pragma once

#include <string>

// Skip intro assets and hold the boot splash until the title appears or a fallback releases it.
namespace BootIntro {
	// Install at DLL attach; unsupported builds remain unchanged.
	void Remove();

	// Game thread only; releasing the splash more than once is harmless.
	void ReleaseSplash();

	// Mod-thread menu notification for the splash fallback.
	void NoteMenu(const std::string& menu);

	// Game-thread frame callback for menu and timeout fallbacks.
	void OnFrame();
}
