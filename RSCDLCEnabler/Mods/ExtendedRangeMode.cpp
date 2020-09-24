#include "ExtendedRangeMode.hpp"

uintptr_t GetStringColor(uintptr_t stringnum, int state) {
	uintptr_t currentStringNum = stringnum; // EDX
	uintptr_t currentStringColor = 0; // EAX
	uintptr_t stringColorPointer = Offsets::ptr_stringColor; // Magic Number

	uintptr_t pointerValue = MemUtil::ReadPtr(stringColorPointer); // ECX

	if (!pointerValue)
		return NULL;

	currentStringColor = MemUtil::ReadPtr(pointerValue + currentStringColor * 0x4 + 0x348);

	if (currentStringColor >= 2) {
		return NULL;
	}

	currentStringColor = currentStringColor * 0xA8;
	currentStringColor += currentStringNum;

	currentStringColor = MemUtil::ReadPtr(pointerValue + currentStringColor * 0x4 + state);

	return currentStringColor;
}

void ERMode::InitStrings(std::vector<uintptr_t>& strings, int state) {
	strings.clear();
	for (int strIndex = 0; strIndex < 6;strIndex++)
		strings.push_back(GetStringColor(strIndex, state));
}

void ERMode::SetColors(std::vector<uintptr_t> strings, std::vector<Color> colors) {
	for (int strIndex = 0; strIndex < 6;strIndex++) {
		if (strings[strIndex] == NULL)
			return;
		*(Color*)strings[strIndex] = colors[strIndex];
	}
}

void ERMode::Initialize() {
	std::map<std::string, RSColor> emptyMap;

	for (int str = 0; str < 6; str++)
		customColors.push_back(emptyMap);
}

void ERMode::SetCustomColors(int strIdx, ColorMap customColorMap) {
	customColors[strIdx] = customColorMap;
}

void ERMode::SetColors(std::vector<uintptr_t> strings, std::string colorType) {
	for (int strIndex = 0; strIndex < 6;strIndex++) {
		if (strings[strIndex] == NULL)
			return;

		*(Color*)strings[strIndex] = customColors[strIndex][colorType];
	}
}

std::vector<Color> oldNormal, oldDisabled, oldEnabled, oldGlow, oldAmb;

void ERMode::ResetAllStrings() {
	for (int str = 0; str < 6;str++)
		ResetString(str);
}

void ERMode::ResetString(int strIndex) {
	std::vector<uintptr_t> stringsGlow, stringsDisabled, stringsAmb, stringsEnabled, stringsPegInTune, stringsPegNotInTune, stringsText, stringsPart, stringsBodyNorm, stringsBodyAcc, stringsBodyPrev;

	InitStrings(stringsGlow, Glow);
	InitStrings(stringsDisabled, Disabled);
	//InitStrings(stringsAmb, Ambient);
	InitStrings(stringsEnabled, Enabled);

	*(Color*)stringsGlow[strIndex] = oldGlow[strIndex];
	*(Color*)stringsDisabled[strIndex] = oldDisabled[strIndex];
	//*(Color*)stringsAmb[strIndex] = oldAmb[strIndex];
	*(Color*)stringsEnabled[strIndex] = oldEnabled[strIndex];

	//Settings::SetStringColors(strIndex, oldGlow[strIndex], false);
}

std::vector<std::vector<Color>> defaultColors;

void ERMode::Toggle7StringMode() {
	static bool colorsSaved = false;
	std::vector<uintptr_t> stringsTest, stringsGlow, stringsDisabled, stringsAmb, stringsEnabled, stringsPegInTune, stringsPegNotInTune, stringsText, stringsPart, stringsBodyNorm, stringsBodyAcc, stringsBodyPrev;

	InitStrings(stringsGlow, Glow);
	InitStrings(stringsDisabled, Disabled);
	InitStrings(stringsAmb, Ambient);
	InitStrings(stringsEnabled, Enabled);
	InitStrings(stringsPegInTune, PegsInTune);
	InitStrings(stringsPegNotInTune, PegsNotInTune);
	InitStrings(stringsText, Text);
	//InitStrings(stringsPart, Particles);
	InitStrings(stringsBodyNorm, BodyNorm);
	InitStrings(stringsBodyAcc, BodyAcc);
	//InitStrings(stringsBodyPrev, BodyPrev);

	if (Settings::IsTwitchSettingEnabled("SolidNotes")) {
		SetColors(stringsGlow, customSolidColor);
		SetColors(stringsEnabled, customSolidColor);
		SetColors(stringsDisabled, customSolidColor);
		SetColors(stringsAmb, customSolidColor);

		return;
	}

	if (MemHelpers::IsExtendedRangeSong()) {
		if (Settings::GetModSetting("CustomStringColors") == 1) { //Zag's colors
			// Zags custom low B color values manually entered; Normal
			//SetColors(stringsNormal, colorsNormal);

			//std::cout << std::hex << stringsEnabled[0] << std::endl;

			// Zags custom low B color values manually entered; Glowed
			SetColors(stringsGlow, colorsGlow);

			// Zags custom low B color values manually entered; Disabled
			SetColors(stringsDisabled, colorsDisabled);

			//name = "GuitarStringsAmbientColorBlind" id = "3175458924" source = "GameColorManager" >
		//	SetColors(stringsAmb, colorsAmbient);

			//name="GuitarStringsEnabledColorBlind" id="237528906"
			SetColors(stringsEnabled, colorsStrEna);

			//name="GuitarPegsTuningBlind" id="1806691030"
			SetColors(stringsPegNotInTune, colorsPegNotInTune);

			//name="GuitarPegsInTuneBlind" id="2547441015"
			SetColors(stringsPegInTune, colorsPegInTune);

			//name="GuitarRegistrarTextIndicatorBlind" id="3186002004"
			SetColors(stringsText, colorsText);

			//name="GuitarRegistrarForkParticlesBlind" id="3239612871"
			//SetColors(stringsPart, colorsPart);

			// name="NotewayBodyPartsBodyNormBlind" id="3629363565"
			SetColors(stringsBodyNorm, colorsNormal);

			// = name = "NotewayBodyPartsAccentBlind" id = "47948252"
			SetColors(stringsBodyAcc, colorsBodyAcc);

			//= name = "NotewayBodyPartsPreviewBlind" id = "338656387"
			//SetColors(stringsBodyPrev, colorsBodyPrev);
		}
		else if (Settings::GetModSetting("CustomStringColors") == 0) { // Default colors 
			//do we even need to do anything in this case?
		}
		else if (Settings::GetModSetting("CustomStringColors") == 2) { // CB user-defined colors
			SetColors(stringsEnabled, "Enabled_CB");
			SetColors(stringsGlow, "Glow_CB");
			SetColors(stringsDisabled, "Disabled_CB");
			SetColors(stringsText, "TextIndicator_CB");
			SetColors(stringsPegInTune, "PegInTune_CB");
			SetColors(stringsPegNotInTune, "PegOutTune_CB");
			SetColors(stringsBodyNorm, "BodyNorm_CB");
			SetColors(stringsBodyAcc, "BodyAcc_CB");
			//etc.
		}
	}
	else {

		if (!colorsSaved && MemHelpers::GetCurrentMenu() == "LearnASong_Game") { //read only once, so it won't change defaults if you change to CB
			for (int i = 0; i < 6; i++) {
				oldDisabled.push_back(*(Color*)stringsDisabled[i]);
				oldEnabled.push_back(*(Color*)stringsEnabled[i]);
				oldGlow.push_back(*(Color*)stringsGlow[i]);
				oldAmb.push_back(*(Color*)stringsAmb[i]);
			}
			colorsSaved = true;
		}

		if (Settings::GetModSetting("CustomStringColors") == 2) { // User defined colors
			SetColors(stringsEnabled, "Enabled_N");
			SetColors(stringsGlow, "Glow_N");
			SetColors(stringsDisabled, "Disabled_N");
			SetColors(stringsText, "TextIndicator_N");
			SetColors(stringsPegInTune, "PegInTune_N");
			SetColors(stringsPegNotInTune, "PegOutTune_N");
			SetColors(stringsBodyNorm, "BodyNorm_N");
			SetColors(stringsBodyAcc, "BodyAcc_N");
			//etc.
		} // If not enabled, colors will auto-reset upon entering a song
	}

	//NOTE: this overrides string colors, no matter if ER song or not
	if (Settings::GetModSetting("CustomStringColors") == 3) { // If you want the color testing menu to work
		if (saveDefaults) {
			defaultColors.clear();
			for (int idx = 0; idx < 17;idx++) {
				InitStrings(stringsTest, (idx * 0x18 + 0x350));

				std::vector<Color> defaults;
				for (int i = 0; i < 6; i++)
					defaults.push_back(*(Color*)stringsTest[i]);

				defaultColors.push_back(defaults);
			}

			saveDefaults = false;
		}

		if (restoreDefaults) {
			for (int idx = 0; idx < 17;idx++) {
				InitStrings(stringsTest, (idx * 0x18 + 0x350));

				for (int i = 0; i < 6; i++)
					*(Color*)stringsTest[i] = defaultColors[idx][i];
			}

			restoreDefaults = false;
		}
		else {
			stringsTest.clear();
			InitStrings(stringsTest, (currentOffsetIdx * 0x18 + 0x350));

			if (currColor == 0)
				SetColors(stringsTest, colorsBlack);
			else
				SetColors(stringsTest, colorsWhite);
		}

		InitStrings(stringsTest, Text);
		SetColors(stringsTest, colorsTest);
		std::cout << "Set colors doe" << std::endl;
	}
}

void ERMode::ToggleRainbowMode() {
	RainbowEnabled = !RainbowEnabled;
}

bool ERMode::IsRainbowEnabled() {
	return RainbowEnabled;
}

void ERMode::DoRainbow() {
	if (!RainbowEnabled)
		return;

	std::vector<uintptr_t> stringsEnabled;
	std::vector<uintptr_t> stringsHigh;
	std::vector<uintptr_t> stringsDisabled;
	std::vector<Color> oldEnabled, oldHigh, oldDisabled;

	for (int i = 0; i < 6; i++) {
		stringsEnabled.push_back(GetStringColor(i, Enabled));
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
			oldEnabled.push_back(*(Color*)stringsEnabled[i]);
			oldHigh.push_back(*(Color*)stringsHigh[i]);
			oldDisabled.push_back(*(Color*)stringsDisabled[i]);

			c.setH(h + (stringOffset * i));

			*(Color*)stringsEnabled[i] = c;
			*(Color*)stringsHigh[i] = c;
			*(Color*)stringsDisabled[i] = c;
		}

		Sleep(16);
	}

	if (oldEnabled.size() == 0)
		return;

	for (int i = 0; i < 6; i++) {
		*(Color*)stringsEnabled[i] = oldEnabled[i];
		*(Color*)stringsHigh[i] = oldHigh[i];
		*(Color*)stringsDisabled[i] = oldDisabled[i];
	}
}


/* IN CASE THIS IS EVER NEEDED */
bool IsMatch(std::vector<uintptr_t> strings, int R, int G, int B) {
	if (strings[0] == NULL)
		return false;

	int r = std::round((*(Color*)strings[0]).r * 255); // Remember, just casting to int doesn't round up :(
	int g = std::round((*(Color*)strings[0]).g * 255);
	int b = std::round((*(Color*)strings[0]).b * 255);
	if (R == 63)
		std::cout << std::dec << R << " " << r << " " << (R == r) << " " << G << " " << g << " " << (G == g) << " " << B << " " << b << " " << (B == b) << std::endl;

	if (R == r && G == g && B == b)
		return true;
	return false;
}


//std::cout << std::hex << stringsEnabled[0] << std::endl;

/*std::cout << "Normal" << i << " " << (*(Color*)stringsNormal[i]).r * 255 << " " << (*(Color*)stringsNormal[i]).g * 255 << " " << (*(Color*)stringsNormal[i]).b * 255 << std::endl;
std::cout << "Disabled" << i << " " << (*(Color*)stringsDisabled[i]).r * 255 << " " << (*(Color*)stringsDisabled[i]).g * 255 << " " << (*(Color*)stringsDisabled[i]).b * 255 << std::endl;
std::cout << "Enabled" << i << " " << (*(Color*)stringsEnabled[i]).r * 255 << " " << (*(Color*)stringsEnabled[i]).g * 255 << " " << (*(Color*)stringsEnabled[i]).b * 255 << std::endl;
std::cout << "Glow" << i << " " << (*(Color*)stringsGlow[i]).r * 255 << " " << (*(Color*)stringsGlow[i]).g * 255 << " " << (*(Color*)stringsGlow[i]).b * 255 << std::endl;
std::cout << "Amb" << i << " " << (*(Color*)stringsAmb[i]).r * 255 << " " << (*(Color*)stringsAmb[i]).g * 255 << " " << (*(Color*)stringsAmb[i]).b * 255 << std::endl;
std::cout << "PegInTune" << i << " " << (*(Color*)stringsPegInTune[i]).r * 255 << " " << (*(Color*)stringsPegInTune[i]).g * 255 << " " << (*(Color*)stringsPegInTune[i]).b * 255 << std::endl;
std::cout << "PegNotInTune" << i << " " << (*(Color*)stringsPegNotInTune[i]).r * 255 << " " << (*(Color*)stringsPegNotInTune[i]).g * 255 << " " << (*(Color*)stringsPegNotInTune[i]).b * 255 << std::endl;
std::cout << "Text" << i << " " << (*(Color*)stringsText[i]).r * 255 << " " << (*(Color*)stringsText[i]).g * 255 << " " << (*(Color*)stringsText[i]).b * 255 << std::endl;
std::cout << "Part" << i << " " << (*(Color*)stringsPart[i]).r * 255 << " " << (*(Color*)stringsPart[i]).g * 255 << " " << (*(Color*)stringsPart[i]).b * 255 << std::endl;
std::cout << "BodyNorm" << i << " " << (*(Color*)stringsBodyNorm[i]).r * 255 << " " << (*(Color*)stringsBodyNorm[i]).g * 255 << " " << (*(Color*)stringsBodyNorm[i]).b * 255 << std::endl;
std::cout << "BodyAcc" << i << " " << (*(Color*)stringsBodyAcc[i]).r * 255 << " " << (*(Color*)stringsBodyAcc[i]).g * 255 << " " << (*(Color*)stringsBodyAcc[i]).b * 255 << std::endl;
std::cout << "BodyPrev" << i << " " << (*(Color*)stringsBodyPrev[i]).r * 255 << " " << (*(Color*)stringsBodyPrev[i]).g * 255 << " " << (*(Color*)stringsBodyPrev[i]).b * 255 << std::endl;
std::cout << std::endl;*/


/*int a0 = 0, a1 = 0, a2 = 0, a3 = 0;

	for (int i = 0; i < 17;i++) {
		stringsTest.clear();
		int current = 0x350 + i * 0x18;
		InitStrings(stringsTest, current);

		if (IsMatch(stringsTest, 191, 95, 95))
			std::cout << "Ambient " << std::hex << current << std::endl;
		else if (IsMatch(stringsTest, 255, 79, 90))
			std::cout << "Enabled " << std::hex << current << std::endl;
		else if (IsMatch(stringsTest, 76, 23, 27))
			std::cout << "Disabled " << std::hex << current << std::endl;
		else if (IsMatch(stringsTest, 255, 0, 16))
			std::cout << "StringsGlow " << std::hex << current << std::endl;
		else if (IsMatch(stringsTest, 191, 0, 15))
			std::cout << "PegsTuning " << std::hex << current << std::endl;
		else if (IsMatch(stringsTest, 0, 0, 0)) {
			a0++;
			if (a0 == 1)
				std::cout << "PegsReset " << std::hex << current << std::endl;
			else if (a0 == 2)
				std::cout << "PegsSuccess " << std::hex << current << std::endl;
		}
		else if (IsMatch(stringsTest, 255, 255, 255))
			std::cout << "PegsOutOfTune " << std::hex << current << std::endl;
		else if (IsMatch(stringsTest, 249, 146, 137))
			std::cout << "BodypartsAccent " << std::hex << current << std::endl;
		else if (IsMatch(stringsTest, 178, 0, 14))
			std::cout << "PegsInTune " << std::hex << current << std::endl;
		else if (IsMatch(stringsTest, 255, 64, 64)) {
			a2++;
			if (a2 == 1)
				std::cout << "RegistarTextIndicator " << std::hex << current << std::endl;
			else if (a2 == 2)
				std::cout << "ForkParticles " << std::hex << current << std::endl;
		}
		else if (IsMatch(stringsTest, 64, 41, 41))
			std::cout << "BodypartsPreview " << std::hex << current << std::endl;
		else if (IsMatch(stringsTest, 255, 0, 0))
			std::cout << "BodypartsNormal " << std::hex << current << std::endl;
		else if (IsMatch(stringsTest, 255, 5, 0)) {
			a3++;
			if (a3 == 1)
				std::cout << "GA_Main " << std::hex << current << std::endl;
			else if (a3 == 2)
				std::cout << "GA_Additive " << std::hex << current << std::endl;
			else if (a3 == 3)
				std::cout << "GA_UI " << std::hex << current << std::endl;
		}
	} */

	/*for (int i = 0; i < 6;i++) {
		std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)((*(Color*)stringsGlow[i]).r * 255);
		std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)((*(Color*)stringsGlow[i]).g * 255);
		std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)((*(Color*)stringsGlow[i]).b * 255);
		std::cout << std::endl;
	}*/