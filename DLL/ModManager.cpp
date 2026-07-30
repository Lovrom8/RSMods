#include "stdafx.h"
#include "ModManager.hpp"
#include "Mods/DropPedal/DropPedal.hpp"
#include "Audio/DelayLinePitchShifter.hpp"

namespace
{
	// Time-domain period-synchronous shifter on the ASIO input path. Starts at unity;
	// the drop pedal hotkeys drive it once the hook is live.
	Audio::DelayLinePitchShifter pitchShifter{ 0 };
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

		if (Settings::ReturnSettingValue("FixBrokenTones") == "on") {
			BugPrevention::PreventStuckTone();
		}

		if (Settings::ReturnSettingValue("FixOculusCrash") == "on") {
			BugPrevention::PreventOculusCrash();
		}
	}

	/// <summary>
	/// Hook Game Functions For Our Own Uses (On Alt-Tab, Draw UI, etc).
	/// </summary>
	void GUI() {
		uint32_t d3d9Base;
		uint32_t adr;
		uint32_t* vTable = NULL;

		// Find D3D Device
		while ((d3d9Base = (uint32_t)GetModuleHandleA("d3d9.dll")) == NULL)
			Sleep(500);
		adr = MemUtil::FindPattern<uint32_t>(d3d9Base, Offsets::d3dDevice_SearchLen, (byte*)Offsets::d3dDevice_Pattern, Offsets::d3dDevice_Mask) + 2;

		bool runningThroughWine = adr == (uint32_t)2;

		LOG_INFO("Running in Wine: " << std::boolalpha << runningThroughWine << std::endl);

		// Proton / Wine Check.
		// We do NOT support linux. There is some issues with the D3D pointers.
		// This check is meant so if someone does load our mods on Linux, we won't just crash. Most mods will just not work.
		if (runningThroughWine) {
			LOG_INFO("Performing Wine check" << std::endl);
			adr = MemUtil::FindPattern<uint32_t>(0x2000000, 0x2B778CC, (byte*)Offsets::d3dDevice_Pattern, Offsets::d3dDevice_Mask) + 2;
			LOG_INFO(adr << std::endl);
		}

		if (!adr) {
			LOG_ERROR("Could not find D3D9 device pointer." << std::endl);
			return;
		}

		if (!*(uint32_t*)adr) { // Wing it
			LOG_ERROR("Could not find DX9 device." << std::endl);
			MessageBoxA(NULL, "Could not find DX9 device, please restart the game!", "Error", NULL);
			return;
		}

		vTable = *(uint32_t**)adr;

		// Third time's the charm?
		if (!vTable || vTable < (uint32_t*)Offsets::baseHandle) {
			LOG_ERROR("Could not find D3D device's vTable address." << std::endl);
			MessageBoxA(NULL, "Could not find D3D device's vTable address \n Restart the game and if you still get this error after a few tries, please report the error!", "Error", NULL);
			return;
		}

		// Hook D3D functions to use for our own D3D work. Reference D3DHooks
		if (!runningThroughWine) {
			oSetVertexDeclaration = (tSetVertexDeclaration)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetVertexDeclaration_Index], (byte*)D3DHooks::Hook_SetVertexDeclaration, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexdeclaration
			oSetVertexShaderConstantF = (tSetVertexShaderConstantF)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetVertexShaderConstantF_Index], (byte*)D3DHooks::Hook_SetVertexShaderConstantF, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexshaderconstantf
			oReset = (tReset)DetourFunction((byte*)vTable[D3DInfo::Reset_Index], (byte*)D3DHooks::Hook_Reset); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-reset
		}

		oSetVertexShader = (tSetVertexShader)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetVertexShader_Index], (byte*)D3DHooks::Hook_SetVertexShader, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setvertexshader
		oSetPixelShader = (tSetPixelShader)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetPixelShader_Index], (byte*)D3DHooks::Hook_SetPixelShader, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setpixelshader
		oSetStreamSource = (tSetStreamSource)MemUtil::TrampHook((byte*)vTable[D3DInfo::SetStreamSource_Index], (byte*)D3DHooks::Hook_SetStreamSource, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-setstreamsource
		oEndScene = (tEndScene)MemUtil::TrampHook((byte*)vTable[D3DInfo::EndScene_Index], (byte*)D3DHooks::Hook_EndScene, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-endscene
		oDrawIndexedPrimitive = (tDrawIndexedPrimitive)MemUtil::TrampHook((byte*)vTable[D3DInfo::DrawIndexedPrimitive_Index], (byte*)D3DHooks::Hook_DIP, 5); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-drawindexedprimitive
		oDrawPrimitive = (tDrawPrimitive)MemUtil::TrampHook((byte*)vTable[D3DInfo::DrawPrimitive_Index], (byte*)D3DHooks::Hook_DP, 7); // https://docs.microsoft.com/en-us/windows/win32/api/d3d9helper/nf-d3d9helper-idirect3ddevice9-drawprimitive
	}

	/// <summary>
	/// Update settings so users don't need to restart the game for every mod they want to toggle on / off.
	/// </summary>
	void UpdateSettings() {
		Settings::UpdateSettings();
		DropPedal::LoadSettings();
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
		// Must run before anything creates an IMMDeviceEnumerator, our own microphone setup
		// included, or the game builds its audio chain before we can see it.
		if (DropPedal::ShouldInstallInputHooks())
		{
			Audio::CaptureHook::Install();
			Audio::AsioHook::Install();
			Audio::AsioHook::SetProcessor(&pitchShifter);
		}

		AudioDevices::SetupMicrophones();
		ApplyBugPrevention();
		ApplyAudioDeviceConfiguration();

		#ifdef _WWISE_LOGS
				Wwise::Logging::Init();
		#endif

		// Look to see if RS_ASIO applied the 2 RTC input bypass.
		// If they did, then we disregard the results from our version of the mod.
		bool rsAsioBypassTwoRTC = false;
		LOG_INFO("RS_ASIO Bypass2RTC: " << std::boolalpha << rsAsioBypassTwoRTC << std::endl);

		if (Settings::ReturnSettingValue("BypassTwoRTCMessageBox") == "on") {
			QualityOfLife::PatchTwoRTC();
		}

		// Patch x86 assembly for Riff Repeater speed logic to make it linear.
		if (Settings::ReturnSettingValue("LinearRiffRepeater") == "on") {
			RiffRepeater::EnableLinearSpeeds();
		}

		// Allow the user to have a small amount of time to Alt+Tab while the game continues playing the audio.
		if (Settings::ReturnSettingValue("AllowAudioInBackground") == "on") {
			VolumeControl::AllowAltTabbingWithAudio();
		}

		DropPedal::InstallHooks();
	}

	/// <summary>
	/// Configures audio device settings including sample rates.
	/// We have to do this early in execution as we need to change it before the audio engine starts up.
	/// </summary>
	void ApplyAudioDeviceConfiguration() 
	{
		Midi::tuningOffset = Settings::GetModSetting("TuningOffset");

		if (Settings::ReturnSettingValue("AltOutputSampleRate") == "on" &&
			Settings::GetModSetting("AlternativeOutputSampleRate") != 48000) {
			LOG_WARNING("[!] Overriding Output Sample Rate to " << Settings::GetModSetting("AlternativeOutputSampleRate") << std::endl);
			AudioDevices::output_SampleRate = Settings::GetModSetting("AlternativeOutputSampleRate");
			AudioDevices::ChangeOutputSampleRate();
		}
	}


	/// <summary>
	/// Handles dynamic toggling of linear riff repeater mode.
	/// </summary>
	void HandleLinearRiffRepeaterToggle() {
		if (Settings::ReturnSettingValue("LinearRiffRepeater") == "on" &&
			!RiffRepeater::currentlyEnabled_LinearRR) {
			RiffRepeater::EnableLinearSpeeds();
		}
		else if (Settings::ReturnSettingValue("LinearRiffRepeater") == "off" &&
			RiffRepeater::currentlyEnabled_LinearRR) {
			RiffRepeater::DisableLinearSpeeds();
		}
	}

	/// <summary>
	/// Scans for MIDI devices when auto-tuning is enabled.
	/// </summary>
	void HandleMidiDeviceScanning() {
		if (!Midi::scannedForMidiDevices && Settings::ReturnSettingValue("AutoTuneForSong") == "on") {
			Midi::scannedForMidiDevices = true;
			Midi::ReadMidiSettingsFromINI(
				Settings::ReturnSettingValue("ChordsMode"),
				Settings::GetModSetting("TuningPedal"),
				Settings::ReturnSettingValue("AutoTuneForSongDevice"),
				Settings::ReturnSettingValue("MidiInDevice")
			);
		}

		if (!Midi::attemptedToDetachMidiInThread && Settings::ReturnSettingValue("MidiInDevice") != "") {
			Midi::attemptedToDetachMidiInThread = true;
			Midi::FindMidiInDevices(Settings::ReturnSettingValue("MidiInDevice"));
			std::thread(Midi::ListenToMidiInThread).detach();
		}
	}

	/// <summary>
	/// Handles rainbow string and note effects.
	/// </summary>
	void PollDropPedalHotkeys()
	{
		DropPedal::PollHotkeys();

		if (DropPedal::ShouldInstallInputHooks() && Audio::AsioHook::IsProcessingEnabled())
		{
			// Atomic store inside; safe from this thread.
			pitchShifter.SetSemitones(DropPedal::IsEnabled() ? DropPedal::GetTargetSemitones() : 0);
		}
	}

	void HandleRainbowEffects() {
		if (ERMode::IsRainbowEnabled() || ERMode::IsRainbowNotesEnabled()) {
			ERMode::DoRainbow();
		}
		else {
			ERMode::Toggle7StringMode();
		}
	}

	bool MoreThanThreeSecondsPassed() {
		const auto currentTime = std::chrono::steady_clock::now();
		return currentTime - GameOverlay::displayVolumeStartTime > std::chrono::seconds(3);
	}

	/// <summary>
	/// Manages the volume control overlay display timer.
	/// </summary>
	void HandleVolumeDisplay() {
		if (Settings::ReturnSettingValue("VolumeControlEnabled") == "on" && MoreThanThreeSecondsPassed()) {
			GameOverlay::displayCurrentVolume = false;	
		}
	}

	/// <summary>
	/// Handles mods that run regardless of game state.
	/// </summary>
	void HandleAlwaysOnMods(GameLoopState& state) {

		DropPedal::Poll();

		if (DropPedal::ShouldInstallInputHooks())
		{
			Audio::AsioHook::Poll();

			// Engine arbitration: exactly one pitch system may be live. Once the ASIO input
			// shifter is processing, it owns pitch; the game-side MultiPitch path stays
			// suppressed for the session. The hotkey thread keeps the shifter's semitones in
			// step, since key changes land there.
			DropPedal::SetInputShifterActive(Audio::AsioHook::IsProcessingEnabled());

			if (DropPedal::RequiresInputShifter() && !Audio::AsioHook::IsProcessingEnabled())
			{
				DropPedal::ReportInputShifterUnavailable();
			}
		}

		if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" &&
			Settings::ReturnSettingValue("RemoveHeadstockWhen") == "startup") {
			D3DHooks::RemoveHeadstockInThisMenu = true;
		}

		if (GameState::LessonMode &&
			Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" &&
			Settings::ReturnSettingValue("ToggleLoftWhen") != "manual") {
			if (state.loftOff) {
				Loft::ToggleLoft();
			}
			state.loftOff = false;

			D3DHooks::GreenScreenWall = true; // Turn off in Lesson Mode(or the videos won't appear). Emulate effect with GreenScreenWall.
		}

		HandleVolumeDisplay();

		if (!state.loftOff && !GameState::LessonMode &&
			Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" &&
			Settings::ReturnSettingValue("ToggleLoftWhen") == "startup") {
			Loft::ToggleLoft();
			state.loftOff = true;

			D3DHooks::GreenScreenWall = false;
		}

		if (!D3DHooks::SkylineOff &&
			Settings::ReturnSettingValue("RemoveSkylineEnabled") == "on" &&
			Settings::ReturnSettingValue("ToggleSkylineWhen") == "startup") {
			D3DHooks::toggleSkyline = true;
		}

		if (!D3DHooks::RemoveLyrics && Settings::ReturnSettingValue("RemoveLyricsWhen") == "startup") {
			D3DHooks::RemoveLyrics = true;
		}

		if (GameState::Menus::IsInPreSongTuner() &&
			Settings::ReturnSettingValue("AutoTuneForSong") == "on" &&
			Settings::ReturnSettingValue("AutoTuneForSongWhen") == "tuner" &&
			!Midi::alreadyAttemptedTuningInTuner &&
			!Midi::alreadyAutomatedTuningInThisSong) {
			Midi::AttemptTuningInTuner();
			state.skipERSleep = true;
		}
	}

	/// <summary>
	/// Main update loop when the game has finished loading.
	/// </summary>
	void HandlePostGameLoadedMods(GameLoopState& state) 
	{
		GameState::currentMenu = GameState::GetCurrentMenu(); // This loads without checking if memory is safe... This can cause crashes if used when GameLoaded is false.

		HandleExternalMonitor(state);
		HandleMicrophoneVolumeOverride();
		HandleAudioBackgroundToggle();
		HandleTwoRTCBypassToggle();
		HandleNonStopPlayTimer();
		HandleLinearRiffRepeaterToggle();
		HandleMidiDeviceScanning();

		GameState::LessonMode = GameState::Menus::IsInLessonModes();

		if (GameState::IsInSong()) {
			HandleInSongState(state);
		}
		else {
			HandleInMenuState(state);
		}

		HandleAlwaysOnMods(state);
		HandleRainbowEffects();
	}

	/// <summary>
	/// Handles all state updates when the player is in menus.
	/// </summary>
	void HandleInMenuState(GameLoopState& state) {
		HandleExtendedRangeInTuner(state);
		CleanupSongSpecificStates(state);
		HandleMenuVisualMods(state);
		HandleMenuFeatures(state);
		HandleHeadstockCacheReset(state);

		GameState::previousMenu = GameState::currentMenu;
	}

	/// <summary>
	/// Enables extended range mode in the tuner if applicable.
	/// </summary>
	void HandleExtendedRangeInTuner(const GameLoopState& state) {
		if (GameState::Menus::IsInPreSongTuner()) {
			if (!ERMode::AttemptedERInTuner) {
				if (!state.skipERSleep) {
					Sleep(1500);
				}
				ERMode::AttemptedERInTuner = true;
				ERMode::UseERInTuner = SongTuning::IsExtendedRangeTuner();
			}
		}
		else {
			ERMode::AttemptedERInTuner = false;
			ERMode::UseERInTuner = false;
		}
	}

	/// <summary>
	/// Cleans up states that are only active during songs.
	/// </summary>
	void CleanupSongSpecificStates(GameLoopState& state) {
		if (Settings::ReturnSettingValue("AllowLooping") == "on") {
			Keybindings::loopStart = NULL;
			Keybindings::loopEnd = NULL;
		}

		if (!GameState::Menus::IsInScoreMenus() && RiffRepeater::currentlyEnabled_Above100) {
			RiffRepeater::DisableTimeStretch();
		}

		if (ERMode::AttemptedERInThisSong) {
			ERMode::UseERExclusivelyInThisSong = false;
			ERMode::UseEROrColorsInThisSong = false;
			ERMode::AttemptedERInThisSong = false;
		}

		if (state.automatedSongTimer &&
			Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on" &&
			Settings::ReturnSettingValue("ShowSongTimerWhen") == "automatic") {
			state.automatedSongTimer = false;
			D3DHooks::showSongTimerOnScreen = false;
		}

		if ((Midi::alreadyAutomatedTuningInThisSong || Midi::alreadyAttemptedTuningInTuner) &&
			!GameState::Menus::IsInPreSongTuner()) {
			Midi::RevertAutomatedTuning();
			Midi::alreadyAttemptedTuningInTuner = false;
			Midi::userWantsToUseAutoTuning = false;
		}

		DropPedal::ResetSongState();
	}

	/// <summary>
	/// Reverts visual mod states when exiting songs.
	/// </summary>
	void HandleMenuVisualMods(GameLoopState& state) {
		if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" &&
			Settings::ReturnSettingValue("RemoveHeadstockWhen") == "song") {
			D3DHooks::RemoveHeadstockInThisMenu = false;
		}

		if (Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" &&
			Settings::ReturnSettingValue("ToggleLoftWhen") == "song") {
			if (state.loftOff) {
				Loft::ToggleLoft();
				state.loftOff = false;
			}
			if (!GameState::LessonMode) {
				D3DHooks::GreenScreenWall = false;
			}
		}

		if (D3DHooks::SkylineOff &&
			Settings::ReturnSettingValue("RemoveSkylineEnabled") == "on" &&
			Settings::ReturnSettingValue("ToggleSkylineWhen") == "song") {
			D3DHooks::toggleSkyline = true;
			D3DHooks::DrawSkylineInMenu = true;
		}
	}

	/// <summary>
	/// Handles menu-specific features like Guitar Speak and song previews.
	/// </summary>
	void HandleMenuFeatures(GameLoopState& state) {
		if (!state.guitarSpeakPresent && Settings::ReturnSettingValue("GuitarSpeak") == "on") {
			state.guitarSpeakPresent = true;
			if (!GuitarSpeak::RunGuitarSpeak()) { // If we are in a menu where we don't want to read bad values
				state.guitarSpeakPresent = false;
			}
		}

		if (Settings::ReturnSettingValue("SongPreviews") == "on") {
			if (!VolumeControl::disabledSongPreviewAudio) {
				VolumeControl::DisableSongPreviewAudio();
			}
		}
		else if (VolumeControl::disabledSongPreviewAudio) { // User originally wanted song previews off, but now wants them on.
			VolumeControl::EnableSongPreviewAudio();
		}

		if (Settings::ReturnSettingValue("ScreenShotScores") == "on" &&
			GameState::Menus::IsInScoreMenus()) {
			Keyboard::TakeScreenshot();
		}
		else {
			Keyboard::takenScreenshotOfThisScreen = false;
		}
	}

	/// <summary>
	/// Resets the headstock texture cache when appropriate.
	/// So we aren't running the same textures over and over again.
	/// </summary>
	void HandleHeadstockCacheReset(GameLoopState& state) {
		if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" &&
			!GameState::Menus::IsInTuningMenus() ||
			GameState::currentMenu == "MissionMenu") {
			D3DHooks::resetHeadstockCache = true;
		}

		// If the current menu is not the same as the previous menu and if it's one of menus where you tune your guitar (i.e. headstock is shown), reset the cache because user may want to change the headstock style
		if (GameState::previousMenu != GameState::currentMenu &&
			GameState::Menus::IsInTuningMenus()) {
			D3DHooks::resetHeadstockCache = true;
			headstockTexturePointers.clear();
		}
	}


	/// <summary>
	/// Handles moving the game window to an external monitor.
	/// </summary>
	void HandleExternalMonitor(GameLoopState& state) 
	{
		if (!state.movedToExternalDisplay && Settings::ReturnSettingValue("SecondaryMonitor") == "on") {
			LaunchOnExternalMonitor::SendRocksmithToScreen(
				Settings::GetModSetting("SecondaryMonitorXPosition"),
				Settings::GetModSetting("SecondaryMonitorYPosition")
			);
			state.movedToExternalDisplay = true;
		}
	}

	/// <summary>
	/// Manages microphone volume override settings.
	/// </summary>
	void HandleMicrophoneVolumeOverride() {
		if (Settings::ReturnSettingValue("OverrideInputVolumeEnabled") == "on" &&
			Settings::ReturnSettingValue("OverrideInputVolumeDevice") != "" &&
			AudioDevices::GetMicrophoneVolume(Settings::ReturnSettingValue("OverrideInputVolumeDevice")) !=
			Settings::GetModSetting("OverrideInputVolume")) 
		{
			AudioDevices::SetMicrophoneVolume(
				Settings::ReturnSettingValue("OverrideInputVolumeDevice"),
				Settings::GetModSetting("OverrideInputVolume")
			);
		}
	}

	/// <summary>
	/// Handles dynamic toggling of audio-in-background feature.
	/// </summary>
	void HandleAudioBackgroundToggle() 
	{
		if (Settings::ReturnSettingValue("AllowAudioInBackground") == "on" && !VolumeControl::allowedAltTabbingWithAudio) {
			VolumeControl::AllowAltTabbingWithAudio();
		} else if (Settings::ReturnSettingValue("AllowAudioInBackground") == "off" && VolumeControl::allowedAltTabbingWithAudio) {
			VolumeControl::DisableAltTabbingWithAudio(); // User originally wanted to NOT allow audio in the background, but they changed their mind, so we have to turn it on/
		}
	}

	/// <summary>
	/// Handles dynamic toggling of the two RTC message box bypass.
	/// </summary>
	void HandleTwoRTCBypassToggle() 
	{
		static bool rsAsioBypassTwoRTC = false;

		if (rsAsioBypassTwoRTC) return;

		if (Settings::ReturnSettingValue("BypassTwoRTCMessageBox") == "off" && *(char*)Offsets::ptr_twoRTCBypass.Get() == Offsets::ptr_twoRTCBypass_patch_call[0]) {
			MemUtil::PatchAdr((LPVOID)Offsets::ptr_twoRTCBypass.Get(), (LPVOID)Offsets::ptr_twoRTCBypass_original, 6);
		}
		else if (Settings::ReturnSettingValue("BypassTwoRTCMessageBox") == "on" && *(char*)Offsets::ptr_twoRTCBypass.Get() == Offsets::ptr_twoRTCBypass_original[0]) {
			QualityOfLife::PatchTwoRTC();
		}
	}


	/// <summary>
	/// Manages custom non-stop play timer settings.
	/// </summary>
	void HandleNonStopPlayTimer() {
		const bool useCustom = Settings::ReturnSettingValue("UseCustomNSPTimer") == "on";
		const double desired = useCustom
			? Settings::GetModSetting("CustomNSPTimeLimit") / 1000.0
			: DefaultNSPTimeLimit;

		const double current = SongTimer::GetNonStopPlayTimer();

		const double eps = std::numeric_limits<double>::epsilon() * std::max(1.0, std::max(std::abs(desired), std::abs(current))) * 4;
		if (std::abs(current - desired) > eps) {
			LOG_INFO("Updating NSP timer...");
			SongTimer::SetNonStopPlayTimer(desired);
		}
	}

	/// <summary>
	/// Handles updates while the game is still loading.
	/// </summary>
	void UpdateGameLoadingState(GameLoopState& state) {
		GameState::currentMenu = GameState::GetCurrentMenu(true); 	// This is the safe version of checking the current menu. It is only used while the game boots, else the game may crash.

		CheckIfGameHasLoaded();
		ConfigureAlternativeSampleRate();
		HandleAutoLoadProfile(state);
	}

	/// <summary>
	/// Configures buffer settings for alternative sample rates.
	/// </summary>
	void ConfigureAlternativeSampleRate() {
		if (Settings::ReturnSettingValue("AltOutputSampleRate") == "on" &&
			Settings::GetModSetting("AlternativeOutputSampleRate") != 48000 &&
			*(int*)Offsets::ptr_sampleRateBuffer.Get() != 5 &&
			*(int*)Offsets::ptr_sampleRateBuffer.Get() != 2) {
			*(int*)Offsets::ptr_sampleRateSize.Get() = 2;
			*(int*)Offsets::ptr_sampleRateBuffer.Get() = 128;
		}
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

	/// <summary>
	/// Handles all state updates when the player is in a song.
	/// </summary>
	void HandleInSongState(GameLoopState& state) {
		state.guitarSpeakPresent = false;
		ERMode::AttemptedERInTuner = false;
		ERMode::UseERInTuner = false;

		LogSongIDForRiffRepeater();
		EnableRiffRepeaterFeatures();
		HandleInSongVisualMods(state);
		HandleMidiAutoTuningInSong();
		DropPedal::HandleTuningInSong();
		HandleSongTimerDisplay(state);
		HandleExtendedRangeInSong(state);
	}

	/// <summary>
	/// Handles automatic profile loading (AKA "Fork in the toaster" mod).
	/// </summary>
	void HandleAutoLoadProfile(GameLoopState& state) {
		if (Settings::ReturnSettingValue("ForceProfileEnabled") != "on" ||
			GameState::Menus::IsInMenusWithDisallowedAutoEnter() ||
			state.forkInToasterNewProfile) {
			return;
		}

		// Skip UPlay login dialog - depending on the menu it might be necessary to press either ESC or Enter, so just spam both
		if (GameState::currentMenu == "SelectionListDialog" ||
			GameState::currentMenu == "UplayLoginDialog") {
			Keyboard::SendEscapeKey();
			Keyboard::AutoEnterGame();
		}
		else if (Settings::ReturnSettingValue("ProfileToLoad") != "" &&
			GameState::currentMenu == "ProfileSelect") { // If the user user says "I want to always load this profile"
			HandleSpecificProfileLoad(state);
		}
		else { 	// User doesn't care what profile we select, just select the first / top one.
			Keyboard::AutoEnterGame();
		}
	}

	/// <summary>
	/// Loads a specific user profile by name.
	/// </summary>
	void HandleSpecificProfileLoad(GameLoopState& state) {
		state.selectedUser = GameState::CurrentSelectedUser();

		if (state.selectedUser == Settings::ReturnSettingValue("ProfileToLoad")) {
			Keyboard::AutoEnterGame();
		}
		else if (state.selectedUser == "New profile") {
			LOG_ERROR("(Auto Load) Invalid Profile Name" << std::endl); // Yeah, the profile they're looking for doesn't exist :(
			state.forkInToasterNewProfile = true;
		}
		else { // Not the profile we're looking for. Move down 1.
			Keyboard::PressDownArrowKey();
		}
	}

	/// <summary>
	/// Logs song ID for Riff Repeater > 100% functionality.
	/// We are in a song we haven't seen in this play session. Log its Id so we can prep for the Riff Repeater > 100% mod.
	/// </summary>
	void LogSongIDForRiffRepeater() {
		if (RiffRepeater::readyToLogSongID && RiffRepeater::LogSongID(GameState::GetSongKey())) {	
			RiffRepeater::readyToLogSongID = false;
		}
	}

	/// <summary>
	/// Enables riff repeater time stretching features.
	/// </summary>
	void EnableRiffRepeaterFeatures() {
		if (Settings::ReturnSettingValue("RRSpeedAboveOneHundred") == "on") {
			RiffRepeater::EnableTimeStretch();
		}
	}


	/// <summary>
	/// Manages visual mod states when entering a song.
	/// </summary>
	void HandleInSongVisualMods(GameLoopState& state) {
		if (Settings::ReturnSettingValue("RemoveHeadstockEnabled") == "on" &&
			Settings::ReturnSettingValue("RemoveHeadstockWhen") == "song") {
			D3DHooks::RemoveHeadstockInThisMenu = true;
		}

		if (Settings::ReturnSettingValue("ToggleLoftEnabled") == "on" &&
			Settings::ReturnSettingValue("ToggleLoftWhen") == "song") {
			if (!state.loftOff) {
				Loft::ToggleLoft();
			}
			state.loftOff = true;
		}

		if (Settings::ReturnSettingValue("RemoveSkylineEnabled") == "on" &&
			Settings::ReturnSettingValue("ToggleSkylineWhen") == "song") {
			if (!D3DHooks::SkylineOff) {
				D3DHooks::toggleSkyline = true;
			}
			D3DHooks::DrawSkylineInMenu = false;
		}
	}

	/// <summary>
	/// Handles MIDI auto-tuning when entering a song.
	/// </summary>
	void HandleMidiAutoTuningInSong() {
		if (Settings::ReturnSettingValue("AutoTuneForSong") == "on" &&
			!Midi::alreadyAutomatedTuningInThisSong &&
			(Settings::ReturnSettingValue("AutoTuneForSongWhen") == "tuner" ||
				(Settings::ReturnSettingValue("AutoTuneForSongWhen") == "manual" && Midi::userWantsToUseAutoTuning))) {
			Midi::AutomateTuning();
		}
	}

	/// <summary>
	/// Shows or hides the song timer based on settings.
	/// </summary>
	void HandleSongTimerDisplay(GameLoopState& state) {
		if (!state.automatedSongTimer &&
			Settings::ReturnSettingValue("ShowSongTimerEnabled") == "on" &&
			Settings::ReturnSettingValue("ShowSongTimerWhen") == "automatic") {
			state.automatedSongTimer = true;
			D3DHooks::showSongTimerOnScreen = true;
		}
	}

	/// <summary>
	/// Detects and attempts to enable extended range mode for appropriate songs.
	/// </summary>
	void HandleExtendedRangeInSong(const GameLoopState& state) {
		if (!ERMode::AttemptedERInThisSong) {
			if (!state.skipERSleep) { // Tuning takes a second, or so, to get set by the game. We use this to make sure we have the right tuning numbers. Otherwise, we would never get ER mode to turn on properly.
				Sleep(1500);
			}
			ERMode::UseERExclusivelyInThisSong = SongTuning::IsExtendedRangeSong();
			ERMode::UseEROrColorsInThisSong = (Settings::ReturnSettingValue("ExtendedRangeEnabled") == "on" &&
				ERMode::UseERExclusivelyInThisSong) ||
				Settings::GetModSetting("CustomStringColors") == 2 ||
				(Settings::ReturnSettingValue("SeparateNoteColors") == "on" &&
					Settings::GetModSetting("SeparateNoteColorsMode") != 1);
			ERMode::AttemptedERInThisSong = true;
		}
	}
}
