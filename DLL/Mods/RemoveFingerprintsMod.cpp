#include "../stdafx.h"
#include "RemoveFingerprintsMod.hpp"
#include "../D3D/D3D.hpp"
#include "../D3D/D3DHelper.hpp"

using Framework::ModContext;
using Framework::DrawContext;
using Framework::DrawResult;
using Framework::DrawOutcome;
using Framework::DrawPath;
namespace Setting = Settings::Setting;

bool RemoveFingerprintsMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::RemoveFingerprints);
}

void RemoveFingerprintsMod::OnInitialize(ModContext& c) {
	c.Draw().Register("RemoveFingerprints", 0, DrawPath::Indexed, [](DrawContext& ctx) -> DrawResult {
		if (IsExtraRemoved(fingerprintMeshes, ctx.thicc)) {
			for (DWORD stage = 0; stage < 2; ++stage) {
				auto crc = ctx.StageCRC(stage);
				if (crc && (*crc == crcFingerprintNumber || *crc == crcFingerprintIcon)) {
					return { DrawOutcome::Hide };
				}
			}
		}
		return { DrawOutcome::Pass };
	});
}

static Framework::ModRegistrar<RemoveFingerprintsMod> _removeFingerprintsReg;
