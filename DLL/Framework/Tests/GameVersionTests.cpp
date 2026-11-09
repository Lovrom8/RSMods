#include "../GameVersion.hpp"

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

using namespace Framework::GameVersion;

namespace {
	int g_failures = 0;

	void Check(const std::string& name, bool ok) {
		if (ok) {
			std::cout << "[PASS] " << name << "\n";
		}
		else {
			std::cout << "[FAIL] " << name << "\n";
			++g_failures;
		}
	}

	void Put16(std::vector<std::uint8_t>& image, std::size_t at, std::uint16_t value) {
		image[at] = static_cast<std::uint8_t>(value);
		image[at + 1] = static_cast<std::uint8_t>(value >> 8);
	}

	void Put32(std::vector<std::uint8_t>& image, std::size_t at, std::uint32_t value) {
		for (int i = 0; i < 4; ++i)
			image[at + i] = static_cast<std::uint8_t>(value >> (8 * i));
	}

	std::vector<std::uint8_t> SyntheticImage(std::uint32_t timeDateStamp, std::uint32_t imageBase, std::uint32_t sizeOfImage) {
		std::vector<std::uint8_t> image(0x400, 0);
		const std::size_t lfanew = 0x80;
		Put16(image, 0, 0x5A4D);
		Put32(image, 0x3C, lfanew);
		Put32(image, lfanew, 0x00004550);
		Put16(image, lfanew + 4, 0x014C);
		Put32(image, lfanew + 4 + 4, timeDateStamp);
		Put16(image, lfanew + 4 + 16, 224);
		const std::size_t optional = lfanew + 24;
		Put16(image, optional, 0x010B);
		Put32(image, optional + 28, imageBase);
		Put32(image, optional + 56, sizeOfImage);
		Put32(image, optional + 64, 0x01234567);
		return image;
	}

	void IdentifiesLearnAndPlayFromHeaders() {
		auto image = SyntheticImage(kLearnAndPlayTimeDateStamp, 0x001D0000, kLearnAndPlaySizeOfImage);
		ImageIdentity id;
		Check("LP headers parse", ParseHeaders(image.data(), image.size(), id));
		Check("LP header checksum read", id.headerChecksum == 0x01234567);
		Check("LP identified", Identify(id) == Build::LearnAndPlayDec2024);

		// Executable identity must survive relocation.
		image = SyntheticImage(kLearnAndPlayTimeDateStamp, 0x00400000, kLearnAndPlaySizeOfImage);
		ParseHeaders(image.data(), image.size(), id);
		Check("LP identified at any base", Identify(id) == Build::LearnAndPlayDec2024);

		image = SyntheticImage(kLearnAndPlayTimeDateStamp, 0x001D0000, kLearnAndPlaySizeOfImage + 0x1000);
		ParseHeaders(image.data(), image.size(), id);
		Check("LP stamp with another size -> unknown", Identify(id) == Build::Unknown);
	}

	void IdentifiesSeptember2022() {
		auto image = SyntheticImage(0x12345678, kSeptember2022ImageBase, 0x04F80000);
		ImageIdentity id;
		ParseHeaders(image.data(), image.size(), id);
		id.fileChecksum = kSeptember2022FileChecksum;
		Check("2022 identified", Identify(id) == Build::September2022);

		id.preferredImageBase = 0x10000000;
		Check("2022 checksum at another base -> unknown", Identify(id) == Build::Unknown);

		id.preferredImageBase = kSeptember2022ImageBase;
		id.fileChecksum = 0x0176EC34;
		Check("other checksum -> unknown", Identify(id) == Build::Unknown);
	}

	void RejectsMalformedHeaders() {
		ImageIdentity id;
		Check("null image rejected", !ParseHeaders(nullptr, 0, id));
		Check("invalid headers -> unknown", Identify(id) == Build::Unknown);

		auto image = SyntheticImage(kLearnAndPlayTimeDateStamp, 0, kLearnAndPlaySizeOfImage);
		image[0] = 'X';
		Check("bad MZ rejected", !ParseHeaders(image.data(), image.size(), id));

		image = SyntheticImage(kLearnAndPlayTimeDateStamp, 0, kLearnAndPlaySizeOfImage);
		Put16(image, 0x80 + 4, 0x8664);
		Check("x64 machine rejected", !ParseHeaders(image.data(), image.size(), id));

		image = SyntheticImage(kLearnAndPlayTimeDateStamp, 0, kLearnAndPlaySizeOfImage);
		Check("truncated buffer rejected", !ParseHeaders(image.data(), 0x100, id));
	}

	void ResolvesSites() {
		const Site both{ "both", 0x00551234, 0x00152544 };
		const Site lpUnmapped{ "2022 only", 0x005A5768, 0 };
		Check("2022 resolves to VA", Resolve(both, Build::September2022, 0x00400000) == 0x00551234);
		Check("2022 ignores module base", Resolve(both, Build::September2022, 0x12340000) == 0x00551234);
		Check("LP resolves base + RVA", Resolve(both, Build::LearnAndPlayDec2024, 0x001D0000) == 0x00322544);
		Check("LP unmapped -> 0", Resolve(lpUnmapped, Build::LearnAndPlayDec2024, 0x001D0000) == 0);
		Check("unknown build -> 0", Resolve(both, Build::Unknown, 0x00400000) == 0);
	}

	void ChecksSiteBytes() {
		std::vector<std::uint8_t> memory(0x100, 0x90);
		const std::uintptr_t base = 0x1000;
		const std::uint8_t code[] = { 0x55, 0x8B, 0xEC, 0xE8, 0x11, 0x22, 0x33, 0x44 };
		std::memcpy(memory.data() + 0x40, code, sizeof(code));

		const ReadMemory read = [&](std::uintptr_t address, void* buffer, std::size_t size) {
			if (address < base || address + size > base + memory.size())
				return false;
			std::memcpy(buffer, memory.data() + (address - base), size);
			return true;
		};

		const Site site{ "test site", 0, 0x40 };
		std::uintptr_t address = 0;
		Check("matching bytes -> ok",
			CheckSite(site, Build::LearnAndPlayDec2024, base, code, nullptr, sizeof(code), read, address) == CheckResult::Ok
			&& address == base + 0x40);

		const std::uint8_t relocated[] = { 0x55, 0x8B, 0xEC, 0xE8, 0xAA, 0xBB, 0xCC, 0xDD };
		Check("rel32 differs -> mismatch",
			CheckSite(site, Build::LearnAndPlayDec2024, base, relocated, nullptr, sizeof(relocated), read, address) == CheckResult::Mismatch);

		const std::uint8_t mask[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0, 0, 0, 0 };
		Check("masked rel32 -> ok",
			CheckSite(site, Build::LearnAndPlayDec2024, base, relocated, mask, sizeof(relocated), read, address) == CheckResult::Ok);

		Check("2022 column unmapped -> unmapped",
			CheckSite(site, Build::September2022, base, code, nullptr, sizeof(code), read, address) == CheckResult::Unmapped
			&& address == 0);

		Check("unknown build -> fail closed",
			CheckSite(site, Build::Unknown, base, code, nullptr, sizeof(code), read, address) == CheckResult::UnknownBuild);

		const Site outside{ "outside", 0, 0x4000 };
		Check("unreadable -> unreadable",
			CheckSite(outside, Build::LearnAndPlayDec2024, base, code, nullptr, sizeof(code), read, address) == CheckResult::Unreadable);

		const std::string line = DescribeOff("Owner", site, Build::LearnAndPlayDec2024, CheckResult::Mismatch, 0x1040);
		Check("off line names owner, site, build and reason",
			line.find("(Owner)") != std::string::npos && line.find("test site") != std::string::npos
			&& line.find("Learn & Play") != std::string::npos && line.find("do not match") != std::string::npos
			&& line.find("0x1040") != std::string::npos);
	}
}

int main() {
	IdentifiesLearnAndPlayFromHeaders();
	IdentifiesSeptember2022();
	RejectsMalformedHeaders();
	ResolvesSites();
	ChecksSiteBytes();

	if (g_failures != 0) {
		std::cout << g_failures << " GameVersion test(s) failed\n";
		return 1;
	}
	std::cout << "All GameVersion tests passed\n";
	return 0;
}
