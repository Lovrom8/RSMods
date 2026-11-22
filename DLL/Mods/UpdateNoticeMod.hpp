#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "../Framework/Framework.hpp"

// Tells the player when RSMods or RS_ASIO has a newer GitHub release.
//   - Worker thread: waits for RS_ASIO to load, asks GitHub for the latest releases over WinHTTP and
//     keeps the tags. It never touches game memory. GitHub is asked about once a month: the last
//     check's time and tags live in the shared cache file, and launches in between compare those
//     stored tags instead of calling the API, so a release day does not send every install at once.
//   - MainThread (OnMenuTick): shows the line through GameNotification once the main menu is ready.
class UpdateNoticeMod : public Framework::IMod {
public:
	MOD_ID(UpdateNoticeMod)
	~UpdateNoticeMod() override;
	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;
	void OnMenuTick(Framework::ModContext& c) override;
	void OnShutdown(Framework::ModContext& c) override;

private:
	void Worker();
	// Asks the worker to finish and joins it. Safe to call more than once.
	void Stop();
	bool WaitFor(std::chrono::milliseconds time);
	std::optional<std::string> FetchLatestTag(const wchar_t* path);
	// Compares the tags with the installed versions and leaves the line for MainThread.
	void Publish(const std::string& rsModsTag, const std::string& rsAsioTag);

	std::thread worker_;
	std::mutex mutex_;
	std::condition_variable wake_;
	bool stop_ = false;
	// The request in flight (a WinHTTP handle), so OnShutdown can abort it instead of waiting out a timeout.
	void* request_ = nullptr;
	// Finished text, set by the worker, taken by MainThread. Empty once checked means nothing to show.
	std::optional<std::string> message_;

	bool started_ = false;
	bool shown_ = false;
	int attempts_ = 0;
	std::optional<std::chrono::steady_clock::time_point> menuReadyAt_;
};
