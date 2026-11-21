#include "stdafx.h"
#include <array>
#include "ModManager.hpp"
#include "Mods/Midi.hpp"

namespace Setting = Settings::Setting;

namespace {
	typedef IDirect3D9* (WINAPI* tDirect3DCreate9)(UINT SDKVersion);

	/// <summary>
	/// Read the IDirect3DDevice9 vTable out of a throwaway device.
	/// Copies the function pointers to a std::array so dummy device can be released immediately without leaks.
	/// </summary>
	/// <param name="d3d9Module"> - Handle of the d3d9.dll the game loaded.</param>
	/// <returns>std::array of device vTable function pointers, or empty array if failed.</returns>
	std::array<void*, 119> GetD3D9DeviceVTable(HMODULE d3d9Module) {
		std::array<void*, 119> vTable{};

		// Resolve the entry point out of the module the game already loaded, rather than importing it.
		// That keeps us from pulling d3d9.dll into the process earlier than the game would itself.
		tDirect3DCreate9 direct3DCreate9 = (tDirect3DCreate9)GetProcAddress(d3d9Module, "Direct3DCreate9");

		if (!direct3DCreate9) {
			LOG_ERROR("d3d9.dll does not export Direct3DCreate9." << std::endl);
			return vTable;
		}

		IDirect3D9* d3d9 = direct3DCreate9(D3D_SDK_VERSION);

		if (!d3d9) {
			LOG_ERROR("Direct3DCreate9 failed." << std::endl);
			return vTable;
		}

		D3DPRESENT_PARAMETERS presentParameters{};
		presentParameters.Windowed = TRUE;
		presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
		presentParameters.hDeviceWindow = GetDesktopWindow();

		IDirect3DDevice9* dummyDevice = NULL;
		HRESULT result = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, presentParameters.hDeviceWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING, &presentParameters, &dummyDevice);

		if (FAILED(result)) {
			result = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, presentParameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParameters, &dummyDevice);
		}

		// DXVK's NULLREF support is unreliable, so it is only worth trying once HAL has already failed.
		if (FAILED(result)) {
			LOG_WARNING("Could not create a HAL device (0x" << std::hex << result << std::dec << "). Retrying with NULLREF." << std::endl);
			result = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, presentParameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParameters, &dummyDevice);
		}

		if (SUCCEEDED(result) && dummyDevice) {
			void** rawVTable = *(void***)dummyDevice;
			if (rawVTable) {
				memcpy(vTable.data(), rawVTable, sizeof(vTable));
			}
			dummyDevice->Release();
		}
		else {
			LOG_ERROR("Could not create a D3D9 device to read the vTable from. Error: 0x" << std::hex << result << std::dec << std::endl);
		}

		d3d9->Release();

		return vTable;
	}

	template <typename T>
	void InstallD3DHook(const char* name, PBYTE target, PBYTE hook, T& original) {
		static_assert(sizeof(T) == sizeof(uintptr_t));
		MemUtil::PatchResult result = MemUtil::PatchResult::Original;
		MemUtil::TrampHook(target, hook, 5, &result, reinterpret_cast<uintptr_t*>(&original));

		switch (result) {
		case MemUtil::PatchResult::Installed:
			LOG_INFO("(D3D) " << name << " hook installed" << std::endl);
			break;
		case MemUtil::PatchResult::Original:
			LOG_WARNING("(D3D) " << name << " hook not installed; original bytes confirmed" << std::endl);
			break;
		case MemUtil::PatchResult::RollbackIncomplete:
			LOG_ERROR("(D3D) " << name << " hook outcome is indeterminate; original trampoline retained" << std::endl);
			break;
		}
	}
}

namespace ModManager {
	void InitializeConfiguration() {
		if (!(std::ifstream("RSMods.ini"))) {
			std::ofstream RSModsFileOutput("RSMods.ini");
			RSModsFileOutput.close();
		}
	}

	/// <summary>
	/// Applies bug prevention patches for game crashing bugs and similar issues that don't depend on a setting.
	/// Runs as soon as the offsets are known: PortAudio builds its device list inside Wwise init, 2-4 s into boot,
	/// so the audio patches have to be in before then to stop the crash on machines with many audio endpoints.
	/// </summary>
	void ApplyAlwaysOnBugPrevention() {
		BugPrevention::PreventPnPCrash();
		BugPrevention::PreventPortAudioInDeviceCrash();
		BugPrevention::PreventExtraAudioDevicesCrash();
		BugPrevention::AllowComplexPasswords();
		BugPrevention::BypassSaveFilePlatformIdCheck();
		BugPrevention::PreventAdvancedDisplayCrash();
		BugPrevention::PreventControllerAxisOverflow();
		BugPrevention::PreventInvalidInputTreeRootCrash();
		BugPrevention::FixCalibrationSampleCount();
	}

	/// <summary>
	/// Applies bug prevention patches the user turns on in RSMods.ini. Needs the settings read first.
	/// </summary>
	void ApplySettingBasedBugPrevention() {
		if (Settings::IsOn(Setting::FixBrokenTones)) {
			BugPrevention::PreventStuckTone();
		}

		if (Settings::IsOn(Setting::FixOculusCrash)) {
			BugPrevention::PreventOculusCrash();
		}
	}

	/// <summary>
	/// Hook Game Functions For Our Own Uses (On Alt-Tab, Draw UI, etc).
	/// </summary>
	void GUI() {
		HMODULE d3d9Module;

		// Wait for the game to load its Direct3D 9 implementation.
		while ((d3d9Module = GetModuleHandleA("d3d9.dll")) == NULL)
			Sleep(500);

		const auto vTable = GetD3D9DeviceVTable(d3d9Module);

		if (vTable[0] == nullptr) {
			LOG_ERROR("Could not find D3D device's vTable address." << std::endl);
			MessageBoxA(NULL, "Could not find D3D device's vTable address \n Restart the game and if you still get this error after a few tries, please report the error!", "Error", NULL);
			return;
		}

		// Publish each original before its hook can run.
		InstallD3DHook("SetVertexDeclaration", (PBYTE)vTable[D3DInfo::SetVertexDeclaration_Index], (PBYTE)D3DHooks::Hook_SetVertexDeclaration, oSetVertexDeclaration);
		InstallD3DHook("SetVertexShaderConstantF", (PBYTE)vTable[D3DInfo::SetVertexShaderConstantF_Index], (PBYTE)D3DHooks::Hook_SetVertexShaderConstantF, oSetVertexShaderConstantF);
		InstallD3DHook("Reset", (PBYTE)vTable[D3DInfo::Reset_Index], (PBYTE)D3DHooks::Hook_Reset, oReset);
		InstallD3DHook("SetVertexShader", (PBYTE)vTable[D3DInfo::SetVertexShader_Index], (PBYTE)D3DHooks::Hook_SetVertexShader, oSetVertexShader);
		InstallD3DHook("SetPixelShader", (PBYTE)vTable[D3DInfo::SetPixelShader_Index], (PBYTE)D3DHooks::Hook_SetPixelShader, oSetPixelShader);
		InstallD3DHook("SetStreamSource", (PBYTE)vTable[D3DInfo::SetStreamSource_Index], (PBYTE)D3DHooks::Hook_SetStreamSource, oSetStreamSource);
		InstallD3DHook("EndScene", (PBYTE)vTable[D3DInfo::EndScene_Index], (PBYTE)D3DHooks::Hook_EndScene, oEndScene);
		InstallD3DHook("DrawIndexedPrimitive", (PBYTE)vTable[D3DInfo::DrawIndexedPrimitive_Index], (PBYTE)D3DHooks::Hook_DIP, oDrawIndexedPrimitive);
		InstallD3DHook("DrawPrimitive", (PBYTE)vTable[D3DInfo::DrawPrimitive_Index], (PBYTE)D3DHooks::Hook_DP, oDrawPrimitive);
		InstallD3DHook("StretchRect", (PBYTE)vTable[D3DInfo::StretchRect_Index], (PBYTE)D3DHooks::Hook_StretchRect, oStretchRect);
		InstallD3DHook("SetRenderTarget", (PBYTE)vTable[D3DInfo::SetRenderTarget_Index], (PBYTE)D3DHooks::Hook_SetRenderTarget, oSetRenderTarget);
		InstallD3DHook("SetTexture", (PBYTE)vTable[D3DInfo::SetTexture_Index], (PBYTE)D3DHooks::Hook_SetTexture, oSetTexture);
		InstallD3DHook("DrawPrimitiveUP", (PBYTE)vTable[D3DInfo::DrawPrimitiveUP_Index], (PBYTE)D3DHooks::Hook_DrawPrimitiveUP, oDrawPrimitiveUP);
		InstallD3DHook("DrawIndexedPrimitiveUP", (PBYTE)vTable[D3DInfo::DrawIndexedPrimitiveUP_Index], (PBYTE)D3DHooks::Hook_DrawIndexedPrimitiveUP, oDrawIndexedPrimitiveUP);

		D3DHooks::InitializeCrcProvider();
	}

	/// <summary>
	/// Update settings so users don't need to restart the game for every mod they want to toggle on / off.
	/// </summary>
	void UpdateSettings() {
		Settings::UpdateSettings();
		ApplySettingBasedBugPrevention();
		Sleep(500);
		CustomSongTitles::LoadSettings();
		Sleep(500);
		CustomSongTitles::HookSongListsKoko();
		Sleep(500);
	}

	/// <summary>
	/// Initializes all core mods and hooks.
	/// </summary>
	void InitializeMods(bool debug) {
		D3DHooks::debug = debug;
		Offsets::Initialize();
		BugPrevention::FixModifyingFunctions();
		ApplyAlwaysOnBugPrevention();
		QualityOfLife::LowerNoteDetectionFloor(); // Before note detection starts
		QualityOfLife::RetryFastLoadUponSoftLock(); // Before the first menu loads
		Settings::Initialize();
		UpdateSettings();
		ERMode::Initialize();
		GUI();
		Midi::InitMidi();
		SongTuning::InstallTunerHook();
		Enumeration::HookEnumerationService();

		if (UltrafastLoad::Enabled()) {
			JsonNumberHash::Install();
			EnumerationDrain::Install();
			if (EnumerationDrain::IsInstalled())
				AssetLoadDrain::Install();
		}
		else if (GameVersion::Current() != Framework::GameVersion::Build::Unknown)
			LOG_INFO("(STARTUP) Ultrafast Load is off in RSMods.ini; the game scans its songs its own way" << std::endl);

		CrowdControl::StartServer();
	}

	/// <summary>
	/// Applies all mods and fixes that must run at startup.
	/// </summary>
	void ApplyStartupMods()
	{
		AudioDevices::SetupMicrophones();
		QualityOfLife::StopTwoRSInstances(); // Looks for the second instance's error dialog once, so it keeps its old timing
    ProfileSaveStreaming::Initialize();

		#ifdef _WWISE_LOGS
				Wwise::Logging::Init();
		#endif
	}

	/// <summary>
	/// Per-tick host game-state upkeep once the game has loaded, run before the mod registry ticks. All the
	/// song-side work this used to fan out to now lives in mods; only menu-side host upkeep remains here.
	/// </summary>
	void HandlePostGameLoadedMods()
	{
		GameState::currentMenu = GameState::GetCurrentMenu(); // This loads without checking if memory is safe... This can cause crashes if used when GameLoaded is false.
		GameState::LessonMode = GameState::Menus::IsInLessonModes();

		if (GameState::IsInSong())
			return;

		GameState::previousMenu = GameState::currentMenu;
	}

	/// <summary>
	/// Refreshes host game state while the game is still loading, before the mod registry ticks.
	/// </summary>
	void UpdateGameLoadingState() {
		GameState::currentMenu = GameState::GetCurrentMenu(true); 	// This is the safe version of checking the current menu. It is only used while the game boots, else the game may crash.
		BootIntro::NoteMenu(GameState::currentMenu);

		CheckIfGameHasLoaded();
	}

	/// <summary>
	/// Checks if the game has finished loading or has the user opened a new user profile?
	/// This prevents the user from being locked in a loop.
	/// </summary>
	void CheckIfGameHasLoaded() {
		if (!GameState::GameLoaded && (GameState::currentMenu == "MainMenu" ||
			GameState::currentMenu == "PlayedRS1Select" ||
			GameState::currentMenu == "SimpleDialog")) {
			GameState::GameLoaded = true;
			LOG_INFO("Game loaded: menu detected as '" << GameState::currentMenu << "'" << std::endl);
		}
	}
}
