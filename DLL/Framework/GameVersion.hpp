#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

// Sites use fixed-image VAs or module-relative RVAs; zero disables an unmapped site.
namespace Framework::GameVersion {
	enum class Build {
		Unknown,
		September2022,
		LearnAndPlayDec2024,
	};

	const char* BuildName(Build build);

	// fileChecksum is computed from the file on disk, not the PE checksum field.
	struct ImageIdentity {
		bool headersValid = false;
		std::uint32_t timeDateStamp = 0;
		std::uint32_t sizeOfImage = 0;
		std::uint32_t preferredImageBase = 0;
		std::uint32_t headerChecksum = 0;
		std::uint32_t fileChecksum = 0;
	};

	inline constexpr std::uint32_t kSeptember2022FileChecksum = 0x00B13D7C;
	inline constexpr std::uint32_t kSeptember2022ImageBase = 0x00400000;
	inline constexpr std::uint32_t kLearnAndPlayTimeDateStamp = 0x67497D00;
	inline constexpr std::uint32_t kLearnAndPlaySizeOfImage = 0x0330C000;

	// Returns false and clears headersValid unless the image is PE32 i386.
	bool ParseHeaders(const std::uint8_t* image, std::size_t available, ImageIdentity& out);

	// Learn & Play uses timestamp and image size; the fixed-base build uses its file checksum.
	Build Identify(const ImageIdentity& identity);

	struct Site {
		const char* name;
		std::uintptr_t september2022Va;
		std::uintptr_t learnAndPlayRva;
	};

	// Returns zero for an unknown build or an unmapped site.
	std::uintptr_t Resolve(const Site& site, Build build, std::uintptr_t moduleBase);

	enum class CheckResult {
		Ok,
		UnknownBuild,
		Unmapped,
		Unreadable,
		Mismatch,
	};

	const char* CheckResultName(CheckResult result);

	// Returns false if the requested memory cannot be read.
	using ReadMemory = std::function<bool(std::uintptr_t address, void* buffer, std::size_t size)>;

	// Mask bytes: 0xFF compares, 0x00 skips relocated or rel32 operands.
	// address receives the resolved site, or zero if unmapped.
	CheckResult CheckSite(const Site& site, Build build, std::uintptr_t moduleBase, const std::uint8_t* expected,
		const std::uint8_t* mask, std::size_t size, const ReadMemory& read, std::uintptr_t& address);

	std::string DescribeOff(const char* owner, const Site& site, Build build, CheckResult result, std::uintptr_t address);
}
