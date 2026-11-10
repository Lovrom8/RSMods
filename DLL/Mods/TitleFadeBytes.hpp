#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstddef>
#include <span>

namespace GfxDurationBytes {
	constexpr uint32_t kActionSize = 16;
	constexpr uint32_t kDurationSize = 8;
	constexpr unsigned int kDurationFullMask = (1u << kDurationSize) - 1;

	struct Site {
		uint32_t fileSize;
		uint32_t actionOffset;
		std::array<unsigned char, kActionSize> action;
		std::array<unsigned char, kDurationSize> duration;

		constexpr uint32_t DurationOffset() const { return actionOffset + 4; }
	};

	inline unsigned int DurationMask(const Site& site, uint32_t offset, size_t size) {
		unsigned int mask = 0;
		for (uint32_t i = 0; i < kDurationSize; ++i) {
			const uint64_t at = static_cast<uint64_t>(site.DurationOffset()) + i;
			if (at >= offset && at < static_cast<uint64_t>(offset) + size)
				mask |= 1u << i;
		}
		return mask;
	}

	inline bool PatchFragment(const Site& site, std::span<unsigned char> bytes, uint32_t offset,
		std::span<const unsigned char> sourceAction) {
		if (sourceAction.size() != site.action.size()
			|| !std::equal(site.action.begin(), site.action.end(), sourceAction.begin()))
			return false;
		const uint64_t first = (std::max)(static_cast<uint64_t>(offset),
			static_cast<uint64_t>(site.DurationOffset()));
		const uint64_t last = (std::min)(static_cast<uint64_t>(offset) + bytes.size(),
			static_cast<uint64_t>(site.DurationOffset() + kDurationSize));
		if (first >= last)
			return false;
		for (uint64_t at = first; at < last; ++at)
			bytes[static_cast<size_t>(at - offset)] = site.duration[static_cast<size_t>(at - site.DurationOffset())];
		return true;
	}
}

namespace TitleFadeBytes {
	constexpr GfxDurationBytes::Site kSite = {
		79861, 0x12254,
		{ 0x96, 0x0D, 0x00, 0x06, 0x51, 0xB8, 0x02, 0x40, 0xB8, 0x1E, 0x85, 0xEB, 0x04, 0x01, 0x08, 0x1A },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	};
	constexpr uint32_t kFileSize = kSite.fileSize;
	constexpr uint32_t kActionOffset = kSite.actionOffset;
	constexpr uint32_t kDurationOffset = kSite.DurationOffset();
	constexpr uint32_t kDurationSize = GfxDurationBytes::kDurationSize;
	constexpr std::array<unsigned char, 16> kAction = kSite.action;
	constexpr unsigned int kDurationFullMask = GfxDurationBytes::kDurationFullMask;

	inline unsigned int DurationMask(uint32_t offset, size_t size) {
		return GfxDurationBytes::DurationMask(kSite, offset, size);
	}

	inline bool PatchFragment(std::span<unsigned char> bytes, uint32_t offset,
		std::span<const unsigned char> sourceAction) {
		return GfxDurationBytes::PatchFragment(kSite, bytes, offset, sourceAction);
	}
}

namespace TitleOutBytes {
	constexpr GfxDurationBytes::Site kSite = {
		79861, 0x1245F,
		{ 0x43, 0x96, 0x0E, 0x00, 0x07, 0x01, 0x00, 0x00, 0x00, 0x04, 0x05, 0x07, 0x03, 0x00, 0x00, 0x00 },
		{ 0x01, 0x6F, 0x12, 0x83, 0x3A, 0x04, 0x05, 0x07 }
	};
}

namespace MainMenuFadeBytes {
	constexpr GfxDurationBytes::Site kSite = {
		78794, 0x12608,
		{ 0x96, 0x0D, 0x00, 0x06, 0x99, 0x99, 0xC9, 0x3F, 0x9A, 0x99, 0x99, 0x99, 0x04, 0x02, 0x08, 0x0C },
		{ 0x4D, 0x62, 0x50, 0x3F, 0xFC, 0xA9, 0xF1, 0xD2 }
	};
}
