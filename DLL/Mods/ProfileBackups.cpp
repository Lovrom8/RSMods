#include "../stdafx.h"
#include "ProfileBackups.hpp"
#include <algorithm>
#include <atomic>
#include <mutex>

namespace fs = std::filesystem;

/// <summary>
/// Copies the Steam save folder (profiles and LocalProfiles.json) to "Profile_AutoBackups\<Steam account>\<tier>\<UTC time>" next to the game,
/// at most once every 10 minutes, and only when something in the folder changed since the last backup.
///
/// Each tier has its own folder and keeps its newest backups:
///		Every 10 Minutes: 6 (the last hour played),
///		Hourly: 6,
///		Quarter Daily (00:00, 06:00, 12:00, 18:00 UTC): 4,
///		Daily (UTC): 30.
/// A new backup goes into every tier that doesn't have one for the current period yet, so each tier holds the first backup of each
/// of its periods. A backup in several tiers is hard links to the same files, so it only takes space once.
///
/// Backups are only made, and old ones only deleted, when the profiles change. Nothing catches up on missed periods: someone who
/// hasn't played in months gets one backup when they start the game, and their old backups are replaced one at a time as they play.
///
/// This is separate from the GUI's "Backup Profile" (Profile_Backups), so neither prunes or misreads the other's folders.
/// </summary>
namespace ProfileBackups {
	namespace {
		/// <summary>
		/// One tier: a folder that gets the first backup of each period (of `seconds` long) and keeps the newest `keep`.
		/// Periods line up with UTC, so an hour starts on the hour and a day at midnight UTC.
		/// </summary>
		struct Tier {
			const wchar_t* folder;
			int64_t seconds;	// Length of one period.
			size_t keep;		// How many backups the folder keeps.
		};
		constexpr Tier tiers[] = {
			{ L"Every 10 Minutes", 10 * 60, 6 },	// The last hour played, for a problem that happened just now.
			{ L"Hourly", 60 * 60, 6 },				// The last 6 hours played, for undoing something from this session.
			{ L"Quarter Daily", 6 * 60 * 60, 4 },	// The last 4 quarter days played, about a day's worth.
			{ L"Daily", 24 * 60 * 60, 30 },			// The last 30 days played, for a problem that took a while to notice.
		};
		const Tier& newestTier = tiers[0]; // Every backup goes in here first, so its newest backup is the newest there is.
		constexpr auto checkEvery = std::chrono::seconds(30);
		constexpr auto saveWaitLimit = std::chrono::seconds(10);
		constexpr wchar_t partialPrefix[] = L".partial_";

		std::atomic<bool> enabled = false;

		// A profile save holds this while the game writes, and a backup holds it while it copies.
		std::timed_mutex filesMutex;

		struct FileState {
			uintmax_t size;
			fs::file_time_type written;
			bool operator==(const FileState&) const = default;
		};
		using FolderState = std::map<std::wstring, FileState>;

		struct Backup {
			fs::path path;
			int64_t time; // UTC seconds
		};

		std::string Utf8(const fs::path& path) {
			const std::u8string text = path.u8string();
			return std::string(text.begin(), text.end());
		}

		/// <summary>
		/// The save folder of the Steam account that's logged in: <Steam>\userdata\<account>\221680\remote.
		/// The game can't tell us this: it only builds "<profile>_PRFLDB" and hands the name to Steam Cloud, which picks the folder.
		/// </summary>
		bool FindSaveFolder(fs::path& folder, DWORD& account) {
			DWORD size = sizeof(account);
			if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam\\ActiveProcess", L"ActiveUser", RRF_RT_REG_DWORD, nullptr, &account, &size) != ERROR_SUCCESS || account == 0)
				return false;

			wchar_t steam[MAX_PATH]{};
			size = sizeof(steam);
			if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath", RRF_RT_REG_SZ, nullptr, steam, &size) != ERROR_SUCCESS)
				return false;

			folder = fs::path(steam) / L"userdata" / std::to_wstring(account) / L"221680" / L"remote";
			std::error_code error;
			return fs::is_directory(folder, error);
		}

		fs::path BackupRoot(DWORD account) {
			wchar_t executable[MAX_PATH]{};
			GetModuleFileNameW(NULL, executable, MAX_PATH);
			return fs::path(executable).parent_path() / L"Profile_AutoBackups" / std::to_wstring(account);
		}

		bool ReadFolder(const fs::path& folder, FolderState& state) {
			state.clear();
			std::error_code error;
			for (fs::directory_iterator it(folder, error), end; !error && it != end; it.increment(error)) {
				if (!it->is_regular_file(error))
					continue;
				const uintmax_t size = it->file_size(error);
				if (error)
					return false;
				const fs::file_time_type written = it->last_write_time(error);
				if (error)
					return false;
				state[it->path().filename().wstring()] = { size, written };
			}
			return !error;
		}

		std::wstring NameFor(int64_t time) {
			tm utc{};
			const __time64_t t = time;
			_gmtime64_s(&utc, &t);
			wchar_t name[32]{};
			wcsftime(name, std::size(name), L"%Y-%m-%d_%H-%M-%S", &utc);
			return name;
		}

		bool TimeOf(const std::wstring& name, int64_t& time) {
			tm utc{};
			wchar_t extra = 0;
			if (swscanf_s(name.c_str(), L"%4d-%2d-%2d_%2d-%2d-%2d%lc", &utc.tm_year, &utc.tm_mon, &utc.tm_mday, &utc.tm_hour, &utc.tm_min, &utc.tm_sec, &extra, 1) != 6)
				return false;
			utc.tm_year -= 1900;
			utc.tm_mon -= 1;
			time = _mkgmtime64(&utc);
			return time != -1 && NameFor(time) == name; // Rejects anything that isn't exactly a name we'd write.
		}

		/// <summary>
		/// A tier's backups, oldest first. Also clears out copies a previous run didn't finish (the game closed mid-copy).
		/// Anything else in the folder is left alone.
		/// </summary>
		std::vector<Backup> ListBackups(const fs::path& tierFolder) {
			std::vector<Backup> backups;
			std::error_code error;
			for (fs::directory_iterator it(tierFolder, error), end; !error && it != end; it.increment(error)) {
				if (!it->is_directory(error))
					continue;
				const std::wstring name = it->path().filename().wstring();
				int64_t time = 0;
				if (name.starts_with(partialPrefix)) {
					std::error_code ignored;
					fs::remove_all(it->path(), ignored);
				}
				else if (TimeOf(name, time))
					backups.push_back({ it->path(), time });
			}
			std::sort(backups.begin(), backups.end(), [](const Backup& a, const Backup& b) { return a.time < b.time; });
			return backups;
		}

		void WriteHowTo(const fs::path& root, const fs::path& saveFolder) {
			std::ofstream howTo(root / L"howto.txt", std::ios::trunc);
			howTo << "RSMods backs up your Rocksmith profiles here while the game runs (at most once every 10 minutes, only when they changed)." << std::endl
				<< "Each folder keeps its newest backups: Every 10 Minutes 6, Hourly 6, Quarter Daily 4, Daily 30." << std::endl
				<< "Before GUI has the backups the RSMods GUI makes when it opens (How Many Backups in the GUI sets how many are kept)." << std::endl
				<< "The RSMods GUI can restore any of these with Revert Profile To Backup." << std::endl
				<< "Backup names are the time of the backup in UTC." << std::endl
				<< "Backups share unchanged files with each other, so don't edit files in here. Copy them out first." << std::endl << std::endl
				<< "To restore one: close Rocksmith, then copy every file from one of the backups into:" << std::endl
				<< Utf8(saveFolder) << std::endl;
		}

		/// <summary>
		/// Copies `source` into "<tierFolder>\<name>". Files that are the same as in `sameFilesIn` are hard links to its copy instead,
		/// so they don't take space twice. Backups are never written to after they're made, so sharing is safe, and deleting one
		/// leaves the others intact. The copy goes to a ".partial_" folder that's renamed when it's complete, so a copy cut short
		/// never looks like a backup.
		/// </summary>
		bool CopyBackup(const fs::path& source, const fs::path& tierFolder, const std::wstring& name, const fs::path* sameFilesIn, fs::path& made) {
			FolderState before, same;
			if (!ReadFolder(source, before) || before.empty())
				return false;
			if (sameFilesIn && !ReadFolder(*sameFilesIn, same))
				same.clear();

			const fs::path partial = tierFolder / (partialPrefix + name);
			std::error_code error;
			fs::create_directories(tierFolder, error);
			fs::remove_all(partial, error);
			if (!fs::create_directories(partial, error))
				return false;

			bool copied = true;
			for (const auto& [file, state] : before) {
				const auto unchanged = same.find(file);
				if (unchanged != same.end() && unchanged->second == state && CreateHardLinkW((partial / file).c_str(), (*sameFilesIn / file).c_str(), nullptr))
					continue;
				if (!CopyFileW((source / file).c_str(), (partial / file).c_str(), TRUE)) { // Keeps the file's modified time.
					copied = false;
					break;
				}
			}

			// Something outside the save wrapper (LocalProfiles.json, a deleted profile) may have written while we copied.
			FolderState after, copy;
			copied = copied && ReadFolder(source, after) && after == before && ReadFolder(partial, copy) && copy == before;

			made = tierFolder / name;
			if (!copied || fs::exists(made, error)) {
				fs::remove_all(partial, error);
				return false;
			}
			fs::rename(partial, made, error);
			if (error) {
				fs::remove_all(partial, error);
				return false;
			}
			return true;
		}

		/// <summary>
		/// Deletes a tier's oldest backups past its `keep`. Only runs after a new backup went in, so backups are only ever replaced.
		/// </summary>
		void Prune(const Tier& tier, const fs::path& tierFolder) {
			const std::vector<Backup> backups = ListBackups(tierFolder);
			for (size_t i = 0; i + tier.keep < backups.size(); i++) {
				std::error_code error;
				fs::remove_all(backups[i].path, error);
				if (error)
					LOG_WARNING("(PROFILE BACKUP) Couldn't delete old backup " << Utf8(tier.folder) << "\\" << Utf8(backups[i].path.filename()) << std::endl);
				else
					LOG_INFO("(PROFILE BACKUP) Deleted old backup " << Utf8(tier.folder) << "\\" << Utf8(backups[i].path.filename()) << std::endl);
			}
		}

		// Steam can't switch accounts while the game runs, so once we've found the folder it stays the same.
		fs::path saveFolder;
		DWORD account = 0;

		/// <summary>
		/// Backs up the profiles if they changed since the last backup and it's a new 10 minutes. Makes one backup at most, however long
		/// it's been since the last. `evenIfRecent` skips the 10 minutes, for when the profiles as they are right now must have a backup.
		/// </summary>
		void BackupIfDue(bool evenIfRecent = false) {
			if (account == 0 && !FindSaveFolder(saveFolder, account)) {
				account = 0;
				return; // Steam hasn't said who's logged in yet. Try again next time.
			}

			const fs::path root = BackupRoot(account);
			std::error_code error;
			fs::create_directories(root, error);
			if (!fs::is_directory(root, error)) {
				LOG_ERROR("(PROFILE BACKUP) Couldn't create " << Utf8(root) << std::endl);
				return;
			}

			const int64_t now = _time64(nullptr);
			const std::vector<Backup> recent = ListBackups(root / newestTier.folder);
			const Backup* newest = recent.empty() ? nullptr : &recent.back();
			if (newest) {
				if (!evenIfRecent && newest->time / newestTier.seconds == now / newestTier.seconds)
					return; // Already have these 10 minutes.

				FolderState current, last;
				if (ReadFolder(saveFolder, current) && ReadFolder(newest->path, last) && current == last)
					return; // Nothing changed since the last backup.
			}

			const std::wstring name = NameFor(now);
			fs::path made;
			{
				std::unique_lock lock(filesMutex);
				if (!CopyBackup(saveFolder, root / newestTier.folder, name, newest ? &newest->path : nullptr, made)) {
					LOG_WARNING("(PROFILE BACKUP) Couldn't back up the profiles this time, will try again" << std::endl);
					return;
				}
			}
			LOG_INFO("(PROFILE BACKUP) Backed up the profiles to " << Utf8(made) << std::endl);

			// The other tiers get it too if it's the first of their period. All hard links to the backup just made.
			for (const Tier& tier : tiers) {
				const fs::path tierFolder = root / tier.folder;
				if (&tier != &newestTier) {
					const std::vector<Backup> backups = ListBackups(tierFolder);
					if (!backups.empty() && backups.back().time / tier.seconds >= now / tier.seconds)
						continue; // Already has this period. (>= also covers the clock going backwards.)

					fs::path linked;
					if (!CopyBackup(made, tierFolder, name, &made, linked)) {
						LOG_WARNING("(PROFILE BACKUP) Couldn't add the backup to " << Utf8(tier.folder) << std::endl);
						continue; // Don't prune a tier that didn't get the new backup.
					}
				}
				Prune(tier, tierFolder);
			}

			WriteHowTo(root, saveFolder);
		}
	}

	bool BackUpBeforeHooking() {
		try {
			BackupIfDue(true);
			if (account == 0) {
				LOG_ERROR("(PROFILE BACKUP) Couldn't find the Steam save folder" << std::endl);
				return false;
			}

			// The newest backup must be the profiles exactly as they are now, not just some older backup.
			const std::vector<Backup> backups = ListBackups(BackupRoot(account) / newestTier.folder);
			FolderState current, newest;
			if (backups.empty() || !ReadFolder(saveFolder, current) || !ReadFolder(backups.back().path, newest) || current != newest) {
				LOG_ERROR("(PROFILE BACKUP) Couldn't back up the profiles" << std::endl);
				return false;
			}
		}
		catch (const std::exception& e) {
			LOG_ERROR("(PROFILE BACKUP) " << e.what() << std::endl);
			return false;
		}

		enabled = true; // The hooks go in before ProfileBackupsMod's first tick, so saves need to wait for backups from now on.
		return true;
	}

	SaveGuard::SaveGuard() {
		if (!enabled)
			return;
		locked = filesMutex.try_lock_for(saveWaitLimit);
		if (!locked) // The copy is stuck. Save anyway; the backup sees the files change and throws its copy away.
			LOG_WARNING("(PROFILE BACKUP) A backup took too long, saving without waiting for it" << std::endl);
	}

	SaveGuard::~SaveGuard() {
		if (locked)
			filesMutex.unlock();
	}
}

using Framework::ModContext;

/// <summary>
/// The setting is read once at startup, like fast profile load and save (which needs this on), so neither changes mid-session.
/// </summary>
void ProfileBackupsMod::OnInitialize(ModContext& c) {
	onAtStartup = c.IsOn(Settings::Setting::BackupProfile);
}

bool ProfileBackupsMod::IsEnabled(const ModContext&) const {
	return onAtStartup;
}

/// <summary>
/// The first check runs on the first tick, so the profiles are backed up as the game starts, before it has saved anything.
/// </summary>
void ProfileBackupsMod::OnEnabled(ModContext&) {
	ProfileBackups::enabled = true;
	nextCheck = {};
}

void ProfileBackupsMod::OnDisabled(ModContext&) {
	ProfileBackups::enabled = false;
}

/// <summary>
/// Runs on MainThread with the other mods rather than on a thread of its own: it only needs a look every 30 seconds,
/// and each extra thread reserves stack address space in a 32-bit game that big profiles already push to its limit.
/// A backup copies only the files that changed since the last one, usually just the played profile, once every 10 minutes at most.
/// </summary>
void ProfileBackupsMod::OnTick(ModContext&) {
	const auto now = std::chrono::steady_clock::now();
	if (now < nextCheck)
		return;
	nextCheck = now + ProfileBackups::checkEvery;

	try {
		ProfileBackups::BackupIfDue();
	}
	catch (const std::exception& e) { // A throw would fault the mod and stop backups for the rest of the session.
		LOG_ERROR("(PROFILE BACKUP) " << e.what() << std::endl);
	}
}

static Framework::ModRegistrar<ProfileBackupsMod> _profileBackupsReg;
