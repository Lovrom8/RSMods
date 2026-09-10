#include "../stdafx.h"
#include "CustomHighwayColorsMod.hpp"
#include "../D3D/D3D.hpp"
#include "../D3D/D3DHooks.hpp"
#include "../Settings.hpp"
#include "../Framework/Framework.hpp"

namespace {
	Framework::ModRegistrar<CustomHighwayColorsMod> _customHighwayColorsReg;

	void GenerateSingleColorTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppTexture, const std::string& colorKey, UINT width, UINT height, int lineHeight, int lines) {
		ColorList colorSet = { Settings::ConvertHexToColor(Settings::ReturnNotewayColor(colorKey)) };
		D3D::GenerateGradientTexture(pDevice, ppTexture, colorSet, width, height, lineHeight, lines);
	}
}

bool CustomHighwayColorsMod::IsEnabled(const Framework::ModContext& c) const {
	return c.IsOn(Settings::Setting::CustomHighwayColors);
}

void CustomHighwayColorsMod::OnEnabled(Framework::ModContext&) {
	s_active = true;
	D3DHooks::RecreateTextures = true;
}

void CustomHighwayColorsMod::OnDisabled(Framework::ModContext&) {
	s_active = false;
	ReleaseTextures();
}

void CustomHighwayColorsMod::OnShutdown(Framework::ModContext&) {
	s_active = false;
	ReleaseTextures();
}

void CustomHighwayColorsMod::RegenerateTextures(IDirect3DDevice9* pDevice) {
	if (!pDevice) return;

	if (!s_active || !Settings::IsOn(Settings::Setting::CustomHighwayColors)) {
		ReleaseTextures();
		return;
	}

	ColorList colorSet = {
		Settings::ConvertHexToColor(Settings::ReturnNotewayColor("CustomHighwayNumbered")),
		Settings::ConvertHexToColor(Settings::ReturnNotewayColor("CustomHighwayUnNumbered"))
	};
	D3D::GenerateGradientTexture(pDevice, &s_notewayTexture, colorSet, 256, 32, 16, 2);

	GenerateSingleColorTexture(pDevice, &s_gutterTexture, "CustomHighwayGutter", 256, 16, 16, 1);
	GenerateSingleColorTexture(pDevice, &s_fretNumTexture, "CustomFretNubmers", 256, 16, 16, 1);
}

void CustomHighwayColorsMod::ReleaseTextures() {
	D3D::ReleaseTexture(&s_notewayTexture);
	D3D::ReleaseTexture(&s_gutterTexture);
	D3D::ReleaseTexture(&s_fretNumTexture);
}
