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


std::vector<String> strings;
void cERMode::Toggle7StringMode() { //TODO: use the GUI to make DDS files and load settings here for matching string colors
	// return;
	strings.clear();
	std::vector<Color> oldColors;

	static bool colorsSaved = false;
	uintptr_t string0Normal = GetStringColor(0, Normal); uintptr_t string1Normal = GetStringColor(1, Normal); uintptr_t string2Normal = GetStringColor(2, Normal); uintptr_t string3Normal = GetStringColor(3, Normal); uintptr_t string4Normal = GetStringColor(4, Normal); uintptr_t string5Normal = GetStringColor(5, Normal);
	uintptr_t string0Glow = GetStringColor(0, Glow); uintptr_t string1Glow = GetStringColor(1, Glow); uintptr_t string2Glow = GetStringColor(2, Glow); uintptr_t string3Glow = GetStringColor(3, Glow); uintptr_t string4Glow = GetStringColor(4, Glow); uintptr_t string5Glow = GetStringColor(5, Glow); 
	uintptr_t string0Disabled = GetStringColor(0, Disabled); uintptr_t string1Disabled = GetStringColor(1, Disabled); uintptr_t string2Disabled = GetStringColor(2, Disabled); uintptr_t string3Disabled = GetStringColor(3, Disabled); uintptr_t string4Disabled = GetStringColor(4, Disabled); uintptr_t string5Disabled = GetStringColor(5, Disabled); 
	uintptr_t string0Amb = GetStringColor(0, Ambient); uintptr_t string1Amb = GetStringColor(1, Ambient); uintptr_t string2Amb = GetStringColor(2, Ambient); uintptr_t string3Amb = GetStringColor(3, Ambient); uintptr_t string4Amb = GetStringColor(4, Ambient); uintptr_t string5Amb = GetStringColor(5, Ambient);
	uintptr_t string0Enabled = GetStringColor(0, Enabled); uintptr_t string1Enabled = GetStringColor(1, Enabled); uintptr_t string2Enabled = GetStringColor(2, Enabled); uintptr_t string3Enabled = GetStringColor(3, Enabled); uintptr_t string4Enabled = GetStringColor(4, Enabled); uintptr_t string5Enabled = GetStringColor(5, Enabled);
	uintptr_t string0PegNotInTune = GetStringColor(0, PegNotInTune); uintptr_t string1PegNotInTune = GetStringColor(1, PegNotInTune); uintptr_t string2PegNotInTune = GetStringColor(2, PegNotInTune); uintptr_t string3PegNotInTune = GetStringColor(3, PegNotInTune); uintptr_t string4PegNotInTune = GetStringColor(4, PegNotInTune); uintptr_t string5PegNotInTune = GetStringColor(5, PegNotInTune);
	uintptr_t string0PegInTune = GetStringColor(0, PegInTune); uintptr_t string1PegInTune = GetStringColor(1, PegInTune); uintptr_t string2PegInTune = GetStringColor(2, PegInTune); uintptr_t string3PegInTune = GetStringColor(3, PegInTune); uintptr_t string4PegInTune = GetStringColor(4, PegInTune); uintptr_t string5PegInTune = GetStringColor(5, PegInTune);
	uintptr_t string0TextNew = GetStringColor(0, Text); uintptr_t string1TextNew = GetStringColor(1, Text); uintptr_t string2TextNew = GetStringColor(2, Text); uintptr_t string3TextNew = GetStringColor(3, Text); uintptr_t string4TextNew = GetStringColor(4, Text); uintptr_t string5TextNew = GetStringColor(5, Text);
	uintptr_t string0Part = GetStringColor(0, Particles); uintptr_t string1Part = GetStringColor(1, Particles); uintptr_t string2Part = GetStringColor(2, Particles); uintptr_t string3Part = GetStringColor(3, Particles); uintptr_t string4Part = GetStringColor(4, Particles); uintptr_t string5Part = GetStringColor(5, Particles);
	uintptr_t string0BodyNorm = GetStringColor(0, BodyNorm); uintptr_t string1BodyNorm = GetStringColor(1, BodyNorm); uintptr_t string2BodyNorm = GetStringColor(2, BodyNorm); uintptr_t string3BodyNorm = GetStringColor(3, BodyNorm); uintptr_t string4BodyNorm = GetStringColor(4, BodyNorm); uintptr_t string5BodyNorm = GetStringColor(5, BodyNorm);
	uintptr_t string0BodyAcc = GetStringColor(0, BodyAcc); uintptr_t string1BodyAcc = GetStringColor(1, BodyAcc); uintptr_t string2BodyAcc = GetStringColor(2, BodyAcc); uintptr_t string3BodyAcc = GetStringColor(3, BodyAcc); uintptr_t string4BodyAcc = GetStringColor(4, BodyAcc); uintptr_t string5BodyAcc = GetStringColor(5, BodyAcc);
	uintptr_t string0BodyPrev = GetStringColor(0, BodyPrev); uintptr_t string1BodyPrev = GetStringColor(1, BodyPrev); uintptr_t string2BodyPrev = GetStringColor(2, BodyPrev); uintptr_t string3BodyPrev = GetStringColor(3, BodyPrev); uintptr_t string4BodyPrev = GetStringColor(4, BodyPrev); uintptr_t string5BodyPrev = GetStringColor(5, BodyPrev);
	
	if (!colorsSaved && MemHelpers.GetCurrentMenu() == "LearnASong_Game") {
		cNormal = *(Color*)string0Normal; //still not sure what "Normal" corresponds to in the games flat files
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
		cBodyPrev = *(Color*)string0BodyPrev;
		

	}

	if(MemHelpers.IsExtendedRangeSong()){
		// Zags custom low B color values manually entered; Normal
		Color c0NormalNew; Color c1NormalNew; Color c2NormalNew; Color c3NormalNew; Color c4NormalNew; Color c5NormalNew; 
		c0NormalNew.r = 0.0; c0NormalNew.g = 0.7764706; c0NormalNew.b = 0.5568628;
		c1NormalNew.r = 0.7490196; c1NormalNew.g = 0.372549; c1NormalNew.b = 0.372549;
		c2NormalNew.r = 0.7490196; c2NormalNew.g = 0.7176471; c2NormalNew.b = 0.372549;
		c3NormalNew.r = 0.3686275; c3NormalNew.g = 0.6156863; c3NormalNew.b = 0.7372549;
		c4NormalNew.r = 0.7490196; c4NormalNew.g = 0.5529412; c4NormalNew.b = 0.372549;
		c5NormalNew.r = 0.372549; c5NormalNew.g = 0.7490196; c5NormalNew.b = 0.5921569;
		*(Color*)string0Normal = c0NormalNew; *(Color*)string1Normal = c1NormalNew;	*(Color*)string2Normal = c2NormalNew;	*(Color*)string3Normal = c3NormalNew;	*(Color*)string4Normal = c4NormalNew;	*(Color*)string5Normal = c5NormalNew;
		
		// Zags custom low B color values manually entered; Glowed
		Color c0GlowNew; Color c1GlowNew; Color c2GlowNew; Color c3GlowNew; Color c4GlowNew; Color c5GlowNew; 
		c0GlowNew.r = 0.0; c0GlowNew.g = 1; c0GlowNew.b = 0.6470588;
		c1GlowNew.r = 1; c1GlowNew.g = 0; c1GlowNew.b = 0.0627451;
		c2GlowNew.r = 1; c2GlowNew.g = 0.7803922; c2GlowNew.b = 0;
		c3GlowNew.r = 0; c3GlowNew.g = 0.6627451; c3GlowNew.b = 1;
		c4GlowNew.r = 1; c4GlowNew.g = 0.4470588; c4GlowNew.b = 0;
		c5GlowNew.r = 0.2627451; c5GlowNew.g = 1; c5GlowNew.b = 0;
		*(Color*)string0Glow = c0GlowNew; *(Color*)string1Glow = c1GlowNew;	*(Color*)string2Glow = c2GlowNew;	*(Color*)string3Glow = c3GlowNew;	*(Color*)string4Glow = c4GlowNew;	*(Color*)string5Glow = c5GlowNew;
		
		// Zags custom low B color values manually entered; Disabled
		Color c0DisabledNew; Color c1DisabledNew; Color c2DisabledNew; Color c3DisabledNew; Color c4DisabledNew; Color c5DisabledNew; 
		c0DisabledNew.r = 0.0;	c0DisabledNew.g = 0.2980392; c0DisabledNew.b = 0.2117647;
		c1DisabledNew.r = 0.2980392; c1DisabledNew.g = 0.09019608; c1DisabledNew.b = 0.1058824;
		c2DisabledNew.r = 0.2980392; c2DisabledNew.g = 0.254902; c2DisabledNew.b = 0;
		c3DisabledNew.r = 0.03529412; c3DisabledNew.g = 0.2117647; c3DisabledNew.b = 0.2980392;
		c4DisabledNew.r = 0.2980392; c4DisabledNew.g = 0.1647059; c4DisabledNew.b = 0.02352941;
		c5DisabledNew.r = 0.09019608; c5DisabledNew.g = 0.2980392; c5DisabledNew.b = 0.01568628;
		*(Color*)string0Disabled = c0DisabledNew; *(Color*)string1Disabled = c1DisabledNew;	*(Color*)string2Disabled = c2DisabledNew;	*(Color*)string3Disabled = c3DisabledNew;	*(Color*)string4Disabled = c4DisabledNew;	*(Color*)string5Disabled = c5DisabledNew;

		//name = "GuitarStringsAmbientColorBlind" id = "3175458924" source = "GameColorManager" >
		
		Color c0AmbNew; Color c1AmbNew; Color c2AmbNew; Color c3AmbNew; Color c4AmbNew; Color c5AmbNew;
		c0AmbNew.r = 0.372549; c0AmbNew.g = 0.7490196; c0AmbNew.b = 0.6431373;
		c1AmbNew.r = 0.7490196; c1AmbNew.g = 0.372549; c1AmbNew.b = 0.372549;
		c2AmbNew.r = 0.7490196; c2AmbNew.g = 0.7176471; c2AmbNew.b = 0.372549;
		c3AmbNew.r = 0.3686275; c3AmbNew.g = 0.6156863; c3AmbNew.b = 0.7372549;
		c4AmbNew.r = 0.7490196; c4AmbNew.g = 0.5529412; c4AmbNew.b = 0.372549;
		c5AmbNew.r = 0.372549; c5AmbNew.g = 0.7490196; c5AmbNew.b = 0.5921569;
		*(Color*)string0Amb = c0AmbNew; *(Color*)string1Amb = c1AmbNew;	*(Color*)string2Amb = c2AmbNew;	*(Color*)string3Amb = c3AmbNew;	*(Color*)string4Amb = c4AmbNew;	*(Color*)string5Amb = c5AmbNew;
		
		//name="GuitarStringsEnabledColorBlind" id="237528906"
			
		Color c0StrEnaNew; Color c1StrEnaNew; Color c2StrEnaNew; Color c3StrEnaNew; Color c4StrEnaNew; Color c5StrEnaNew;
		c0StrEnaNew.r = 0; c0StrEnaNew.g = 0.7764706; c0StrEnaNew.b = 0.5568628;
		c1StrEnaNew.r = 1; c1StrEnaNew.g = 0.3098039; c1StrEnaNew.b = 0.3529412;
		c2StrEnaNew.r = 0.8862745; c2StrEnaNew.g = 0.7568628; c2StrEnaNew.b = 0.007843138;
		c3StrEnaNew.r = 0.1137255; c3StrEnaNew.g = 0.6745098; c3StrEnaNew.b = 0.9764706;
		c4StrEnaNew.r = 1; c4StrEnaNew.g = 0.572549; c4StrEnaNew.b = 0.08627451;
		c5StrEnaNew.r = 0.2470588; c5StrEnaNew.g = 0.8; c5StrEnaNew.b = 0.04705882;
		*(Color*)string0Enabled = c0StrEnaNew; *(Color*)string1Enabled = c1StrEnaNew;	*(Color*)string2Enabled = c2StrEnaNew;	*(Color*)string3Enabled = c3StrEnaNew;	*(Color*)string4Enabled = c4StrEnaNew;	*(Color*)string5Enabled = c5StrEnaNew;
		
		 //name="GuitarPegsTuningBlind" id="1806691030"
		
		Color c0PegNotInTuneNew; Color c1PegNotInTuneNew; Color c2PegNotInTuneNew; Color c3PegNotInTuneNew; Color c4PegNotInTuneNew; Color c5PegNotInTuneNew;
		c0PegNotInTuneNew.r = 0.003921569; c0PegNotInTuneNew.g = 0.6; c0PegNotInTuneNew.b = 0.4117647;
		c1PegNotInTuneNew.r = 0.7490196; c1PegNotInTuneNew.g = 0; c1PegNotInTuneNew.b = 0.05882353;
		c2PegNotInTuneNew.r = 0.6862745; c2PegNotInTuneNew.g = 0.4588235; c2PegNotInTuneNew.b = 0;
		c3PegNotInTuneNew.r = 0; c3PegNotInTuneNew.g = 0.3764706; c3PegNotInTuneNew.b = 0.7294118;
		c4PegNotInTuneNew.r = 0.6980392; c4PegNotInTuneNew.g = 0.2196078; c4PegNotInTuneNew.b = 0;
		c5PegNotInTuneNew.r = 0.1254902; c5PegNotInTuneNew.g = 0.5490196; c5PegNotInTuneNew.b = 0;
		*(Color*)string0PegNotInTune = c0PegNotInTuneNew; *(Color*)string1PegNotInTune = c1PegNotInTuneNew;	*(Color*)string2PegNotInTune = c2PegNotInTuneNew;	*(Color*)string3PegNotInTune = c3PegNotInTuneNew;	*(Color*)string4PegNotInTune = c4PegNotInTuneNew;	*(Color*)string5PegNotInTune = c5PegNotInTuneNew;
		
		 //name="GuitarPegsInTuneBlind" id="2547441015"
		
		Color c0PegInTuneNew; Color c1PegInTuneNew; Color c2PegInTuneNew; Color c3PegInTuneNew; Color c4PegInTuneNew; Color c5PegInTuneNew; 
		c0PegInTuneNew.r = 0; c0PegInTuneNew.g = 0.6980392; c0PegInTuneNew.b = 0.4745098;
		c1PegInTuneNew.r = 0.6980392; c1PegInTuneNew.g = 0; c1PegInTuneNew.b = 0.05490196;
		c2PegInTuneNew.r = 0.6980392; c2PegInTuneNew.g = 0.5568628; c2PegInTuneNew.b = 0;
		c3PegInTuneNew.r = 0; c3PegInTuneNew.g = 0.4627451; c3PegInTuneNew.b = 0.6980392;
		c4PegInTuneNew.r = 0.6980392; c4PegInTuneNew.g = 0.2196078; c4PegInTuneNew.b = 0;
		c5PegInTuneNew.r = 0.1843137; c5PegInTuneNew.g = 0.6980392; c5PegInTuneNew.b = 0;
		*(Color*)string0PegInTune = c0PegInTuneNew; *(Color*)string1PegInTune = c1PegInTuneNew;	*(Color*)string2PegInTune = c2PegInTuneNew;	*(Color*)string3PegInTune = c3PegInTuneNew;	*(Color*)string4PegInTune = c4PegInTuneNew;	*(Color*)string5PegInTune = c5PegInTuneNew;
		

		 //name="GuitarRegistrarTextIndicatorBlind" id="3186002004"
			
		Color c0TextNew; Color c1TextNew; Color c2TextNew; Color c3TextNew; Color c4TextNew; Color c5TextNew;
		c0TextNew.r = 0.2470588; c0TextNew.g = 1; c0TextNew.b = 0.7843137;
		c1TextNew.r = 1; c1TextNew.g = 0.2509804; c1TextNew.b = 0.2509804;
		c2TextNew.r = 1; c2TextNew.g = 1; c2TextNew.b = 0.2509804;
		c3TextNew.r = 0.2509804; c3TextNew.g = 0.5019608; c3TextNew.b = 1;
		c4TextNew.r = 1; c4TextNew.g = 0.627451; c4TextNew.b = 0.2509804;
		c5TextNew.r = 0.2509804; c5TextNew.g = 1; c5TextNew.b = 0.2509804;
		*(Color*)string0TextNew = c0TextNew; *(Color*)string1TextNew = c1TextNew;	*(Color*)string2TextNew = c2TextNew;	*(Color*)string3TextNew = c3TextNew;	*(Color*)string4TextNew = c4TextNew;	*(Color*)string5TextNew = c5TextNew;
		
		//name="GuitarRegistrarForkParticlesBlind" id="3239612871"
		
		Color c0PartNew; Color c1PartNew; Color c2PartNew; Color c3PartNew; Color c4PartNew; Color c5PartNew;
		c0PartNew.r = 0.2470588; c0PartNew.g = 1; c0PartNew.b = 0.7843137;
		c1PartNew.r = 1; c1PartNew.g = 0.2509804; c1PartNew.b = 0.2509804;
		c2PartNew.r = 1; c2PartNew.g = 1; c2PartNew.b = 0.2509804;
		c3PartNew.r = 0.2509804; c3PartNew.g = 0.5019608; c3PartNew.b = 1;
		c4PartNew.r = 1; c4PartNew.g = 0.627451; c4PartNew.b = 0.2509804;
		c5PartNew.r = 0.2509804; c5PartNew.g = 1; c5PartNew.b = 0.2509804;
		*(Color*)string0Part = c0PartNew; *(Color*)string1Part = c1PartNew;	*(Color*)string2Part = c2PartNew;	*(Color*)string3Part = c3PartNew;	*(Color*)string4Part = c4PartNew;	*(Color*)string5Part = c5PartNew;
			
		// name="NotewayBodyPartsBodyNormBlind" id="3629363565"

		Color c0BodyNormNew; Color c1BodyNormNew; Color c2BodyNormNew; Color c3BodyNormNew; Color c4BodyNormNew; Color c5BodyNormNew;
		c0BodyNormNew.r = 0; c0BodyNormNew.g = 0.7764706; c0BodyNormNew.b = 0.5568628;
		c1BodyNormNew.r = 1; c1BodyNormNew.g = 0; c1BodyNormNew.b = 0;
		c2BodyNormNew.r = 1; c2BodyNormNew.g = 1; c2BodyNormNew.b = 0;
		c3BodyNormNew.r = 0; c3BodyNormNew.g = 0.1176471; c3BodyNormNew.b = 1;
		c4BodyNormNew.r = 1; c4BodyNormNew.g = 0.5450981; c4BodyNormNew.b = 0.1843137;
		c5BodyNormNew.r = 0; c5BodyNormNew.g = 1; c5BodyNormNew.b = 0.1764706;
		*(Color*)string0BodyNorm = c0BodyNormNew; *(Color*)string1BodyNorm = c1BodyNormNew;	*(Color*)string2BodyNorm = c2BodyNormNew;	*(Color*)string3BodyNorm = c3BodyNormNew;	*(Color*)string4BodyNorm = c4BodyNormNew;	*(Color*)string5BodyNorm = c5BodyNormNew;
			
		// = name = "NotewayBodyPartsAccentBlind" id = "47948252"
		
		Color c0BodyAccNew; Color c1BodyAccNew; Color c2BodyAccNew; Color c3BodyAccNew; Color c4BodyAccNew; Color c5BodyAccNew;
		c0BodyAccNew.r = 1; c0BodyAccNew.g = 1; c0BodyAccNew.b = 1;
		c1BodyAccNew.r = 0.9764706; c1BodyAccNew.g = 0.572549; c1BodyAccNew.b = 0.5372549;
		c2BodyAccNew.r = 0.9843137; c2BodyAccNew.g = 0.9490196; c2BodyAccNew.b = 0.5843138;
		c3BodyAccNew.r = 0.5490196; c3BodyAccNew.g = 0.9098039; c3BodyAccNew.b = 0.9921569;
		c4BodyAccNew.r = 0.9882353; c4BodyAccNew.g = 0.8196079; c4BodyAccNew.b = 0.5254902;
		c5BodyAccNew.r = 0.6901961; c5BodyAccNew.g = 0.9568627; c5BodyAccNew.b = 0.5137255;
		*(Color*)string0BodyAcc = c0BodyAccNew; *(Color*)string1BodyAcc = c1BodyAccNew;	*(Color*)string2BodyAcc = c2BodyAccNew;	*(Color*)string3BodyAcc = c3BodyAccNew;	*(Color*)string4BodyAcc = c4BodyAccNew;	*(Color*)string5BodyAcc = c5BodyAccNew;
		
		//= name = "NotewayBodyPartsPreviewBlind" id = "338656387"
		
		Color c0BodyPrevNew; Color c1BodyPrevNew; Color c2BodyPrevNew; Color c3BodyPrevNew; Color c4BodyPrevNew; Color c5BodyPrevNew;
		c0BodyPrevNew.r = 0.1607843; c0BodyPrevNew.g = 0.2470588; c0BodyPrevNew.b = 0.2235294;
		c1BodyPrevNew.r = 0.2509804; c1BodyPrevNew.g = 0.1607843; c1BodyPrevNew.b = 0.1607843;
		c2BodyPrevNew.r = 0.2509804; c2BodyPrevNew.g = 0.227451; c2BodyPrevNew.b = 0.1607843;
		c3BodyPrevNew.r = 0.1607843; c3BodyPrevNew.g = 0.2078431; c3BodyPrevNew.b = 0.2509804;
		c4BodyPrevNew.r = 0.2509804; c4BodyPrevNew.g = 0.2; c4BodyPrevNew.b = 0.1607843;
		c5BodyPrevNew.r = 0.1647059; c5BodyPrevNew.g = 0.2509804; c5BodyPrevNew.b = 0.1607843;
		*(Color*)string0BodyPrev = c0BodyPrevNew; *(Color*)string1BodyPrev = c1BodyPrevNew;	*(Color*)string2BodyPrev = c2BodyPrevNew;	*(Color*)string3BodyPrev = c3BodyPrevNew;	*(Color*)string4BodyPrev = c4BodyPrevNew;	*(Color*)string5BodyPrev = c5BodyPrevNew;
		 

		/* Color cNnew, cDisabledNew, cGlowNew; // existing code. NFI what it did...
		cNnew = cN;
		cNnew.setH(5);
		cDisabledNew = cD;
		cDisabledNew.setH(5);
		cGlowNew = cH;
		cGlowNew.setH(5);

		*(Color*)string0n = cNnew;
		
		*(Color*)string0h = cGlowNew;
		
		*(Color*)string0d = cDisabledNew;
		*/
		

		
		
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
