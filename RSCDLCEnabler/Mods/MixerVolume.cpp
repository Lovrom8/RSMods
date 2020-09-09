#include "MixerVolume.hpp"

float MixerVolume::SongVolume() { // Returns Song Volume
	uintptr_t volumeAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_songVolume, Offsets::ptr_songVolumeOffsets);

	if (!volumeAddr)
		return false;

	return *(float*)volumeAddr;
}

float MixerVolume::PlayerOneGuitarVolume() { // Returns Player One's Guitar Volume
	uintptr_t volumeAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_mixerVolumeBase, Offsets::ptr_playerOneGuitarOffsets);

	if (!volumeAddr)
		return false;

	return *(float*)volumeAddr;
}

float MixerVolume::PlayerOneBassVolume() { // Returns Player One's Bass Volume
	uintptr_t volumeAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_mixerVolumeBase, Offsets::ptr_playerOneBassOffsets);

	if (!volumeAddr)
		return false;

	return *(float*)volumeAddr;
}

float MixerVolume::PlayerTwoGuitarVolume() { // Returns Player Two's Guitar Volume
	uintptr_t volumeAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_mixerVolumeBase, Offsets::ptr_playerTwoGuitarOffsets);

	if (!volumeAddr)
		return false;

	return *(float*)volumeAddr;
}

float MixerVolume::PlayerTwoBassVolume() { // Returns Player Two's Bass Volume
	uintptr_t volumeAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_mixerVolumeBase, Offsets::ptr_playerTwoBassOffsets);

	if (!volumeAddr)
		return false;

	return *(float*)volumeAddr;
}

float MixerVolume::MicrophoneVolume() { // Returns Microphone Volume
	uintptr_t volumeAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_mixerVolumeBase, Offsets::ptr_micOffsets);

	if (!volumeAddr)
		return false;

	return *(float*)volumeAddr;
}

float MixerVolume::VoiceOverVolume() { // Returns VoiceOver Volume
	uintptr_t volumeAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_mixerVolumeBase, Offsets::ptr_voOffsets);

	if (!volumeAddr)
		return false;

	return *(float*)volumeAddr;
}

float MixerVolume::SFXVolume() { // Returns SFX Volume
	uintptr_t volumeAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_mixerVolumeBase, Offsets::ptr_sfxOffsets);

	if (!volumeAddr)
		return false;

	return *(float*)volumeAddr;
}