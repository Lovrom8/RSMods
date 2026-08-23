#pragma once

#include "../Mods/AudioDevices.hpp"
#include "../Framework/Framework.hpp"

// Forces the audio engine's sample-rate buffer markers during boot when the user runs an alternative
// output sample rate, and patches the analogous input/capture constants to match. Loading-phase only
// (the engine only reads them while coming up) and apply-only.
class AlternativeSampleRateMod : public Framework::IMod {
public:
	MOD_ID(AlternativeSampleRateMod)
	
	bool IsEnabled(const Framework::ModContext& c) const override;
	void OnInitialize(Framework::ModContext& c) override;
	void OnTick(Framework::ModContext& c) override;
};
