#include "../stdafx.h"
#include "SongAccuracyMod.hpp"
#include "../SongTimer.hpp"
#include "../NoteData.h"

using Framework::ModContext;
using Framework::SettingDef;
using Framework::SettingDefs;
namespace Setting = Settings::Setting;

SettingDefs SongAccuracyMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::DisplayCurrentAccuracy, "DisplayCurrentAccuracy", "Display Current Accuracy")
	};
}

bool SongAccuracyMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::DisplayCurrentAccuracy);
}

float SongAccuracyMod::ReadAccuracy() {
	const bool isLAS = GameState::Menus::IsInLearnASongModes();
	const bool isSA = GameState::Menus::IsInScoreAttackModes();

	uintptr_t addr = 0;
	if (isLAS) {
		addr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_noteData, Offsets::ptr_noteDataOffsets);
	}
	else if (isSA) {
		addr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_scoreAttackNoteData, Offsets::ptr_scoreAttackNoteDataOffsets);
	}
	else {
		return 0.0f;
	}

	if (!addr) return 0.0f;

	if (isLAS) {
		const LearnASongNoteData* data = reinterpret_cast<LearnASongNoteData*>(addr);
		return data->getAccuracy();
	}
	else if (isSA) {
		const ScoreAttackNoteData* data = reinterpret_cast<ScoreAttackNoteData*>(addr);
		return data->getAccuracy();
	}

	return 0.0f;
}

void SongAccuracyMod::OnSongTick(ModContext& c) {
	Publish(c);
}

void SongAccuracyMod::OnMenuTick(ModContext& c) {
	Publish(c);
}

void SongAccuracyMod::Publish(ModContext& c) const {
	Framework::HudText snapshot;
	snapshot.visible = c.IsOn(Setting::DisplayCurrentAccuracy) &&
		GameState::IsInSong() &&
		SongTimer::SongTimer() != 0.f;

	if (snapshot.visible) {
		const float accuracy = ReadAccuracy();
		std::stringstream ss;
		ss << std::fixed << std::setprecision(2) << accuracy << "%";
		snapshot.text = ss.str();
	}

	c.Hud().Set("song-accuracy", { Framework::HudAnchor::TopRight, 10 }, std::move(snapshot));
}

static Framework::ModRegistrar<SongAccuracyMod> _songAccuracyReg;
