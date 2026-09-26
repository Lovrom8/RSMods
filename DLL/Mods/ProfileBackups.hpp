#pragma once

#include <chrono>

#include "../Framework/Framework.hpp"

namespace ProfileBackups {
	/// <summary>
	/// Makes sure the profiles, as they are right now, have a backup: makes one if needed.
	/// Fast profile load and save only goes in if this returns true.
	/// </summary>
	bool BackUpBeforeHooking();

	/// <summary>
	/// Held around the game writing the profile, so a backup never copies a half-written file.
	/// Does nothing when backups are off.
	/// </summary>
	class SaveGuard {
	public:
		SaveGuard();
		~SaveGuard();
		SaveGuard(const SaveGuard&) = delete;
		SaveGuard& operator=(const SaveGuard&) = delete;
	private:
		bool locked = false;
	};
}

class ProfileBackupsMod : public Framework::IMod {
public:
	MOD_ID(ProfileBackupsMod)

	// Also owns FastProfileLoadAndSave: ProfileSaveStreaming reads it at startup and only runs with backups on.
	Framework::SettingDefs Settings() const override;

	bool IsEnabled(const Framework::ModContext& c) const override;

	void OnInitialize(Framework::ModContext& c) override;
	void OnEnabled(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;
	void OnTick(Framework::ModContext& c) override;

private:
	bool onAtStartup = false;
	std::chrono::steady_clock::time_point nextCheck{};
};
