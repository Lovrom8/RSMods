#include "../BootPatchBundlePolicy.hpp"

#include <cassert>
#include <string_view>

using Framework::BootPatch::Bundle;
using Framework::BootPatch::BundleFor;
using Framework::BootPatch::DisabledFeatures;
using Framework::BootPatch::Member;

int main() {
	assert(BundleFor(Member::SourceSignIn) == Bundle::SignIn);
	assert(BundleFor(Member::ProfileCompletion) == Bundle::SignIn);
	assert(BundleFor(Member::ProfileAutoLoad) == Bundle::ProfileAutoLoad);
	assert(BundleFor(Member::DlcStorePrefetch) == Bundle::UltrafastCore);
	assert(BundleFor(Member::DlcOwnership) == Bundle::UltrafastCore);
	assert(BundleFor(Member::LazyVfs) == Bundle::UltrafastCore);
	assert(BundleFor(Member::NativeBegin) == Bundle::UltrafastCore);
	assert(BundleFor(Member::BootIntro) == Bundle::BootIntro);
	assert(BundleFor(Member::BootLoft) == Bundle::BootLoft);
	assert(BundleFor(Member::TitleFade) == Bundle::TitleFade);
	assert(BundleFor(Member::DxvkFrameBudget) == Bundle::DxvkFrameBudget);

	assert(std::string_view(DisabledFeatures(Bundle::SignIn)) == "source sign-in and profile completion");
	assert(std::string_view(DisabledFeatures(Bundle::ProfileAutoLoad)) == "profile autoload");
	assert(std::string_view(DisabledFeatures(Bundle::UltrafastCore))
		== "DLC store prefetch, DLC ownership, LazyVfs, NativeBegin");
	assert(std::string_view(DisabledFeatures(Bundle::DxvkFrameBudget)) == "DXVK frame budget");
	assert(std::string_view(DisabledFeatures(Bundle::BootIntro)) == "boot intro");
	assert(std::string_view(DisabledFeatures(Bundle::BootLoft)) == "boot loft");
	assert(std::string_view(DisabledFeatures(Bundle::TitleFade)) == "title fade");
}
