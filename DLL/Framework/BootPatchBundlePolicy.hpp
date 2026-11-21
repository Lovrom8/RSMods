#pragma once

#include <cstddef>

namespace Framework::BootPatch {
	enum class Bundle {
		SignIn,
		ProfileAutoLoad,
		UltrafastCore,
		DxvkFrameBudget,
		Count
	};

	enum class Member {
		SourceSignIn,
		ProfileCompletion,
		ProfileAutoLoad,
		DlcStorePrefetch,
		DlcOwnership,
		LazyVfs,
		NativeBegin,
		BootIntro,
		BootLoft,
		TitleFade,
		DxvkFrameBudget,
		Count
	};

	inline constexpr std::size_t BundleCount = static_cast<std::size_t>(Bundle::Count);

	constexpr Bundle BundleFor(Member member) {
		switch (member) {
		case Member::SourceSignIn:
		case Member::ProfileCompletion:
		case Member::BootLoft: return Bundle::SignIn;
		case Member::ProfileAutoLoad: return Bundle::ProfileAutoLoad;
		case Member::DxvkFrameBudget: return Bundle::DxvkFrameBudget;
		case Member::DlcStorePrefetch:
		case Member::DlcOwnership:
		case Member::LazyVfs:
		case Member::NativeBegin:
		case Member::BootIntro:
		case Member::TitleFade: return Bundle::UltrafastCore;
		case Member::Count: return Bundle::Count;
		}
		return Bundle::Count;
	}

	constexpr const char* MemberName(Member member) {
		switch (member) {
		case Member::SourceSignIn: return "UplayLogin source sign-in";
		case Member::ProfileCompletion: return "profile completion";
		case Member::ProfileAutoLoad: return "ProfileAutoLoad";
		case Member::DlcStorePrefetch: return "DLC store prefetch";
		case Member::DlcOwnership: return "DLC ownership";
		case Member::LazyVfs: return "LazyVfs";
		case Member::NativeBegin: return "NativeBegin";
		case Member::BootIntro: return "BootIntro";
		case Member::BootLoft: return "BootLoft";
		case Member::TitleFade: return "TitleFade";
		case Member::DxvkFrameBudget: return "DxvkFrameBudget";
		case Member::Count: return "unknown";
		}
		return "unknown";
	}

	constexpr const char* BundleName(Bundle bundle) {
		switch (bundle) {
		case Bundle::SignIn: return "source sign-in, profile completion and BootLoft";
		case Bundle::ProfileAutoLoad: return "profile autoload";
		case Bundle::UltrafastCore: return "ultrafast load core";
		case Bundle::DxvkFrameBudget: return "DXVK frame budget";
		case Bundle::Count: return "unknown";
		}
		return "unknown";
	}

	constexpr const char* DisabledFeatures(Bundle bundle) {
		switch (bundle) {
		case Bundle::SignIn: return "source sign-in, profile completion and BootLoft";
		case Bundle::ProfileAutoLoad: return "profile autoload";
		case Bundle::UltrafastCore:
			return "DLC store prefetch, DLC ownership, LazyVfs, NativeBegin, BootIntro, TitleFade";
		case Bundle::DxvkFrameBudget: return "DXVK frame budget";
		case Bundle::Count: return "unknown";
		}
		return "unknown";
	}
}
