#pragma once

enum StringState {
	Ambient = 0x350,
	Enabled = 0x368,
	Disabled = 0x380,
	Glow = 0x398,
	PegsTuning = 0x3b0,
	PegsReset = 0x3c8,
	PegsInactive = 0x3e0,
	PegsInTune = 0x3f8,
	PegsNotInTune = 0x410,
	OddHighlightedFretNumber = 0x428,
	EvenHighlightedFretNumber = 0x440, //Particles?
	FretWire = 0x458,
	Text = 0x488,
	BodyNorm = 0x4b8,
	BodyAcc = 0x4d0
	/*Ambient = 0x350, //name = "GuitarStringsAmbientColorBlind" id = "3175458924
	Enabled = 0x368, //name="GuitarStringsEnabledColorBlind" id="237528906"
	Disabled = 0x380,  //name="GuitarStringsDisabledColorBlind" id="4184626561"
	Glow = 0x398, //name="GuitarStringsGlowBlind" id="3217126622
	PegsTuning = 0x3b0, //name = "GuitarPegsTuningBlind"id = "1806691030"
	PegsSuccess = 0x3c8, //name = "GuitarPegsSuccessBlind" id = "2074129191
	PegsReset = 0x3e0, //name = "GuitarPegsResetBlind"id = "2670600760"
	PegInTune = 0x3f8,  //name="GuitarPegsInTuneBlind" id="2547441015"
	PegNotInTune = 0x410, //name = "GuitarPegsOutTuneBlind" id = "1163525281"
	Text = 0x470, //name="GuitarRegistrarTextIndicatorBlind" id="3186002004"
	Particles = 0x488, //name="GuitarRegistrarForkParticlesBlind" id="3239612871"
	BodyAcc = 0x4a0, // = name = "NotewayBodypartsAccentBlind" id = "47948252"
	BodyNorm = 0x4b8 //= name = "NotewayBodypartsPreviewBlind" id = "338656387"
	//BodyPrev == ?? - doesn't seem to be used while in a song*/

	// GC stuff not used, becuase can't use GC in non E standard anyway, 
	//but including theoretical values for the "replace all and self defined CB mode options.

	/*
	in theory based on order within gamecolormanager.flat and adding 18 hex to each address;
	350 - start point may be off however....
	368 - name="GuitarStringsAmbientColorBlind"id="3175458924"
	380 - name="GuitarStringsEnabledColorBlind"id="237528906"
	398 - name="GuitarStringsDisabledColorBlind"id="4184626561"
	3b0 - name="GuitarStringsGlowBlind"id="3217126622"

	3c8 - name="GuitarPegsTuningBlind"id="1806691030"
	3e0 - name="GuitarPegsResetBlind"id="2670600760"
	3f8 - name="GuitarPegsSuccessBlind"id="2074129191"

	410 - name="GuitarPegsInTuneBlind"id="2547441015"
	428 - name="GuitarPegsOutTuneBlind"id="1163525281"
	440 - name="GuitarRegistrarTextIndicatorBlind"id="3186002004"
	458 - name="GuitarRegistrarForkParticlesBlind"id="3239612871"
	470 - name="NotewayBodypartsNormalBlind"id="3629363565"
	488 - name="NotewayBodypartsAccentBlind"id="47948252"
	4a0 - name="NotewayBodypartsPreviewBlind"id="338656387"

	4b8 - name="GuitarcadeMainBlind"id="2332417892"
	4d0 - name="GuitarcadeAdditiveBlind"id="1537455274"
	4e8 - name="GuitarcadeUIBlind"id="2107771575"
	*/
};