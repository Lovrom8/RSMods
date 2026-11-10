#include "../../Mods/TitleFadeBytes.hpp"
#include <cassert>
#include <iostream>

int main() {
    for (const auto* site : { &TitleFadeBytes::kSite, &TitleOutBytes::kSite, &MainMenuFadeBytes::kSite }) {
        auto action = site->action;
        for (size_t i = 0; i < action.size(); ++i) {
            auto changed = site->action;
            changed[i] ^= 1;
            auto buffer = site->action;
            assert(!GfxDurationBytes::PatchFragment(*site, buffer, site->actionOffset, changed));
            assert(buffer == site->action);
        }
        unsigned int mask = 0;
        for (uint32_t i = 0; i < GfxDurationBytes::kDurationSize; ++i) {
            unsigned char fragment = site->action[4 + i];
            assert(GfxDurationBytes::PatchFragment(*site, { &fragment, 1 }, site->DurationOffset() + i, action));
            assert(fragment == site->duration[i]);
            mask |= GfxDurationBytes::DurationMask(*site, site->DurationOffset() + i, 1);
        }
        assert(mask == GfxDurationBytes::kDurationFullMask);
        auto buffer = action;
        assert(GfxDurationBytes::PatchFragment(*site, buffer, site->actionOffset, action));
        for (size_t i = 0; i < buffer.size(); ++i)
            assert(buffer[i] == (i >= 4 && i < 12 ? site->duration[i - 4] : action[i]));
    }
    std::cout << "Title fade byte checks ALL PASS\n";
}
