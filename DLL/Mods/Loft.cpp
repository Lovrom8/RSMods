#include "../stdafx.h"
#include "Loft.hpp"

namespace {
	bool s_toggleOffLoftWhenDoneWithMod = false;
}

namespace Loft {
	/// <summary>
	/// Turn the background / "map" on or off.
	/// </summary>
	void ToggleLoft() {
		uintptr_t farAddr = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_loft, Offsets::ptr_loft_farOffsets);

		if (!farAddr) {
			LOG_ERROR("Invalid Pointer: ToggleLoft()" << std::endl);
			return;
		}

		if (*(float*)farAddr == 10000)
			*(float*)farAddr = 1; // Loft Off
		else
			*(float*)farAddr = 10000; // Loft On
	}

	/// <summary>
	/// Shake the camera around randomly.
	/// </summary>
	/// <param name="enable"> - Should we turn it on, or off?</param>
	void ToggleDrunkMode(bool enable) {
		uintptr_t noLoft = MemUtil::FindDMAAddy(Offsets::baseHandle + Offsets::ptr_loft, Offsets::ptr_loft_farOffsets);

		if (enable) {
			// Turn on loft so the effects of the mod are actually shown.
			if (*(float*)noLoft == 1) {
				s_toggleOffLoftWhenDoneWithMod = true;
				ToggleLoft();
			}
		}
		else {
			MemUtil::SetStaticValue(Offsets::ptr_drunkShit.Get(), 0.3333333333f, sizeof(float));

			// User originally had the loft off, but then we turned on this mod, so turn the loft back off.
			if (s_toggleOffLoftWhenDoneWithMod) {
				ToggleLoft();
				s_toggleOffLoftWhenDoneWithMod = false;
			}
		}
	}
}