#include "../stdafx.h"
#include "CalibrationMod.hpp"
#include "../Wwise/SoundEngine.hpp"

using Framework::ModContext;

void CalibrationMod::OnInitialize(ModContext& c) {
	c.Menu().Register("calibration", "Calibration", 15, [this] { DrawMenu(); });
}

void CalibrationMod::DrawMenu() {
	static float NewNoiseFloor = -59.3134f;
	static float NewToneBalance = -19.4793f;
	static float NewInputVolume = -3.22313f;
	static float NewInputVolumeReturn = -3.22313f;

	ImGui::SliderFloat("Noise Floor", &NewNoiseFloor, -100.f, 10.f);
	ImGui::SliderFloat("Tone Balance", &NewToneBalance, -100.f, 10.f);
	ImGui::SliderFloat("Input Volume", &NewInputVolume, -100.f, 10.f);
	ImGui::SliderFloat("Input Volume Return", &NewInputVolumeReturn, -100.f, 10.f);

	if (ImGui::Button("Calibrate")) {
		// Noise Floor
		Wwise::SoundEngine::SetRTPCValue("P1_NoiseFloor", NewNoiseFloor, 0x1234, 0, AkCurveInterpolation_Linear);
		Wwise::SoundEngine::SetRTPCValue("P1_NoiseFloor", NewNoiseFloor, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);

		// Tone Balance
		Wwise::SoundEngine::SetRTPCValue("Meter_Tone_Balance_Return", NewToneBalance, 0x1234, 0, AkCurveInterpolation_Linear);
		Wwise::SoundEngine::SetRTPCValue("Meter_Tone_Balance_Return", NewToneBalance, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);

		// Input Volume
		Wwise::SoundEngine::SetRTPCValue("P1_InputVol_Calibration", NewInputVolume, 0x1234, 0, AkCurveInterpolation_Linear);
		Wwise::SoundEngine::SetRTPCValue("P1_InputVol_Calibration", NewInputVolume, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);

		// Input Volume Return
		Wwise::SoundEngine::SetRTPCValue("P1_InputVol_Calibration_Return", NewInputVolumeReturn, 0x1234, 0, AkCurveInterpolation_Linear);
		Wwise::SoundEngine::SetRTPCValue("P1_InputVol_Calibration_Return", NewInputVolumeReturn, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
	}
}

static Framework::ModRegistrar<CalibrationMod> _calibrationReg;
