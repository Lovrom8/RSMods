#pragma once
#include "windows.h"
#include <vector>
#include <string>
#include <iostream>

class cCollectColors {
public:
	std::string cCollectColors::getAmbientStringColor(int stringHue, bool isColorBlindMode);
	std::string getDisabledStringColor(int stringHue, float stringSaturation, float stringLightness, bool isColorBlindMode);
	std::string getGlowStringColor(int stringHue);
	std::string getTuningPegColor(int stringHue, bool isColorBlindMode);
	std::string getPegResetColor();
	std::string getPegSuccessColor(bool isColorBlindMode);
	std::string getPegInTuneColor(int stringHue, bool isColorBlindMode);
	std::string getPegOutTuneColor();
	std::string getRegTextIndicatorColor(int stringHue, bool isColorBlindMode);
	std::string getRegForkParticlesColor(int stringHue, bool isColorBlindMode);
	std::string getNotewayNormalColor(int stringHue, float stringSaturation, float stringLightness, bool isColorBlindMode);
	std::string getNotewayAccentColor(int stringHue, bool isColorBlindMode);
	std::string getNotewayPreviewColor(int stringHue, bool isColorBlindMode);
	std::string getGuitarcadeMainColor(int stringHue, int stringNumber, bool isColorBlindMode);
	std::string getGuitarcadeAdditiveColor(int stringHue, int stringNumber, bool isColorBlindMode);
	std::string getGuitarcadeUIColor(int stringHue, int stringNumber, bool isColorBlindMode);
	float Min(float fR, float fG, float fB);
	float Max(float fR, float fG, float fB);
	void RGBToHSL(int R, int G, int B, int& H, int& S, int& L);
};

extern cCollectColors CollectColors;