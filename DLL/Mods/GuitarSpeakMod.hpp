#pragma once

#include "../Framework/Framework.hpp"

#include <thread>

class GuitarSpeakMod : public Framework::IMod {
public:
	MOD_ID(GuitarSpeakMod)

	void OnMenuTick(Framework::ModContext& c) override;
	void OnSettingsChanged(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;
	void OnShutdown(Framework::ModContext& c) override;

	Framework::SettingDefs Settings() const override;

private:
	void StopListening();

	// The listen loop polls the guitar every 50 ms, so it gets its own thread rather than MainThread.
	std::jthread listener;
};
