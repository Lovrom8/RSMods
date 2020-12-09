#include "MemHelpers.hpp"

byte MemHelpers::getLowestStringTuning() {
	uintptr_t addrTuning = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_tuning, Offsets::ptr_tuningOffsets);

	if (!addrTuning)
		return NULL;

	if (Settings::ReturnSettingValue("ExtendedRangeDropTuning") == "on")
		return (*(Tuning*)addrTuning).lowE;

	return (*(Tuning*)addrTuning).strA;
}

byte* MemHelpers::GetCurrentTuning(bool verbose) {
	uintptr_t addrTuning = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_tuning, Offsets::ptr_tuningOffsets);

	if (!addrTuning)
		return NULL;

	byte* AllTunings = new byte[6]{ (*(Tuning*)addrTuning).lowE, (*(Tuning*)addrTuning).strA, (*(Tuning*)addrTuning).strD, (*(Tuning*)addrTuning).strG, (*(Tuning*)addrTuning).strB, (*(Tuning*)addrTuning).highE };

	if (verbose) {
		for (int i = 0; i < 6; i++)
			std::cout << "String" << i << " - " << (int)AllTunings[i] << std::endl;
	}

	return AllTunings;
}

// In the tuner menu only, the current tuning will be saved in the string which we can cross-reference to the JSON list of tunings 
Tuning MemHelpers::GetTuningAtTuner() {
	std::string pathToTuningList = "RSMods/CustomMods/tuning.database.json";

	if (!std::filesystem::exists(pathToTuningList)) // If we can't find the list of tunings, just return a default value
		return Tuning();

	uintptr_t addrTuningText = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_tuningText, Offsets::ptr_tuningTextOffsets);

	if (!addrTuningText)
		return Tuning();

	std::string unsanitized_tuningText = std::string((const char*)addrTuningText);

	while (unsanitized_tuningText.find("\xe2\x99\xaf") != std::string::npos) { // Unicode # (sharp)
		size_t badHash = unsanitized_tuningText.find("\xe2\x99\xaf");
		std::string partOne = unsanitized_tuningText.substr(0, badHash);
		std::string partTwo = unsanitized_tuningText.substr(badHash + 2, unsanitized_tuningText.length() - 1);
		unsanitized_tuningText = partOne + partTwo;
		unsanitized_tuningText.at(badHash) = '#';
	}
	while (unsanitized_tuningText.find('\xe2\x99\xad') != std::string::npos) { // Unicode b (flat)
		size_t badFlat = unsanitized_tuningText.find("\xe2\x99\xad");
		std::string partOne = unsanitized_tuningText.substr(0, badFlat);
		std::string partTwo = unsanitized_tuningText.substr(badFlat + 2, unsanitized_tuningText.length() - 1);
		unsanitized_tuningText = partOne + partTwo;
		unsanitized_tuningText.at(badFlat) = 'b';
	}

	std::string tuningText = unsanitized_tuningText;

	if (tuningText == (std::string)"CUSTOM TUNING")
		return Tuning();

	tuningText.erase(std::remove_if(tuningText.begin(), tuningText.end(), isspace), tuningText.end());  // In the JSON, tunings have no whitespaces, so get rid of them

	std::ifstream jsonFile(pathToTuningList);
	nlohmann::json tuningJson;
	jsonFile >> tuningJson;

	jsonFile.close();
	tuningJson = tuningJson["Static"]["TuningDefinitions"]; // Skip directly to the part we are interested in 

	for (auto& tuning : tuningJson.items()) { // Unforunately we can't use json.contains due to difference in formatting
		std::string jsonKeyUpper = tuning.key(), jsonKeyOriginal = tuning.key(); // Also you can't just make a separate copy of the uppercase string, so we keep both 
		std::transform(jsonKeyUpper.begin(), jsonKeyUpper.end(), jsonKeyUpper.begin(), ::toupper);

		if (jsonKeyUpper == tuningText) {
			tuningJson = tuningJson[jsonKeyOriginal]["Strings"];

			return Tuning(tuningJson["string0"], tuningJson["string1"], tuningJson["string2"], tuningJson["string3"], tuningJson["string4"], tuningJson["string5"]);
		}	
	}

	return Tuning();
}

bool MemHelpers::IsExtendedRangeSong() {
	uintptr_t addrTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timer, Offsets::ptr_timerOffsets);

	if (!addrTimer)
		return false;

	/*
	Old method:
	
	byte currentTuning = MemHelpers::getLowestStringTuning();

	if (currentTuning > 24 && currentTuning <= (256 + Settings::GetModSetting("ExtendedRangeMode")))
		return true;
	return false;
	
	Below is the new method
	*/

	int* highestLowestTuning = MemHelpers::GetHighestLowestString();

	int lowestTuning = highestLowestTuning[1];

	delete[] highestLowestTuning;

	// Does the user's settings allow us to toggle on drop tunings (ER on B, trigger on C# Drop B)
	if (Settings::ReturnSettingValue("ExtendedRangeDropTuning") == "on" && lowestTuning <= Settings::GetModSetting("ExtendedRangeMode"))
		return true;

	// Does the user's settings allow us to toggle Exteneded Range Mode for this tuning
	if (lowestTuning < Settings::GetModSetting("ExtendedRangeMode"))
		return true;

	return false;
}

bool MemHelpers::IsExtendedRangeTuner() {
	uintptr_t addrTuningText = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_tuningText, Offsets::ptr_tuningTextOffsets);

	if (!addrTuningText) {
		std::cout << "Wait For Tuning Text" << std::endl;
		return false;
	}
		

	Tuning tuner_songTuning = GetTuningAtTuner();

	if (tuner_songTuning.lowE == 1024) {
		std::cout << "No Tuning Found" << std::endl;
		return false;
	}
		
	int lowestTuning = tuner_songTuning.lowE;

	if (lowestTuning > 24)
		lowestTuning -= 256;

	// Does the user's settings allow us to toggle on drop tunings (ER on B, trigger on C# Drop B)
	if (Settings::ReturnSettingValue("ExtendedRangeDropTuning") == "on" && lowestTuning <= Settings::GetModSetting("ExtendedRangeMode"))
		return true;

	// Does the user's settings allow us to toggle Exteneded Range Mode for this tuning
	if (lowestTuning < Settings::GetModSetting("ExtendedRangeMode"))
		return true;

	std::cout << "Tunings: " << lowestTuning << " vs " << Settings::GetModSetting("ExtendedRangeMode") << std::endl;
	return false;
}

// [0] - Highest, [1] - Lowest
int* MemHelpers::GetHighestLowestString() {
	int highestTuning = 0, lowestTuning = 256, currentStringTuning = 0;
	std::unique_ptr<byte[]> songTuning = std::unique_ptr<byte[]>(MemHelpers::GetCurrentTuning());

	// Get Highest And Lowest Strings
	for (int i = 0; i < 6; i++) {

		// Create a buffer so we can work on a value near 256, and not worry about the tunings at, and above, E Standard that start at 0 because the tuning number breaks the 256 limit of a unsigned char.
		currentStringTuning = (int)songTuning[i];

		// 24 would be 2 octaves above E standard which is where RSMods cuts the tuning numbers at. Anything above maybe +3 should never be used, but for consistency we allow it.
		if (currentStringTuning <= 24)
			currentStringTuning += 256;

		// Find the highest tuned string.
		if (currentStringTuning > highestTuning)
			highestTuning = currentStringTuning;

		// Find the lowest tuned string.
		if (currentStringTuning < lowestTuning)
			lowestTuning = currentStringTuning;
	}

	// Change tuning number (255 = Eb Standard, 254 D Standard, etc) to drop number (-1 = Eb Standard, -2 D Standard, etc).
	if (highestTuning != 0)
		highestTuning -= 256;
	if (lowestTuning != 0)
		lowestTuning -= 256;

	int* returnValue = new int[2]{ highestTuning, lowestTuning };
	return returnValue;
}

bool MemHelpers::IsSongInDrop() {
	std::unique_ptr<byte[]> songTuning = std::unique_ptr<byte[]>(MemHelpers::GetCurrentTuning());
	byte NEGATE_DROP = songTuning[0] + 2;

	if (songTuning[1] == NEGATE_DROP && songTuning[2] == NEGATE_DROP && songTuning[3] == NEGATE_DROP && songTuning[4] == NEGATE_DROP && songTuning[5] == NEGATE_DROP) // If the tuning number is the same across the board, return true, but if one or more fail, return false.
		return true;
	return false;
}

bool MemHelpers::IsSongInStandard() {
	std::unique_ptr<byte[]> songTuning = std::unique_ptr<byte[]>(MemHelpers::GetCurrentTuning());
	byte commonTuning = songTuning[0];
	if (songTuning[1] == commonTuning && songTuning[2] == commonTuning && songTuning[3] == commonTuning && songTuning[4] == commonTuning && songTuning[5] == commonTuning) // If the tuning number is the same across the board, return true, but if one or more fail, return false.
		return true;
	return false;
}

int MemHelpers::GetTrueTuning() {
	uintptr_t trueTunePointer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_trueTuning, Offsets::ptr_trueTuningOffsets);

	if (!trueTunePointer)
		return 440; // Just to be sure, it's 440 most of the time.

	int trueTuning = floor(*(float*)trueTunePointer);

	return trueTuning;
}

std::string MemHelpers::GetCurrentMenu(bool GameNotLoaded) {
	if (GameNotLoaded) { // It seems like the third level of the pointer isn't initialized until you reach the UPLAY login screen, but the second level actually is, and in there it keeps either an empty string, "TitleMenu", "MainOverlay" (before you reach the login) or some gibberish that's always the same (after that) 
		uintptr_t preMainMenuAdr = MemUtil::FindDMAAddy(Offsets::ptr_currentMenu, Offsets::ptr_preMainMenuOffsets, GameNotLoaded);

		std::string currentMenu((char*)preMainMenuAdr);

		if (lastMenu == "TitleScreen" && lastMenu != currentMenu)  // I.e. check if its neither one of the possible states
			canGetRealMenu = true;
		else {
			lastMenu = currentMenu;
			return "pre_enter_prompt";
		}
	}

	if (!canGetRealMenu)
		return "pre_enter_prompt";

	uintptr_t currentMenuAdr = MemUtil::FindDMAAddy(Offsets::ptr_currentMenu, Offsets::ptr_currentMenuOffsets, GameNotLoaded); // If game hasn't loaded, take the safer, but possibly slower route

	// Thank you for serving the cause comrade, but you aren't needed any more
	//if (currentMenuAdr > 0x30000000) // Ghetto checks for life, if you haven't eneterd the game it usually points to 0x6XXXXXXX and if you try to dereference that, you get yourself a nice crash
	//	return "pre_enter_prompt";

	if (!currentMenuAdr)
		return "where are we actually";

	std::string currentMenu((char*)currentMenuAdr);
	//std::cout << currentMenu << std::endl;
	return currentMenu;
}

void MemHelpers::ToggleLoft() {

	// Old Method (Works for most builds but bugged out for some)
	//uintptr_t nearAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_loft, Offsets::ptr_loft_nearOffsets);
	//if (*(float*)nearAddr == 10)
	//	*(float*)nearAddr = 10000;
	//else
	//	*(float*)nearAddr = 10;

	uintptr_t farAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_loft, Offsets::ptr_loft_farOffsets);

	if (*(float*)farAddr == 10000)
		*(float*)farAddr = 1;
	else
		*(float*)farAddr = 10000;
}

std::string MemHelpers::ShowSongTimer() {
	uintptr_t addrTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timer, Offsets::ptr_timerOffsets);

	if (!addrTimer)
		return "";

	return std::to_string(*(float*)addrTimer);
}

void MemHelpers::ToggleCB(bool enabled) {
	uintptr_t addrTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timer, Offsets::ptr_timerOffsets);
	uintptr_t cbEnabled = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_colorBlindMode, Offsets::ptr_colorBlindModeOffsets);

	if (!addrTimer)
		return;

	if (*(byte*)cbEnabled != (byte)enabled) //JIC, no need to write the same value constantly
		*(byte*)cbEnabled = enabled;
}

void MemHelpers::PatchCDLCCheck() {
	uint8_t* VerifySignatureOffset = Offsets::cdlcCheckAdr;

	if (VerifySignatureOffset) {
		if (!MemUtil::PatchAdr(VerifySignatureOffset + 8, (UINT*)Offsets::patch_CDLCCheck, 2))
			printf("Failed to patch verify_signature!\n");
		else
			printf("Patch verify_signature success!\n");
	}
}

int* MemHelpers::GetWindowSize() {
	RECT WindowSize;
	if (GetWindowRect(FindWindow(NULL, L"Rocksmith 2014"), &WindowSize))
	{
		int width = WindowSize.right - WindowSize.left;
		int height = WindowSize.bottom - WindowSize.top;

		int* dimensions = new int[2]{ width, height };

		return dimensions;
	}
	else
		return new int[2]{ 0, 0 };
}

bool MemHelpers::IsInStringArray(std::string stringToCheckIfInsideArray, std::string* stringArray, std::vector<std::string> stringVector) {
	if (stringArray != NULL) {
		for (unsigned int i = 0; i < stringArray->length(); i++) {
			if (stringToCheckIfInsideArray == stringArray[i])
				return true;
		}
	}
	else if (stringVector != std::vector<std::string>()) {
		for (unsigned int i = 0; i < stringVector.size(); i++) {
			if (stringToCheckIfInsideArray == stringVector[i])
				return true;
		}
	}

	return false;
}

// textColorHex is Hex for AA,RR,GG,BB or FFFFFFFF (8 F's). If your text doesn't show up, make sure you lead with FF (or 255 in hex).
void MemHelpers::DX9DrawText(std::string textToDraw, int textColorHex, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY, LPDIRECT3DDEVICE9 pDevice)
{
	int* WindowSize = MemHelpers::GetWindowSize();

	// If the user changes resolutions, we want to scale the text dynamically. This also covers the first font creation as the font and WindowSize variables are all null to begin with.
	if (WindowSizeX != (WindowSize[0] / 96) || WindowSizeY != (WindowSize[1] / 72) || CustomDX9Font == NULL) {
		WindowSizeX = (WindowSize[0] / 96);
		WindowSizeY = (WindowSize[1] / 72);

		CustomDX9Font = D3DXCreateFontA(pDevice, WindowSizeX, WindowSizeY, FW_NORMAL, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &DX9FontEncapsulation); // Create a new font
	}

	delete[] WindowSize;

	// Create Area To Draw Text
	RECT TextRectangle{ topLeftX, topLeftY, bottomRightX, bottomRightY }; // Left, Top, Right, Bottom

	// Preload And Draw The Text (Supposed to reduce the performance hit (It's D3D/DX9 but still good practice))
	DX9FontEncapsulation->PreloadTextA(textToDraw.c_str(), textToDraw.length());
	DX9FontEncapsulation->DrawTextA(NULL, textToDraw.c_str(), -1, &TextRectangle, DT_LEFT | DT_NOCLIP, textColorHex);

	// Let's clean up our junk, since fonts don't do it automatically.
	if (DX9FontEncapsulation) {
		DX9FontEncapsulation->Release();
		DX9FontEncapsulation = NULL;
	}
}

void MemHelpers::ToggleDrunkMode(bool enable) {
	uintptr_t noLoft = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_loft, Offsets::ptr_loft_farOffsets);

	if (enable) {
		if (*(float*)noLoft == 1) {
			D3DHooks::ToggleOffLoftWhenDoneWithMod = true;
			MemHelpers::ToggleLoft();
		}
	}
	else {
		*(float*)Offsets::ptr_drunkShit = 0.3333333333f;

		if (D3DHooks::ToggleOffLoftWhenDoneWithMod) {
			MemHelpers::ToggleLoft();
			D3DHooks::ToggleOffLoftWhenDoneWithMod = false;
		}
	}
}

bool MemHelpers::IsInSong() {
	return IsInStringArray(GetCurrentMenu(), 0, songModes);
}


float MemHelpers::RiffRepeaterSpeed(float newSpeed) {
	uintptr_t riffRepeaterSpeed = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_songSpeed, Offsets::ptr_songSpeedOffsets);

	if (newSpeed != NULL) // If we aren't just trying to see the current speed.
		*(float*)riffRepeaterSpeed = newSpeed;
	return *(float*)riffRepeaterSpeed;
}

void MemHelpers::AutomatedOpenRRSpeedAbuse() {
	Sleep(5000); // Main animation from Tuner -> In game where we can control the menu
	std::cout << "Triggering RR" << std::endl;

	Util::SendKey(VK_SPACE); // Open RR Menu
	Sleep(666); // Menu animations do be slow

	Util::SendKey(VK_DOWN); // Difficulty

	Util::SendKey(VK_DOWN); // Speed

	RiffRepeaterSpeed(100.00001f); // Allow us to change speed values. Keep this number as close to 100 as possible to allow NSP to use this number. It doesn't like being reset to 100 at the bottom of this function.

	Util::SendKey(VK_LEFT); // Trigger our new speed requirement

	Util::SendKey(VK_RIGHT); // Reset back to 100% in the UI

	Util::SendKey(VK_DELETE); // Return to the song

	RiffRepeaterSpeed(100.f); // Reset to 100% speed so the end user doesn't experience any speed ups / slow downs when an event isn't triggered.

	automatedSongSpeedInThisSong = true; // Don't run this again in the same song if we put this in a loop.
	useNewSongSpeed = true; // Show RR Speed Text
}
