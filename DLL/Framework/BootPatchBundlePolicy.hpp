#pragma once

#include <cstddef>

namespace Framework::BootPatch {
	enum class Bundle {
		SignIn,
		ProfileAutoLoad,
		UltrafastCore,
		DxvkFrameBudget,
		BootIntro,
		BootLoft,
		TitleFade,
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

	// Sign-in and the accelerated-load members are dependency bundles. Cosmetic or
	// flow-only members have independent bundles so their failure cannot disable the core.
	constexpr Bundle BundleFor(Member member) {
		switch (member) {
		case Member::SourceSignIn:
		case Member::ProfileCompletion: return Bundle::SignIn;
		case Member::BootLoft: return Bundle::BootLoft;
		case Member::ProfileAutoLoad: return Bundle::ProfileAutoLoad;
		case Member::DxvkFrameBudget: return Bundle::DxvkFrameBudget;
		case Member::DlcStorePrefetch:
		case Member::DlcOwnership:
		case Member::LazyVfs:
		case Member::NativeBegin: return Bundle::UltrafastCore;
		case Member::BootIntro: return Bundle::BootIntro;
		case Member::TitleFade: return Bundle::TitleFade;
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
		case Bundle::SignIn: return "source sign-in and profile completion";
		case Bundle::ProfileAutoLoad: return "profile autoload";
		case Bundle::UltrafastCore: return "ultrafast load core";
		case Bundle::DxvkFrameBudget: return "DXVK frame budget";
		case Bundle::BootIntro: return "boot intro";
		case Bundle::BootLoft: return "boot loft";
		case Bundle::TitleFade: return "title fade";
		case Bundle::Count: return "unknown";
		}
		return "unknown";
	}

	constexpr const char* DisabledFeatures(Bundle bundle) {
		switch (bundle) {
		case Bundle::SignIn: return "source sign-in and profile completion";
		case Bundle::ProfileAutoLoad: return "profile autoload";
		case Bundle::UltrafastCore:
			return "DLC store prefetch, DLC ownership, LazyVfs, NativeBegin";
		case Bundle::DxvkFrameBudget: return "DXVK frame budget";
		case Bundle::BootIntro: return "boot intro";
		case Bundle::BootLoft: return "boot loft";
		case Bundle::TitleFade: return "title fade";
		case Bundle::Count: return "unknown";
		}
		return "unknown";
	}
}
