#include "GameVersion.hpp"

#include <cstring>
#include <sstream>
#include <vector>

namespace Framework::GameVersion {
	namespace {
		std::uint16_t ReadU16(const std::uint8_t* p) {
			return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
		}

		std::uint32_t ReadU32(const std::uint8_t* p) {
			return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8)
				| (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
		}

		// PE32 header field offsets.
		constexpr std::size_t kDosLfanew = 0x3C;
		constexpr std::size_t kNtFileHeader = 4;
		constexpr std::size_t kNtOptionalHeader = 4 + 20;
		constexpr std::size_t kOptionalHeader32Size = 224;
		constexpr std::uint32_t kMaxLfanew = 0x100000;
	}

	const char* BuildName(Build build) {
		switch (build) {
		case Build::September2022: return "Sept 2022";
		case Build::LearnAndPlayDec2024: return "Learn & Play Dec 2024";
		default: return "unknown build";
		}
	}

	bool ParseHeaders(const std::uint8_t* image, std::size_t available, ImageIdentity& out) {
		out = ImageIdentity{};
		if (!image || available < 0x40 || ReadU16(image) != 0x5A4D)
			return false;

		const std::uint32_t lfanew = ReadU32(image + kDosLfanew);
		if (lfanew < 0x40 || lfanew > kMaxLfanew || lfanew + kNtOptionalHeader + kOptionalHeader32Size > available)
			return false;

		const std::uint8_t* nt = image + lfanew;
		if (ReadU32(nt) != 0x00004550)
			return false;

		const std::uint8_t* file = nt + kNtFileHeader;
		const std::uint16_t machine = ReadU16(file);
		const std::uint16_t optionalSize = ReadU16(file + 16);
		if (machine != 0x014C || optionalSize < kOptionalHeader32Size)
			return false;

		const std::uint8_t* optional = nt + kNtOptionalHeader;
		if (ReadU16(optional) != 0x010B)
			return false;

		out.timeDateStamp = ReadU32(file + 4);
		out.preferredImageBase = ReadU32(optional + 28);
		out.sizeOfImage = ReadU32(optional + 56);
		out.headerChecksum = ReadU32(optional + 64);
		out.headersValid = true;
		return true;
	}

	Build Identify(const ImageIdentity& identity) {
		if (!identity.headersValid)
			return Build::Unknown;
		if (identity.timeDateStamp == kLearnAndPlayTimeDateStamp && identity.sizeOfImage == kLearnAndPlaySizeOfImage)
			return Build::LearnAndPlayDec2024;
		if (identity.fileChecksum == kSeptember2022FileChecksum && identity.preferredImageBase == kSeptember2022ImageBase)
			return Build::September2022;
		return Build::Unknown;
	}

	std::uintptr_t Resolve(const Site& site, Build build, std::uintptr_t moduleBase) {
		switch (build) {
		case Build::September2022:
			return site.september2022Va;
		case Build::LearnAndPlayDec2024:
			if (site.learnAndPlayRva == 0 || moduleBase == 0)
				return 0;
			return moduleBase + site.learnAndPlayRva;
		default:
			return 0;
		}
	}

	const char* CheckResultName(CheckResult result) {
		switch (result) {
		case CheckResult::Ok: return "ok";
		case CheckResult::UnknownBuild: return "the executable is not a supported build";
		case CheckResult::Unmapped: return "the site is not mapped for this build";
		case CheckResult::Unreadable: return "the site cannot be read";
		case CheckResult::Mismatch: return "the bytes at the site do not match";
		default: return "unknown";
		}
	}

	CheckResult CheckSite(const Site& site, Build build, std::uintptr_t moduleBase, const std::uint8_t* expected,
		const std::uint8_t* mask, std::size_t size, const ReadMemory& read, std::uintptr_t& address) {
		address = 0;
		if (build == Build::Unknown)
			return CheckResult::UnknownBuild;

		address = Resolve(site, build, moduleBase);
		if (address == 0)
			return CheckResult::Unmapped;
		if (size == 0)
			return CheckResult::Ok;
		if (!expected || !read)
			return CheckResult::Unreadable;

		std::vector<std::uint8_t> actual(size);
		if (!read(address, actual.data(), size))
			return CheckResult::Unreadable;

		for (std::size_t i = 0; i < size; ++i) {
			const std::uint8_t m = mask ? mask[i] : 0xFF;
			if ((actual[i] & m) != (expected[i] & m))
				return CheckResult::Mismatch;
		}
		return CheckResult::Ok;
	}

	std::string DescribeOff(const char* owner, const Site& site, Build build, CheckResult result, std::uintptr_t address) {
		std::ostringstream line;
		line << "(" << (owner ? owner : "mod") << ") " << (site.name ? site.name : "site") << " off on " << BuildName(build)
			<< ": " << CheckResultName(result);
		if (address != 0)
			line << " (0x" << std::hex << std::uppercase << address << ")";
		return line.str();
	}
}
