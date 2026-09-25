#pragma once

namespace BugPrevention {
	void PreventOculusCrash();
	void PreventStuckTone();
	void PreventPnPCrash();
	void AllowComplexPasswords();
	void BypassSaveFilePlatformIdCheck();
	void PreventAdvancedDisplayCrash();
	void PreventPortAudioInDeviceCrash();
	void PreventExtraAudioDevicesCrash();
	void PreventControllerAxisOverflow();
	void PreventInvalidInputTreeRootCrash();
	void FixCalibrationSampleCount();
	void FixModifyingFunctions();
}