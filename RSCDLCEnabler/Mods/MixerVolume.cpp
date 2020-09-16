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

float MixerVolume::SelectVolume(std::string volumeToSelect) { // Does whatever cycle it needs to, to find the volume it needs. Use this for external functions.... I know this is inefficient, but the other ways I tried didn't work.
	switch (GetIndex(mixerInternalNames, volumeToSelect)) {
		case 0:
			return SongVolume();
		case 1:
			return PlayerOneGuitarVolume();
		case 2:
			return PlayerOneBassVolume();
		case 3:
			return PlayerTwoGuitarVolume();
		case 4:
			return PlayerTwoBassVolume();
		case 5:
			return MicrophoneVolume();
		case 6:
			return VoiceOverVolume();
		case 7:
			return SFXVolume();
		default:
			return 0.0f;
	}
}

// Courtesy of https://www.geeksforgeeks.org/how-to-find-index-of-a-given-element-in-a-vector-in-cpp/
int MixerVolume::GetIndex(std::vector<std::string> vector, std::string string)
{
	auto subString = std::find(vector.begin(),
		vector.end(), string);

	if (subString != vector.end())
		return std::distance(vector.begin(), subString);
	else
		return -1; // If the element is not present in the vector
}