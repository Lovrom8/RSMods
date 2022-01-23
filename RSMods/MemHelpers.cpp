#include "MemHelpers.hpp"

/// <summary>
/// **DEPRECATED | USE GetCurrentTuning(false)**
/// </summary>
/// <returns>NULL if invalid address, else if ExtendedRangeDropTuning == true LowE-String Tuning else A-String Tuning.</returns>
byte MemHelpers::getLowestStringTuning() {
	uintptr_t addrTuning = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_tuning, Offsets::ptr_tuningOffsets);

	// Null Pointer Check
	if (!addrTuning) {
		std::cout << "Invalid Pointer: getLowestStringTuning" << std::endl;
		return NULL;
	}
		

	if (Settings::ReturnSettingValue("ExtendedRangeDropTuning") == "on")
		return (*(Tuning*)addrTuning).lowE;

	return (*(Tuning*)addrTuning).strA;
}

/// <summary>
/// Get Tuning of all 6 strings (even on bass)
/// </summary>
/// <param name="verbose"> - Should we show the tuning in the console **DEBUG BUILD ONLY**</param>
/// <returns>Current Tuning in a Byte[6] array.</returns>
byte* MemHelpers::GetCurrentTuning(bool verbose) {
	uintptr_t addrTuning = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_tuning, Offsets::ptr_tuningOffsets, true);

	// Null Pointer Check
	if (!addrTuning) {
		// std::cout << "Invalid Pointer: GetCurrentTuning" << std::endl; // Disabled because it causes log to get huge real quick
		return NULL;
	}
		

	byte* AllTunings = new byte[6]{ (*(Tuning*)addrTuning).lowE, (*(Tuning*)addrTuning).strA, (*(Tuning*)addrTuning).strD, (*(Tuning*)addrTuning).strG, (*(Tuning*)addrTuning).strB, (*(Tuning*)addrTuning).highE };

	// Print tuning to console. **DEBUG BUILD ONLY**
	if (verbose) {
		for (int i = 0; i < 6; i++)
			std::cout << "String" << i << " - " << (int)AllTunings[i] << std::endl;
	}

	return AllTunings;
}

/// <summary>
/// Gets Current Tuning in the tuner (based on the tuning name)
/// </summary>
/// <returns>Guess of Current Tuning</returns>
Tuning MemHelpers::GetTuningAtTuner() {
	std::string pathToTuningList = "RSMods/CustomMods/tuning.database.json";

	// If we can't find the list of tunings, just return a default value
	if (!std::filesystem::exists(pathToTuningList)) { 
		std::cout << "Invalid File: GetTuningAtTuner - Path To Tuning File Doesn't Exist." << std::endl;
		return Tuning();
	}

	uintptr_t addrTuningText = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_tuningText, Offsets::ptr_tuningTextOffsets);

	// Null Pointer Check
	if (!addrTuningText) {
		std::cout << "Invalid Pointer: GetTuningAtTuner" << std::endl;
		return Tuning();
	}
		

	std::string unsanitized_tuningText = std::string((const char*)addrTuningText);

	// Rocksmith converts all ASCII "#" to the unicode version. Since we have to use std::string (and can't use std::wstring) with nlohmann, we convert the corrupt character combination to an ASCII "#".
	while (unsanitized_tuningText.find("\xe2\x99\xaf") != std::string::npos) { // Unicode # (sharp)
		size_t badHash = unsanitized_tuningText.find("\xe2\x99\xaf");
		std::string partOne = unsanitized_tuningText.substr(0, badHash);
		std::string partTwo = unsanitized_tuningText.substr(badHash + 2, unsanitized_tuningText.length() - 1);
		unsanitized_tuningText = partOne + partTwo;
		unsanitized_tuningText.at(badHash) = '#';
	}

	// Rocksmith converts all ASCII "b" to the unicode version. Since we have to use std::string (and can't use std::wstring) with nlohmann, we convert the corrupt character combination to an ASCII "b".
	// Note "b" is capitalized at the end because we later assume all tunings are capital since Rocksmith will parse tuning names as uppercase. Since we use the non-UTF value we have to convert the "b" to a "B" for our later comparison to work.
	while (unsanitized_tuningText.find('\xe2\x99\xad') != std::string::npos) { // Unicode b (flat)
		size_t badFlat = unsanitized_tuningText.find("\xe2\x99\xad");
		std::string partOne = unsanitized_tuningText.substr(0, badFlat);
		std::string partTwo = unsanitized_tuningText.substr(badFlat + 2, unsanitized_tuningText.length() - 1);
		unsanitized_tuningText = partOne + partTwo;
		unsanitized_tuningText.at(badFlat) = 'B';
	}

	std::string tuningText = unsanitized_tuningText;

	// If it's a custom tuning we don't know the tuning, so we might as well stop here.
	if (tuningText == (std::string)"CUSTOM TUNING") {
		std::cout << "Invalid Tuning: CUSTOM TUNING" << std::endl;
		return Tuning();
	}
		

	// In the JSON, tunings have no whitespaces, so get rid of them
	tuningText.erase(std::remove_if(tuningText.begin(), tuningText.end(), isspace), tuningText.end());

	// Parse RSMods unpacked tuning definition file.
	std::ifstream jsonFile(pathToTuningList);
	nlohmann::json tuningJson;
	jsonFile >> tuningJson;
	jsonFile.close();
	tuningJson = tuningJson["Static"]["TuningDefinitions"]; // Skip directly to the part we are interested in

	// Unforunately we can't use json.contains due to difference in formatting
	for (auto& tuning : tuningJson.items()) { 
		std::string jsonKeyUpper = tuning.key(), jsonKeyOriginal = tuning.key(); // Also you can't just make a separate copy of the uppercase string, so we keep both 
		std::transform(jsonKeyUpper.begin(), jsonKeyUpper.end(), jsonKeyUpper.begin(), ::toupper);

		if (jsonKeyOriginal == tuningText || jsonKeyUpper == tuningText) { // If the tuning is all uppercase or if standard-case matches
			tuningJson = tuningJson[jsonKeyOriginal]["Strings"];
			return Tuning(tuningJson["string0"], tuningJson["string1"], tuningJson["string2"], tuningJson["string3"], tuningJson["string4"], tuningJson["string5"]);
		}
	}

	std::cout << "Invalid Tuning: Tuning doesn't exist in RSMods tuning list" << std::endl;
	return Tuning();
}

/// <returns>Should we Display The Extended Range Colors?</returns>
bool MemHelpers::IsExtendedRangeSong() {
	uintptr_t addrTimerEnabled = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timer, Offsets::ptr_timerBaseOffsets);

	// Null Pointer Check
	if (!addrTimerEnabled) {
		std::cout << "Invalid Pointer: IsExtendedRangeSong" << std::endl;
		return false;
	}
	
	if (Settings::ReturnSettingValue("ExtendedRangeEnabled") != "on")
		return false;

	// Get lowest tuned string
	int* highestLowest = MemHelpers::GetHighestLowestString();
	int lowestTuning = highestLowest[1];

	// Get the current tuning if available
	byte* currentTuning = MemHelpers::GetCurrentTuning();
	Tuning tuning = Tuning();
	if (currentTuning)
		tuning = Tuning(currentTuning[0], currentTuning[1], currentTuning[2], currentTuning[3], currentTuning[4], currentTuning[5]);

	// Cleanup Duty
	delete[] highestLowest;
	if (currentTuning)
		delete[] currentTuning;

	// When a charter makes a bad bass tuning, and leaves the last two strings blank, let's fix that.
	if (Settings::ReturnSettingValue("ExtendedRangeFixBassTuning") == "on") { 
		tuning.strB = tuning.strG;
		tuning.highE = tuning.strG;
	}

	bool dropTuning = IsSongInDrop(tuning);

	// HighestLowest Tuning Pointer is invalid
	if (lowestTuning == 666) {
		 std::cout << "Invalid Tuning: GetHighestLowestString -> IsExtendedRangeSong" << std::endl;
		return false;
	}

	// Bass below C standard fix (A220 range)
	if (GetTrueTuning() <= 260)
		lowestTuning -= 12;
	
	// Does the user's settings allow us to toggle on drop tunings (ER on B, trigger on C# Drop B)
	if (Settings::ReturnSettingValue("ExtendedRangeDropTuning") == "on" && lowestTuning <= Settings::GetModSetting("ExtendedRangeMode") && dropTuning) {
		 std::cout << "Successful: IsExtendedRangeSong in DROP where " << lowestTuning << " is less than, or equal to, " << Settings::GetModSetting("ExtendedRangeMode") << std::endl;
		return true;
	}

	// Does the user's settings allow us to toggle Exteneded Range Mode for this tuning
	if (lowestTuning <= Settings::GetModSetting("ExtendedRangeMode") && (!dropTuning || lowestTuning <= Settings::GetModSetting("ExtendedRangeMode") - 2)) {
		 std::cout << "Successful: IsExtendedRangeSong in standard where " << lowestTuning << " is less than, or equal to, " << Settings::GetModSetting("ExtendedRangeMode") << " minus 2. Drop Tuned: " << std::boolalpha << dropTuning << std::endl;
		return true;
	}
		
	 std::cout << "Failed: IsExtendedRangeSong. Drop at " << Settings::GetModSetting("ExtendedRangeMode") << " but received " << lowestTuning << " with drop tuning " << Settings::ReturnSettingValue("ExtendedRangeDropTuning") << std::endl;
	return false;
}

/// <returns>Should we Display The Extended Range Colors In The Tuner?</returns>
bool MemHelpers::IsExtendedRangeTuner() {
	uintptr_t addrTuningText = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_tuningText, Offsets::ptr_tuningTextOffsets);

	// Null Pointer Check or not in a pre-song tuner
	if (!addrTuningText) {
		std::cout << "Invalid Pointer: IsExtendedRangeTuner" << std::endl;
		return false;
	}

	// Verify the user actually wants Extended Range enabled.
	if (Settings::ReturnSettingValue("ExtendedRangeEnabled") != "on")
		return false;
	
	Tuning tuner_songTuning = GetTuningAtTuner();

	// Tuning Not Found
	if (tuner_songTuning.lowE == 69) {
		std::cout << "Invalid Tuning: IsExtendedRangeTuner" << std::endl;
		return false;
	}

	int lowestTuning = tuner_songTuning.lowE;

	// Bass below C standard fix (A220 range)
	if (GetTrueTuning() <= 260)
		lowestTuning -= 12;

	bool inDrop = IsSongInDrop(tuner_songTuning);

	// The games stores tunings in unsigned chars (bytes) so we need to convert it to our int Extended Range toggle.
	if (lowestTuning > 24)
		lowestTuning -= 256;

	// Does the user's settings allow us to toggle on drop tunings (ER on B, trigger on C# Drop B)
	if (Settings::ReturnSettingValue("ExtendedRangeDropTuning") == "on" && inDrop && lowestTuning <= Settings::GetModSetting("ExtendedRangeMode")) {
		std::cout << "Successful: IsExtendedRangeTuner in DROP where " << lowestTuning << " is less than, or equal to, " << Settings::GetModSetting("ExtendedRangeMode") << std::endl;
		return true;
	}
		

	// Does the user's settings allow us to toggle Exteneded Range Mode for this tuning
	if (lowestTuning <= Settings::GetModSetting("ExtendedRangeMode") && (!inDrop || lowestTuning <= Settings::GetModSetting("ExtendedRangeMode") - 2)) {
		std::cout << "Successful: IsExtendedRangeTuner in standard where " << lowestTuning << " is less than, or equal to, " << Settings::GetModSetting("ExtendedRangeMode") << " minus 2. Drop Tuned: " << std::boolalpha << inDrop << std::endl;
		return true;
	}
		
	std::cout << "Failed: IsExtendedRangeTuner. Drop at " << Settings::GetModSetting("ExtendedRangeMode") << " but received " << lowestTuning << " with drop tuning " << Settings::ReturnSettingValue("ExtendedRangeDropTuning") << std::endl;
	return false;
}

/// <summary>
/// Gets the highest tuned string, and the lowest tuned string.
/// </summary>
/// <returns>[0] - Highest, [1] - Lowest</returns>
int* MemHelpers::GetHighestLowestString() {
	int highestTuning = 0, lowestTuning = 256, currentStringTuning = 0;
	std::unique_ptr<byte[]> songTuning = std::unique_ptr<byte[]>(MemHelpers::GetCurrentTuning());

	// Null Pointer Check
	if (!songTuning) {
		int* fakeReturns = new int[2]{ 666, 666 };
		// std::cout << "Failed: GetHighestLowestString. GetCurrentTuning returned an invalid tuning" << std::endl; // Disabled because it causes log to get huge real quick
		return fakeReturns;
	}

	int numberOfStrings = (Settings::ReturnSettingValue("ExtendedRangeFixBassTuning") == "on" && (songTuning[4] == 0 || songTuning[4] == 12) && (songTuning[5] == 0 || songTuning[5] == 12)) ? 4 : 6; // When a charter makes a bad bass tuning, and leaves the last two strings blank, let's fix that.

	bool bassOctaveEffect = GetTrueTuning() == 220;

	// Get Highest And Lowest Strings
	for (int i = 0; i < numberOfStrings; i++) {

		// Create a buffer so we can work on a value near 256, and not worry about the tunings at, and above, E Standard that start at 0 because the tuning number breaks the 256 limit of a unsigned char.
		currentStringTuning = (int)songTuning[i];

		if (bassOctaveEffect) // Is the song done in A220? If so, we need to lower the tunings so our math will still work.
			currentStringTuning -= 12;

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

	if (bassOctaveEffect) { // Is the song done in A220? If so, we need to add the effect back to our highest / lowest tunings.
		highestTuning += 12;
		lowestTuning += 12;
	}

	// Change tuning number (255 = Eb Standard, 254 D Standard, etc) to drop number (-1 = Eb Standard, -2 D Standard, etc).
	if (highestTuning != 0)
		highestTuning -= 256;
	if (lowestTuning != 0)
		lowestTuning -= 256;

	int* returnValue = new int[2]{ highestTuning, lowestTuning };
	
	return returnValue;
}

/// <summary>
/// Gets the highest tuned string, and the lowest tuned string.
/// </summary>
/// <returns>[0] - Highest, [1] - Lowest</returns>
int* MemHelpers::GetHighestLowestString(Tuning tuningOverride) {
	int highestTuning = 0, lowestTuning = 256, currentStringTuning = 0;

	// Null Pointer Check
	if (tuningOverride.lowE == 69) {
		int* fakeReturns = new int[2]{ 666, 666 };
		// std::cout << "Failed: GetHighestLowestString. GetCurrentTuning returned an invalid tuning" << std::endl; // Disabled because it causes log to get huge real quick
		return fakeReturns;
	}

	int numberOfStrings = (Settings::ReturnSettingValue("ExtendedRangeFixBassTuning") == "on" && (tuningOverride.strB == 0 || tuningOverride.strB == 12) && (tuningOverride.highE == 0 || tuningOverride.highE == 12)) ? 4 : 6; // When a charter makes a bad bass tuning, and leaves the last two strings blank, let's fix that.

	bool bassOctaveEffect = GetTrueTuning() == 220;

	// If the song is in A220, we need to remove 12 from the bass tuning to get the real tuning.
	if (bassOctaveEffect) {
		tuningOverride.lowE		-= 12;
		tuningOverride.strA		-= 12;
		tuningOverride.strD		-= 12;
		tuningOverride.strG		-= 12;
		tuningOverride.strB		-= 12;
		tuningOverride.highE	-= 12;
	}

	// Get the value for the strings.
	int string_lowE = tuningOverride.lowE <= 24		? tuningOverride.lowE + 256		: tuningOverride.lowE;
	int string_A = tuningOverride.strA <= 24		? tuningOverride.strA + 256		: tuningOverride.strA;
	int string_D = tuningOverride.strD <= 24		? tuningOverride.strD + 256		: tuningOverride.strD;
	int string_G = tuningOverride.strG <= 24		? tuningOverride.strG + 256		: tuningOverride.strG;
	int string_B = tuningOverride.strB <= 24		? tuningOverride.strB + 256		: tuningOverride.strB;
	int string_highE = tuningOverride.highE <= 24	? tuningOverride.highE + 256	: tuningOverride.highE;

	// Get the highest tuning used.
	highestTuning = string_lowE > highestTuning ? string_lowE	: highestTuning;
	highestTuning = string_A	> highestTuning ? string_A		: highestTuning;
	highestTuning = string_D	> highestTuning ? string_D		: highestTuning;
	highestTuning = string_G	> highestTuning ? string_G		: highestTuning;

	// Get the lowest tuning used
	lowestTuning = string_lowE	< lowestTuning	? string_lowE	: lowestTuning;
	lowestTuning = string_A		< lowestTuning	? string_A		: lowestTuning;
	lowestTuning = string_D		< lowestTuning	? string_D		: lowestTuning;
	lowestTuning = string_G		< lowestTuning	? string_G		: lowestTuning;

	if (numberOfStrings == 6) {
		// Get the highest tuning used (guitar).
		highestTuning = string_B		> highestTuning ? string_B		: highestTuning;
		highestTuning = string_highE	> highestTuning ? string_highE	: highestTuning;

		// Get the lowest tuning used (guitar).
		lowestTuning = string_B			< lowestTuning	? string_B		: lowestTuning;
		lowestTuning = string_highE		< lowestTuning	? string_highE	: lowestTuning;
	}

	// Is the song done in A220? If so, we need to add the effect back to our highest / lowest tunings.
	if (bassOctaveEffect) { 
		highestTuning += 12;
		lowestTuning += 12;
	}

	// Change tuning number (255 = Eb Standard, 254 D Standard, etc) to drop number (-1 = Eb Standard, -2 D Standard, etc).
	if (highestTuning != 0)
		highestTuning -= 256;
	if (lowestTuning != 0)
		lowestTuning -= 256;

	int* returnValue = new int[2]{ highestTuning, lowestTuning };

	return returnValue;
}


/// <param name="tuning"> - Song Tuning</param>
/// <returns>Are we playing in Drop tuning? ex: D Drop C</returns>
bool MemHelpers::IsSongInDrop(Tuning tuning) {
	int NEGATE_DROP = tuning.lowE + 2;
	return tuning.strA == NEGATE_DROP && tuning.strD == NEGATE_DROP && tuning.strG == NEGATE_DROP && tuning.strB == NEGATE_DROP && tuning.highE == NEGATE_DROP;
}

/// <param name="tuning"> - Song Tuning</param>
/// <returns>Are we playing in Standard tuning? ex: E Standard</returns>
bool MemHelpers::IsSongInStandard(Tuning tuning) {
	int COMMON_TUNING = tuning.lowE;
	return tuning.strA == COMMON_TUNING && tuning.strD == COMMON_TUNING && tuning.strG == COMMON_TUNING && tuning.strB == COMMON_TUNING && tuning.highE == COMMON_TUNING;
}

/// <returns>True Tuning. ex: A440, A432</returns>
int MemHelpers::GetTrueTuning() {
	uintptr_t trueTunePointer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_trueTuning, Offsets::ptr_trueTuningOffsets);

	// Null Pointer Check
	if (!trueTunePointer) {
		// std::cout << "Invalid Pointer: GetTrueTuning" << std::endl; // Disabled because it causes log to get huge real quick
		return 440;
	}
		
	int trueTuning = floor(*(float*)trueTunePointer);
	return trueTuning;
}


/// <param name="GameNotLoaded"> - Should we trust the pointer?</param>
/// <returns>Internal Menu Name</returns>
std::string MemHelpers::GetCurrentMenu(bool GameNotLoaded) {
	// It seems like the third level of the pointer isn't initialized until you reach the UPLAY login screen,
	// but the second level actually is, and in there it keeps either an empty string, "TitleMenu", "MainOverlay"
	// (before you reach the login) or some gibberish that's always the same (after that) 
	if (GameNotLoaded) { 
		uintptr_t preMainMenuAdr = MemUtil::FindDMAAddy(Offsets::ptr_currentMenu, Offsets::ptr_preMainMenuOffsets, GameNotLoaded);

		std::string currentMenu((char*)preMainMenuAdr);

		// I.e. check if its neither one of the possible states
		if (lastMenu == "TitleScreen" && lastMenu != currentMenu)  
			canGetRealMenu = true;
		else {
			lastMenu = currentMenu;
			return "pre_enter_prompt";
		}
	}

	if (!canGetRealMenu)
		return "pre_enter_prompt";

	// If game hasn't loaded, take the safer, but possibly slower route
	uintptr_t currentMenuAdr = MemUtil::FindDMAAddy(Offsets::ptr_currentMenu, Offsets::ptr_currentMenuOffsets, GameNotLoaded); 

	// Null Pointer Check
	if (!currentMenuAdr) {
		std::cout << "Invalid Pointer: GetCurrentMenu(" << std::boolalpha << GameNotLoaded << ") @ LVL 3" << std::endl;
		return "where are we actually";
	}

	std::string currentMenu((char*)currentMenuAdr);
	return currentMenu;
}

/// <summary>
/// Turn the background / "map" on or off.
/// </summary>
void MemHelpers::ToggleLoft() {

	uintptr_t farAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_loft, Offsets::ptr_loft_farOffsets);

	if (*(float*)farAddr == 10000)
		*(float*)farAddr = 1; // Loft Off
	else
		*(float*)farAddr = 10000; // Loft On
}

float MemHelpers::SongTimer() {
	uintptr_t addrTimerBase = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timer, Offsets::ptr_timerBaseOffsets);
	uintptr_t addrTimerRare = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timerRare, Offsets::ptr_timerRareOffsets);

	// Null Pointer Check
	if (!addrTimerBase) {
		std::cout << "Invalid Pointer: (BASE) ShowSongTimer" << std::endl;
		return 0.f;
	}

	// Null Pointer Check
	// At this point, we can verify that the timer is a valid time.
	if (!addrTimerRare) {
		std::cout << "Invalid Pointer: (RARE) ShowSongTimer" << std::endl;
		return *(float*)addrTimerBase;
	}

	// We entered a song where the base timer does not work.
	// Cause for this is unknown but we need to check, or time based mods (looping, song timer) will break.
	// Ex: Desolate Motion, or Rocksmith 2012 Theme.
	if (IsInStringArray(GetCurrentMenu(), songModes) && *(float*)addrTimerBase == 0.f && *(float*)addrTimerRare != 0.f) {
		return *(float*)addrTimerRare;
	}
	// This is the default case, and will be used 99.99% of the time.
	else {
		return *(float*)addrTimerBase;
	}
}

/// <summary>
/// Should we turn on / off ColorBlind colors
/// </summary>
/// <param name="enabled"> - Should we turn on colors or turn off?</param>
void MemHelpers::ToggleCB(bool enabled) {
	uintptr_t addrTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timer, Offsets::ptr_timerBaseOffsets);
	uintptr_t cbEnabled = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_colorBlindMode, Offsets::ptr_colorBlindModeOffsets);

	// Null Pointers Check
	if (!addrTimer || !cbEnabled) {
		// std::cout << "Invalid Pointers: ToggleCB(" << std::boolalpha << enabled << ")" << std::endl; // Disabled because it causes log to get huge real quick
		return;
	}
	
	// JIC, no need to write the same value constantly
	if (*(byte*)cbEnabled != (byte)enabled) 
		*(byte*)cbEnabled = enabled;
}


/// <returns>Size of Rocksmith Window</returns>
Resolution MemHelpers::GetWindowSize() {
	RECT WindowSize;

	Resolution currentSize;
	if (GetWindowRect(FindWindow(NULL, L"Rocksmith 2014"), &WindowSize))
	{
		currentSize.width = WindowSize.right - WindowSize.left;
		currentSize.height = WindowSize.bottom - WindowSize.top;
	}
	
	return currentSize;
}

/// <param name="stringToCheckIfInsideArray"> - Input</param>
/// <param name="stringArray"> - Is input in list | ARRAY? (NULLABLE)</param>
/// <param name="stringVector"> - Is input in list | VECTOR? (NULLABLE)</param>
bool MemHelpers::IsInStringArray(std::string stringToCheckIfInsideArray, std::vector<std::string> stringVector) {
	if (stringVector != std::vector<std::string>()) {
		for (unsigned int i = 0; i < stringVector.size(); i++) {
			if (stringToCheckIfInsideArray == stringVector[i])
				return true;
		}
	}

	return false;
}

/// <summary>
/// Draw text on screen
/// </summary>
/// <param name="textToDraw"> - What text should be written?</param>
/// <param name="textColorHex"> - What color? Given in hex in the AA,RR,GG,BB format.</param>
/// <param name="topLeftX"> - top LEFT of textbox</param>
/// <param name="topLeftY"> - TOP left of textbox</param>
/// <param name="bottomRightX"> - bottom RIGHT of textbox</param>
/// <param name="bottomRightY"> - BOTTOM right of textbox</param>
/// <param name="pDevice"> - Device Pointer</param>
/// <param name="setFontSize"> - Override font size</param>
/// <param name="format"> - DrawText format</param>
void MemHelpers::DX9DrawText(std::string textToDraw, int textColorHex, int topLeftX, int topLeftY, int bottomRightX, int bottomRightY, LPDIRECT3DDEVICE9 pDevice, Resolution setFontSize, DWORD format)
{
	Resolution WindowSize = MemHelpers::GetWindowSize();

	// Allow Font Size Declaration
	bool useInputFontSize = (setFontSize.width != NULL && setFontSize.height != NULL);

	// If the user changes resolutions, we want to scale the text dynamically. This also covers the first font creation as the font and fontSize variables are all null to begin with.
	if ((fontWidth != (WindowSize.width / 96) || fontHeight != (WindowSize.height / 72) || CustomDX9Font == NULL) && !useInputFontSize) {
		fontWidth = (WindowSize.width / 96);
		fontHeight = (WindowSize.height / 72);

		CustomDX9Font = D3DXCreateFontA(pDevice, fontWidth, fontHeight, FW_NORMAL, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, Settings::ReturnSettingValue("OnScreenFont").c_str(), &DX9FontEncapsulation); // Create a new font
	}
	else if (useInputFontSize)
		CustomDX9Font = D3DXCreateFontA(pDevice, setFontSize.width, setFontSize.height, FW_NORMAL, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, Settings::ReturnSettingValue("OnScreenFont").c_str(), &DX9FontEncapsulation); // Create a new font

	// Create Area To Draw Text
	RECT TextRectangle{ topLeftX, topLeftY, bottomRightX, bottomRightY }; // Left, Top, Right, Bottom

	// Preload And Draw The Text (Supposed to reduce the performance hit (It's D3D/DX9 but still good practice))
	DX9FontEncapsulation->PreloadTextA(textToDraw.c_str(), textToDraw.length());
	DX9FontEncapsulation->DrawTextA(NULL, textToDraw.c_str(), -1, &TextRectangle, format, textColorHex);

	// Let's clean up our junk, since fonts don't do it automatically.
	if (DX9FontEncapsulation) {
		DX9FontEncapsulation->Release();
		DX9FontEncapsulation = NULL;
	}
}

/// <summary>
/// Shake the camera around randomly.
/// </summary>
/// <param name="enable"> - Should we turn it on, or off?</param>
void MemHelpers::ToggleDrunkMode(bool enable) {
	uintptr_t noLoft = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_loft, Offsets::ptr_loft_farOffsets);

	
	if (enable) {
		// Turn on loft so the effects of the mod are actually shown.
		if (*(float*)noLoft == 1) {
			D3DHooks::ToggleOffLoftWhenDoneWithMod = true;
			MemHelpers::ToggleLoft();
		}
	}
	else {
		*(float*)Offsets::ptr_drunkShit = 0.3333333333f;

		// User originally had the loft off, but then we turned on this mod, so turn the loft back off.
		if (D3DHooks::ToggleOffLoftWhenDoneWithMod) {
			MemHelpers::ToggleLoft();
			D3DHooks::ToggleOffLoftWhenDoneWithMod = false;
		}
	}
}

/// <summary>
/// Are we in a song?
/// </summary>
bool MemHelpers::IsInSong() {
	return IsInStringArray(GetCurrentMenu(), songModes);
}

/// <summary>
/// Get the status of if the user is in multiplayer
/// </summary>
/// <returns>Is the user in multiplayer</returns>
bool MemHelpers::IsMultiplayer() {
	return *(int*)MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_multiplayer, Offsets::ptr_multiplayerOffsets); // No need to null check because if it's null, then we assume it's singleplayer (which is zero).
}

/// <summary>
/// Get the current selected profile name. **Only works on Profile Selection screen**
/// </summary>
/// <returns>Profile Name</returns>
std::string MemHelpers::CurrentSelectedUser() {
	uintptr_t badValue = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_selectedProfileName, Offsets::ptr_selectedProfileNameOffsets);

	// If the pointer is invalid just return nothing
	if (!badValue) {
		std::cout << "Invalid Pointer: CurrentSelectedUser" << std::endl;
		return (std::string)"";
	}
		

	int i = 0;

	// This value 90% of the time starts with an invalid pointer. We must wait ~2.5 seconds to guarantee that it is correct, or until the pointer changes (whichever comes first).
	while (badValue < 0x10000000 && i < 25) {
		Sleep(100);
		badValue = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_selectedProfileName, Offsets::ptr_selectedProfileNameOffsets);
		i++;
	}

	return std::string((const char*)badValue);
}

/// <summary>
/// Gets the SongKey of the current playing song, based on the initial preview.
/// </summary>
/// <returns>Last played Song Key</returns>
std::string MemHelpers::GetSongKey() {
	uintptr_t previewEvent = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_previewName, Offsets::ptr_previewNameOffsets);

	if (previewEvent) {
		std::string previewName = std::string((char*)previewEvent);

		// If the preview name contains "Play_", which is required by Wwise.
		if (previewName.length() > 13 && previewName._Starts_with("Play_")) {
			
			// Verify that we are working with a "_Preview" audio.
			// "_Invalid" is used when the user turns off their song previews.
			if (previewName.compare(previewName.length() - 9, 9, "_Preview") || previewName.compare(previewName.length() - 9, 9, "_Invalid")) {
				lastSongKey = previewName.substr(5, previewName.length() - 13);
			}
		}
	}
	return lastSongKey;
}

/// <summary>
/// When the user passes a note in the pause menu, notes become grey. This gets the time at which notes go from being grey (deactivated) to being colored (activated).
/// </summary>
/// <returns>Time where all notes before it are grey / deactivated.</returns>
float MemHelpers::GetGreyNoteTimer() {
	uintptr_t greyNoteTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_greyOutNoteTimer, Offsets::ptr_greyOutNoteTimerOffsets);

	if (!greyNoteTimer) {
		std::cout << "greyNoteTimer = NULL" << std::endl;
		return NULL;
	}

	return *(float*)greyNoteTimer;
}

/// <summary>
/// Sets the time at which all notes before it turn grey (deactivated) and all notes after it are colorful (activated)
/// </summary>
/// <param name="timeInSeconds"> - Time to set the "deactivate before" at.</param>
void MemHelpers::SetGreyNoteTimer(float timeInSeconds) {
	uintptr_t greyNoteTimer = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_greyOutNoteTimer, Offsets::ptr_greyOutNoteTimerOffsets);

	if (!greyNoteTimer) {
		std::cout << "greyNoteTimer = NULL" << std::endl;
		return;
	}

	*(float*)greyNoteTimer = timeInSeconds;
}
