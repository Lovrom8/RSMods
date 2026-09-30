#include "stdafx.h"
#include "QualityOfLife.hpp"
#include "EngineString.hpp"

#include <unordered_map>

namespace {
	// A menu only finishes loading once its object is ready AND its Flash movie exists. The movie is only made once the game
	// answers the menu's request for its movie file, and it only answers if it can find that file in its index of the game's
	// files. When the intro ends, the game (re)builds that index in the background, which takes a second or two (longer on slow
	// drives). With a very short intro (Fast Load) the title screen asks for its movie while the index is still being built, the
	// file isn't in it yet, no answer is ever sent, nothing asks again, and the game sits on the loading screen forever.
	// Asking again once the file has been indexed lets the game finish the menu the normal way.
	// A normal answer arrives within a frame or two (5-30 ms). Waiting on both time and frames between requests means a long
	// hitch can't trigger a retry while the real answer is still on its way, which would make the movie twice.
	constexpr ULONGLONG softLockRetryDelayMs = 250;
	constexpr unsigned softLockRetryDelayFrames = 3;
	constexpr ULONGLONG softLockGiveUpMs = 30000;		// Indexing finishes long before this, even on slow drives.
	constexpr int softLockLogEvery = 20;				// Log the first retry, then about every 5 seconds.
	unsigned menuLoadFrames = 0;

	struct StuckMenu {
		ULONGLONG firstStuck = 0;
		ULONGLONG since = 0;
		unsigned sinceFrame = 0;
		int retries = 0;
		bool gaveUp = false;
		bool seen = false;
		std::string name;
	};

	std::unordered_map<uintptr_t, StuckMenu> stuckMenus;		// By menu load state. Game thread only.
	std::unordered_map<uint64_t, uintptr_t> menuMovies;			// Owner id -> menu movie object. Game thread only.

	uint32_t ReadU32(uintptr_t address) {
		uint32_t value = 0;
		MemUtil::TryRead(address, value);
		return value;
	}

	uint8_t ReadU8(uintptr_t address) {
		uint8_t value = 0;
		MemUtil::TryRead(address, value);
		return value;
	}

	uint64_t OwnerIdOf(uintptr_t movie) {
		const uint32_t owner = ReadU32(movie + 0x0c);
		return (static_cast<uint64_t>(ReadU32(owner + 0x54)) << 32) | ReadU32(owner + 0x50);
	}

	bool IsLiveMenuMovie(uintptr_t movie, uint64_t ownerId) {
		return ReadU32(movie) == Offsets::ptr_menuMovieVTable.GetValue() && OwnerIdOf(movie) == ownerId;
	}

	/// The game's movie request: movie object in EAX, 8 byte request id by value on the stack, cleans its own stack (RET 8).
	void __declspec(naked) __cdecl SendMenuMovieRequest(uintptr_t /*function*/, uintptr_t /*movie*/, uint32_t /*requestIdHigh*/, uint32_t /*requestIdLow*/) {
		__asm {
			push ebp
			mov ebp, esp
			push ebx
			push esi
			push edi
			push dword ptr [ebp + 20]
			push dword ptr [ebp + 16]
			mov eax, dword ptr [ebp + 12]
			call dword ptr [ebp + 8]
			pop edi
			pop esi
			pop ebx
			pop ebp
			ret
		}
	}

	void __stdcall RememberMenuMovie(uintptr_t movie) {
		if (menuMovies.size() > 256) {
			for (auto it = menuMovies.begin(); it != menuMovies.end();)
				it = IsLiveMenuMovie(it->second, it->first) ? std::next(it) : menuMovies.erase(it);
		}
		menuMovies[OwnerIdOf(movie)] = movie;
	}

	/// Runs every frame, just before the game updates the menus it's loading.
	void __stdcall CheckForStuckMenus(uintptr_t menuLoader) {
		menuLoadFrames++;
		for (auto& [state, menu] : stuckMenus)
			menu.seen = false;

		// +0x144: menus still waiting on their object and movie.
		const uint32_t begin = ReadU32(menuLoader + 0x144), end = ReadU32(menuLoader + 0x148);
		if (begin && end >= begin && (end - begin) / 4 <= 256) {
			const ULONGLONG now = GetTickCount64();
			for (uint32_t slot = begin; slot < end; slot += 4) {
				const uint32_t handle = ReadU32(slot);
				const uint32_t record = ReadU32(ReadU32(menuLoader + 0x10 + ((handle >> 12) & 0xff) * 0x0c) + (handle & 0xfff) * 4);
				const uint32_t state = ReadU32(record + 0x0c);
				// Load state: +0x10 object ready, +0x11 movie made, +0x18 owner id.
				if (!state || !ReadU8(state + 0x10) || ReadU8(state + 0x11))
					continue;

				StuckMenu& menu = stuckMenus[state];
				menu.seen = true;
				if (!menu.firstStuck) {
					menu.firstStuck = menu.since = now;
					menu.sinceFrame = menuLoadFrames;
					menu.name = EngineStrings::SafeRead(ReadU32(record + 0x08) + 0x10, 128);
				}
				if (menu.gaveUp || now - menu.since < softLockRetryDelayMs || menuLoadFrames - menu.sinceFrame < softLockRetryDelayFrames)
					continue;

				if (now - menu.firstStuck > softLockGiveUpMs) {
					menu.gaveUp = true;
					LOG_ERROR("(QOL) Menu '" << menu.name << "' still hasn't got its movie after " << menu.retries << " requests, giving up" << std::endl);
					continue;
				}

				// Restart the wait, so each retry gets the same chance to be answered.
				menu.since = now;
				menu.sinceFrame = menuLoadFrames;
				menu.retries++;
				const bool logThisOne = menu.retries == 1 || menu.retries % softLockLogEvery == 0;
				const uint64_t ownerId = (static_cast<uint64_t>(ReadU32(state + 0x1c)) << 32) | ReadU32(state + 0x18);
				const auto movie = menuMovies.find(ownerId);
				if (movie == menuMovies.end() || !IsLiveMenuMovie(movie->second, ownerId) || ReadU32(movie->second + 0x58)) {
					if (logThisOne)
						LOG_WARNING("(QOL) Menu '" << menu.name << "' is stuck loading, but its movie can't be found to retry" << std::endl);
					continue;
				}

				if (logThisOne)
					LOG_WARNING("(QOL) Menu '" << menu.name << "' never got its movie, asking for it again (request " << menu.retries << ", " << (now - menu.firstStuck) << " ms)" << std::endl);
				// Same request id the movie is already listening for (+0x40), so the game's own answer handling takes it from here.
				SendMenuMovieRequest(Offsets::func_menuMovieRequest.GetValue(), movie->second, ReadU32(movie->second + 0x40), ReadU32(movie->second + 0x44));
			}
		}

		for (auto it = stuckMenus.begin(); it != stuckMenus.end();) {
			if (it->second.seen) {
				++it;
				continue;
			}
			if (it->second.retries)
				LOG_INFO("(QOL) Menu '" << it->second.name << "' got its movie after " << it->second.retries << " requests (" << (GetTickCount64() - it->second.firstStuck) << " ms)" << std::endl);
			it = stuckMenus.erase(it);
		}
	}

	void __declspec(naked) menuMovieSetupHook() {
		__asm {
			pushfd
			pushad
			push ecx // The menu movie object
			call RememberMenuMovie
			popad
			popfd
			push ebp
			mov ebp, esp
			and esp, 0xFFFFFFF8 // Original six-byte prologue
			push offset Offsets::ptr_menuMovieSetupJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	void __declspec(naked) menuLoadUpdateHook() {
		__asm {
			pushfd
			pushad
			push dword ptr [esp + 40] // The menu loader (first argument, above 8 registers, the flags and the return address)
			call CheckForStuckMenus
			popad
			popfd
			push ebp
			mov ebp, esp
			and esp, 0xFFFFFFF8 // Original six-byte prologue
			push offset Offsets::ptr_menuLoadUpdateJmpBck
			jmp MemUtil::JumpToVersioned
		}
	}

	// Non-Exclusive Fullscreen. The game's display flags: 0x80 non-exclusive fullscreen, 0x04 exclusive fullscreen.
	constexpr uint32_t displayFlagNonExclusive = 0x80;
	constexpr uint32_t displayFlagExclusive = 0x04;

	constexpr LONG_PTR windowFrameStyles = WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
	constexpr LONG_PTR windowFrameExStyles = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE;

	// The mode is read every frame, but the window is only re-checked this often while nothing changes
	// (something else, like Launch On External Monitor, may move or resize it).
	constexpr ULONGLONG displayRecheckIntervalMs = 250;

	enum class DisplayMode { Unknown, Windowed, NonExclusive, Exclusive };

	DisplayMode lastDisplayMode = DisplayMode::Unknown;
	ULONGLONG nextDisplayCheck = 0;

	// Set once we've stripped the window's frame. The saved values are what windowed mode gets back.
	bool borderlessApplied = false;
	LONG_PTR savedWindowStyle = 0;
	LONG_PTR savedWindowExStyle = 0;
	RECT savedWindowRect{};

	DisplayMode ReadDisplayMode() {
		uintptr_t renderer = 0;
		uint32_t flags = 0;

		if (!MemUtil::TryRead(Offsets::ptr_renderer, renderer) || !renderer)
			return DisplayMode::Unknown;
		if (!MemUtil::TryRead(renderer + Offsets::rendererDisplayFlagsOffset, flags))
			return DisplayMode::Unknown;

		if (flags & displayFlagNonExclusive)
			return DisplayMode::NonExclusive;
		if (flags & displayFlagExclusive)
			return DisplayMode::Exclusive;
		return DisplayMode::Windowed;
	}

	void MakeWindowBorderless(HWND hWnd) {
		const LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
		const LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);

		RECT windowRect{};
		MONITORINFO monitor{ sizeof(monitor) };
		if (!GetWindowRect(hWnd, &windowRect) || !GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &monitor))
			return;

		const bool framed = (style & windowFrameStyles) || (exStyle & windowFrameExStyles);
		if (!framed && EqualRect(&windowRect, &monitor.rcMonitor))
			return;

		if (!borderlessApplied) {
			savedWindowStyle = style;
			savedWindowExStyle = exStyle;
			savedWindowRect = windowRect;
			borderlessApplied = true;
		}

		if (framed) {
			SetWindowLongPtr(hWnd, GWL_STYLE, (style & ~windowFrameStyles) | WS_POPUP);
			SetWindowLongPtr(hWnd, GWL_EXSTYLE, exStyle & ~windowFrameExStyles);
		}

		const RECT& screen = monitor.rcMonitor;
		SetWindowPos(hWnd, nullptr, screen.left, screen.top, screen.right - screen.left, screen.bottom - screen.top,
			SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
	}

	void RestoreWindowFrame(HWND hWnd) {
		SetWindowLongPtr(hWnd, GWL_STYLE, savedWindowStyle);
		SetWindowLongPtr(hWnd, GWL_EXSTYLE, savedWindowExStyle);
		SetWindowPos(hWnd, nullptr, savedWindowRect.left, savedWindowRect.top,
			savedWindowRect.right - savedWindowRect.left, savedWindowRect.bottom - savedWindowRect.top,
			SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
		borderlessApplied = false;
	}
}

namespace QualityOfLife {
	/// <summary>
	/// Patches Two RTC cables error message.
	/// </summary>
	void PatchTwoRTC()
	{
		char patch[25];
		std::fill_n(patch, 25, static_cast<char>(0x90));
		MemUtil::PatchAdr(Offsets::ptr_twoRTCBypass, patch, sizeof(patch));
	}

	/// <summary>
	/// RS spawns two processes, one of which complains about Steam not being active. 
	/// We find that one and close it.
	/// </summary>
	HANDLE GetMessageBoxProcess()
	{
		HWND hWnd = FindWindowW(L"#32770", L"Error."); // Dialog box class
		if (!hWnd) {
			return nullptr; 
		}

		DWORD dwProcessId = 0;
		GetWindowThreadProcessId(hWnd, &dwProcessId);
		if (dwProcessId == 0) {
			return nullptr;
		}
		
		return OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, dwProcessId);
	}

	/// <summary>
	/// Stops the game from starting two instances
	/// </summary>
	void StopTwoRSInstances() {
		HANDLE handle = GetMessageBoxProcess();

		if (!handle) {
			return;
		}

		std::vector<wchar_t> processNameBuffer(MAX_PATH);
		HMODULE hMod;

		if (DWORD cNeeded; EnumProcessModulesEx(handle, &hMod, sizeof(hMod), &cNeeded, LIST_MODULES_32BIT | LIST_MODULES_64BIT))
		{
			if (GetModuleBaseNameW(handle, hMod, processNameBuffer.data(), processNameBuffer.size()))
			{
				std::wstring processName(processNameBuffer.data());

				if (processName == L"Rocksmith2014.exe") {
					std::string narrowName;
					narrowName.resize(processName.size());
					std::transform(processName.begin(), processName.end(), narrowName.begin(),
						[](wchar_t ch) { return static_cast<char>(ch); });
					LOG_INFO("Found parasitic process '" << narrowName << "'. Terminating." << std::endl);

					if (!TerminateProcess(handle, 0)) {
						LOG_ERROR("Failed to terminate process. Error: " << GetLastError() << std::endl);
					}
				}
				else {
					std::string narrowName;
					narrowName.resize(processName.size());
					std::transform(processName.begin(), processName.end(), narrowName.begin(),
						[](wchar_t ch) { return static_cast<char>(ch); });
					LOG_INFO("Found dialog box, but process '" << narrowName << "' is not the target. Not terminating." << std::endl);
				}
			}
		}

		CloseHandle(handle);
	}

	/// <summary>
	/// Rocksmith ignores any note below MIDI 24 (C1, 32.7 Hz), so a bass low E tuned more than 4 semitones down at A440
	/// can never be detected. This lowers that floor to 18, about as low as the game can pick up a pitch at 48 kHz (~23.4 Hz).
	/// The game reads the floor when note detection starts, so this has to be patched early.
	/// </summary>
	void LowerNoteDetectionFloor() {
		MemUtil::PatchAdr(Offsets::ptr_noteDetectionFloor, "\x12", 1);

		LOG_INFO("Lowered note detection floor" << std::endl);
	}

	/// <summary>
	/// With Fast Load the title screen can get stuck on the loading screen forever: it asks for its Flash movie while the game is
	/// still indexing its files, gets no answer, and never asks again. While a menu is ready except for its movie, ask for the movie
	/// again every 250 ms (and 3 frames), for up to 30 seconds. Does nothing while menus load normally.
	/// </summary>
	void RetryFastLoadUponSoftLock() {
		const uintptr_t menuLoadUpdate = Offsets::ptr_menuLoadUpdate.GetValue();
		const uintptr_t menuMovieSetup = Offsets::ptr_menuMovieSetup.GetValue();
		if (!menuLoadUpdate || !menuMovieSetup || !Offsets::func_menuMovieRequest.GetValue()) {
			LOG_INFO("(QOL) Fast Load soft lock retry not supported on this game version" << std::endl);
			return;
		}

		// Movie setup first, so every menu movie is known before anything can be retried.
		if (!MemUtil::PlaceHook(Offsets::ptr_menuMovieSetup, menuMovieSetupHook, 6)) {
			LOG_ERROR("(QOL) Fast Load soft lock retry: failed to hook menu movie setup" << std::endl);
			return;
		}
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(menuMovieSetup), 6);

		if (!MemUtil::PlaceHook(Offsets::ptr_menuLoadUpdate, menuLoadUpdateHook, 6)) {
			LOG_ERROR("(QOL) Fast Load soft lock retry: failed to hook the menu load update" << std::endl);
			return;
		}
		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(menuLoadUpdate), 6);

		LOG_INFO("(QOL) Fast Load soft lock retry installed" << std::endl);
	}

	/// <summary>
	/// Makes the game's "Non-Exclusive Fullscreen" mode (Fullscreen=1 in Rocksmith.ini) fill the screen.
	/// In that mode the game only renders into a desktop sized back buffer. The window keeps its title bar and its windowed
	/// size, so it looks just like windowed mode. While the mode is active, strip the window's frame and stretch it over the
	/// monitor it's on. Going back to windowed mode puts the frame and the old size back.
	/// Call once per EndScene from the render thread.
	/// </summary>
	void FixNonExclusiveFullscreen() {
		const DisplayMode mode = ReadDisplayMode();
		const ULONGLONG now = GetTickCount64();

		if (mode == lastDisplayMode && now < nextDisplayCheck)
			return;

		lastDisplayMode = mode;
		nextDisplayCheck = now + displayRecheckIntervalMs;

		HWND hWnd = D3DHooks::GetGameWindow();
		if (!hWnd || IsIconic(hWnd))
			return;

		switch (mode) {
			case DisplayMode::NonExclusive:
				MakeWindowBorderless(hWnd);
				break;
			case DisplayMode::Windowed:
				if (borderlessApplied)
					RestoreWindowFrame(hWnd);
				break;
			default:
				// Exclusive fullscreen owns the screen, so leave the window alone. If we stripped the frame, it comes
				// back once the game is windowed again.
				break;
		}
	}
}