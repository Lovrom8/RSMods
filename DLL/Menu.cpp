#include "stdafx.h"
#include "Menu.hpp"
#include "Mods/AudioDevices.hpp"
#include "Mods/VoiceOverControl.hpp"
#include "Mods/Midi.hpp"
#include "Mods/EnumerationDrain.hpp"
#include "Framework/Framework.hpp"

namespace Menu {
	void GenerateTestingTextures(IDirect3DDevice9* pDevice) {
		// **DEPRECATED** Generate solid color textures. Useful for testing
		D3D::GenerateSolidTexture(pDevice, &Red, D3DCOLOR_ARGB(255, 000, 255, 255));
		D3D::GenerateSolidTexture(pDevice, &Green, D3DCOLOR_ARGB(255, 0, 255, 0));
		D3D::GenerateSolidTexture(pDevice, &Blue, D3DCOLOR_ARGB(255, 0, 0, 255));
		D3D::GenerateSolidTexture(pDevice, &Yellow, D3DCOLOR_ARGB(255, 255, 255, 0));

		// **DEPRECATED** Generate texture from dds file.
		D3DXCreateTextureFromFile(pDevice, L"notes_gradient_normal.dds", &gradientTextureNormal); //if those don't exist, note heads will be "invisible" | 6-String Model
		D3DXCreateTextureFromFile(pDevice, L"notes_gradient_seven.dds", &gradientTextureSeven); // 7-String Note Colors
		D3DXCreateTextureFromFile(pDevice, L"gradient_map_additive.dds", &additiveNoteTexture); // Note Stems
	}

	/// <summary>
	/// Checks if the EndScene call originates from an overlay (e.g., Steam)
	/// instead of the game to prevent rendering our UI twice.
	/// </summary>
	/// <returns>True if the call is from an overlay, false otherwise.</returns>
	bool IsOverlayCall() {
		return (uint32_t)_ReturnAddress() > Offsets::baseEnd.Get();
	}

	// DLC scan progress in the upper right of the 16:9 menu frame, replacing the native "Enumerating downloadable content..."
	// banner that EnumerationDrain hides. Sizes follow the menu frame, so ultrawide and any resolution place it the same way.
	ImFont* overlayFont = nullptr;

	void RenderEnumerationProgress() {
		if (!EnumerationDrain::IsInstalled())
			return;

		const auto progress = EnumerationDrain::GetProgress();
		static float visibility = 0.0f;
		const ImGuiIO& io = ImGui::GetIO();
		// The completed bar holds 2 s (1 s when nothing was new), fading over its last 0.8 s.
		const double hold = progress.upToDate ? 1.0 : 2.0;
		const bool wanted = progress.active && (!progress.completed || progress.completionElapsedSeconds < hold);
		const float fadeTarget = !wanted ? 0.0f
			: progress.completed ? (std::min)(1.0f, static_cast<float>((hold - progress.completionElapsedSeconds) / 0.8))
			: 1.0f;
		const float step = (std::min)(1.0f, io.DeltaTime / 0.25f);
		visibility = fadeTarget < visibility ? fadeTarget : (std::min)(fadeTarget, visibility + step);
		if (visibility <= 0.001f)
			return;

		const bool indeterminate = !progress.completed && (progress.detected == 0 || progress.processed == 0);
		// No easing: the game calls EndScene more than once per frame, so DeltaTime is near zero and an eased fill lags behind.
		const float fraction = progress.completed ? 1.0f
			: progress.detected ? static_cast<float>(progress.processed) / progress.detected : 0.0f;

		// Menu frame: the centred 16:9 box of the backbuffer.
		const float menuWidth = (std::min)(io.DisplaySize.x, io.DisplaySize.y * (16.0f / 9.0f));
		const float menuLeft = (io.DisplaySize.x - menuWidth) * 0.5f;
		const float unit = menuWidth / 1366.0f;
		const float width = 376.0f * unit;
		const float right = menuLeft + menuWidth - 24.0f * unit;
		const float top = 24.0f * unit;
		const float fontScale = unit / io.FontGlobalScale;

		ImGui::SetNextWindowPos(ImVec2(right - width, top), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(width, 0.0f), ImGuiCond_Always);
		constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f * unit, 10.0f * unit));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, visibility);
		ImGui::Begin("##RSModsEnumerationProgress", nullptr, flags);
		if (overlayFont)
			ImGui::PushFont(overlayFont);
		ImGui::SetWindowFontScale(fontScale);
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const auto alpha = [&](int a) { return static_cast<int>(a * visibility); };

		// Rocksmith palette: cyan highlight, white text with a soft shadow, deep grey trough.
		const ImU32 cyan = IM_COL32(60, 200, 240, alpha(255));
		const ImU32 green = IM_COL32(90, 220, 110, alpha(255));
		const ImU32 greenFlash = IM_COL32(170, 255, 180, alpha(255));
		const ImU32 trough = IM_COL32(24, 26, 28, alpha(255));
		const ImU32 white = IM_COL32(255, 255, 255, alpha(255));
		const ImU32 shadow = IM_COL32(0, 0, 0, alpha(160));
		const ImU32 ink = IM_COL32(16, 22, 26, alpha(255));
		const ImU32 dim = IM_COL32(215, 215, 215, alpha(255));

		const auto shadowedText = [&](ImVec2 at, ImU32 colour, const char* text) {
			drawList->AddText(ImVec2(at.x + 1.5f * unit, at.y + 1.5f * unit), shadow, text);
			drawList->AddText(at, colour, text);
		};

		const char* title = progress.completed ? (progress.upToDate ? "UP TO DATE" : "UPDATED!") : "ENUMERATING DLC";
		const float titleWidth = ImGui::CalcTextSize(title).x;
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - titleWidth) * 0.5f);
		shadowedText(ImGui::GetCursorScreenPos(), white, title);
		ImGui::Dummy(ImVec2(titleWidth, ImGui::GetTextLineHeight()));

		// Bar. When done it flashes green three times over the first 0.45 s, then holds green.
		const float barHeight = 16.0f * unit;
		const float rounding = 5.0f * unit;
		const ImVec2 barMin = ImGui::GetCursorScreenPos();
		const ImVec2 barMax(barMin.x + ImGui::GetContentRegionAvail().x, barMin.y + barHeight);
		const bool flash = progress.completed && progress.completionElapsedSeconds < 0.45 && fmod(progress.completionElapsedSeconds, 0.15) < 0.075;
		const ImU32 fill = progress.completed ? (flash ? greenFlash : green) : cyan;
		drawList->AddRectFilled(barMin, barMax, trough, rounding);
		float fillRight = barMin.x;
		if (indeterminate) {
			// Sweep a short highlight while the total is unknown.
			const float span = barMax.x - barMin.x;
			const float sweepWidth = span * 0.22f;
			const float phase = static_cast<float>(fmod(ImGui::GetTime(), 1.4) / 1.4);
			const float sweepLeft = barMin.x - sweepWidth + (span + sweepWidth) * phase;
			drawList->PushClipRect(barMin, barMax, true);
			drawList->AddRectFilled(ImVec2(sweepLeft, barMin.y), ImVec2(sweepLeft + sweepWidth, barMax.y), cyan, rounding);
			drawList->PopClipRect();
		}
		else if (fraction > 0.0f) {
			fillRight = barMin.x + (barMax.x - barMin.x) * fraction;
			drawList->PushClipRect(barMin, ImVec2(fillRight, barMax.y), true);
			drawList->AddRectFilled(barMin, barMax, fill, rounding);
			drawList->PopClipRect();
		}

		// Count, smaller so it sits inside the bar: dark over the fill, white over the trough.
		ImGui::SetWindowFontScale(fontScale * 0.62f);
		const std::string countText = std::to_string(progress.processed) + " / " + std::to_string(progress.detected);
		const ImVec2 countSize = ImGui::CalcTextSize(countText.c_str());
		const ImVec2 countAt((barMin.x + barMax.x - countSize.x) * 0.5f, (barMin.y + barMax.y - countSize.y) * 0.5f);
		drawList->PushClipRect(barMin, ImVec2(fillRight, barMax.y), true);
		drawList->AddText(countAt, ink, countText.c_str());
		drawList->PopClipRect();
		drawList->PushClipRect(ImVec2(fillRight, barMin.y), barMax, true);
		shadowedText(countAt, white, countText.c_str());
		drawList->PopClipRect();
		ImGui::Dummy(ImVec2(barMax.x - barMin.x, barHeight + 6.0f * unit));

		// Stats: "8 NOT ADDED / 2 NEW / 827 TOTAL / 12.3s". Not added = duplicates, not owned, or unreadable.
		ImGui::SetWindowFontScale(fontScale * 0.72f);
		std::ostringstream stats;
		stats << progress.notRegistered << " NOT ADDED / " << progress.newDlc << " NEW / " << progress.totalDlc << " TOTAL / "
			<< std::fixed << std::setprecision(1) << progress.elapsedSeconds << "s";
		const std::string statsText = stats.str();
		const float statsWidth = ImGui::CalcTextSize(statsText.c_str()).x;
		ImGui::SetCursorPosX((std::max)(0.0f, (ImGui::GetWindowSize().x - statsWidth) * 0.5f));
		shadowedText(ImGui::GetCursorScreenPos(), dim, statsText.c_str());
		ImGui::Dummy(ImVec2(statsWidth, ImGui::GetTextLineHeight()));
		ImGui::SetWindowFontScale(1.0f);

		// Translucent blue-grey plate with the game's panel gradient (lighter top, darker bottom), behind everything.
		const ImVec2 windowMin = ImGui::GetWindowPos();
		const ImVec2 windowMax(windowMin.x + ImGui::GetWindowSize().x, windowMin.y + ImGui::GetWindowSize().y);
		const float plateRounding = 6.0f * unit;
		const ImU32 plateTop = IM_COL32(96, 106, 120, alpha(200));
		const ImU32 plateBottom = IM_COL32(38, 44, 54, alpha(215));
		ImDrawList* background = ImGui::GetBackgroundDrawList();
		background->AddRectFilled(windowMin, windowMax, plateTop, plateRounding);
		background->AddRectFilledMultiColor(ImVec2(windowMin.x, windowMin.y + plateRounding), ImVec2(windowMax.x, windowMax.y - plateRounding),
			plateTop, plateTop, plateBottom, plateBottom);
		background->AddRectFilled(ImVec2(windowMin.x, windowMax.y - plateRounding), windowMax, plateBottom, plateRounding, ImDrawFlags_RoundCornersBottom);

		if (overlayFont)
			ImGui::PopFont();
		ImGui::End();
		ImGui::PopStyleVar(3);
	}

	/// <summary>
	/// Renders the ImGui frame, including the main mod menu.
	/// </summary>
	void RenderImGuiMenu() {
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		RenderEnumerationProgress();

		if (Menu::menuEnabled) {
			ImGui::Begin("RS Mods");
			Menu::AddMidiMenu();
			Menu::AddCalibrationMenu();
			Menu::AddMicrophonesMenu();
			Menu::AddVoicelinesMenu();
			ImGui::End();
		}

		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

		ImGui::CaptureKeyboardFromApp(false);
		ImGui::CaptureMouseFromApp(false);
	}

	/// <summary>
	/// Regenerate string colors when the user changes their string colors in the GUI.
	/// Needed to have real-time textures.
	/// </summary>
	void UpdateStringTextures(IDirect3DDevice9* pDevice) {
		if (generateTexture) {
			D3D::GenerateTextures(pDevice, D3D::Strings);
			D3D::GenerateTextures(pDevice, D3D::Notes);

			generateTexture = false;
		}
	}

	void Init(IDirect3DDevice9* pDevice, LONG_PTR WndProc) {
		if (ImGuiInit) {
			return;
		}
		
		ImGuiInit = true;

		// Create ImGUI
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImFont* font = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoFont_data, RobotoFont_size, 20);
		io.FontDefault = font;
		// The enumeration progress bar uses the game's own face, Arial Bold from the Windows font folder, at about the native
		// banner's size at 768p. Roboto if the file is missing.
		{
			char fontPath[MAX_PATH]{};
			const UINT length = GetWindowsDirectoryA(fontPath, sizeof(fontPath));
			if (length > 0 && length + sizeof("\\Fonts\\arialbd.ttf") < sizeof(fontPath)) {
				strcat_s(fontPath, "\\Fonts\\arialbd.ttf");
				if (GetFileAttributesA(fontPath) != INVALID_FILE_ATTRIBUTES)
					overlayFont = io.Fonts->AddFontFromFileTTF(fontPath, 24.0f);
			}
			if (!overlayFont)
				overlayFont = io.Fonts->AddFontFromMemoryCompressedTTF(RobotoFont_data, RobotoFont_size, 26);
		}

		// Hook WndProc (Keypress manager)
		D3DDEVICE_CREATION_PARAMETERS d3dcp;
		pDevice->GetCreationParameters(&d3dcp);
		D3DHooks::hThisWnd = d3dcp.hFocusWindow;
		D3DHooks::oWndProc = (WNDPROC)SetWindowLongPtr(D3DHooks::hThisWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);

		ImGui_ImplWin32_Init(D3DHooks::hThisWnd);
		ImGui_ImplDX9_Init(pDevice);
		ImGui::GetIO().ImeWindowHandle = D3DHooks::hThisWnd;
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		LOG_INFO("ImGUI Init" << std::endl);

		Framework::Registry().EnqueueSettingsUpdate([] { Settings::UpdateSettings(); });

		CreateTextures(pDevice);
	}

	void AddStringColorTestingUI() {
		static bool CB = false;

		static std::string previewValue = "Select a string";
		if (ImGui::BeginCombo("String Colors", previewValue.c_str()))
		{
			for (int n = 0; n < 6; n++)
			{
				const bool is_selected = (selectedString == n);
				if (ImGui::Selectable(comboStringsItems[n], is_selected, ImGuiSelectableFlags_::ImGuiSelectableFlags_DontClosePopups))
					selectedString = n;

				if (is_selected) {
					previewValue = std::to_string(selectedString);

					RSColor currColors = D3D::GetCustomColors(selectedString, CB)["Enabled"];
					strR = (int)currColors.r * 255;
					strG = (int)currColors.g * 255;
					strB = (int)currColors.b * 255;
				}
			}

			ImGui::EndCombo();
		}

		ImGui::SliderInt("R", &strR, 0, 255);
		ImGui::SliderInt("G", &strG, 0, 255);
		ImGui::SliderInt("B", &strB, 0, 255);
		ImGui::Checkbox("CB", &CB);

		if (ImGui::Button("Generate Texture")) {
			Settings::SetStringColors(selectedString, RSColor(strR, strG, strB), CB);
			generateTexture = true;
		}

		if (ImGui::Button("Restore default colors"))
			ERMode::ResetString(selectedString);
	}

	void CreateTextures(IDirect3DDevice9* pDevice) {
		D3DXCreateTextureFromFile(pDevice, L"nonexistenttexture.dds", &nonexistentTexture); // Black Notes
		D3DXCreateTextureFromFile(pDevice, L"headstock.png", &customHeadstockTexture); // Custom Headstock

		// Green Screen Wall textures. Look at the uses of the textures for more information.
		D3DXCreateTextureFromFile(pDevice, L"stage0.png", &customGreenScreenWall_Stage0); // Background Tile
		D3DXCreateTextureFromFile(pDevice, L"stage1.png", &customGreenScreenWall_Stage1); // Noise
		D3DXCreateTextureFromFile(pDevice, L"stage2.png", &customGreenScreenWall_Stage2); // Caustic (Indirect)
		D3DXCreateTextureFromFile(pDevice, L"stage3.png", &customGreenScreenWall_Stage3); // Narnia / Venue Fade In Mask.
		D3DXCreateTextureFromFile(pDevice, L"stage4.png", &customGreenScreenWall_Stage4); // White square
		D3DXCreateTextureFromFile(pDevice, L"stage5.png", &customGreenScreenWall_Stage5); // Pipes and wall trim
		D3DXCreateTextureFromFile(pDevice, L"stage6.png", &customGreenScreenWall_Stage6); // N Mask of Background tile
		D3DXCreateTextureFromFile(pDevice, L"ChordFHM.png", &customChordPanelFHMTexture); // Custom Chord Panel FHM (for Metallica93).
	}

	void AddMidiMenu() {
		if (static std::string previewValue = "Select a device"; ImGui::BeginCombo("MIDI devices", previewValue.c_str())) 
		{
			for (size_t i = 0; i < Midi::NumberOfOutPorts; ++i)
			{
				const bool isSelected = (selectedDevice == static_cast<int>(i));
				const auto& device = Midi::midiOutDevices[i];

				if (ImGui::Selectable(device.szPname, isSelected, ImGuiSelectableFlags_DontClosePopups))
				{
					selectedDevice = i;
					Midi::SelectedMidiOutDevice = i;
				}

				if (isSelected)
				{
					previewValue = device.szPname;
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::SliderInt("Program Change", &Midi::MidiPC, 0, 127);
		ImGui::SliderInt("Control Change", &Midi::MidiCC, 0, 127);

		if (ImGui::Button("Send PC MIDI Message"))
			Midi::SendDataToThread_PC(Midi::AsMidiByte(Midi::MidiPC));

		if (ImGui::Button("Send CC MIDI Message"))
			Midi::SendDataToThread_CC(Midi::AsMidiByte(Midi::MidiCC));
	}

	void AddCalibrationMenu() {
		ImGui::Begin("Calibration");

		static float NewNoiseFloor = -59.3134f;
		static float NewToneBalance = -19.4793f;
		static float NewInputVolume = -3.22313f;
		static float NewInputVolumeReturn = -3.22313f;

		ImGui::SliderFloat("Noise Floor", &NewNoiseFloor, -100.f, 10.f);
		ImGui::SliderFloat("Tone Balance", &NewToneBalance, -100.f, 10.f);
		ImGui::SliderFloat("Input Volume", &NewInputVolume, -100.f, 10.f);
		ImGui::SliderFloat("Input Volume Return", &NewInputVolumeReturn, -100.f, 10.f);

		if (ImGui::Button("Calibrate"))
		{
			// Noise Floor
			Wwise::SoundEngine::SetRTPCValue("P1_NoiseFloor", NewNoiseFloor, 0x1234, 0, AkCurveInterpolation_Linear);
			Wwise::SoundEngine::SetRTPCValue("P1_NoiseFloor", NewNoiseFloor, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);

			// Tone Balance
			Wwise::SoundEngine::SetRTPCValue("Meter_Tone_Balance_Return", NewToneBalance, 0x1234, 0, AkCurveInterpolation_Linear);
			Wwise::SoundEngine::SetRTPCValue("Meter_Tone_Balance_Return", NewToneBalance, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);

			// Input Volume
			Wwise::SoundEngine::SetRTPCValue("P1_InputVol_Calibration", NewInputVolume, 0x1234, 0, AkCurveInterpolation_Linear);
			Wwise::SoundEngine::SetRTPCValue("P1_InputVol_Calibration", NewInputVolume, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);

			// Input Volume Return
			Wwise::SoundEngine::SetRTPCValue("P1_InputVol_Calibration_Return", NewInputVolumeReturn, 0x1234, 0, AkCurveInterpolation_Linear);
			Wwise::SoundEngine::SetRTPCValue("P1_InputVol_Calibration_Return", NewInputVolumeReturn, AK_INVALID_GAME_OBJECT, 0, AkCurveInterpolation_Linear);
		}

		ImGui::End();
	}

	void AddMicrophonesMenu() {
		ImGui::Begin("Microphones");

		static std::string previewMicrophone = "Select a Microphone";
		static std::string selectedMicrophone = "";
		static std::vector<std::string> microphones;

		if (microphones.empty()) 
		{
			for (const auto& [key, value] : AudioDevices::activeMicrophones)
			{
				microphones.push_back(key);
			}
		}

		if (ImGui::BeginCombo("Microphones", previewMicrophone.c_str())) 
		{
			for (const auto& microphone : microphones)
			{
				const bool isSelected = (selectedMicrophone == microphone);

				if (ImGui::Selectable(microphone.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups))
				{
					selectedMicrophone = microphone;
				}

				if (isSelected)
				{
					previewMicrophone = microphone;
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		// Button for testing setting volume on the microphone.
		if (ImGui::Button("Random Volume"))
			AudioDevices::SetMicrophoneVolume(selectedMicrophone, rand() % 100);

		ImGui::End();
	}

	void AddVoicelinesMenu() {
		ImGui::Begin("Voicelines");

		static std::string previewVoiceline = "Select a voiceline";
		static std::vector<VoiceOver> selectedVoiceOverList = VoiceOverControl::VO_ResultsScreens;

		// Drop-down list of all voice-overs in the selected Voice-Over list.
		if (ImGui::BeginCombo("Voicelines", previewVoiceline.c_str())) 
		{
			for (const auto& voiceOver : selectedVoiceOverList)
			{
				const bool isSelected = (VoiceOverControl::selectedVoiceOver.EventName == voiceOver.EventName);

				if (ImGui::Selectable(voiceOver.Text.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups))
				{
					VoiceOverControl::selectedVoiceOver = voiceOver;
				}

				if (isSelected)
				{
					previewVoiceline = voiceOver.Text;
					ImGui::SetItemDefaultFocus();
				}
			}

			ImGui::EndCombo();
		}

		if (ImGui::Button("Play selected voiceline"))
			VoiceOverControl::PlayVoiceOver(VoiceOverControl::selectedVoiceOver);

		ImGui::End();
	}
}
