#include "../stdafx.h"
#include "MicrophoneVolumeOverrideMod.hpp"
#include "AudioDevices.hpp"

using Framework::ModContext;
using Framework::Availability;
namespace Setting = Settings::Setting;

bool MicrophoneVolumeOverrideMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::OverrideInputVolumeEnabled);
}

void MicrophoneVolumeOverrideMod::OnInitialize(ModContext& c) {
	c.Menu().Register("microphones", "Microphones", 20, [this] { DrawMenu(); }, Availability::Initialized);
}

void MicrophoneVolumeOverrideMod::OnMenuTick(ModContext& c) {
	SyncVolume(c);
}

void MicrophoneVolumeOverrideMod::OnSongTick(ModContext& c) {
	SyncVolume(c);
}

// Forces the configured input device to the desired volume. Idempotent: only writes when the
// device's current level differs, so it can run every tick without fighting the user's own changes.
void MicrophoneVolumeOverrideMod::SyncVolume(ModContext& c) {
	const std::string device = c.Value(Setting::OverrideInputVolumeDevice);
	if (device.empty())
		return;

	const int desired = c.Int(Setting::OverrideInputVolume);
	if (AudioDevices::GetMicrophoneVolume(device) != desired) {
		AudioDevices::SetMicrophoneVolume(device, desired);
	}
}

void MicrophoneVolumeOverrideMod::DrawMenu() {
	static std::string previewMicrophone = "Select a Microphone";
	static std::string selectedMicrophone = "";
	static std::vector<std::string> microphones;

	if (microphones.empty()) {
		for (const auto& [key, value] : AudioDevices::activeMicrophones) {
			microphones.push_back(key);
		}
	}

	if (ImGui::BeginCombo("Microphones", previewMicrophone.c_str())) {
		for (const auto& microphone : microphones) {
			const bool isSelected = (selectedMicrophone == microphone);

			if (ImGui::Selectable(microphone.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups)) {
				selectedMicrophone = microphone;
			}

			if (isSelected) {
				previewMicrophone = microphone;
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}

	// Button for testing setting volume on the microphone.
	if (ImGui::Button("Random Volume"))
		AudioDevices::SetMicrophoneVolume(selectedMicrophone, rand() % 100);
}

static Framework::ModRegistrar<MicrophoneVolumeOverrideMod> _micVolumeOverrideReg;
