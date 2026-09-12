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
}

namespace ModManager {
	void InitializeConfiguration() {
		if (!(std::ifstream("RSMods.ini"))) {
			std::ofstream RSModsFileOutput("RSMods.ini");
			RSModsFileOutput.close();
		}
	}

	/// <summary>
	/// Applies bug prevention patches for game crashing bugs and similar issues.
	/// </summary>
	void ApplyBugPrevention() {
		BugPrevention::PreventPnPCrash();
		QualityOfLife::StopTwoRSInstances();
		BugPrevention::AllowComplexPasswords();
		BugPrevention::PreventAdvancedDisplayCrash();
		BugPrevention::PreventPortAudioInDeviceCrash();
		BugPrevention::PreventExtraAudioDevicesCrash();
		BugPrevention::FixCalibrationSampleCount();

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

		// Hook D3D functions to use for our own D3D work. Reference D3DHooks
		oSetVertexDeclaration = (tSetVertexDeclaration)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetVertexDeclaration_Index], (byte*)D3DHooks::Hook_SetVertexDeclaration, 5); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexdeclaration
		oSetVertexShaderConstantF = (tSetVertexShaderConstantF)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetVertexShaderConstantF_Index], (byte*)D3DHooks::Hook_SetVertexShaderConstantF, 5); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexshaderconstantf
		oReset = (tReset)MemUtil::TrampHook((byte*)vTable[D3DInfo::Reset_Index], (byte*)D3DHooks::Hook_Reset, 5); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-reset
		oSetVertexShader = (tSetVertexShader)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetVertexShader_Index], (byte*)D3DHooks::Hook_SetVertexShader, 5); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexshader
		oSetPixelShader = (tSetPixelShader)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetPixelShader_Index], (byte*)D3DHooks::Hook_SetPixelShader, 5); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setpixelshader
		oSetStreamSource = (tSetStreamSource)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetStreamSource_Index], (byte*)D3DHooks::Hook_SetStreamSource, 5); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setstreamsource
		oEndScene = (tEndScene)MemUtil::TrampHook((byte*)vTable[D3DInfo::EndScene_Index], (byte*)D3DHooks::Hook_EndScene, 5); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-endscene
		oDrawIndexedPrimitive = (tDrawIndexedPrimitive)MemUtil::TrampHook((byte*)vTable[D3DInfo::DrawIndexedPrimitive_Index], (byte*)D3DHooks::Hook_DIP, 5); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-drawindexedprimitive
		oDrawPrimitive = (tDrawPrimitive)MemUtil::TrampHook((byte*)vTable[D3DInfo::DrawPrimitive_Index], (byte*)D3DHooks::Hook_DP, 5); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-drawprimitive
		oStretchRect = (tStretchRect)MemUtil::TrampHook((byte*)vTable[D3DInfo::StretchRect_Index], (byte*)D3DHooks::Hook_StretchRect, 5); // Widens the game's 16:9 letterbox composite for the ultrawide mod.
		oSetRenderTarget = (tSetRenderTarget)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetRenderTarget_Index], (byte*)D3DHooks::Hook_SetRenderTarget, 5); // Tracks whether the scene target is bound for the ultrawide mod.
		oDrawPrimitiveUP = (tDrawPrimitiveUP)MemUtil::TrampHook((byte*)vTable[D3DInfo::DrawPrimitiveUP_Index], (byte*)D3DHooks::Hook_DrawPrimitiveUP, 5); // Ultrawide correction covers every draw entry point.
		oDrawIndexedPrimitiveUP = (tDrawIndexedPrimitiveUP)MemUtil::TrampHook((byte*)vTable[D3DInfo::DrawIndexedPrimitiveUP_Index], (byte*)D3DHooks::Hook_DrawIndexedPrimitiveUP, 5);
	}

	/// <summary>
	/// Update settings so users don't need to restart the game for every mod they want to toggle on / off.
	/// </summary>
	void UpdateSettings() {
		Settings::UpdateSettings();
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
		Settings::Initialize();
		UpdateSettings();
		ERMode::Initialize();
		GUI();
		Midi::InitMidi();
		Enumeration::HookEnumerationService();

		CrowdControl::StartServer();
	}

	/// <summary>
	/// Applies all mods and fixes that must run at startup.
	/// </summary>
	void ApplyStartupMods()
	{
		AudioDevices::SetupMicrophones();
		ApplyBugPrevention();

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

		// Returning to a menu ends the song: drop the A/B loop markers so the next song starts fresh.
		if (Settings::IsOn(Setting::AllowLooping)) {
			Keybindings::loopStart = NULL;
			Keybindings::loopEnd = NULL;
		}

		D3DHooks::UpdateHeadstockCacheForMenu();
		GameState::previousMenu = GameState::currentMenu;
	}

	/// <summary>
	/// Refreshes host game state while the game is still loading, before the mod registry ticks.
	/// </summary>
	void UpdateGameLoadingState() {
		GameState::currentMenu = GameState::GetCurrentMenu(true); 	// This is the safe version of checking the current menu. It is only used while the game boots, else the game may crash.

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
		}
	}
}
