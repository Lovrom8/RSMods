#include "CollectColors.h"

cCollectColors CollectColors;

/*
This is probably pretty janky, but it's based on the research I did below
https://docs.google.com/spreadsheets/d/1X3UjY68Z5oOuZpV_UpckOy-G-XZe7lWaskc31C7FxOs/edit?usp=sharing

Feel free to modify this pile of shit, I don't know how we wanted it formatted.
*/

std::string getAmbientStringColor(int stringHue, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue + 4) + ", " + std::to_string(.423) + ", " + std::to_string(.558);
	else
		return std::to_string(stringHue - 1) + ", " + std::to_string(.423) + ", " + std::to_string(.558);
}

std::string getDisabledStringColor(int stringHue, float stringSaturation, float stringLightness, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue), +", " + std::to_string(stringSaturation - .144) + ", " + std::to_string(stringLightness / 3.136);
	else
		return std::to_string(stringHue), +", " + std::to_string(stringSaturation - .428) + ", " + std::to_string(stringLightness / 2.812);
}

std::string getGlowStringColor(int stringHue) {
	return std::to_string(stringHue - 4) + ", " + std::to_string(1) + ", " + std::to_string(.50);
}

std::string getTuningPegColor(int stringHue, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue - 4) + ", " + std::to_string(1) + ", " + std::to_string(.3385);
	else
		return std::to_string(stringHue) + ", " + std::to_string(1) + ", " + std::to_string(.3453);
}

std::string getPegResetColor() {
	return std::to_string(0) + ", " + std::to_string(0) + ", " + std::to_string(0);
}

std::string getPegSuccessColor(bool isColorBlindMode) {
	if(!isColorBlindMode)
		return std::to_string(0) + ", " + std::to_string(0) + ", " + std::to_string(0);
	else
		return std::to_string(0) + ", " + std::to_string(0) + ", " + std::to_string(1);
}

std::string getPegInTuneColor(int stringHue, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue + 4) + ", " + std::to_string(1) + ", " + std::to_string(.349);
	else
		return std::to_string(stringHue + 1) + ", " + std::to_string(1) + ", " + std::to_string(.349);
}

std::string getPegOutTuneColor() {
	return std::to_string(0) + ", " + std::to_string(0) + ", " + std::to_string(1);
}

std::string getRegTextIndicatorColor(int stringHue, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue + 3) + ", " + std::to_string(1) + ", " + std::to_string(.625);
	else
		return std::to_string(stringHue + 2) + ", " + std::to_string(1) + ", " + std::to_string(.624);
}

std::string getRegForkParticlesColor(int stringHue, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue + 5) + ", " + std::to_string(1) + ", " + std::to_string(.625);
	else
		return std::to_string(stringHue + 5) + ", " + std::to_string(1) + ", " + std::to_string(.624);
}

std::string getNotewayNormalColor(int stringHue, float stringSaturation, float stringLightness, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue + 5) + ", " + std::to_string(1) + ", " + std::to_string(.50);
	else
		return std::to_string(stringHue) + ", " + std::to_string(stringSaturation) + ", " + std::to_string(stringLightness);
}

std::string getNotewayAccentColor(int stringHue, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue + 3) + ", " + std::to_string(.909) + ", " + std::to_string(.761);
	else
		return std::to_string(0) + ", " + std::to_string(0) + ", " + std::to_string(1);
}

std::string getNotewayPreviewColor(int stringHue, bool isColorBlindMode) {
	if (!isColorBlindMode)
		return std::to_string(stringHue - 1) + ", " + std::to_string(.219) + ", " + std::to_string(.206);
	else
		return std::to_string(stringHue) + ", " + std::to_string(.212) + ", " + std::to_string(.204);
}

std::string getGuitarcadeMainColor(int stringHue, int stringNumber, bool isColorBlindMode) {
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

std::string getGuitarcadeAdditiveColor(int stringHue, int stringNumber, bool isColorBlindMode) {
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

std::string getGuitarcadeUIColor(int stringHue, int stringNumber, bool isColorBlindMode) {
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