#pragma once

// Keep profile completion separate from automatic title advancement.
namespace NativeBegin {
	// Required profile continuation belongs to the sign-in bundle.
	void InstallProfileCompletion();

	// Automatic Begin belongs to the optional ultrafast bundle.
	void Install();
}
