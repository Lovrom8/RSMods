#pragma once

namespace QualityOfLife {
	void PatchTwoRTC();
	HANDLE GetMessageBoxProcess();
	void StopTwoRSInstances();
	void LowerNoteDetectionFloor();
	void RetryFastLoadUponSoftLock();
	void FixNonExclusiveFullscreen();
}