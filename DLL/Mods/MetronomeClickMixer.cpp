#include "../stdafx.h"
#include "MetronomeClickMixer.hpp"

using Metronome::ClickMixer;

void ClickMixer::SetBeats(BeatMap newBeats) {
	beats.store(std::make_shared<const BeatMap>(std::move(newBeats)));
}

void ClickMixer::ClearBeats() {
	beats.store(nullptr);
}

void ClickMixer::SetLevels(ClickLevels levels) {
	accentLevel.store(levels.accent);
	beatLevel.store(levels.beat);
}

void ClickMixer::SetOffset(std::chrono::milliseconds offset) {
	offsetMs.store(static_cast<int>(offset.count()));
}

void ClickMixer::Mute() {
	muted.store(true);
}

void ClickMixer::Unmute() {
	muted.store(false);
}

bool ClickMixer::IsMuted() const {
	return muted.load();
}

// TODO: Nothing calls this yet. It needs a hook on Wwise's Vorbis file-source decoder output, as in
// RSModsPlus' DLL/Audio/SongShift/WwiseMusicHook.cpp, which hands over each decoded block of the song.
// Then: for every beat whose frame (seconds + offset) * sampleRate falls in [firstFrame, firstFrame + frameCount),
// add the accent or beat click (loaded from the configured WAV, or a built-in one) scaled by its level,
// saturating to int16. A click that starts near the end of a block continues into the next one.
void ClickMixer::MixInto(int16_t*, uint32_t, uint32_t, uint32_t) const {
}
