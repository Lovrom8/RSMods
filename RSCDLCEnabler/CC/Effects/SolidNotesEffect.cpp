#include "SolidNotesEffect.hpp"
#include "nlohmann/json.hpp"

using namespace CrowdControl::Enums;
using nlohmann::json;

namespace CrowdControl::Effects { // Changes current note heads to a custom generated texture
	EffectResult SolidNotesCustomEffect::Test(Request request)
	{
		std::cout << "SolidNotesCustomEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	std::map<std::string, std::string> CustomColorMap = {
		{"red", "#FF0000"},
		{"yellow", "#000000"},
		{"blue", "#000000"},
		{"orange", "#000000"},
		{"green", "#000000"},
		{"red", "#000000"}
	};

	EffectResult SolidNotesCustomEffect::Start(Request request)
	{
		std::cout << "SolidNotesCustomEffect::Start()" << std::endl;


		if (!ERMode::ColorsSaved) {
			return EffectResult::Retry;
		}

		running = true;

		//Get color from parameters
		auto color = request.parameters.at(0).get<std::string>();
		auto hexColor = CustomColorMap[color];

		//TODO: finish this - currMsg should contain a color in hex set by an user 
		//Settings::ParseSolidColorsMessage(currMsg);
		D3DHooks::regenerateUserDefinedTexture = true;

		Settings::UpdateTwitchSetting("SolidNotes", "on");

		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void SolidNotesCustomEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult SolidNotesCustomEffect::Stop()
	{
		std::cout << "SolidNotesCustomEffect::Stop()" << std::endl;

		running = false;
		Settings::UpdateTwitchSetting("SolidNotes", "off");
		ERMode::ResetAllStrings();

		return EffectResult::Success;
	}

	//////////////////////////////////////////////////////////////

	EffectResult SolidNotesRandomEffect::Test(Request request)
	{
		std::cout << "SolidNotesRandomEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult SolidNotesRandomEffect::Start(Request request)
	{
		std::cout << "SolidNotesRandomEffect::Start()" << std::endl;

		if (!ERMode::ColorsSaved) {
			return EffectResult::Retry;
		}

		std::cout << "SolidNotesRandomEffect - Colors Saved" << std::endl;

		running = true;
		Settings::UpdateTwitchSetting("SolidNotes", "on");

		static std::uniform_real_distribution<> urd(0, 9);
		currentRandomTexture = urd(rng);

		ERMode::customSolidColor.clear();
		ERMode::customSolidColor.insert(ERMode::customSolidColor.begin(), randomTextureColors[currentRandomTexture].begin(), randomTextureColors[currentRandomTexture].end());

		twitchUserDefinedTexture = randomTextures[currentRandomTexture];

		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void SolidNotesRandomEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult SolidNotesRandomEffect::Stop()
	{
		std::cout << "SolidNotesRandomEffect::Stop()" << std::endl;

		running = false;
		Settings::UpdateTwitchSetting("SolidNotes", "off");
		ERMode::ResetAllStrings();

		return EffectResult::Success;
	}

	//////////////////////////////////////////////////////////////

	EffectResult SolidNotesCustomRGBEffect::Test(Request request)
	{
		std::cout << "SolidNotesCustomRGBEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult SolidNotesCustomRGBEffect::Start(Request request)
	{
		std::cout << "SolidNotesCustomRGBEffect::Start()" << std::endl;


		if (!ERMode::ColorsSaved) {
			return EffectResult::Retry;
		}

		running = true;

		//Get color from parameters
		byte r, g, b;
		request.parameters.at(0).get_to(r);
		request.parameters.at(1).get_to(g);
		request.parameters.at(2).get_to(b);

		std::cout << "Color: " << (int)r << "," << (int)g << "," << (int)b << std::endl;

		//TODO: finish this - currMsg should contain a color in hex set by an user 
		//Settings::ParseSolidColorsMessage(currMsg);
		D3DHooks::regenerateUserDefinedTexture = true;

		Settings::UpdateTwitchSetting("SolidNotes", "on");

		endTime = std::chrono::steady_clock::now() + std::chrono::seconds(duration);

		return EffectResult::Success;
	}

	void SolidNotesCustomRGBEffect::Run()
	{
		// Stop automatically after duration has elapsed
		if (running) {
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<double> duration = (endTime - now);

			if (duration.count() <= 0) Stop();
		}
	}

	EffectResult SolidNotesCustomRGBEffect::Stop()
	{
		std::cout << "SolidNotesCustomRGBEffect::Stop()" << std::endl;

		running = false;
		Settings::UpdateTwitchSetting("SolidNotes", "off");
		ERMode::ResetAllStrings();

		return EffectResult::Success;
	}
}