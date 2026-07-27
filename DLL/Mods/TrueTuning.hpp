#pragma once

namespace TrueTuning
{
	void DisableTrueTuning();
	void EnableTrueTuning();

	// Transpose what note detection expects, in semitones from the song's own tuning.
	void SetReferenceSemitones(int semitones);
}
