#include "SolidNotesEffect.hpp"
#include "../../Lib/Json/json.hpp"
#include <sstream>

namespace CrowdControl::Effects {
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus SolidNotesCustomEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("SolidNotesCustomEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Default colors for strings.
	/// </summary>
	std::map<std::string, std::string> CustomColorMap = {
		{"red", "FF0000"},
		{"yellow", "FFFF00"},
		{"blue", "0000FF"},
		{"orange", "FFA500"},
		{"green", "00FF00"},
		{"purple", "800080"}
	};

	/// <summary>
	/// Change the color of the strings to the ones specified in the request.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus SolidNotesCustomEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("SolidNotesCustomEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		running = true;

		//Get color from parameters
		auto color = request.parameters.at(0).get<std::string>();
		auto hexColor = CustomColorMap[color];

		// Update note texture
		Settings::UpdateModSetting("SolidNoteColor", hexColor);
		D3DHooks::regenerateUserDefinedTexture = true;

		Settings::UpdateTwitchSetting("SolidNotes", "on");

		SetDuration(request);

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus SolidNotesCustomEffect::Stop()
	{
		_LOG_INIT;

		_LOG("SolidNotesCustomEffect::Stop()" << std::endl);

		Settings::UpdateTwitchSetting("SolidNotes", "off");
		running = false;
		//ERMode::ResetAllStrings();

		return EffectStatus::Success;
	}

	//////////////////////////////////////////////////////////////

	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus SolidNotesRandomEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("SolidNotesRandomEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Change the color of the strings to the ones specified in the request.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus SolidNotesRandomEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("SolidNotesRandomEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		_LOG("SolidNotesRandomEffect - Colors Saved" << std::endl);
		
		static std::uniform_real_distribution<> urd(0, randomTextureCount - 1);
		currentRandomTexture = urd(rng);

		_LOG("SolidNotesRandomEffect - Picked color " << currentRandomTexture << "/" << randomTextureCount << std::endl);

		// Set random solid color
		ERMode::customSolidColor.clear();
		ERMode::customSolidColor.insert(ERMode::customSolidColor.begin(), 6, randomTextureColors[currentRandomTexture]);

		twitchUserDefinedTexture = randomTextures[currentRandomTexture];

		Settings::UpdateTwitchSetting("SolidNotes", "on");

		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus SolidNotesRandomEffect::Stop()
	{
		_LOG_INIT;

		_LOG("SolidNotesRandomEffect::Stop()" << std::endl);

		Settings::UpdateTwitchSetting("SolidNotes", "off");
		running = false;
		//ERMode::ResetAllStrings();

		return EffectStatus::Success;
	}

	//////////////////////////////////////////////////////////////

	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus SolidNotesCustomRGBEffect::Test(Request request)
	{
		_LOG_INIT;

		_LOG("SolidNotesCustomRGBEffect::Test()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Change the color of the strings to the ones specified in the request.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectStatus::Success if test completed without any issues. EffectStatus::Retry if we have to retry.</returns>
	EffectStatus SolidNotesCustomRGBEffect::Start(Request request)
	{
		_LOG_INIT;

		_LOG("SolidNotesCustomRGBEffect::Start()" << std::endl);

		if (!CanStart(&EffectList::AllEffects))
			return EffectStatus::Retry;

		//Get color from parameters
		byte r, g, b;
		request.parameters.at(0).get_to(r);
		request.parameters.at(1).get_to(g);
		request.parameters.at(2).get_to(b);

		_LOG("Color: " << (int)r << "," << (int)g << "," << (int)b << std::endl);

		// Convert RGB to hex
		std::stringstream ss; 
		ss << std::setw(6) << std::setfill('0') << std::hex;
		ss << (r << 16 | g << 8 | b);

		// Update note texture
		Settings::UpdateModSetting("SolidNoteColor", ss.str());
		D3DHooks::regenerateUserDefinedTexture = true;

		Settings::UpdateTwitchSetting("SolidNotes", "on");

		SetDuration(request);
		running = true;

		return EffectStatus::Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectStatus::Success</returns>
	EffectStatus SolidNotesCustomRGBEffect::Stop()
	{
		_LOG_INIT;

		_LOG("SolidNotesCustomRGBEffect::Stop()" << std::endl);

		Settings::UpdateTwitchSetting("SolidNotes", "off");
		running = false;
		//ERMode::ResetAllStrings();

		return EffectStatus::Success;
	}
}