#pragma once
#include "windows.h"
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include "../Structs.hpp"
#include "../Offsets.hpp"
#include "../MemUtil.hpp"
#include "../Settings.hpp"
#include "../MemHelpers.hpp"

namespace ERMode {
	void Toggle7StringMode();
	void DoRainbow();
	void ToggleRainbowMode();
	bool IsRainbowEnabled();
	void ResetString(int strIndex);
	void ResetAllStrings();
	void SetCustomColors(int strIdx, ColorMap customColorMap);
	void Initialize();
	void InitStrings(std::vector<uintptr_t>& strings, int state);
	void SetColors(std::vector<uintptr_t> strings, std::vector<Color> colors);
	void SetColors(std::vector<uintptr_t> strings, std::string colorType);

	inline bool Is7StringSong = false;
	inline bool RainbowEnabled = false;

	inline int currentOffsetIdx = 0;
	inline int currColor = 0;
	inline bool saveDefaults = true;
	inline bool restoreDefaults = false;

	inline std::vector<ColorMap> customColors;
	inline std::vector<Color> customSolidColor;

	inline std::vector<Color> colorsTest = {
		{0.382f, 0.213f, 0.435f},
		{0.382f, 0.213f, 0.435f},
		{0.382f, 0.213f, 0.435f},
		{0.382f, 0.213f, 0.435f},
		{0.382f, 0.213f, 0.435f},
		{0.382f, 0.213f, 0.435f}
	};

	inline std::vector<Color> colorsWhite = {
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f}
	};

	inline std::vector<Color> colorsBlack = {
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f}
	};

	inline std::vector<Color> colorsNormal = {
		{ 0.0f, 0.7764706f, 0.5568628f },
		{ 0.7490196f, 0.372549f, 0.372549f },
		{ 0.7490196f, 0.7176471f, 0.372549f },
		{ 0.3686275f, 0.6156863f, 0.7372549f },
		{ 0.7490196f, 0.5529412f, 0.372549f },
		{ 0.372549f, 0.7490196f, 0.5921569f },
	};

	inline std::vector<Color> colorsGlow = {
		{ 0.0f, 1.0f, 0.6470588f },
		{ 1.0f, 0.0f, 0.0627451f },
		{ 1.0f, 0.7803922f, 0.0f },
		{ 0.0f, 0.6627451f, 1.0f },
		{ 1.0f, 0.4470588f, 0.0f },
		{ 0.2627451f, 1.0f, 0.0f },
	};

	inline std::vector<Color> colorsDisabled = {
		{ 0.0f, 0.2980392f, 0.2117647f },
		{ 0.2980392f, 0.09019608f, 0.1058824f },
		{ 0.2980392f, 0.254902f, 0.0f },
		{ 0.03529412f, 0.2117647f, 0.2980392f },
		{ 0.2980392f, 0.1647059f, 0.02352941f },
		{ 0.09019608f, 0.2980392f, 0.01568628f },
	};

	inline std::vector<Color> colorsAmbient = {
		{ 0.372549f, 0.7490196f, 0.6431373f },
		{ 0.7490196f, 0.372549f, 0.372549f },
		{ 0.7490196f, 0.7176471f, 0.372549f },
		{ 0.3686275f, 0.6156863f, 0.7372549f },
		{ 0.7490196f, 0.5529412f, 0.372549f },
		{ 0.372549f, 0.7490196f, 0.5921569f },
	};

	inline std::vector<Color> colorsStrEna = {
		{ 0.0f, 0.7764706f, 0.5568628f },
		{ 1.0f, 0.3098039f, 0.3529412f },
		{ 0.8862745f, 0.7568628f, 0.007843138f },
		{ 0.1137255f, 0.6745098f, 0.9764706f },
		{ 1.0f, 0.572549f, 0.08627451f },
		{ 0.2470588f, 0.8f, 0.04705882f },
	};

	inline std::vector<Color> colorsPegNotInTune = {
		{ 0.003921569f, 0.6f, 0.4117647f },
		{ 0.7490196f, 0.0f, 0.05882353f },
		{ 0.6862745f, 0.4588235f, 0.0f },
		{ 0.0f, 0.3764706f, 0.7294118f },
		{ 0.6980392f, 0.2196078f, 0.0f },
		{ 0.1254902f, 0.5490196f, 0.0f },
	};

	inline std::vector<Color> colorsPegInTune = {
		{ 0.003921569f, 0.6f, 0.4117647f },
		{ 0.7490196f, 0.0f, 0.05882353f },
		{ 0.6862745f, 0.4588235f, 0.0f },
		{ 0.0f, 0.3764706f, 0.7294118f },
		{ 0.6980392f, 0.2196078f, 0.0f },
		{ 0.1254902f, 0.5490196f, 0.0f },
	};

	inline std::vector<Color> colorsText = {
		{ 0.2470588f, 1.0f, 0.7843137f },
		{ 1.0f, 0.2509804f, 0.2509804f },
		{ 1.0f, 1.0f, 0.2509804f },
		{ 0.2509804f, 0.5019608f, 1.0f },
		{ 1.0f, 0.627451f, 0.2509804f },
		{ 0.2509804f, 1.0f, 0.2509804f },
	};

	inline std::vector<Color> colorsPart = {
		{ 0.2470588f, 1.0f, 0.7843137f },
		{ 1.0f, 0.2509804f, 0.2509804f },
		{ 1.0f, 1.0f, 0.2509804f },
		{ 0.2509804f, 0.5019608f, 1.0f },
		{ 1.0f, 0.627451f, 0.2509804f },
		{ 0.2509804f, 1.0f, 0.2509804f },
	};

	inline std::vector<Color> colorsBodyNorm = {
		{ 0.0f, 0.7764706f, 0.5568628f },
		{ 1.0f, 0.0f, 0.0f },
		{ 1.0f, 1.0f, 0.0f },
		{ 0.0f, 0.1176471f, 1.0f },
		{ 1.0f, 0.5450981f, 0.1843137f },
		{ 0.0f, 1.0f, 0.1764706f },
	};

	inline std::vector<Color> colorsBodyAcc = {
		{ 1.0f, 1.0f, 1.0f },
		{ 0.9764706f, 0.572549f, 0.5372549f },
		{ 0.9843137f, 0.9490196f, 0.5843138f },
		{ 0.5490196f, 0.9098039f, 0.9921569f },
		{ 0.9882353f, 0.8196079f, 0.5254902f },
		{ 0.6901961f, 0.9568627f, 0.5137255f },
	};

	inline std::vector<Color> colorsBodyPrev = {
		{ 0.1607843f, 0.2470588f, 0.2235294f },
		{ 0.2509804f, 0.1607843f, 0.1607843f },
		{ 0.2509804f, 0.227451f, 0.1607843f },
		{ 0.1607843f, 0.2078431f, 0.2509804f },
		{ 0.2509804f, 0.2f, 0.1607843f },
		{ 0.1647059f, 0.2509804f, 0.1607843f },
	};
};


