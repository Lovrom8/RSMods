#include "stdafx.h"
#include "SongTuning.hpp"
#include "Mods/DropPedal/DropPedal.hpp"

/// <summary>
/// Get Tuning of all 6 strings (even on bass)
/// </summary>
/// <param name="verbose"> - Should we show the tuning in the console **DEBUG BUILD ONLY**</param>
/// <returns>Current Tuning in a Byte[6] array.</returns>
std::array<byte, 6> SongTuning::GetCurrentTuning(bool verbose) {
	uintptr_t addrTuning = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_tuning, Offsets::ptr_tuningOffsets, true);

	if (!addrTuning) {
		return {};
	}

	auto tuningData = reinterpret_cast<Tuning*>(addrTuning);

	std::array<byte, 6> allTunings = {
		tuningData->lowE,
		tuningData->strA,
		tuningData->strD,
		tuningData->strG,
		tuningData->strB,
		tuningData->highE
	};

	// Print tuning to console. **DEBUG BUILD ONLY**
	if (verbose) 
	{
		for (int i = 0; i < 6; i++)
		{
			LOG_INFO("String" << i << " - " << static_cast<int>(allTunings[i]) << std::endl);
		}
	}

	return allTunings;
}

/// <summary>
/// Gets Current Tuning in the tuner (based on the tuning name)
/// </summary>
/// <returns>Guess of Current Tuning</returns>
Tuning SongTuning::GetTuningAtTuner() {
	std::string pathToTuningList = "RSMods/CustomMods/tuning.database.json";

	// If we can't find the list of tunings, just return a default value
	if (!std::filesystem::exists(pathToTuningList)) {
		LOG_ERROR("Invalid File: GetTuningAtTuner - Path To Tuning File Doesn't Exist." << std::endl);
		return Tuning();
	}

	uintptr_t addrTuningText = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_tuningText, Offsets::ptr_tuningTextOffsets);

	if (!addrTuningText) {
		LOG_ERROR("Invalid Pointer: GetTuningAtTuner" << std::endl);
		return Tuning();
	}

	auto unsanitizedTuningText = std::string((const char*)addrTuningText);

	// Rocksmith converts all ASCII "#" to the unicode version. Since we have to use std::string (and can't use std::wstring) with nlohmann, we convert the corrupt character combination to an ASCII "#".
	while (unsanitizedTuningText.find("\xe2\x99\xaf") != std::string::npos) { // Unicode # (sharp)
		size_t badHash = unsanitizedTuningText.find("\xe2\x99\xaf");
		std::string partOne = unsanitizedTuningText.substr(0, badHash);
		std::string partTwo = unsanitizedTuningText.substr(badHash + 2, unsanitizedTuningText.length() - 1);
		unsanitizedTuningText = partOne + partTwo;
		unsanitizedTuningText.at(badHash) = '#';
	}

	// Rocksmith converts all ASCII "b" to the unicode version. Since we have to use std::string (and can't use std::wstring) with nlohmann, we convert the corrupt character combination to an ASCII "b".
	// Note "b" is capitalized at the end because we later assume all tunings are capital since Rocksmith will parse tuning names as uppercase. Since we use the non-UTF value we have to convert the "b" to a "B" for our later comparison to work.
	while (unsanitizedTuningText.find('\xe2\x99\xad') != std::string::npos) { // Unicode b (flat)
		size_t badFlat = unsanitizedTuningText.find("\xe2\x99\xad");
		std::string partOne = unsanitizedTuningText.substr(0, badFlat);
		std::string partTwo = unsanitizedTuningText.substr(badFlat + 2, unsanitizedTuningText.length() - 1);
		unsanitizedTuningText = partOne + partTwo;
		unsanitizedTuningText.at(badFlat) = 'B';
	}

	std::string tuningText = unsanitizedTuningText;

	// If it's a custom tuning we don't know the tuning, so we might as well stop here.
	if (tuningText == (std::string)"CUSTOM TUNING") {
		LOG_WARNING("Invalid Tuning: CUSTOM TUNING" << std::endl);
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

	// Unfortunately we can't use json.contains due to difference in formatting
	for (auto const& tuning : tuningJson.items()) {
		std::string jsonKeyUpper = tuning.key();
		std::string jsonKeyOriginal = tuning.key(); // Also you can't just make a separate copy of the uppercase string, so we keep both 
		std::transform(jsonKeyUpper.begin(), jsonKeyUpper.end(), jsonKeyUpper.begin(), ::toupper);

		if (jsonKeyOriginal == tuningText || jsonKeyUpper == tuningText) { // If the tuning is all uppercase or if standard-case matches
			tuningJson = tuningJson[jsonKeyOriginal]["Strings"];
			return Tuning(tuningJson["string0"], tuningJson["string1"], tuningJson["string2"], tuningJson["string3"], tuningJson["string4"], tuningJson["string5"]);
		}
	}

	LOG_WARNING("Invalid Tuning: Tuning doesn't exist in RSMods tuning list" << std::endl);
	return Tuning();
}

/// <returns>Should we Display The Extended Range Colors?</returns>
bool SongTuning::IsExtendedRangeSong() {
	uintptr_t addrTimerEnabled = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_timer, Offsets::ptr_timerBaseOffsets);
	if (!addrTimerEnabled) {
		LOG_ERROR("Invalid Pointer: IsExtendedRangeSong" << std::endl);
		return false;
	}

	if (Settings::ReturnSettingValue("ExtendedRangeEnabled") != "on")
		return false;

	auto highestLowest = GetHighestLowestString();
	int lowestTuning = highestLowest[1];

	// Get the current tuning if available
	auto currentTuning = GetCurrentTuning();

	auto tuning = Tuning();
	if (currentTuning != std::array<byte, 6>{}) {
		tuning = Tuning(currentTuning[0], currentTuning[1], currentTuning[2], currentTuning[3], currentTuning[4], currentTuning[5]);
	}

	// When a charter makes a bad bass tuning, and leaves the last two strings blank, let's fix that.
	if (Settings::ReturnSettingValue("ExtendedRangeFixBassTuning") == "on") {
		tuning.strB = tuning.strG;
		tuning.highE = tuning.strG;
	}

	bool dropTuning = IsSongInDrop(tuning);

	// HighestLowest Tuning Pointer is invalid
	if (lowestTuning == 666) {
		LOG_ERROR("Invalid Tuning: GetHighestLowestString -> IsExtendedRangeSong" << std::endl);
		return false;
	}

	// Bass below C standard fix (A220 range)
	if (GetTrueTuning() <= 260)
		lowestTuning -= 12;

	// Does the user's settings allow us to toggle on drop tunings (ER on B, trigger on C# Drop B)
	if (Settings::ReturnSettingValue("ExtendedRangeDropTuning") == "on" && lowestTuning <= Settings::GetModSetting("ExtendedRangeMode") && dropTuning) {
		LOG_INFO("Successful: IsExtendedRangeSong in DROP where " << lowestTuning << " is less than, or equal to, " << Settings::GetModSetting("ExtendedRangeMode") << std::endl);
		return true;
	}

	// Does the user's settings allow us to toggle Extended Range Mode for this tuning
	if (lowestTuning <= Settings::GetModSetting("ExtendedRangeMode") && (!dropTuning || lowestTuning <= Settings::GetModSetting("ExtendedRangeMode") - 2)) {
		LOG_INFO("Successful: IsExtendedRangeSong in standard where " << lowestTuning << " is less than, or equal to, " << Settings::GetModSetting("ExtendedRangeMode") << " minus 2. Drop Tuned: " << std::boolalpha << dropTuning << std::endl);
		return true;
	}

	LOG_INFO("Failed: IsExtendedRangeSong. Drop at " << Settings::GetModSetting("ExtendedRangeMode") << " but received " << lowestTuning << " with drop tuning " << Settings::ReturnSettingValue("ExtendedRangeDropTuning") << std::endl);
	return false;
}

/// <returns>Should we Display The Extended Range Colors In The Tuner?</returns>
bool SongTuning::IsExtendedRangeTuner() {
	uintptr_t addrTuningText = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_tuningText, Offsets::ptr_tuningTextOffsets);

	// Either null or is not in a pre-song tuner
	if (!addrTuningText) {
		LOG_ERROR("Invalid Pointer: IsExtendedRangeTuner" << std::endl);
		return false;
	}

	// Verify the user actually wants Extended Range enabled.
	if (Settings::ReturnSettingValue("ExtendedRangeEnabled") != "on")
		return false;

	Tuning tunerSongTuning = GetTuningAtTuner();

	// Tuning Not Found
	if (tunerSongTuning.lowE == 69) {
		LOG_ERROR("Invalid Tuning: IsExtendedRangeTuner" << std::endl);
		return false;
	}

	int lowestTuning = tunerSongTuning.lowE;

	// Bass below C standard fix (A220 range)
	if (GetTrueTuning() <= 260)
		lowestTuning -= 12;

	bool inDrop = IsSongInDrop(tunerSongTuning);

	// The games stores tunings in unsigned chars (bytes) so we need to convert it to our int Extended Range toggle.
	if (lowestTuning > 24)
		lowestTuning -= 256;

	// Does the user's settings allow us to toggle on drop tunings (ER on B, trigger on C# Drop B)
	if (Settings::ReturnSettingValue("ExtendedRangeDropTuning") == "on" && inDrop && lowestTuning <= Settings::GetModSetting("ExtendedRangeMode")) {
		LOG_INFO("Successful: IsExtendedRangeTuner in DROP where " << lowestTuning << " is less than, or equal to, " << Settings::GetModSetting("ExtendedRangeMode") << std::endl);
		return true;
	}

	// Does the user's settings allow us to toggle Extended Range Mode for this tuning
	if (lowestTuning <= Settings::GetModSetting("ExtendedRangeMode") && (!inDrop || lowestTuning <= Settings::GetModSetting("ExtendedRangeMode") - 2)) {
		LOG_INFO("Successful: IsExtendedRangeTuner in standard where " << lowestTuning << " is less than, or equal to, " << Settings::GetModSetting("ExtendedRangeMode") << " minus 2. Drop Tuned: " << std::boolalpha << inDrop << std::endl);
		return true;
	}

	LOG_INFO("Failed: IsExtendedRangeTuner. Drop at " << Settings::GetModSetting("ExtendedRangeMode") << " but received " << lowestTuning << " with drop tuning " << Settings::ReturnSettingValue("ExtendedRangeDropTuning") << std::endl);
	return false;
}

/// <summary>
/// Gets the highest tuned string, and the lowest tuned string.
/// </summary>
/// <returns>[0] - Highest, [1] - Lowest</returns>
std::array<int, 2> SongTuning::GetHighestLowestString() {
	int highestTuning = 0;
	int lowestTuning = 256;
	int currentStringTuning = 0;

	auto songTuning = GetCurrentTuning();
	if (songTuning == std::array<byte, 6>{}) {
		return { 666, 666 };
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

	return { highestTuning, lowestTuning };
}

/// <summary>
/// Gets the highest tuned string, and the lowest tuned string.
/// </summary>
/// <returns>[0] - Highest, [1] - Lowest</returns>
std::array<int, 2> SongTuning::GetHighestLowestString(Tuning tuningOverride) {
	int highestTuning = 0;
	int lowestTuning = 256;

	if (tuningOverride.lowE == 69) {
		return { 666, 666 };
	}

	int numberOfStrings = (Settings::ReturnSettingValue("ExtendedRangeFixBassTuning") == "on" && (tuningOverride.strB == 0 || tuningOverride.strB == 12) && (tuningOverride.highE == 0 || tuningOverride.highE == 12)) ? 4 : 6; // When a charter makes a bad bass tuning, and leaves the last two strings blank, let's fix that.

	bool bassOctaveEffect = GetTrueTuning() == 220;

	// If the song is in A220, we need to remove 12 from the bass tuning to get the real tuning.
	if (bassOctaveEffect) {
		tuningOverride.lowE -= 12;
		tuningOverride.strA -= 12;
		tuningOverride.strD -= 12;
		tuningOverride.strG -= 12;
		tuningOverride.strB -= 12;
		tuningOverride.highE -= 12;
	}

	int string_lowE = tuningOverride.lowE <= 24 ? tuningOverride.lowE + 256 : tuningOverride.lowE;
	int string_A = tuningOverride.strA <= 24 ? tuningOverride.strA + 256 : tuningOverride.strA;
	int string_D = tuningOverride.strD <= 24 ? tuningOverride.strD + 256 : tuningOverride.strD;
	int string_G = tuningOverride.strG <= 24 ? tuningOverride.strG + 256 : tuningOverride.strG;
	int string_B = tuningOverride.strB <= 24 ? tuningOverride.strB + 256 : tuningOverride.strB;
	int string_highE = tuningOverride.highE <= 24 ? tuningOverride.highE + 256 : tuningOverride.highE;

	// Get the highest tuning used.
	highestTuning = string_lowE > highestTuning ? string_lowE : highestTuning;
	highestTuning = string_A > highestTuning ? string_A : highestTuning;
	highestTuning = string_D > highestTuning ? string_D : highestTuning;
	highestTuning = string_G > highestTuning ? string_G : highestTuning;

	// Get the lowest tuning used
	lowestTuning = string_lowE < lowestTuning ? string_lowE : lowestTuning;
	lowestTuning = string_A < lowestTuning ? string_A : lowestTuning;
	lowestTuning = string_D < lowestTuning ? string_D : lowestTuning;
	lowestTuning = string_G < lowestTuning ? string_G : lowestTuning;

	if (numberOfStrings == 6) {
		// Get the highest tuning used (guitar).
		highestTuning = string_B > highestTuning ? string_B : highestTuning;
		highestTuning = string_highE > highestTuning ? string_highE : highestTuning;

		// Get the lowest tuning used (guitar).
		lowestTuning = string_B < lowestTuning ? string_B : lowestTuning;
		lowestTuning = string_highE < lowestTuning ? string_highE : lowestTuning;
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

	return { highestTuning, lowestTuning };
}


/// <param name="tuning"> - Song Tuning</param>
/// <returns>Are we playing in Drop tuning? ex: D Drop C</returns>
bool SongTuning::IsSongInDrop(Tuning tuning) {
	int NEGATE_DROP = tuning.lowE + 2;
	return tuning.strA == NEGATE_DROP && tuning.strD == NEGATE_DROP && tuning.strG == NEGATE_DROP && tuning.strB == NEGATE_DROP && tuning.highE == NEGATE_DROP;
}

/// <param name="tuning"> - Song Tuning</param>
/// <returns>Are we playing in Standard tuning? ex: E Standard</returns>
bool SongTuning::IsSongInStandard(Tuning tuning) {
	int COMMON_TUNING = tuning.lowE;
	return tuning.strA == COMMON_TUNING && tuning.strD == COMMON_TUNING && tuning.strG == COMMON_TUNING && tuning.strB == COMMON_TUNING && tuning.highE == COMMON_TUNING;
}

bool SongTuning::TryGetTrueTuning(float& trueTuning, uintptr_t& address)
{
	address = MemUtil::FindDMAAddy(
		Offsets::baseHandle + Offsets::ptr_trueTuning,
		Offsets::ptr_trueTuningOffsets,
		true);

	if (address == 0)
	{
		return false;
	}

	const float value = *reinterpret_cast<const float*>(address);
	if (!std::isfinite(value) || value <= 0.0f)
	{
		address = 0;
		return false;
	}

	trueTuning = value;
	return true;
}

/// <returns>True Tuning. ex: A440, A432</returns>
int SongTuning::GetTrueTuning()
{
	float rawTuningValue = 0.0f;
	if (DropPedal::TryGetAuthoredTrueTuning(rawTuningValue))
	{
		return static_cast<int>(floor(rawTuningValue));
	}

	uintptr_t address = 0;
	if (!TryGetTrueTuning(rawTuningValue, address))
	{
		return 440;
	}

	return static_cast<int>(floor(rawTuningValue));
}
