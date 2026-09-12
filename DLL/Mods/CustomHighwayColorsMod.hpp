#pragma once

#include <atomic>
#include <memory>
#include <d3d9.h>
#include "../Framework/Framework.hpp"

struct HighwayTexturePack;

class CustomHighwayColorsMod : public Framework::IMod {
public:
	MOD_ID(CustomHighwayColorsMod);

	bool IsEnabled(const Framework::ModContext& c) const override;
	Framework::SettingDefs Settings() const override;
	void OnInitialize(Framework::ModContext& c) override;
	void OnEnabled(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;
	void OnSettingsChanged(Framework::ModContext& c) override;
	void OnShutdown(Framework::ModContext& c) override;

	static void RegenerateTextures(IDirect3DDevice9* pDevice);
	static void ReleaseTextures();
	static void UpdateColorPresence();

	static LPDIRECT3DTEXTURE9 GetNotewayTexture() noexcept;
	static LPDIRECT3DTEXTURE9 GetGutterTexture() noexcept;
	static LPDIRECT3DTEXTURE9 GetFretNumTexture() noexcept;

private:
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

	static void GenerateSingleColorTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppTexture, const std::string& colorKey, UINT width, UINT height, int lineHeight, int lines);

	static inline std::atomic<bool> s_active = false;
	static inline std::atomic<bool> s_hasNotewayColors = false;
	static inline std::atomic<bool> s_hasGutterColor = false;
	static inline std::atomic<bool> s_hasFretNumColor = false;
	static inline std::atomic<std::shared_ptr<const HighwayTexturePack>> s_textures;
};
