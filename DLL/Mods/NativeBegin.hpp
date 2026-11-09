#pragma once

// Begin native sign-in once per launch when the title is ready, then save and skip the warning.
namespace NativeBegin {
	// Install the title Tick hook at DLL attach; unsupported builds remain unchanged.
	void Install();
}
