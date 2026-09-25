#include "../stdafx.h"
#include "CollectColors.hpp"

/*
This is probably pretty janky, but it's based on the research I did below
https://docs.google.com/spreadsheets/d/1X3UjY68Z5oOuZpV_UpckOy-G-XZe7lWaskc31C7FxOs/edit?usp=sharing
*/


/// <summary>
/// Convert input color (from INI) to Ambient String Color
/// </summary>
/// <param name="stringHue"> - Hue of input color</param>
/// <param name="colorBlind"> - Should we make a ColorBlind color</param>
RSColor CollectColors::GetAmbientStringColor(int stringHue, bool colorBlind) {
	if (colorBlind)
		H = stringHue - 1;
	else
		H = stringHue + 4;

	S = 0.423f;
	L = 0.558f;

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Disabled String Color
/// </summary>
/// <param name="stringHue"> - Hue of input color</param>
/// <param name="stringSaturation"> - Saturation of input color</param>
/// <param name="stringLightness"> - Lightness of input color</param>
/// <param name="colorBlind"> - Should we make a ColorBlind color</param>
RSColor CollectColors::GetDisabledStringColor(int stringHue, float stringSaturation, float stringLightness, bool colorBlind) {
	if (colorBlind) {
		S = stringSaturation - 0.428f;
		L = stringLightness / 2.812f;
	}
	else {
		S = stringSaturation - 0.144f;
		L = stringLightness / 3.136f;
	}

	H = stringHue;

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Glow String Color
/// </summary>
/// <param name="stringHue"> - Hue of input color</param>
RSColor CollectColors::GetGlowStringColor(int stringHue) {
	H = stringHue - 4;
	S = 1.0f;
	L = 0.5f;

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Tuning Peg Color
/// </summary>
/// <param name="stringHue"> - Hue of input color</param>
RSColor CollectColors::GetTuningPegColor(int stringHue) {
	H = stringHue;
	S = 1.0f;
	L = 0.25f;

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Peg Reset Color
/// </summary>
/// <returns>Always returns black</returns>
RSColor CollectColors::GetPegResetColor() {
	H = 0;
	S = 0.0f;
	L = 0.0f;

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Peg Success Color
/// </summary>
/// <param name="colorBlind"> - Should we make a ColorBlind color</param>
/// <returns>Returns white if ColorBlind, black if not.</returns>
RSColor CollectColors::GetPegSuccessColor(bool colorBlind) {
	if (colorBlind) {
		L = 1.0f;
	}
	else {
		L = 0.0f;
	}

	H = 0;
	S = 0.0f;

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Peg In-Tune Color
/// </summary>
/// <param name="stringHue"> - Hue of input color</param>
/// <param name="colorBlind"> - Should we make a ColorBlind color</param>
RSColor CollectColors::GetPegInTuneColor(int stringHue, bool colorBlind) {
	if (colorBlind) {
		H = stringHue - 1;
	}
	else {
		H = stringHue + 4;
	}

	S = 1.0f;
	L = 0.349f;

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Peg Out-Tune Color
/// </summary>
/// <returns>Always returns white</returns>
RSColor CollectColors::GetPegOutTuneColor() {
	H = 0;
	S = 0.f;
	L = 1.f;

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Reg Text Indicator Color
/// </summary>
/// <param name="stringHue"> - Hue of input color</param>
/// <param name="colorBlind"> - Should we make a ColorBlind color</param>
RSColor CollectColors::GetRegTextIndicatorColor(int stringHue, bool colorBlind) {
	if (colorBlind) {
		H = stringHue + 2;
		L = 0.624f;
	}
	else {
		H = stringHue + 3;
		L = 0.625f;
	}

	S = 1.0f;

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Fork Particles Color
/// </summary>
/// <param name="stringHue"> - Hue of input color</param>
/// <param name="colorBlind"> - Should we make a ColorBlind color</param>
RSColor CollectColors::GetRegForkParticlesColor(int stringHue, bool colorBlind) {
	if (colorBlind) {
		L = 0.624f;
	}
	else {
		L = 0.625f;
	}

	H = stringHue + 5;
	S = 1.0f;

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Noteway Normal Color
/// </summary>
/// <param name="stringHue"> - Hue of input color</param>
/// <param name="stringSaturation"> - Saturation of input color</param>
/// <param name="stringLightness"> - Lightness of input color</param>
/// <param name="colorBlind"> - Should we make a ColorBlind color</param>
RSColor CollectColors::GetNotewayNormalColor(int stringHue, float stringSaturation, float stringLightness, bool colorBlind) {
	if (colorBlind) {
		H = stringHue;
		S = stringSaturation;
		L = stringLightness;
	}
	else {
		H = stringHue + 5;
		S = 1.0f;
		L = .5f;
	}

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Noteway Accent Color
/// </summary>
/// <param name="stringHue"> - Hue of input color</param>
/// <param name="colorBlind"> - Should we make a ColorBlind color</param>
RSColor CollectColors::GetNotewayAccentColor(int stringHue, bool colorBlind) {
	if (colorBlind) {
		H = 0;
		S = 0.0f;
		L = 1.0f;
	}
	else {
		H = stringHue + 3;
		S = 0.909f;
		L = 0.761f;
	}

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Noteway Preview Color
/// </summary>
/// <param name="stringHue"> - Hue of input color</param>
/// <param name="colorBlind"> - Should we make a ColorBlind color</param>
RSColor CollectColors::GetNotewayPreviewColor(int stringHue, bool colorBlind) {
	if (colorBlind) {
		H = stringHue;
		S = 0.212f;
		L = 0.204f;
	}
	else {
		H = stringHue - 1;
		S = 0.219f;
		L = 0.206f;
	}

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Guitarcade Main Color
/// </summary>
/// <param name="stringHue"> - Hue of input color</param>
/// <param name="stringNumber"> - String# from thickest to thinnest (zero-indexed)</param>
/// <param name="colorBlind"> - Should we make a ColorBlind color</param>
RSColor CollectColors::GetGuitarcadeMainColor(int stringHue, int stringNumber, bool colorBlind) {
	if (colorBlind) {
		H = stringHue - 4;
	}
	else {
		H = stringHue + 12;
	}

	S = 1.0f;

	if (stringNumber == 4) // B string (Normally lime green)
		L = 0.429f;
	else if (stringNumber == 5) // e string (Normally purple-ish)
		L = 0.375f;
	else // E, A, D, & G strings
		L = 0.50f;

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Guitarcade Additive Color
/// </summary>
/// <param name="stringHue"> - Hue of input color</param>
/// <param name="stringNumber"> - String# from thickest to thinnest (zero-indexed)</param>
/// <param name="colorBlind"> - Should we make a ColorBlind color</param>
RSColor CollectColors::GetGuitarcadeAdditiveColor(int stringHue, int stringNumber, bool colorBlind) {
	if (colorBlind) {
		H = stringHue + 2;
	}
	else {
		H = stringHue + 12;
	}

	S = 1.0f;

	if (stringNumber == 4) // B string (Normally lime green)
		L = 0.429f;
	else if (stringNumber == 5) // e string (Normally purple-ish)
		L = 0.375f;
	else // E, A, D, & G strings
		L = 0.50f;

	return GetColor();
}

/// <summary>
/// Convert input color (from INI) to Guitarcade UI Color
/// </summary>
/// <param name="stringHue"> - Hue of input color</param>
/// <param name="stringNumber"> - String# from thickest to thinnest (zero-indexed)</param>
/// <param name="colorBlind"> - Should we make a ColorBlind color</param>
RSColor CollectColors::GetGuitarcadeUIColor(int stringHue, int stringNumber, bool colorBlind) {
	if (colorBlind) {
		H = stringHue + 2;
	}
	else {
		H = stringHue + 12;
	}

	S = 1.0f;

	if (stringNumber == 4) // B string (Normally lime green)
		L = 0.429f;
	else if (stringNumber == 5) // e string (Normally purple-ish)
		L = 0.375f;
	else // E, A, D, & G strings
		L = 0.50f;

	return GetColor();
}

// Functions

/// <param name="fR"> - Red</param>
/// <param name="fG"> - Green</param>
/// <param name="fB"> - Blue</param>
/// <returns>Lowest value</returns>
float CollectColors::Min(float fR, float fG, float fB) {
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

/// <param name="fR"> - Red</param>
/// <param name="fG"> - Green</param>
/// <param name="fB"> - Blue</param>
/// <returns>Highest value</returns>
float CollectColors::Max(float fR, float fG, float fB) {
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

/// <summary>
/// Convert RGB to HSL. From pseudocode in Agoston's Computer Graphics and Geometric Modeling: Implementation and Algorithms
/// </summary>
/// <param name="R"> - Red **INPUT**</param>
/// <param name="G"> - Green **INPUT**</param>
/// <param name="B"> - Blue **INPUT**</param>
/// <param name="H"> - Hue **OUTPUT**</param>
/// <param name="S"> - Saturation **OUTPUT**</param>
/// <param name="L"> - Lightness **OUTPUT**</param>
void CollectColors::RGB2HSL(float inR, float inG, float inB, int& outH, float& outS, float& outL) { //R,G,B [0,1], H [0,360], S,L [0,1] 
	float minVal = Min(inR, inG, inB);
	float maxVal = Max(inR, inG, inB);
	float d, h;

	outL = (minVal + maxVal) / 2;

	if (maxVal != minVal) {
		d = maxVal - minVal;

		outS = (outL <= 0.5) ? (d / (minVal + maxVal)) : (d / (2 - maxVal - minVal));

		if (inR == maxVal)
			h = (inG - inB) / d;
		else if (inG == maxVal)
			h = 2 + (inB - inR) / d;
		else if (inB == maxVal)
			h = 4 + (inR - inG) / d;
		else
			h = 0.0f;

		outH = static_cast<int>(60 * h);

		if (outH < 0)
			outH += 360;
	}
}

/// <summary>
/// Convert HSL to RGB.  From pseudocode in Agoston's Computer Graphics and Geometric Modeling: Implementation and Algorithms
/// </summary>
/// <param name="H"> - Hue **INPUT**</param>
/// <param name="S"> - Saturation **INPUT**</param>
/// <param name="L"> - Lightness **INPUT**</param>
/// <param name="R"> - Red **OUTPUT**</param>
/// <param name="G"> - Green **OUTPUT**</param>
/// <param name="B"> - Blue **OUTPUT**</param>
void CollectColors::HSL2RGB(float inH, float inS, float inL, float& outR, float& outG, float& outB) {
	float v, minVal, sv, fract, vsf, mid1, mid2;
	int sextant;

	v = (float)(((double)inL <= 0.5) ? ((double)inL * (1.0 + (double)inS)) : ((double)inL + (double)inS - (double)inL * (double)inS));

	if (v > 0) {
		minVal = 2 * inL - v;
		sv = (v - minVal) / v;
		inH = (inH == 360) ? 0 : inH / 60;

		sextant = (int)floor(inH);
		fract = inH - sextant;
		vsf = v * sv * fract;
		mid1 = minVal + vsf;
		mid2 = v - vsf;

		switch (sextant)
		{
			case 0: outR = v; outG = mid1; outB = minVal; break;
			case 1: outR = mid2; outG = v; outB = minVal; break;
			case 2: outR = minVal; outG = v; outB = mid1; break;
			case 3: outR = minVal; outG = mid2; outB = v; break;
			case 4: outR = mid1; outG = minVal; outB = v; break;
			case 5: outR = v; outG = minVal; outB = mid2; break;
		}
	}
	else {
		outR = 0.0f;
		outG = 0.0f;
		outB = 0.0f;
	}
}

/// <summary>
/// Convert HSL to RGB
/// </summary>
RSColor CollectColors::GetColor() {
	HSL2RGB((float)H, S, L, R, G, B);
	return RSColor(R, G, B);
}

/// <summary>
/// Set Global Color
/// </summary>
void CollectColors::SetColors(RSColor color) {
	R = color.r;
	G = color.g;
	B = color.b;

	RGB2HSL(R, G, B, H, S, L);
}