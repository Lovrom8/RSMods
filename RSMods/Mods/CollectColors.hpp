#pragma once
#include "windows.h"
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include "../Structs.hpp"

namespace CollectColors {
	RSColor GetAmbientStringColor(int stringHue, bool colorBlind);
	RSColor GetDisabledStringColor(int stringHue, float stringSaturation, float stringLightness, bool colorBlind);
	RSColor GetGlowStringColor(int stringHue);
	RSColor GetTuningPegColor(int stringHue, bool colorBlind);
	RSColor GetPegResetColor();
	RSColor GetPegSuccessColor(bool colorBlind);
	RSColor GetPegInTuneColor(int stringHue, bool colorBlind);
	RSColor GetPegOutTuneColor();
	RSColor GetRegTextIndicatorColor(int stringHue, bool colorBlind);
	RSColor GetRegForkParticlesColor(int stringHue, bool colorBlind);
	RSColor GetNotewayNormalColor(int stringHue, float stringSaturation, float stringLightness, bool colorBlind);
	RSColor GetNotewayAccentColor(int stringHue, bool colorBlind);
	RSColor GetNotewayPreviewColor(int stringHue, bool colorBlind);
	RSColor GetGuitarcadeMainColor(int stringHue, int stringNumber, bool colorBlind);
	RSColor GetGuitarcadeAdditiveColor(int stringHue, int stringNumber, bool colorBlind);
	RSColor GetGuitarcadeUIColor(int stringHue, int stringNumber, bool colorBlind);

	float Min(float fR, float fG, float fB);
	float Max(float fR, float fG, float fB);
	void SetColors(RSColor);
	void RGB2HSL(float R, float G, float B, int& H, float& S, float& L);
	void HSL2RGB(float H, float S, float L, float& R, float& G, float& B);

	RSColor GetColor();
	inline int H;
	inline float S, L, R, G, B;
};