#include "stdafx.h"
#include "SettingEdits.hpp"
#include "IniPatch.hpp"
#include "Settings.hpp"
#include "Framework/Framework.hpp"

#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <stdexcept>

namespace SettingEdits {
	namespace {
		using Clock = std::chrono::steady_clock;

		constexpr char kIniFile[] = "RSMods.ini";
		constexpr char kTemporaryFile[] = "RSMods.ini.game.tmp"; // Not the GUI's ".tmp"
		constexpr char kGuiWindowMarker[] = "RSModsGUI";         // Window property the GUI sets on its main window
		constexpr ULONG_PTR kSettingEditData = 1;                // COPYDATASTRUCT.dwData, as for the GUI's own messages
		constexpr UINT kGuiWaitMs = 2000;

		// Plenty for the save to land. A file that still differs after this has a newer value from the GUI (the user
		// changed the same setting there), or the save failed; either way the file wins from then on.
		constexpr auto kPendingLimit = std::chrono::seconds(10);

		struct Pending {
			std::string value;
			bool isInt = false;
			Clock::time_point since;
		};

		std::mutex g_mutex; // Guards everything below
		std::map<std::string, Pending> g_pending;
		std::deque<Edit> g_queue;
		std::condition_variable g_wake;
		bool g_workerStarted = false;
		Status g_status;

		void SetStatus(std::string text, bool error) {
			std::lock_guard lock(g_mutex);
			g_status = Status{ std::move(text), error };
		}

		// MainThread: the same updates the GUI's "update mod|custom" message makes.
		void SetInMemory(const std::string& key, const std::string& value, bool isInt) {
			if (!isInt) {
				Settings::UpdateModSetting(key, value);
				return;
			}
			try { Settings::UpdateCustomSetting(key, std::stoi(value)); } catch (...) {}
		}

		std::string InMemory(const std::string& key, bool isInt) {
			return isInt ? std::to_string(Settings::GetModSetting(key)) : Settings::ReturnSettingValue(key);
		}

		std::string FullIniPath() {
			char buffer[MAX_PATH];
			const DWORD length = GetFullPathNameA(kIniFile, MAX_PATH, buffer, nullptr);
			return length > 0 && length < MAX_PATH ? std::string(buffer, length) : std::string(kIniFile);
		}

		HWND FindGuiWindow() {
			HWND found = nullptr;
			EnumWindows([](HWND hwnd, LPARAM result) -> BOOL {
				if (!GetPropA(hwnd, kGuiWindowMarker))
					return TRUE;
				*reinterpret_cast<HWND*>(result) = hwnd;
				return FALSE;
			}, reinterpret_cast<LPARAM>(&found));
			return found;
		}

		// The reverse of the GUI's WM_COPYDATA messages: "set\n<ini path>\n<key>\n<section>\n<name>\n<value>",
		// answered 1 once the value is in the GUI's settings (it saves on its own thread). The path lets a GUI set up
		// for another game folder decline. No marked window means no GUI.
		bool SendToGui(const Edit& edit, const std::string& iniPath) {
			const HWND gui = FindGuiWindow();
			if (!gui)
				return false;

			const std::string message = "set\n" + iniPath + "\n" + edit.key + "\n" + edit.section + "\n" + edit.name + "\n" + edit.value;
			COPYDATASTRUCT data{ kSettingEditData, static_cast<DWORD>(message.size() + 1), const_cast<char*>(message.c_str()) };
			DWORD_PTR result = 0;
			return SendMessageTimeoutA(gui, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&data),
				SMTO_BLOCK | SMTO_ABORTIFHUNG, kGuiWaitMs, &result) && result == 1;
		}

		std::string ReadIniFile() {
			std::ifstream in(kIniFile, std::ios::binary);
			if (!in) {
				// Missing is fine, it gets created. Unreadable must not turn into a file holding only these edits.
				if (GetFileAttributesA(kIniFile) != INVALID_FILE_ATTRIBUTES)
					throw std::runtime_error("couldn't read RSMods.ini");
				return {};
			}

			std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
			if (in.bad())
				throw std::runtime_error("couldn't read RSMods.ini");
			return text;
		}

		// Written whole and swapped in, so the game's own rereads never see a half-written file.
		void PatchIniFile(const std::vector<Edit>& edits) {
			std::string text = ReadIniFile();
			for (const auto& edit : edits)
				text = IniPatch::SetValue(text, edit.section, edit.name, edit.value);

			{
				std::ofstream out(kTemporaryFile, std::ios::binary | std::ios::trunc);
				out.write(text.data(), static_cast<std::streamsize>(text.size()));
				out.flush();
				if (!out)
					throw std::runtime_error("couldn't write next to RSMods.ini");
			}

			for (int attempt = 1; ; ++attempt) {
				if (MoveFileExA(kTemporaryFile, kIniFile, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
					return;

				const DWORD error = GetLastError();
				if (attempt == 5) {
					DeleteFileA(kTemporaryFile);
					throw std::runtime_error("couldn't replace RSMods.ini (error " + std::to_string(error) + ")");
				}
				Sleep(50); // The game's own reads open the file without allowing a replace, but only briefly
			}
		}

		void SaveBatch(std::deque<Edit> batch, const std::string& iniPath) {
			const std::string label = batch.back().label;

			std::vector<Edit> toFile;
			for (auto& edit : batch) {
				if (!SendToGui(edit, iniPath))
					toFile.push_back(std::move(edit));
			}

			if (toFile.empty()) {
				SetStatus("Sent " + label + " to the RSMods GUI to save", false);
				return;
			}

			try {
				PatchIniFile(toFile);
				SetStatus("Saved " + label + " to RSMods.ini", false);
			}
			catch (const std::exception& e) {
				LOG_ERROR("Couldn't save in-game setting change: " << e.what() << std::endl);
				SetStatus("Couldn't save " + label + ": " + e.what(), true);
			}
		}

		void Worker() {
			const std::string iniPath = FullIniPath();

			for (;;) {
				std::deque<Edit> batch;
				{
					std::unique_lock lock(g_mutex);
					g_wake.wait(lock, [] { return !g_queue.empty(); });
					batch.swap(g_queue);
				}
				SaveBatch(std::move(batch), iniPath);
			}
		}
	}

	void Apply(Edit edit) {
		Framework::Registry().EnqueueSettingsUpdate([key = edit.key, value = edit.value, isInt = edit.isInt] {
			SetInMemory(key, value, isInt);
		});

		{
			std::lock_guard lock(g_mutex);
			g_pending[edit.key] = Pending{ edit.value, edit.isInt, Clock::now() };
			g_queue.push_back(std::move(edit));
			if (!g_workerStarted) {
				g_workerStarted = true;
				std::thread(Worker).detach();
			}
		}
		g_wake.notify_one();
	}

	std::optional<std::string> PendingValue(const std::string& key) {
		std::lock_guard lock(g_mutex);
		auto it = g_pending.find(key);
		if (it == g_pending.end() || Clock::now() - it->second.since > kPendingLimit)
			return std::nullopt;
		return it->second.value;
	}

	void ReapplyPending() {
		const auto now = Clock::now();
		std::lock_guard lock(g_mutex);
		for (auto it = g_pending.begin(); it != g_pending.end(); ) {
			const auto& [key, pending] = *it;
			if (InMemory(key, pending.isInt) == pending.value || now - pending.since > kPendingLimit) {
				it = g_pending.erase(it); // The file has it now, or never will
				continue;
			}
			SetInMemory(key, pending.value, pending.isInt);
			++it;
		}
	}

	Status LastStatus() {
		std::lock_guard lock(g_mutex);
		return g_status;
	}
}
