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
	void FixIndexKeyTableOverflow();
	void FixLyricsGlyphCountOverflow();
	void FixPhraseDifficultyCacheOverflow();
	void FixCalibrationSampleCount();
	void FixModifyingFunctions();
}