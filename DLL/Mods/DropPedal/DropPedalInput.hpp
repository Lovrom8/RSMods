#pragma once

namespace DropPedalInput
{
	void AdjustTarget(int semitoneDelta);
	void ToggleEnabled();
	void AdjustBaseTuning(int semitoneDelta);
	void PollPendingPitchPush();
}
