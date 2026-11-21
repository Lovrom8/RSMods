#pragma once

#include "Framework/InstructionPatch.hpp"
#include "MemUtil.hpp"

namespace QualityOfLife {
	HANDLE GetMessageBoxProcess();
	void StopTwoRSInstances();
	void LowerNoteDetectionFloor();
	void RetryFastLoadUponSoftLock();
}
