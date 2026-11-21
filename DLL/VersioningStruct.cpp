#include "stdafx.h"
#include "VersioningStruct.h"
#include "Framework/GameVersion.hpp"

namespace {
	static DWORD GetChecksum() {
		DWORD headerSum = 0;
		DWORD checksum = 0;

		char executable[MAX_PATH];
		GetModuleFileNameA(NULL, executable, MAX_PATH);

		// Runs under the loader lock: imagehlp is an import, so this maps and sums the file without loading anything.
		MapFileAndCheckSumA(executable, &headerSum, &checksum);

		return checksum;
	}

}

namespace {
	Framework::GameVersion::ImageIdentity ReadModuleIdentity() {
		Framework::GameVersion::ImageIdentity identity{};
		const auto module = reinterpret_cast<const std::uint8_t*>(GetModuleHandle(nullptr));
		if (!module)
			return identity;

		// Read headers through ReadProcessMemory so invalid layouts fail without faulting.
		std::uint8_t headers[0x1000]{};
		SIZE_T bytesRead = 0;
		if (!ReadProcessMemory(GetCurrentProcess(), module, headers, sizeof(headers), &bytesRead))
			return identity;

		Framework::GameVersion::ParseHeaders(headers, bytesRead, identity);
		identity.fileChecksum = GetChecksum();
		return identity;
	}

	bool ReadGameMemory(std::uintptr_t address, void* buffer, std::size_t size) {
		SIZE_T bytesRead = 0;
		return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<LPCVOID>(address), buffer, size, &bytesRead) && bytesRead == size;
	}
}

namespace GameVersion {
	const Framework::GameVersion::ImageIdentity& Identity() {
		static const Framework::GameVersion::ImageIdentity identity = ReadModuleIdentity();
		return identity;
	}

	Framework::GameVersion::Build Current() {
		static const Framework::GameVersion::Build build = Framework::GameVersion::Identify(Identity());
		return build;
	}

	std::uintptr_t ModuleBase() {
		return reinterpret_cast<std::uintptr_t>(GetModuleHandle(nullptr));
	}

	std::uintptr_t Resolve(const Framework::GameVersion::Site& site) {
		return Framework::GameVersion::Resolve(site, Current(), ModuleBase());
	}

	std::uintptr_t CheckedSite(const char* owner, const Framework::GameVersion::Site& site, const std::uint8_t* expected,
		const std::uint8_t* mask, std::size_t size) {
		std::uintptr_t address = 0;
		const auto result = Framework::GameVersion::CheckSite(site, Current(), ModuleBase(), expected, mask, size, ReadGameMemory, address);
		if (result == Framework::GameVersion::CheckResult::Ok)
			return address;

		LOG_WARNING(Framework::GameVersion::DescribeOff(owner, site, Current(), result, address) << std::endl);
		return 0;
	}

	void LogIdentity() {
		const auto& id = Identity();
		LOG_INFO("(STARTUP) Executable: " << Framework::GameVersion::BuildName(Current()) << " (TimeDateStamp 0x" << std::hex
			<< id.timeDateStamp << ", SizeOfImage 0x" << id.sizeOfImage << ", header CheckSum 0x" << id.headerChecksum
			<< ", file checksum 0x" << id.fileChecksum << ", module base 0x" << ModuleBase() << std::dec << ")" << std::endl);
	}
}

template class VersioningStruct<unsigned int>;

template <typename T> T& VersioningStruct<T>::Get()
{
	const auto version = GetVersion();
	if (version == VersionType::Unknown)
	{
		unsupportedValue = T{};
		return unsupportedValue;
	}
	return myVersions.at(static_cast<int>(version));
}

template <typename T> T VersioningStruct<T>::GetValue()
{
	const auto version = GetVersion();
	return version == VersionType::Unknown ? T{} : myVersions.at(static_cast<int>(version));
}

template <typename T> VersionType VersioningStruct<T>::GetVersion()
{
	static bool cachedVersion = false;
	static VersionType version = VersionType::Unknown;

	if (cachedVersion)
	{
		return version;
	}

	const auto build = GameVersion::Current();
	if (build == Framework::GameVersion::Build::September2022)
	{
		version = VersionType::RemasteredSeptember2022;
	}
	else if (build == Framework::GameVersion::Build::LearnAndPlayDec2024)
	{
		version = VersionType::LPDecember2024;
	}
	else
	{
		version = VersionType::Unknown;
	}

	cachedVersion = true;
	return version;
}
