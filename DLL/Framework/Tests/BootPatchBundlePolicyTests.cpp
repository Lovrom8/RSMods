#include "../BootPatchBundlePolicy.hpp"

#include <cassert>
#include <string_view>

using Framework::BootPatch::Bundle;
using Framework::BootPatch::BundleFor;
using Framework::BootPatch::DisabledFeatures;
using Framework::BootPatch::Member;

int main() {
	assert(BundleFor(Member::SourceSignIn) == Bundle::SignIn);
	assert(BundleFor(Member::ProfileAutoLoad) == Bundle::ProfileAutoLoad);
	assert(BundleFor(Member::DlcStorePrefetch) == Bundle::UltrafastCore);
	assert(BundleFor(Member::DlcOwnership) == Bundle::UltrafastCore);
	assert(BundleFor(Member::LazyVfs) == Bundle::UltrafastCore);
	assert(BundleFor(Member::NativeBegin) == Bundle::UltrafastCore);
	assert(BundleFor(Member::BootIntro) == Bundle::UltrafastCore);
	assert(BundleFor(Member::BootLoft) == Bundle::UltrafastCore);
	assert(BundleFor(Member::TitleFade) == Bundle::UltrafastCore);
	assert(BundleFor(Member::DxvkFrameBudget) == Bundle::DxvkFrameBudget);

	assert(std::string_view(DisabledFeatures(Bundle::SignIn)) == "source sign-in");
	assert(std::string_view(DisabledFeatures(Bundle::ProfileAutoLoad)) == "profile autoload");
	assert(std::string_view(DisabledFeatures(Bundle::UltrafastCore))
		== "DLC store prefetch, DLC ownership, LazyVfs, NativeBegin, BootIntro, BootLoft, TitleFade");
	assert(std::string_view(DisabledFeatures(Bundle::DxvkFrameBudget)) == "DXVK frame budget");
}
