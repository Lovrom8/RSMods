#include "MetronomeWav.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <optional>

using Metronome::MonoSound;

namespace {
	constexpr uint16_t kFormatPcm = 1;
	constexpr uint16_t kFormatFloat = 3;
	constexpr uint16_t kFormatExtensible = 0xFFFE;
	constexpr size_t kChunkHeaderBytes = 8;
	constexpr size_t kMinFormatBytes = 16;
	constexpr size_t kExtensibleSubFormatOffset = 24;

	struct WavFormat {
		uint16_t tag = 0;
		uint16_t channels = 0;
		uint32_t sampleRate = 0;
		uint16_t bitsPerSample = 0;
	};

	template <typename T>
	T ReadLittleEndian(const std::vector<char>& bytes, size_t offset) {
		T value{};
		std::memcpy(&value, bytes.data() + offset, sizeof(T));
		return value;
	}

	// The sub-format's first field is the plain format tag (PCM or float).
	WavFormat ParseFormat(const std::vector<char>& bytes, size_t offset, size_t size) {
		WavFormat format{ ReadLittleEndian<uint16_t>(bytes, offset), ReadLittleEndian<uint16_t>(bytes, offset + 2),
			ReadLittleEndian<uint32_t>(bytes, offset + 4), ReadLittleEndian<uint16_t>(bytes, offset + 14) };
		if (format.tag == kFormatExtensible && size >= kExtensibleSubFormatOffset + 2)
			format.tag = ReadLittleEndian<uint16_t>(bytes, offset + kExtensibleSubFormatOffset);
		return format;
	}

	float DecodeSample(const char* sample, const WavFormat& format) {
		switch (format.bitsPerSample) {
		case 8: return (static_cast<uint8_t>(*sample) - 128) / 128.f; // 8-bit WAV is unsigned.
		case 16: { int16_t v; std::memcpy(&v, sample, 2); return v / 32768.f; }
		case 24: {
			const int32_t v = (static_cast<uint8_t>(sample[0]) << 8 | static_cast<uint8_t>(sample[1]) << 16 | static_cast<uint8_t>(sample[2]) << 24) >> 8;
			return v / 8388608.f;
		}
		case 32:
			if (format.tag == kFormatFloat) { float v; std::memcpy(&v, sample, 4); return v; }
			{ int32_t v; std::memcpy(&v, sample, 4); return static_cast<float>(v / 2147483648.0); }
		}
		return 0.f;
	}

	bool IsSupported(const WavFormat& format) {
		const bool pcm = format.tag == kFormatPcm && (format.bitsPerSample == 8 || format.bitsPerSample == 16 || format.bitsPerSample == 24 || format.bitsPerSample == 32);
		const bool floating = format.tag == kFormatFloat && format.bitsPerSample == 32;
		return (pcm || floating) && format.channels > 0 && format.sampleRate > 0;
	}

	MonoSound MixDown(const std::vector<char>& bytes, size_t dataOffset, size_t dataSize, const WavFormat& format) {
		const size_t bytesPerSample = format.bitsPerSample / 8;
		const size_t bytesPerFrame = bytesPerSample * format.channels;

		MonoSound sound{ std::vector<float>(dataSize / bytesPerFrame), format.sampleRate };
		for (size_t frame = 0; frame < sound.samples.size(); ++frame) {
			float sum = 0.f;
			for (uint16_t channel = 0; channel < format.channels; ++channel)
				sum += DecodeSample(bytes.data() + dataOffset + frame * bytesPerFrame + channel * bytesPerSample, format);
			sound.samples[frame] = sum / format.channels;
		}
		return sound;
	}
}

std::variant<MonoSound, std::string> Metronome::LoadWav(const std::filesystem::path& path) {
	std::ifstream file(path, std::ios::binary);
	if (!file) return std::string("file not found or not readable");
	const std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	if (bytes.size() < 12 || std::memcmp(bytes.data(), "RIFF", 4) != 0 || std::memcmp(bytes.data() + 8, "WAVE", 4) != 0)
		return std::string("not a WAV file");

	std::optional<WavFormat> format;
	for (size_t offset = 12; offset + kChunkHeaderBytes <= bytes.size();) {
		const uint32_t chunkSize = ReadLittleEndian<uint32_t>(bytes, offset + 4);
		const size_t body = offset + kChunkHeaderBytes;
		const size_t available = std::min<size_t>(chunkSize, bytes.size() - body);

		if (std::memcmp(bytes.data() + offset, "fmt ", 4) == 0 && available >= kMinFormatBytes)
			format = ParseFormat(bytes, body, available);
		else if (std::memcmp(bytes.data() + offset, "data", 4) == 0) {
			if (!format) return std::string("no format chunk before the audio");
			if (!IsSupported(*format)) return std::string("unsupported WAV encoding (use PCM or 32-bit float)");
			return MixDown(bytes, body, available, *format);
		}

		offset = body + chunkSize + (chunkSize & 1); // Chunks are padded to an even size.
	}
	return std::string("no audio data");
}
