#pragma once

#include "../RSColor.h"

namespace CollectColors {
	RSColor GetAmbientStringColor(int stringHue, bool colorBlind);
	RSColor GetDisabledStringColor(int stringHue, float stringSaturation, float stringLightness, bool colorBlind);
	RSColor GetGlowStringColor(int stringHue);
	RSColor GetTuningPegColor(int stringHue);
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
	void RGB2HSL(float inR, float inG, float inB, int& outH, float& outS, float& outL);
	void HSL2RGB(float inH, float inS, float inL, float& outR, float& outG, float& outB);

	RSColor GetColor();
	inline int H;
	inline float S, L, R, G, B;

	// Use Exact Colors. These work on their own values (no shared H / S / L state) and keep the hue wrapped into [0, 360).
	struct HSLColor {
		float h = 0.0f; // Degrees, [0, 360)
		float s = 0.0f; // [0, 1]
		float l = 0.0f; // [0, 1]
	};

	HSLColor ToHSL(const RSColor& color);
	RSColor FromHSL(const HSLColor& color);
	RSColor Recolor(const RSColor& pixel, const RSColor& anchor, const RSColor& target);
	float ColorDistance(const RSColor& a, const RSColor& b);
	size_t NearestColor(const RSColor& target, const ColorList& candidates, size_t preferred);
};