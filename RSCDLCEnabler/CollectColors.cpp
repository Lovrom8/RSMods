#include "CollectColors.h"
#include <iostream>
#include <cstdlib>
#include <cmath>

cCollectColors CollectColors;

/*
This is probably pretty janky, but it's based on the research I did below
https://docs.google.com/spreadsheets/d/1X3UjY68Z5oOuZpV_UpckOy-G-XZe7lWaskc31C7FxOs/edit?usp=sharing

Feel free to modify this pile of shit, I don't know how we wanted it formatted.
*/

std::string cCollectColors::getAmbientStringColor(int stringHue, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue + 4) + ", " + std::to_string(.423) + ", " + std::to_string(.558);
	else
		return std::to_string(stringHue - 1) + ", " + std::to_string(.423) + ", " + std::to_string(.558);
}

std::string cCollectColors::getDisabledStringColor(int stringHue, float stringSaturation, float stringLightness, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue), +", " + std::to_string(stringSaturation - .144) + ", " + std::to_string(stringLightness / 3.136);
	else
		return std::to_string(stringHue), +", " + std::to_string(stringSaturation - .428) + ", " + std::to_string(stringLightness / 2.812);
}

std::string cCollectColors::getGlowStringColor(int stringHue) {
	return std::to_string(stringHue - 4) + ", " + std::to_string(1) + ", " + std::to_string(.50);
}

std::string cCollectColors::getTuningPegColor(int stringHue, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue - 4) + ", " + std::to_string(1) + ", " + std::to_string(.3385);
	else
		return std::to_string(stringHue) + ", " + std::to_string(1) + ", " + std::to_string(.3453);
}

std::string cCollectColors::getPegResetColor() {
	return std::to_string(0) + ", " + std::to_string(0) + ", " + std::to_string(0);
}

std::string cCollectColors::getPegSuccessColor(bool isColorBlindMode) {
	if(!isColorBlindMode)
		return std::to_string(0) + ", " + std::to_string(0) + ", " + std::to_string(0);
	else
		return std::to_string(0) + ", " + std::to_string(0) + ", " + std::to_string(1);
}

std::string cCollectColors::getPegInTuneColor(int stringHue, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue + 4) + ", " + std::to_string(1) + ", " + std::to_string(.349);
	else
		return std::to_string(stringHue + 1) + ", " + std::to_string(1) + ", " + std::to_string(.349);
}

std::string cCollectColors::getPegOutTuneColor() {
	return std::to_string(0) + ", " + std::to_string(0) + ", " + std::to_string(1);
}

std::string cCollectColors::getRegTextIndicatorColor(int stringHue, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue + 3) + ", " + std::to_string(1) + ", " + std::to_string(.625);
	else
		return std::to_string(stringHue + 2) + ", " + std::to_string(1) + ", " + std::to_string(.624);
}

std::string cCollectColors::getRegForkParticlesColor(int stringHue, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue + 5) + ", " + std::to_string(1) + ", " + std::to_string(.625);
	else
		return std::to_string(stringHue + 5) + ", " + std::to_string(1) + ", " + std::to_string(.624);
}

std::string cCollectColors::getNotewayNormalColor(int stringHue, float stringSaturation, float stringLightness, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue + 5) + ", " + std::to_string(1) + ", " + std::to_string(.50);
	else
		return std::to_string(stringHue) + ", " + std::to_string(stringSaturation) + ", " + std::to_string(stringLightness);
}

std::string cCollectColors::getNotewayAccentColor(int stringHue, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue + 3) + ", " + std::to_string(.909) + ", " + std::to_string(.761);
	else
		return std::to_string(0) + ", " + std::to_string(0) + ", " + std::to_string(1);
}

std::string cCollectColors::getNotewayPreviewColor(int stringHue, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue - 1) + ", " + std::to_string(.219) + ", " + std::to_string(.206);
	else
		return std::to_string(stringHue) + ", " + std::to_string(.212) + ", " + std::to_string(.204);
}

std::string cCollectColors::getGuitarcadeMainColor(int stringHue, int stringNumber, bool isColorBlindMode) {
	if (!isColorBlindMode) {
		if (stringNumber == 4)
			return std::to_string(stringHue + 12) + ", " + std::to_string(1) + ", " + std::to_string(.429);
		if (stringNumber == 5)
			return std::to_string(stringHue + 12) + ", " + std::to_string(1) + ", " + std::to_string(.375);
		else
			return std::to_string(stringHue + 12) + ", " + std::to_string(1) + ", " + std::to_string(.50);
	}
	else {
		if (stringNumber == 4)
			return std::to_string(stringHue - 4) + ", " + std::to_string(1) + ", " + std::to_string(.429);
		if (stringNumber == 5)
			return std::to_string(stringHue - 4) + ", " + std::to_string(1) + ", " + std::to_string(.375);
		else
			return std::to_string(stringHue - 4) + ", " + std::to_string(1) + ", " + std::to_string(.50);
	}
}

std::string cCollectColors::getGuitarcadeAdditiveColor(int stringHue, int stringNumber, bool isColorBlindMode) {
	if (!isColorBlindMode) {
		if (stringNumber == 4)
			return std::to_string(stringHue + 12) + ", " + std::to_string(1) + ", " + std::to_string(.429);
		if (stringNumber == 5)
			return std::to_string(stringHue + 12) + ", " + std::to_string(1) + ", " + std::to_string(.375);
		else
			return std::to_string(stringHue + 12) + ", " + std::to_string(1) + ", " + std::to_string(.50);
	}
	else {
		if (stringNumber == 4)
			return std::to_string(stringHue + 2) + ", " + std::to_string(1) + ", " + std::to_string(.429);
		if (stringNumber == 5)
			return std::to_string(stringHue + 2) + ", " + std::to_string(1) + ", " + std::to_string(.375);
		else
			return std::to_string(stringHue + 2) + ", " + std::to_string(1) + ", " + std::to_string(.50);
	}
}

std::string cCollectColors::getGuitarcadeUIColor(int stringHue, int stringNumber, bool isColorBlindMode) {
	if (!isColorBlindMode) {
		if (stringNumber == 4)
			return std::to_string(stringHue + 12) + ", " + std::to_string(1) + ", " + std::to_string(.429);
		if (stringNumber == 5)
			return std::to_string(stringHue + 12) + ", " + std::to_string(1) + ", " + std::to_string(.375);
		else
			return std::to_string(stringHue + 12) + ", " + std::to_string(1) + ", " + std::to_string(.50);
	}
	else {
		if (stringNumber == 4)
			return std::to_string(stringHue + 2) + ", " + std::to_string(1) + ", " + std::to_string(.429);
		if (stringNumber == 5)
			return std::to_string(stringHue + 2) + ", " + std::to_string(1) + ", " + std::to_string(.375);
		else
			return std::to_string(stringHue + 2) + ", " + std::to_string(1) + ", " + std::to_string(.50);
	}
}

// This section about Min, Max, RGBToHSL was totally not stolen off StackOverflow :)

float cCollectColors::Min(float fR, float fG, float fB)
{
	float fMin = fR;
	if (fG < fMin)
	{
		fMin = fG;
	}
	if (fB < fMin)
	{
		fMin = fB;
	}
	return fMin;
}


float cCollectColors::Max(float fR, float fG, float fB)
{
	float fMax = fR;
	if (fG > fMax)
	{
		fMax = fG;
	}
	if (fB > fMax)
	{
		fMax = fB;
	}
	return fMax;
}

void cCollectColors::RGBToHSL(int R, int G, int B, int& H, int& S, int& L)
{
	int r = 100.0;
	int g = 100.0;
	int b = 200.0;

	float fR = r / 255.0;
	float fG = g / 255.0;
	float fB = b / 255.0;


	float fCMin = Min(fR, fG, fB);
	float fCMax = Max(fR, fG, fB);


	L = 50 * (fCMin + fCMax);

	if (fCMin == fCMax)
	{
		S = 0;
		H = 0;
		return;

	}
	else if (L < 50)
	{
		S = 100 * (fCMax - fCMin) / (fCMax + fCMin);
	}
	else
	{
		S = 100 * (fCMax - fCMin) / (2.0 - fCMax - fCMin);
	}

	if (fCMax == fR)
	{
		H = 60 * (fG - fB) / (fCMax - fCMin);
	}
	if (fCMax == fG)
	{
		H = 60 * (fB - fR) / (fCMax - fCMin) + 120;
	}
	if (fCMax == fB)
	{
		H = 60 * (fR - fG) / (fCMax - fCMin) + 240;
	}
	if (H < 0)
	{
		H = H + 360;
	}
}
