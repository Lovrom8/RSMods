#pragma once

namespace BugPrevention {
	void PreventOculusCrash();
	void PreventStuckTone();
	void PreventPnPCrash();
	void AllowComplexPasswords();
	void PreventAdvancedDisplayCrash();
	void PreventPortAudioInDeviceCrash();
	void PreventExtraAudioDevicesCrash();
	void PreventInvalidInputTreeRootCrash();
	void FixCalibrationSampleCount();
	void FixModifyingFunctions();
}