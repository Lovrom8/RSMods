#include "../stdafx.h"
#include "VoiceOverControlMod.hpp"
#include "VoiceOverControl.hpp"

using Framework::ModContext;

void VoiceOverControlMod::OnInitialize(ModContext& c) {
	c.Menu().Register("voicelines", "Voicelines", 30, [this] { DrawMenu(); });
}

void VoiceOverControlMod::DrawMenu() {
	static std::string previewVoiceline = "Select a voiceline";
	static std::vector<VoiceOver> selectedVoiceOverList = VoiceOverControl::VO_ResultsScreens;

	// Drop-down list of all voice-overs in the selected Voice-Over list.
	if (ImGui::BeginCombo("Voicelines", previewVoiceline.c_str())) {
		for (const auto& voiceOver : selectedVoiceOverList) {
			const bool isSelected = (VoiceOverControl::selectedVoiceOver.EventName == voiceOver.EventName);

			if (ImGui::Selectable(voiceOver.Text.c_str(), isSelected, ImGuiSelectableFlags_DontClosePopups)) {
				VoiceOverControl::selectedVoiceOver = voiceOver;
			}

			if (isSelected) {
				previewVoiceline = voiceOver.Text;
				ImGui::SetItemDefaultFocus();
			}
		}

		ImGui::EndCombo();
	}

	if (ImGui::Button("Play selected voiceline"))
		VoiceOverControl::PlayVoiceOver(VoiceOverControl::selectedVoiceOver);
}

static Framework::ModRegistrar<VoiceOverControlMod> _voiceOverControlReg;
