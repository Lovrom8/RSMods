#pragma once

namespace Audio
{
	enum class SampleFormat
	{
		Unsupported,
		Float32,
		Int32,
		Int24,
		Int16
	};

	struct CaptureFormat
	{
		SampleFormat sampleFormat = SampleFormat::Unsupported;
		uint32_t sampleRate = 0;
		uint32_t channelCount = 0;

		bool IsUsable() const
		{
			return sampleFormat != SampleFormat::Unsupported && sampleRate > 0 && channelCount > 0;
		}
	};

	std::string DescribeFormat(const CaptureFormat& format);
}
