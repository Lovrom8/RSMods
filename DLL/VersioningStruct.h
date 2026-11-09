#pragma once

#include "Framework/GameVersion.hpp"

enum class VersionType : int
{
	Start,
	RemasteredSeptember2022 = Start,
	LPDecember2024,
	Count,
	End = Count,
};

namespace GameVersion {
	const Framework::GameVersion::ImageIdentity& Identity();
	Framework::GameVersion::Build Current();
	std::uintptr_t ModuleBase();

	inline bool IsSeptember2022() { return Current() == Framework::GameVersion::Build::September2022; }
	inline bool IsLearnAndPlay() { return Current() == Framework::GameVersion::Build::LearnAndPlayDec2024; }

	// Zero indicates an unknown build or unmapped site.
	std::uintptr_t Resolve(const Framework::GameVersion::Site& site);

	// Return zero on byte mismatch after logging the site, build and reason.
	std::uintptr_t CheckedSite(const char* owner, const Framework::GameVersion::Site& site, const std::uint8_t* expected,
		const std::uint8_t* mask, std::size_t size);

	void LogIdentity();
}

template <typename T>
class VersioningStruct
{
public:
	VersioningStruct();
	~VersioningStruct() = default;

	VersioningStruct(const std::vector<T>& versions);

	operator T();

public:
	T& Get();
	T GetValue();

private:
	static VersionType GetVersion();

private:
	std::vector<T> myVersions;
};

template <typename T> VersioningStruct<T>::VersioningStruct()
	: myVersions(std::vector<T>(static_cast<int>(VersionType::Count), 0))
{
}

template <typename T> VersioningStruct<T>::VersioningStruct(const std::vector<T>& versions)
	: myVersions(versions)
{
	if (myVersions.size() != static_cast<size_t>(VersionType::Count))
	{
		myVersions.resize(static_cast<int>(VersionType::Count), NULL);
	}
}

template <typename T> VersioningStruct<T>::operator T()
{
	return Get();
}
