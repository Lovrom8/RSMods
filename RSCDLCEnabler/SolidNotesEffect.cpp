#include "SolidNotesEffect.hpp"

using namespace CrowdControl::Enums;

namespace CrowdControl::Effects { // Changes current note heads to a custom generated texture
	EffectResult SolidNotesCustomEffect::Test()
	{
		std::cout << "SolidNotesCustomEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult SolidNotesCustomEffect::Start()
	{
		std::cout << "SolidNotesCustomEffect::Start()" << std::endl;

		if (!ERMode::ColorsSaved) {
			return EffectResult::Retry;
		}
		
		running = true;

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

	EffectResult SolidNotesRandomEffect::Test()
	{
		std::cout << "SolidNotesRandomEffect::Test()" << std::endl;

		return EffectResult::Success;
	}

	EffectResult SolidNotesRandomEffect::Start()
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
}