#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <thread>

#include "Enumeration.hpp"
#include "../Framework/Framework.hpp"

class EnumerationMod : public Framework::IMod {
public:
	MOD_ID(EnumerationMod)

	void OnInitialize(Framework::ModContext& c) override;
	void OnSettingsChanged(Framework::ModContext& c) override;
	void OnTick(Framework::ModContext& c) override;
	void OnShutdown(Framework::ModContext& c) override;

	Framework::SettingDefs Settings() const override;

private:
	void UpdateSettings(const Framework::ModContext& c);
	void MonitorDlcDirectory(std::stop_token st);
	bool WaitFor(std::stop_token st, std::chrono::milliseconds duration);

	std::atomic<bool> automatic{ false };
	std::atomic<int> intervalMs{ 0 };
	std::atomic<bool> enumerationRequested{ false }; // Set by monitor thread; consumed on the main thread in OnTick.
	std::mutex waitMutex;
	std::condition_variable_any waitCondition;
	std::jthread monitorThread;
};
