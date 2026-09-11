#include "../stdafx.h"
#include "EnumerationMod.hpp"

using Framework::ModContext;
using Framework::KeyEdge;
using Framework::Availability;
using Framework::KeyEvent;
using Framework::SettingDefs;
using Framework::Enum;
using Framework::Numeric;
using Settings::When;
namespace Setting = Settings::Setting;

SettingDefs EnumerationMod::Settings() const {
	return {
		Enum(Setting::ForceReEnumerationEnabled, "Force Enumeration")
			.Ini("Toggle Switches", "ForceReEnumeration")
			.Choices({ "off", "manual", "automatic" }, "off"),
		Numeric(Setting::CheckForNewSongsInterval, "Song Scan Interval (seconds)")
			.Ini("Mod Settings", "CheckForNewSongsInterval")
			.Default("5000")
			.Range(100, 100000000)
			.Scale(0.001)
			.WithVisibleWhen(Setting::ForceReEnumerationEnabled, "automatic"),
	};
}

void EnumerationMod::OnInitialize(ModContext& c) {
	c.Commands().BindSetting(
		Setting::Key::ForceReEnumeration,
		KeyEdge::Up,
		Availability::Active,
		[](ModContext&, const KeyEvent&) {
			Enumeration::ForceEnumeration();
		},
		[](const ModContext& context, const KeyEvent&) {
			return context.When(Setting::ForceReEnumerationEnabled) == When::Manual;
		},
		"Force Enumeration");

	UpdateSettings(c);
	monitorThread = std::jthread([this](std::stop_token st) { MonitorDlcDirectory(st); });
}

void EnumerationMod::OnSettingsChanged(ModContext& c) {
	UpdateSettings(c);
}

// Runs on the main thread. ForceEnumeration() writes game memory, so it must never be
// called from the monitor thread; the monitor only flags that the DLC count changed.
void EnumerationMod::OnTick(ModContext&) {
	if (enumerationRequested.exchange(false))
		Enumeration::ForceEnumeration();
}

void EnumerationMod::OnShutdown(ModContext&) {
	monitorThread.request_stop();

	if (monitorThread.joinable()) {
		monitorThread.join();
	}
}

void EnumerationMod::UpdateSettings(const ModContext& c) {
	automatic.store(c.When(Setting::ForceReEnumerationEnabled) == When::Automatic);

	intervalMs.store(c.Int(Setting::CheckForNewSongsInterval));
}

void EnumerationMod::MonitorDlcDirectory(std::stop_token st) {
	while (!GameState::GameLoaded) {
		if (WaitFor(st, std::chrono::seconds(5))) return;
	}

	int previousDlcCount = Enumeration::GetCurrentDLCCount();

	while (true) {
		const auto interval = std::chrono::milliseconds(intervalMs.load());
		if (WaitFor(st, interval)) return;
		if (!automatic.load()) continue;

		const int currentDlcCount = Enumeration::GetCurrentDLCCount();
		if (previousDlcCount != currentDlcCount) enumerationRequested.store(true);
		previousDlcCount = currentDlcCount;
	}
}

bool EnumerationMod::WaitFor(std::stop_token st, std::chrono::milliseconds duration) {
	std::unique_lock lock(waitMutex);
	waitCondition.wait_for(lock, st, duration, [] { return false; });
	return st.stop_requested();
}


static Framework::ModRegistrar<EnumerationMod> _enumerationReg;
