#include "../stdafx.h"
#include "CustomHighwayColorsMod.hpp"
#include "../D3D/D3D.hpp"
#include "../D3D/D3DHelper.hpp"
#include "../D3D/D3DHooks.hpp"
#include "../Settings.hpp"
#include "../Framework/Framework.hpp"
#include "DrawMeshTags.hpp"

struct HighwayTexturePack {
	LPDIRECT3DTEXTURE9 noteway = nullptr;
	LPDIRECT3DTEXTURE9 gutter = nullptr;
	LPDIRECT3DTEXTURE9 fretNum = nullptr;

	~HighwayTexturePack() {
		D3D::ReleaseTexture(&noteway);
		D3D::ReleaseTexture(&gutter);
		D3D::ReleaseTexture(&fretNum);
	}
};

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

void CustomHighwayColorsMod::UpdateColorPresence() {
	s_hasNotewayColors.store(
		!Settings::ReturnNotewayColor("CustomHighwayNumbered").empty() &&
		!Settings::ReturnNotewayColor("CustomHighwayUnNumbered").empty(),
		std::memory_order_relaxed
	);
	s_hasGutterColor.store(
		!Settings::ReturnNotewayColor("CustomHighwayGutter").empty(),
		std::memory_order_relaxed
	);
	s_hasFretNumColor.store(
		!Settings::ReturnNotewayColor("CustomFretNubmers").empty(),
		std::memory_order_relaxed
	);
}

LPDIRECT3DTEXTURE9 CustomHighwayColorsMod::GetNotewayTexture() noexcept {
	auto pack = s_textures.load(std::memory_order_acquire);
	return pack ? pack->noteway : nullptr;
}

LPDIRECT3DTEXTURE9 CustomHighwayColorsMod::GetGutterTexture() noexcept {
	auto pack = s_textures.load(std::memory_order_acquire);
	return pack ? pack->gutter : nullptr;
}

LPDIRECT3DTEXTURE9 CustomHighwayColorsMod::GetFretNumTexture() noexcept {
	auto pack = s_textures.load(std::memory_order_acquire);
	return pack ? pack->fretNum : nullptr;
}

void CustomHighwayColorsMod::OnInitialize(Framework::ModContext& c) {
	c.Draw().Register("CustomHighwayColors", -10, Framework::DrawPath::Indexed, [](Framework::DrawContext& ctx) -> Framework::DrawResult {
		if (IsToBeRemoved(noteHighway, ctx.mesh)) {
			auto pack = s_textures.load(std::memory_order_acquire);
			if (!pack) return { Framework::DrawOutcome::Pass };

			auto crc = ctx.StageCRC(1);
			if (crc) {
				if (*crc == D3D::Crc::NoteLanes && s_hasNotewayColors.load(std::memory_order_relaxed) && pack->noteway) {
					return { Framework::DrawOutcome::ReplaceTexture, 1, pack->noteway };
				}
				if (*crc == D3D::Crc::NotewayFretNumbers && s_hasFretNumColor.load(std::memory_order_relaxed) && pack->fretNum) {
					return { Framework::DrawOutcome::ReplaceTexture, 1, pack->fretNum };
				}
				if (*crc == D3D::Crc::NotewayGutters && s_hasGutterColor.load(std::memory_order_relaxed) && pack->gutter) {
					return { Framework::DrawOutcome::ReplaceTexture, 1, pack->gutter };
				}
			}
		}
		return { Framework::DrawOutcome::Pass };
	});

	c.Draw().RegisterTextureLifecycle(&CustomHighwayColorsMod::RegenerateTextures, &CustomHighwayColorsMod::ReleaseTextures);
}

void CustomHighwayColorsMod::OnEnabled(Framework::ModContext& c) {
	s_active = true;
	c.Draw().CancelTextureRelease();
	UpdateColorPresence();
	D3DHooks::RecreateTextures = true;
}

void CustomHighwayColorsMod::OnDisabled(Framework::ModContext& c) {
	s_active = false;
	c.Draw().RequestTextureRelease();
}

void CustomHighwayColorsMod::OnSettingsChanged(Framework::ModContext&) {
	UpdateColorPresence();
	D3DHooks::RecreateTextures = true;
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

	UpdateColorPresence();

	ColorList colorSet = {
		Settings::ConvertHexToColor(Settings::ReturnNotewayColor("CustomHighwayNumbered")),
		Settings::ConvertHexToColor(Settings::ReturnNotewayColor("CustomHighwayUnNumbered"))
	};

	auto newPack = std::make_shared<HighwayTexturePack>();
	D3D::GenerateGradientTexture(pDevice, &newPack->noteway, colorSet, 256, 32, 16, 2);
	GenerateSingleColorTexture(pDevice, &newPack->gutter, "CustomHighwayGutter", 256, 16, 16, 1);
	GenerateSingleColorTexture(pDevice, &newPack->fretNum, "CustomFretNubmers", 256, 16, 16, 1);

	s_textures.store(std::move(newPack), std::memory_order_release);
}

void CustomHighwayColorsMod::ReleaseTextures() {
	s_textures.store(nullptr, std::memory_order_release);
}
