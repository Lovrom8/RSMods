#pragma once

// Host glue for Framework::InputCapture: patches dinput8's shared device vtables so the game's
// mouse reads go through the framework's filters.
namespace DirectInputCapture
{
	// Once, from Menu::Init on the render thread. Not DllMain: creating a DirectInput object under the loader lock can deadlock.
	bool Install();
	void Shutdown();
}
