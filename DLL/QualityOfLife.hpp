#pragma once

#include "Framework/InstructionPatch.hpp"
#include "MemUtil.hpp"

namespace QualityOfLife {
	MemUtil::PatchResult PatchTwoRTC(bool enable);
	HANDLE GetMessageBoxProcess();
	void StopTwoRSInstances();
	void LowerNoteDetectionFloor();
	void RetryFastLoadUponSoftLock();
}
