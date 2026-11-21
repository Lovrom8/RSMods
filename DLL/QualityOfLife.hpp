#pragma once

#include "Framework/InstructionPatch.hpp"

namespace QualityOfLife {
	Framework::InstructionPatch::Result PatchTwoRTC(bool enable);
	HANDLE GetMessageBoxProcess();
	void StopTwoRSInstances();
	void LowerNoteDetectionFloor();
	void RetryFastLoadUponSoftLock();
}
