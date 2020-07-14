#include "ExtendedRangeMode.h"

cERMode ERMode;

uintptr_t GetStringColor(uintptr_t stringnum, string_state state) {
	uintptr_t edx = stringnum;
	uintptr_t eax = 0;
	uintptr_t magic1 = Offsets.ptr_stringColor;

	uintptr_t ecx = MemUtil.ReadPtr(magic1);

	if (!ecx)
		return NULL;

	eax = MemUtil.ReadPtr(ecx + eax * 0x4 + 0x348);

	if (eax >= 2) {
		return NULL;
	}

	eax = eax * 0xA8;
	eax += edx;

	eax = MemUtil.ReadPtr(ecx + eax * 0x4 + state);

	return eax;
}

struct String { //maybe do sth with this
	uintptr_t strPtr;
	Color oldColor;

	void Init(int strNum, string_state state) {
		strPtr = GetStringColor(strNum, state);
	}

	void SetColor(int stringIdx, float R, float G, float B) {
		if (stringIdx < 0 || stringIdx > 6) {
			std::cout << "wat doink" << std::endl;
			return;
		}

		Color c;
		c.r = R;
		c.g = G;
		c.b = B;

		*(Color*)strPtr = c;
	}

};

Color cNormal;
Color cDisabled;
Color cGlow;
Color cAmb;
Color cEnabled;
Color cPegNotInTune;
Color cPegInTune;
Color cTextNew;
Color cPart;
Color cBodyNorm;
Color cBodyAcc;
Color cBodyPrev;

void InitStrings(std::vector<uintptr_t>& strings, string_state state) {
	for (int strIndex = 0; strIndex < 6;strIndex++)
		strings.push_back(GetStringColor(strIndex, state));
}

void SetColors(std::vector<uintptr_t> strings, std::vector<Color> colors) {
	for (int strIndex = 0; strIndex < 6;strIndex++) {
		*(Color*)strings[strIndex] = colors[strIndex];
	}
}

void cERMode::Toggle7StringMode() { //TODO: use the GUI to make DDS files and load settings here for matching string colors
	static bool colorsSaved = false;
	std::vector<uintptr_t> stringsNormal, stringsGlow, stringsDisabled, stringsAmb, stringsEnabled, stringsPegInTune, stringsPegNotInTune, stringsText, stringsPart, stringsBodyNorm, stringsBodyAcc, stringsBodyPrev;

	InitStrings(stringsNormal, Normal);
	InitStrings(stringsGlow, Glow);
	InitStrings(stringsDisabled, Disabled);
	InitStrings(stringsAmb, Ambient);
	InitStrings(stringsEnabled, Enabled);
	InitStrings(stringsPegInTune, PegInTune);
	InitStrings(stringsPegNotInTune, PegNotInTune);
	InitStrings(stringsText, Text);
	InitStrings(stringsPart, Particles);
	InitStrings(stringsBodyNorm, BodyNorm);
	InitStrings(stringsBodyAcc, BodyAcc);
	InitStrings(stringsBodyPrev, BodyPrev);

	if (!colorsSaved && MemHelpers.GetCurrentMenu() == "LearnASong_Game") { //uncomment if we need to read the values
		/*cNormal = *(Color*)string0Normal; //still not sure what "Normal" corresponds to in the games flat files
		cDisabled = *(Color*)string0Disabled;
		cGlow = *(Color*)string0Glow;
		cAmb = *(Color*)string0Amb;
		cEnabled = *(Color*)string0Enabled;
		cPegNotInTune = *(Color*)string0PegNotInTune;
		cPegInTune = *(Color*)string0PegInTune;
		cTextNew = *(Color*)string0TextNew;
		cPart = *(Color*)string0Part;
		cBodyNorm = *(Color*)string0BodyNorm;
		cBodyAcc = *(Color*)string0BodyAcc;
		cBodyPrev = *(Color*)string0BodyPrev;*/

		for (int i = 0; i < 6;i++) {
			std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)((*(Color*)stringsGlow[i]).r * 255);
			std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)((*(Color*)stringsGlow[i]).g * 255);
			std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)((*(Color*)stringsGlow[i]).b * 255);
			std::cout << std::endl;
		}

		colorsSaved = true;
	}

	if (MemHelpers.IsExtendedRangeSong()) {
		if (Settings.GetModSetting("CustomStringColors") == 1) { //Zag's colors
			// Zags custom low B color values manually entered; Normal
			SetColors(stringsNormal, colorsNormal);

			// Zags custom low B color values manually entered; Glowed
			SetColors(stringsGlow, colorsGlow);

			// Zags custom low B color values manually entered; Disabled
			SetColors(stringsDisabled, colorsDisabled);

			//name = "GuitarStringsAmbientColorBlind" id = "3175458924" source = "GameColorManager" >
			SetColors(stringsAmb, colorsAmbient);

			//name="GuitarStringsEnabledColorBlind" id="237528906"
			SetColors(stringsEnabled, colorsStrEna);

			//name="GuitarPegsTuningBlind" id="1806691030"
			SetColors(stringsPegNotInTune, colorsPegNotInTune);

			//name="GuitarPegsInTuneBlind" id="2547441015"
			SetColors(stringsPegInTune, colorsPegInTune);

			//name="GuitarRegistrarTextIndicatorBlind" id="3186002004"
			SetColors(stringsText, colorsText);

			//name="GuitarRegistrarForkParticlesBlind" id="3239612871"
			SetColors(stringsPart, colorsPart);

			// name="NotewayBodyPartsBodyNormBlind" id="3629363565"
			SetColors(stringsBodyNorm, colorsNormal);

			// = name = "NotewayBodyPartsAccentBlind" id = "47948252"
			SetColors(stringsBodyAcc, colorsBodyAcc);

			//= name = "NotewayBodyPartsPreviewBlind" id = "338656387"
			SetColors(stringsBodyPrev, colorsBodyPrev);
		}
		else if (Settings.GetModSetting("CustomStringColors") == 0) { //default colors 
			//do we even need to do anything in this case?
		}
		else if (Settings.GetModSetting("CustomStringColors") == 2) { //custom colors
			SetColors(stringsNormal, Settings.GetCustomColors(true)); //TODO: actually determine which colors need to be change to what values
			SetColors(stringsGlow, Settings.GetCustomColors(true));
			SetColors(stringsAmb, Settings.GetCustomColors(true));
			SetColors(stringsDisabled, Settings.GetCustomColors(true));
			SetColors(stringsEnabled, Settings.GetCustomColors(true));
			//etc.
		}
	}
	else {
		//restore orignal colors, I guess
	}
}

void cERMode::ToggleRainbowMode() {
	RainbowEnabled = !RainbowEnabled;
}

void cERMode::DoRainbow() {
	if (!RainbowEnabled)
		return;

	std::vector<uintptr_t> stringsNormal;
	std::vector<uintptr_t> stringsHigh;
	std::vector<uintptr_t> stringsDisabled;
	std::vector<Color> oldNormal, oldHigh, oldDisabled;

	for (int i = 0; i < 6; i++) {
		stringsNormal.push_back(GetStringColor(i, Normal));
		stringsHigh.push_back(GetStringColor(i, Glow));
		stringsDisabled.push_back(GetStringColor(i, Disabled));
	}

	Color c;
	c.r = 1.f;
	c.g = 0.f;
	c.b = 0.f;

	float h = 0.f;
	float speed = 2.f;
	float stringOffset = 20.f;

	while (RainbowEnabled) {

		h += speed;
		if (h >= 360.f) { h = 0.f; }

		for (int i = 0; i < 6; i++) {
			oldNormal.push_back(*(Color*)stringsNormal[i]);
			oldHigh.push_back(*(Color*)stringsHigh[i]);
			oldDisabled.push_back(*(Color*)stringsDisabled[i]);

			c.setH(h + (stringOffset * i));

			*(Color*)stringsNormal[i] = c;
			*(Color*)stringsHigh[i] = c;
			*(Color*)stringsDisabled[i] = c;
		}

		Sleep(16);
	}

	if (oldNormal.size() == 0)
		return;

	for (int i = 0; i < 6; i++) {
		*(Color*)stringsNormal[i] = oldNormal[i];
		*(Color*)stringsHigh[i] = oldHigh[i];
		*(Color*)stringsDisabled[i] = oldDisabled[i];
	}
}
