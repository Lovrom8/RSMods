#include "stdafx.h"
#include "Keybindings.hpp"
#include "Mods/RiffRepeater.hpp"

namespace {
	Framework::KeyEvent CaptureKeyEvent(WPARAM keyPressed, LPARAM lParam, Framework::KeyEdge edge) {
		Framework::KeyEvent event;
		event.virtualKey = static_cast<std::uint32_t>(keyPressed);
		event.edge = edge;
		event.control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
		event.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
		event.alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
		event.repeat = edge == Framework::KeyEdge::Down && (lParam & (1LL << 30)) != 0;
		return event;
	}
}

namespace Keybindings {
	void HandleKeyUp(WPARAM keyPressed, LPARAM lParam) {
		Framework::Inbox().PostKeyEvent(CaptureKeyEvent(
			keyPressed, lParam, Framework::KeyEdge::Up));
	}

	void HandleKeyDown(WPARAM keyPressed, LPARAM lParam) {
		Framework::Inbox().PostKeyEvent(CaptureKeyEvent(
			keyPressed, lParam, Framework::KeyEdge::Down));
	}

	void InitializeCommands() {
		auto& commands = Framework::Commands();
		commands.SetKeyResolver([](std::string_view setting) {
			return Settings::GetKeyBind(std::string(setting));
		});
		commands.BindKey(nullptr, "ReloadSettings", 'A', Framework::KeyEdge::Up,
			Framework::Availability::Initialized,
			[](Framework::ModContext&, const Framework::KeyEvent&) {
				Framework::Registry().EnqueueSettingsUpdate([] { Settings::UpdateSettings(); });
				LOG_INFO("Triggered Setting Update" << std::endl);
			},
			[](const Framework::ModContext&, const Framework::KeyEvent& event) {
				return event.control;
			});
		commands.BindKey(nullptr, "ToggleDebugMenu", VK_BACK, Framework::KeyEdge::Up,
			Framework::Availability::Initialized,
			[](Framework::ModContext&, const Framework::KeyEvent&) {
				Menu::menuEnabled = !Menu::menuEnabled;
			},
			[](const Framework::ModContext&, const Framework::KeyEvent&) {
				return D3DHooks::debug;
			});
	}

	void UpdateSettingsOnGUIChange(LPARAM lParam) {
		auto pcds = reinterpret_cast<COPYDATASTRUCT*>(lParam);
		if (pcds->dwData != 1) return;

		std::string currMsg = static_cast<char*>(pcds->lpData);
		LOG_INFO(currMsg << std::endl);

		if (Contains(currMsg, "update")) {
			if (Contains(currMsg, "all"))
				Framework::Registry().EnqueueSettingsUpdate([] { Settings::UpdateSettings(); });
			else
				Framework::Registry().EnqueueSettingsUpdate([currMsg] { Settings::ParseSettingUpdate(currMsg); });
		}
		else if (Contains(currMsg, "WwiseEvent")) {
			auto msgParts = Settings::SplitByWhitespace(currMsg);
			if (msgParts.size() == 2)
				VoiceOverControl::PlayVoiceOver(msgParts[1]);
		}
		else if (Contains(currMsg, "TurboSpeed")) { // Deprecated - use CrowdControl
			if (Contains(currMsg, "enable"))
				RiffRepeater::EnableTimeStretch();
			else
				RiffRepeater::DisableTimeStretch();
		}
		else if (Contains(currMsg, "enable"))
			Twitch::effectQueue.push_back(currMsg);
		else if (Contains(currMsg, "Reconnect"))
			CrowdControl::StartServerLoop();
	}
}
