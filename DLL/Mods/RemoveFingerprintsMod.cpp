#include "../stdafx.h"
#include "RemoveFingerprintsMod.hpp"
#include "../D3D/D3D.hpp"
#include "../D3D/D3DHelper.hpp"
#include "DrawMeshTags.hpp"

using Framework::ModContext;
using Framework::DrawContext;
using Framework::DrawResult;
using Framework::DrawOutcome;
using Framework::DrawPath;
using Framework::SettingDef;
using Framework::SettingDefs;
namespace Setting = Settings::Setting;

SettingDefs RemoveFingerprintsMod::Settings() const {
	return {
		SettingDef::Toggle(Setting::RemoveFingerprints, "RemoveFingerprints", "Remove Fingerprints")
	};
}

bool RemoveFingerprintsMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(Setting::RemoveFingerprints);
}

void RemoveFingerprintsMod::OnInitialize(ModContext& c) {
	c.Draw().Register("RemoveFingerprints", 0, DrawPath::Indexed, [](DrawContext& ctx) -> DrawResult {
		if (IsExtraRemoved(fingerprintMeshes, ctx.thicc)) {
			for (DWORD stage = 0; stage < 2; ++stage) {
				if (DrawMesh::StageMatchesAny(ctx, stage, { D3D::Crc::FingerprintNumber, D3D::Crc::FingerprintIcon })) {
					return { DrawOutcome::Hide };
				}
			}
		}
		return { DrawOutcome::Pass };
	});
}

static Framework::ModRegistrar<RemoveFingerprintsMod> _removeFingerprintsReg;
