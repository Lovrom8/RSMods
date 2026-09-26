#include "stdafx.h"
#include "Menu.hpp"
#include "Mods/AudioDevices.hpp"
#include "Mods/VoiceOverControl.hpp"
#include "Mods/Midi.hpp"
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

	/// <summary>
	/// Renders the ImGui frame, including the main mod menu.
	/// </summary>
	void RenderImGuiMenu() {
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

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
