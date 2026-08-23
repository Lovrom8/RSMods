#include "../../stdafx.h"
#include "SolidNotesEffect.hpp"
#include "../../Framework/Framework.hpp"

namespace Setting = Settings::Setting;

namespace CrowdControl::Effects {
	using enum Enums::EffectStatus;

	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus SolidNotesCustomEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("SolidNotesCustomEffect::Test()" << std::endl);

		if (!CanStart())
			return Retry;

		return Success;
	}

	/// <summary>
	/// Default colors for strings.
	/// </summary>
	const std::map<std::string, std::string, std::less<>> CustomColorMap = {
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
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus SolidNotesCustomEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("SolidNotesCustomEffect::Start()" << std::endl);

		running = true;

		//Get color from parameters
		auto color = request.parameters.at(0).get<std::string>();
		const auto& hexColor = CustomColorMap.at(color);

		// Update note texture
		Framework::Registry().EnqueueSettingsUpdate([hexColor] {
			Settings::UpdateModSetting(Setting::SolidNoteColor, hexColor);
			Settings::UpdateTwitchSetting(Setting::Twitch::SolidNotes, "on");
			D3DHooks::regenerateUserDefinedTexture = true;
		});

		SetDuration(request);

		return Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success</returns>
	Enums::EffectStatus SolidNotesCustomEffect::OnStop()
	{
		LOG_INFO("SolidNotesCustomEffect::Stop()" << std::endl);

		Framework::Registry().EnqueueSettingsUpdate([] {
			Settings::UpdateTwitchSetting(Setting::Twitch::SolidNotes, "off");
		});
		running = false;
		//ERMode::ResetAllStrings();

		return Success;
	}

	//////////////////////////////////////////////////////////////

	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus SolidNotesRandomEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("SolidNotesRandomEffect::Test()" << std::endl);

		if (!CanStart())
			return Retry;

		return Success;
	}

	/// <summary>
	/// Change the color of the strings to the ones specified in the request.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus SolidNotesRandomEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("SolidNotesRandomEffect::Start()" << std::endl);

		LOG_INFO("SolidNotesRandomEffect - Colors Saved" << std::endl);
		
		static std::uniform_real_distribution<> urd(0, randomTextureCount - 1);
		currentRandomTexture = urd(rng);

		LOG_INFO("SolidNotesRandomEffect - Picked color " << currentRandomTexture << "/" << randomTextureCount << std::endl);

		// Set random solid color
		ERMode::customSolidColor.clear();
		ERMode::customSolidColor.insert(ERMode::customSolidColor.begin(), 6, randomTextureColors[currentRandomTexture]);

		twitchUserDefinedTexture = randomTextures[currentRandomTexture];

		Framework::Registry().EnqueueSettingsUpdate([] {
			Settings::UpdateTwitchSetting(Setting::Twitch::SolidNotes, "on");
		});

		SetDuration(request);
		running = true;

		return Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success</returns>
	Enums::EffectStatus SolidNotesRandomEffect::OnStop()
	{
		LOG_INFO("SolidNotesRandomEffect::Stop()" << std::endl);

		Framework::Registry().EnqueueSettingsUpdate([] {
			Settings::UpdateTwitchSetting(Setting::Twitch::SolidNotes, "off");
		});
		running = false;
		//ERMode::ResetAllStrings();

		return Success;
	}

	//////////////////////////////////////////////////////////////

	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus SolidNotesCustomRGBEffect::Test(const Structs::Request& request)
	{
		LOG_INFO("SolidNotesCustomRGBEffect::Test()" << std::endl);

		if (!CanStart())
			return Retry;

		return Success;
	}

	/// <summary>
	/// Change the color of the strings to the ones specified in the request.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>Enums::EffectStatus::Success if test completed without any issues. Enums::EffectStatus::Retry if we have to retry.</returns>
	Enums::EffectStatus SolidNotesCustomRGBEffect::OnStart(const Structs::Request& request)
	{
		LOG_INFO("SolidNotesCustomRGBEffect::Start()" << std::endl);

		//Get color from parameters
		byte r, g, b;
		request.parameters.at(0).get_to(r);
		request.parameters.at(1).get_to(g);
		request.parameters.at(2).get_to(b);

		LOG_INFO("Color: " << (int)r << "," << (int)g << "," << (int)b << std::endl);

		// Convert RGB to hex
		std::stringstream ss; 
		ss << std::setw(6) << std::setfill('0') << std::hex;
		ss << (r << 16 | g << 8 | b);

		// Update note texture and publish its regeneration request as one ordered settings change.
		const std::string hexColor = ss.str();
		Framework::Registry().EnqueueSettingsUpdate([hexColor] {
			Settings::UpdateModSetting(Setting::SolidNoteColor, hexColor);
			Settings::UpdateTwitchSetting(Setting::Twitch::SolidNotes, "on");
			D3DHooks::regenerateUserDefinedTexture = true;
		});

		SetDuration(request);
		running = true;

		return Success;
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>Enums::EffectStatus::Success</returns>
	Enums::EffectStatus SolidNotesCustomRGBEffect::OnStop()
	{
		LOG_INFO("SolidNotesCustomRGBEffect::Stop()" << std::endl);

		Framework::Registry().EnqueueSettingsUpdate([] {
			Settings::UpdateTwitchSetting(Setting::Twitch::SolidNotes, "off");
		});
		running = false;
		//ERMode::ResetAllStrings();

		return Success;
	}
}
