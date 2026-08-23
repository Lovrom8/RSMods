#include "../stdafx.h"
#include "AlternativeSampleRateMod.hpp"

using Framework::ModContext;
using Framework::GamePhase;
namespace Setting = Settings::Setting;

bool AlternativeSampleRateMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::AltOutputSampleRate);
}

// TODO: determine this is early enough in the process - this value needs to be set 
// before the audio engine starts!
void AlternativeSampleRateMod::OnInitialize(ModContext& c) {
	const int sampleRate = c.Int(Setting::AlternativeOutputSampleRate);

	if (!IsEnabled(c) || sampleRate == 48000)
		return;

	LOG_WARNING("[!] Overriding Output Sample Rate to " << sampleRate << std::endl);

	AudioDevices::output_SampleRate = sampleRate;
	AudioDevices::ChangeOutputSampleRate();

	// Input follows output - RS-ASIO requires the engine's input and output rate to both match the
	// interface's single native rate, so there's no legitimate case where a user would want them to
	// differ. No separate input toggle/value is exposed.
	AudioDevices::input_SampleRate = sampleRate;
	AudioDevices::ChangeInputSampleRate();
}

// Patch the markers only during the loading phase, before the engine finishes coming up. The buffer
// checks keep it idempotent and avoid clobbering the values (5 / 2) the engine sets for itself.
void AlternativeSampleRateMod::OnTick(ModContext& c) {
	if (c.phase != GamePhase::Loading || c.Int(Setting::AlternativeOutputSampleRate) == 48000) {
		return;
	}

	if (*(int*)Offsets::ptr_sampleRateBuffer.Get() != 5 &&
		*(int*)Offsets::ptr_sampleRateBuffer.Get() != 2) {
		*(int*)Offsets::ptr_sampleRateSize.Get() = 2;
		*(int*)Offsets::ptr_sampleRateBuffer.Get() = 128;
	}
}

static Framework::ModRegistrar<AlternativeSampleRateMod> _altSampleRateReg;
