#include "stdafx.h"
#include "Twitch.hpp"
#include "Framework/Framework.hpp"
#include "D3D/D3DHooks.hpp"

namespace Setting = Settings::Setting;

namespace Twitch {
	/// <summary>
	/// Handle Twitch Toggle Message.
	/// </summary>
	/// <param name="currMsg"> - Mod to Trigger.</param>
	/// <param name="type"> - Should We Enable or Disable.</param>
	/// <returns>BOOL. If effects triggered.</returns>
	bool HandleMessage(std::string const& currMsg, std::string const& type) {
		// Twitch wants Rainbow String mod.
		if (Contains(currMsg, Setting::Twitch::RainbowStrings)) {
			ERMode::RainbowEnabled = type == "enable";
		}

		// Twitch wants Drunk Mode.
		else if (Contains(currMsg, Setting::Twitch::DrunkMode)) {
			Loft::ToggleDrunkMode(type == "enable");
		}

		// Twitch wants Solid Note colors.
		else if (Contains(currMsg, Setting::Twitch::SolidNotes)) {
			// Don't apply any effects if we haven't even been in a song yet
			if (!ERMode::ColorsSaved)
				return false;

			if (type == "enable") {
				if (Contains(currMsg, "Random")) {
					static std::uniform_real_distribution<> urd(0, 9);
					D3DHooks::currentRandomTexture = (int)urd(rng);

					ERMode::customSolidColor.clear();
					if (D3DHooks::currentRandomTexture >= 0 && D3DHooks::currentRandomTexture < (int)D3DHooks::randomTextureColors.size()) {
						ERMode::customSolidColor.insert(ERMode::customSolidColor.begin(), 6, D3DHooks::randomTextureColors[D3DHooks::currentRandomTexture]);
					}

					if (D3DHooks::currentRandomTexture >= 0 && D3DHooks::currentRandomTexture < (int)D3DHooks::randomTextures.size()) {
						D3DHooks::twitchUserDefinedTexture = D3DHooks::randomTextures[D3DHooks::currentRandomTexture];
					}
				}
				else {
					Framework::Registry().EnqueueSettingsUpdate([currMsg, type] {
						Settings::ParseSolidColorsMessage(currMsg);
						Settings::ParseTwitchToggle(currMsg, type);
						D3DHooks::regenerateUserDefinedTexture = true;
					});
					return true;
				}
			}
			else
				ERMode::ResetAllStrings();
		}

		Framework::Registry().EnqueueSettingsUpdate([currMsg, type] {
			Settings::ParseTwitchToggle(currMsg, type);
		});
		return true;
	}

	/// <summary>
	/// Trigger Twitch Effect
	/// </summary>
	/// <param name="currEffectMsg"> - Mod to Trigger.</param>
	void HandleEffect(std::string const& currEffectMsg) {
		auto msgParts = Settings::SplitByWhitespace(currEffectMsg);
		std::string effectName = msgParts[1];

		LOG_INFO("Entering the thread for: " << currEffectMsg << std::endl);

		// Don't allow the current effect to apply twice. Also blocks mods from triggering when not in a song.
		while (IsCurrentEffectAlreadyAppliedOrNotInSong(effectName))
			Sleep(150);

		LOG_INFO("Enabled effects count: " << enabledEffects.size() << std::endl);
		LOG_INFO("Enabling " << effectName << std::endl);

		if (HandleMessage(currEffectMsg, "enable")) {
			// Sleep for the duration of the effect.
			Sleep(std::stoi(msgParts.back()) * 1000);

			// Disable the effect after it's done
			HandleMessage(currEffectMsg, "disable");
			DisableEffect(effectName);
		}
	}

	void DisableEffect(const std::string& effectName) 
	{
		LOG_INFO("Disabling " << effectName << std::endl);

		if (Contains(effectName, enabledEffects)) // JIC
			std::erase(enabledEffects, effectName);
	}

	static bool IsCurrentEffectAlreadyAppliedOrNotInSong(const std::string& effectName)
	{
		return Contains(effectName, enabledEffects) || !GameState::IsInSong();
	}

	void ParseEffectQueue() {
		for (auto it = effectQueue.begin(); it != effectQueue.end();) {
			std::string effectName = Settings::SplitByWhitespace(*it)[1];

			if (!Contains(effectName, enabledEffects)) {
				enabledEffects.push_back(effectName);

				// Send full effect message to the thread
				std::thread effectThread(HandleEffect, *it);
				effectThread.detach();

				it = effectQueue.erase(it);
			}
			else
				++it;
		}
	}
}
