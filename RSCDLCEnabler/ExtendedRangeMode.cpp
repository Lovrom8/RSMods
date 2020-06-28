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

/*
  <property
	type="AssocColor"
	name="GuitarStringsEnabledDefault"
	id="3307419348" source="GameColorManager">
	<set index="0" value="1, 0.3098039, 0.3529412" />
	<set index="1" value="0.8862745, 0.7568628, 0.007843138" />
	<set index="2" value="0.1137255, 0.6745098, 0.9764706" />
	<set index="3" value="1, 0.572549, 0.08627451" />
	<set index="4" value="0.2470588, 0.8, 0.04705882" />
	<set index="5" value="0.7843137, 0.145098, 0.9294118" />
  </property>
  <property
	type="AssocColor"
	name="GuitarStringsEnabledColorBlind"
	id="237528906" source="GameColorManager">
	<set index="0" value="0, 0.7764706, 0.5568628" />
	<set index="1" value="1, 0.3098039, 0.3529412" />
	<set index="2" value="0.8862745, 0.7568628, 0.007843138" />
	<set index="3" value="0.1137255, 0.6745098, 0.9764706" />
	<set index="4" value="1, 0.572549, 0.08627451" />
	<set index="5" value="0.2470588, 0.8, 0.04705882" />
  </property>
  
 */


Color cN;
Color cD;
Color cH;


std::vector<String> strings;
void cERMode::Toggle7StringMode() { //TODO: use the GUI to make DDS files and load settings here for matching string colors
	return;
	strings.clear();
	std::vector<Color> oldColors;

	static bool colorsSaved = false;
	uintptr_t string0n = GetStringColor(0, normal);
	uintptr_t string0h = GetStringColor(0, highlight);
	uintptr_t string0d = GetStringColor(0, disabled);

	if (!colorsSaved && MemHelpers.GetCurrentMenu() == "LearnASong_Game") {
		cN = *(Color*)string0n;
		cD = *(Color*)string0d;
		cH = *(Color*)string0h;
	}

	if(MemHelpers.IsExtendedRangeSong()){
		Color cNnew, cDnew, cHnew;
		cNnew = cN;
		cNnew.setH(5);
		cDnew = cD;
		cDnew.setH(5);
		cHnew = cH;
		cHnew.setH(5);

		*(Color*)string0n = cNnew;
		
		*(Color*)string0h = cHnew;
		
		*(Color*)string0d = cDnew;
	}
	else {
	
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
		stringsNormal.push_back(GetStringColor(i, normal));
		stringsHigh.push_back(GetStringColor(i, highlight));
		stringsDisabled.push_back(GetStringColor(i, disabled));
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
