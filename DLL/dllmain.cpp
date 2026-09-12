#include "stdafx.h"
#include "Proxy.hpp"
#include "ModManager.hpp"
#include "Framework/Framework.hpp"
#include "Mods/Midi.hpp"
#include "D3DOverlay.hpp"

namespace Setting = Settings::Setting;

#if defined(_DEBUG) || defined(_WWISE_LOGS)
bool debug = true;
#else
bool debug = false;
#endif

#ifdef _WWISE_LOGS
bool wwiseLogging = true;
#else
bool wwiseLogging = false;
#endif

#ifndef _RSMODS_VERSION
#define _RSMODS_VERSION "RSMODS Version: 1.2.8.4 SRC. DEBUG: " << std::boolalpha << debug << ". Wwise Logs: " << std::boolalpha << wwiseLogging << "."
#endif

/// <summary>
/// Send Midi Data Async. Only really used in debug builds to test MIDI commands.
/// Secondary purpose of remaking D3D textures every 32 ticks (~ 1 second).
/// </summary>
/// <returns>NULL. Loops while game is open.</returns>
unsigned WINAPI MidiThread() {
	// Initial some values.
	int currentCount = 0;

	while (!GameState::GameClosing) {
		// If this is the 32nd loop, remake the D3D textures.
		// This allows us to have real-time updates to textures.
		if (currentCount == 31) {
			currentCount = 0;
			D3DHooks::RecreateTextureTimer = true;
		}

		// If we have sent a Midi PC/CC value to this thread, send the Midi value.
		if (Midi::sendPC)
			Midi::SendProgramChange(Midi::dataToSendPC);

		if (Midi::sendCC)
			Midi::SendControlChange(Midi::dataToSendCC);

		Sleep(Midi::sleepFor);
		currentCount++;
	}

	return 0;
}

unsigned WINAPI RiffRepeaterThread() {
	// Wait for the game to enter the main menu before attempting to read the current song info, in order to prevent crashes
	while (!GameState::GameLoaded)
		Sleep(5000);

	std::string previousSongKey = "";

	// We can only user Riff Repeater while the game is open, so verify it's open. Runs every 100 ms.
	while (!GameState::GameClosing) {
		Sleep(100);

		const auto songKey = GameState::GetSongKey();
		if (songKey != previousSongKey) {
			previousSongKey = songKey;

			RiffRepeater::HandleSongChange(previousSongKey);
		}

		RiffRepeater::SaveSpeedToFileOnChange();
	}

	return 0;
}

const bool ensureForcedTopMode = false;

/// <summary>
/// Handle Keypress Inputs. Used to toggle mods on / off. WARNING: RUNS (almost) EVERY FRAME
/// </summary>
/// <param name="hWnd"> - ID of Rocksmith</param>
/// <param name="msg"> - Reason Function was called. KEYUP = Keypress | COPYDATA = Settings Update | CLOSE = Game Closing.</param>
/// <param name="keyPressed"> - Virtual Key of the key pressed. Reference here: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes </param>
/// <param name="lParam"> - Data Sent</param>
/// <returns>Verification that message was sent.</returns>
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM keyPressed, LPARAM lParam) {
	if (Menu::menuEnabled && ImGui_ImplWin32_WndProcHandler(hWnd, msg, keyPressed, lParam))
		return true;

	if (Settings::IsOn(Setting::PreventMidSongPause) && D3DHooks::cachedIsInSong) {
		switch (msg) {
			case WM_NCACTIVATE:
			case WM_ACTIVATEAPP:
			case WM_ACTIVATE:
				return CallWindowProc(D3DHooks::oWndProc, hWnd, msg, TRUE, lParam);
			case WM_KILLFOCUS:
				return false;
		}
	}

	switch (msg) {
		case WM_SYSCOMMAND:
			// Makes ALT + ENTER cause F11 to be pressed.
			// This is mainly so a user can use a common shortcut, that works in most games now-a-days.
			if (keyPressed == SC_KEYMENU && lParam == VK_RETURN) {
				WndProc(hWnd, WM_KEYUP, VK_F11, 0);
				return true;
			}

			// Prevent a weird bug when trying to play Guitarcade or Score Attack with a key combination.
			if (keyPressed == SC_MOVE + 0x2 && GameState::Menus::IsInOnlineModes()) {
				return true;
			}

			break;
		case WM_KEYUP:
			Keybindings::HandleKeyUp(keyPressed, lParam);
			break;
		case WM_KEYDOWN:
			Keybindings::HandleKeyDown(keyPressed, lParam);
			break;
		case WM_CLOSE:
			GameState::GameClosing = true;
			Framework::Inbox().Wake();
			break;
		case WM_COPYDATA:
			Keybindings::UpdateSettingsOnGUIChange(lParam);
			break;

		default:
			POINT mPos;
			GetCursorPos(&mPos);
			ScreenToClient(hWnd, &mPos);
			ImGui::GetIO().MousePos.x = mPos.x;
			ImGui::GetIO().MousePos.y = mPos.y;
			break;
	}

	return CallWindowProc(D3DHooks::oWndProc, hWnd, msg, keyPressed, lParam);
}

void UpdateGameWindowStacking() {
	if (Settings::IsOn(Setting::PreventMidSongPause)) {
		bool actuallyInSong = GameState::IsInSong();

		if (ensureForcedTopMode) {
			static bool lastState = false;
			if (actuallyInSong != lastState) {
				HWND position = actuallyInSong ? HWND_TOPMOST : HWND_NOTOPMOST;

				SetWindowPos(D3DHooks::GetGameWindow(), position, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

				lastState = actuallyInSong;
			}
		}
	
		D3DHooks::cachedIsInSong = actuallyInSong;
	}
}

/// <summary>
/// Hook on game boot. Initialize all our own UI elements (text on screen, and ImGUI).
/// </summary>
/// <param name="pDevice"> - Device Pointer</param>
/// <returns>HRESULT of the official EndScene</returns>
HRESULT APIENTRY D3DHooks::Hook_EndScene(IDirect3DDevice9* pDevice) {
	HRESULT originalReturn = oEndScene(pDevice);
	if (Menu::IsOverlayCall()) {
		return originalReturn;
	}

	// Don't draw our overlay onto a lost / not-yet-reset device (e.g. mid Alt+Tab out of exclusive fullscreen).
	if (FAILED(pDevice->TestCooperativeLevel())) {
		return originalReturn;
	}

	Menu::Init(pDevice, (LONG_PTR)WndProc);
	Menu::RenderImGuiMenu();
	Menu::UpdateStringTextures(pDevice);
	UpdateGameWindowStacking();
	D3DHooks::UpdateUltrawideState(pDevice);
	GameOverlay::RenderOverlay(pDevice);
	D3DHooks::RegenerateTwitchNoteColors(pDevice);

	return originalReturn;
}

/// <summary>
/// Manage Queue of Twitch Effects.
/// </summary>
/// <returns>NULL. Loops while game is open.</returns>
unsigned WINAPI HandleEffectQueueThread() {
	while (!GameState::GameClosing) {
		if (Twitch::effectQueue.empty() && GameState::IsInSong()) {
			Twitch::ParseEffectQueue();
		}

		Sleep(250);
	}
	return 0;
}

/// <summary>
/// Main Thread where we trigger the mods to startup.
/// </summary>
/// <returns>NULL. Loops while game is open.</returns>
unsigned WINAPI MainThread() {
	LOG_NOHEAD(_RSMODS_VERSION << std::endl);

	Keybindings::InitializeCommands();
	ModManager::InitializeConfiguration();
	ModManager::InitializeMods(debug);
	Framework::Registry().InstantiatePending();
	ModManager::ApplyStartupMods();
	Framework::Registry().DispatchInitialize();

	auto nextModTick = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
	while (!GameState::GameClosing) {
		Framework::Inbox().WaitUntil(nextModTick);
		if (GameState::GameClosing) break;

		// Commands are drained independently of the maintenance tick. Both paths run on this
		// thread, so command actions and mod lifecycle/tick callbacks cannot race one another.
		const Framework::GamePhase commandPhase = !GameState::GameLoaded
			? Framework::GamePhase::Loading
			: GameState::IsInSong() ? Framework::GamePhase::Song : Framework::GamePhase::Menu;
		Framework::Registry().DispatchCommands(commandPhase, GameState::GameLoaded);

		const auto now = std::chrono::steady_clock::now();
		if (now < nextModTick) continue;

		if (GameState::GameLoaded) {
			ModManager::HandlePostGameLoadedMods();
			Framework::Registry().Tick(GameState::IsInSong() ? Framework::GamePhase::Song : Framework::GamePhase::Menu);
		}
		else {
			ModManager::UpdateGameLoadingState();
			Framework::Registry().Tick(Framework::GamePhase::Loading);
		}

		// Missed maintenance deadlines are not replayed as a burst of catch-up ticks.
		nextModTick = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
	}

	CrowdControl::StopServer();
	Framework::Registry().Shutdown();

	return 0;
}

/// <summary>
/// Unhook all threads to take advantage of multi-threading.
/// </summary>
void Initialize() {
	LogSettings::startupTime = clock();

	Wwise::Exports::Initialize();

	std::thread(MainThread).detach(); // Mod Toggle based on menus
	std::thread(HandleEffectQueueThread).detach(); // Twitch Effects
	std::thread(MidiThread).detach(); // MIDI Auto Tuning / True Tuning
	std::thread(RiffRepeaterThread).detach(); // RR Speed Above 100% Log
}

/// <summary>
/// Build an absolute path to a file sitting next to the game executable.
/// SetupLogging() runs under the loader lock at DLL_PROCESS_ATTACH, where the
/// process working directory is not yet guaranteed to be the Rocksmith folder,
/// so a bare relative filename would resolve against the wrong directory.
/// </summary>
static std::string PathNextToExecutable(const std::string& fileName) {
	char executable[MAX_PATH]{};
	GetModuleFileNameA(NULL, executable, MAX_PATH);

	std::string path(executable);
	const size_t slash = path.find_last_of("\\/");
	if (slash != std::string::npos)
		path.resize(slash + 1);
	else
		path.clear();

	return path + fileName;
}

void SetupLogging() {
	// Opt-in: only log to file if RSMods_debug.txt already exists (same as before).
	const std::string debugLogPath = PathNextToExecutable("RSMods_debug.txt");
	const bool debugLogPresent = std::ifstream(debugLogPath).good();

	if (debug) {
		AllocConsole();

		// Connect stdin, stdout to the debug console.
		FILE* streamRead = nullptr;
		FILE* streamConsole = nullptr;
		freopen_s(&streamRead, "CONIN$", "r", stdin);
		freopen_s(&streamConsole, "CONOUT$", "w", stdout);
	}

	if (debugLogPresent) {
		Logger::GetInstance().InitFile(debugLogPath);
	}
}

/// <summary>
/// Hook into the game for us to run our own code. **DISPLAYS DEBUG CONSOLE ON DEBUG BUILD**
/// </summary>
/// <param name="hModule"></param>
/// <param name="dwReason"></param>
/// <param name="lpReserved"></param>
/// <returns>Always returns TRUE</returns>
BOOL APIENTRY DllMain(HMODULE hModule, uint32_t dwReason, LPVOID lpReserved) {
	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			SetupLogging();
			DisableThreadLibraryCalls(hModule); // Disables the DLL_THREAD_ATTACH and DLL_THREAD_DETACH notifications. | https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-disablethreadlibrarycalls
			Proxy::Init(); // Proxy all real XInput commands to the actual xinput1_3.dll.
			Initialize(); // Inject our mod code.
			return TRUE;
		case DLL_PROCESS_DETACH:
			Proxy::Shutdown(); // Kill Proxy to xinput1_3.dll

			if (Menu::ImGuiInit)
			{
				ImGui_ImplWin32_Shutdown();
				ImGui_ImplDX9_Shutdown();
				ImGui::DestroyContext();
			}
			return TRUE;
		default:
			// Should never happen, but handle gracefully
			break;
	}
	
	return TRUE;
}
