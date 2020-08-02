#pragma once
#include "windows.h"
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include "../Structs.h"

class cCollectColors {
public:
	RSColor GetAmbientStringColor(int stringHue, bool isColorBlindMode);
	RSColor GetDisabledStringColor(int stringHue, float stringSaturation, float stringLightness, bool isColorBlindMode);
	RSColor GetGlowStringColor(int stringHue);
	RSColor GetTuningPegColor(int stringHue, bool isColorBlindMode);
	RSColor GetPegResetColor();
	RSColor GetPegSuccessColor(bool isColorBlindMode);
	RSColor GetPegInTuneColor(int stringHue, bool isColorBlindMode);
	RSColor GetPegOutTuneColor();
	RSColor GetRegTextIndicatorColor(int stringHue, bool isColorBlindMode);
	RSColor GetRegForkParticlesColor(int stringHue, bool isColorBlindMode);
	RSColor GetNotewayNormalColor(int stringHue, float stringSaturation, float stringLightness, bool isColorBlindMode);
	RSColor GetNotewayAccentColor(int stringHue, bool isColorBlindMode);
	RSColor GetNotewayPreviewColor(int stringHue, bool isColorBlindMode);
	RSColor GetGuitarcadeMainColor(int stringHue, int stringNumber, bool isColorBlindMode);
	RSColor GetGuitarcadeAdditiveColor(int stringHue, int stringNumber, bool isColorBlindMode);
	RSColor GetGuitarcadeUIColor(int stringHue, int stringNumber, bool isColorBlindMode);

	float Min(float fR, float fG, float fB);
	float Max(float fR, float fG, float fB);
	void SetColors(RSColor);
	void RGB2HSL(float R, float G, float B, int& H, float& S, float& L);
	void HSL2RGB(float H, float S, float L, float& R, float& G, float& B);

private:
	RSColor GetColor();
	int H;
	float S, L, R, G, B;
};

extern cCollectColors CollectColors;