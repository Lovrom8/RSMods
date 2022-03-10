#include "SolidNotesEffect.hpp"
#include "../../Lib/Json/json.hpp"
#include <sstream>

namespace CrowdControl::Effects { // Changes current note heads to a custom generated texture
	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult SolidNotesCustomEffect::Test(Request request)
	{
		std::cout << "SolidNotesCustomEffect::Test()" << std::endl;

		if (!ERMode::ColorsSaved || !MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		return EffectResult::Success;
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
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult SolidNotesCustomEffect::Start(Request request)
	{
		std::cout << "SolidNotesCustomEffect::Start()" << std::endl;

		if (!ERMode::ColorsSaved || !MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		running = true;

		//Get color from parameters
		auto color = request.parameters.at(0).get<std::string>();
		auto hexColor = CustomColorMap[color];

		// Update note texture
		Settings::UpdateModSetting("SolidNoteColor", hexColor);
		D3DHooks::regenerateUserDefinedTexture = true;

		Settings::UpdateTwitchSetting("SolidNotes", "on");

		if (request.parameters.contains("duration"))
			request.parameters.at("duration").get_to(duration);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void SolidNotesCustomEffect::Run()
	{
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectResult::Success</returns>
	EffectResult SolidNotesCustomEffect::Stop()
	{
		std::cout << "SolidNotesCustomEffect::Stop()" << std::endl;

		running = false;
		Settings::UpdateTwitchSetting("SolidNotes", "off");
		//ERMode::ResetAllStrings();

		return EffectResult::Success;
	}

	//////////////////////////////////////////////////////////////

	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult SolidNotesRandomEffect::Test(Request request)
	{
		std::cout << "SolidNotesRandomEffect::Test()" << std::endl;

		if (!ERMode::ColorsSaved || !MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// Change the color of the strings to the ones specified in the request.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult SolidNotesRandomEffect::Start(Request request)
	{
		std::cout << "SolidNotesRandomEffect::Start()" << std::endl;

		if (!ERMode::ColorsSaved || !MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;


		std::cout << "SolidNotesRandomEffect - Colors Saved" << std::endl;

		running = true;

		Settings::UpdateTwitchSetting("SolidNotes", "on");

		static std::uniform_real_distribution<> urd(0, 9);
		currentRandomTexture = urd(rng);

		// Set random solid color
		ERMode::customSolidColor.clear();
		ERMode::customSolidColor.insert(ERMode::customSolidColor.begin(), randomTextureColors[currentRandomTexture].begin(), randomTextureColors[currentRandomTexture].end());

		twitchUserDefinedTexture = randomTextures[currentRandomTexture];

		if (request.parameters.contains("duration"))
			request.parameters.at("duration").get_to(duration);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void SolidNotesRandomEffect::Run()
	{
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectResult::Success</returns>
	EffectResult SolidNotesRandomEffect::Stop()
	{
		std::cout << "SolidNotesRandomEffect::Stop()" << std::endl;

		running = false;
		Settings::UpdateTwitchSetting("SolidNotes", "off");
		//ERMode::ResetAllStrings();

		return EffectResult::Success;
	}

	//////////////////////////////////////////////////////////////

	
	/// <summary>
	/// Test the twitch mod's requirements.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult SolidNotesCustomRGBEffect::Test(Request request)
	{
		std::cout << "SolidNotesCustomRGBEffect::Test()" << std::endl;

		if (!ERMode::ColorsSaved || !MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		return EffectResult::Success;
	}

	/// <summary>
	/// Change the color of the strings to the ones specified in the request.
	/// </summary>
	/// <param name="request"> - JSON Request</param>
	/// <returns>EffectResult::Success if test completed without any issues. EffectResult::Retry if we have to retry.</returns>
	EffectResult SolidNotesCustomRGBEffect::Start(Request request)
	{
		std::cout << "SolidNotesCustomRGBEffect::Start()" << std::endl;

		if (!ERMode::ColorsSaved || !MemHelpers::IsInSong() || EffectList::AreIncompatibleEffectsEnabled(incompatibleEffects) || running)
			return EffectResult::Retry;

		running = true;

		//Get color from parameters
		byte r, g, b;
		request.parameters.at(0).get_to(r);
		request.parameters.at(1).get_to(g);
		request.parameters.at(2).get_to(b);

		std::cout << "Color: " << (int)r << "," << (int)g << "," << (int)b << std::endl;

		// Convert RGB to hex
		std::stringstream ss; 
		ss << std::setw(6) << std::setfill('0') << std::hex;
		ss << (r << 16 | g << 8 | b);

		// Update note texture
		Settings::UpdateModSetting("SolidNoteColor", ss.str());
		D3DHooks::regenerateUserDefinedTexture = true;

		Settings::UpdateTwitchSetting("SolidNotes", "on");

		SetDuration(request);
		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	/// <summary>
	/// Ensure that the mod only lasts for the time specified in the JSON request.
	/// </summary>
	void SolidNotesCustomRGBEffect::Run()
	{
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	/// <summary>
	/// Stops the mod.
	/// </summary>
	/// <returns>EffectResult::Success</returns>
	EffectResult SolidNotesCustomRGBEffect::Stop()
	{
		std::cout << "SolidNotesCustomRGBEffect::Stop()" << std::endl;

		running = false;
		Settings::UpdateTwitchSetting("SolidNotes", "off");
		//ERMode::ResetAllStrings();

		return EffectResult::Success;
	}
}