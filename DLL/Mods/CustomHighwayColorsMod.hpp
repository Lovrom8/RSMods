#pragma once

#include <atomic>
#include <d3d9.h>
#include "../Framework/Framework.hpp"

class CustomHighwayColorsMod : public Framework::IMod {
public:
	MOD_ID(CustomHighwayColorsMod);

	bool IsEnabled(const Framework::ModContext& c) const override;
	void OnEnabled(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;
	void OnShutdown(Framework::ModContext& c) override;

	static void RegenerateTextures(IDirect3DDevice9* pDevice);
	static void ReleaseTextures();

	static LPDIRECT3DTEXTURE9 GetNotewayTexture() noexcept { return s_notewayTexture; }
	static LPDIRECT3DTEXTURE9 GetGutterTexture() noexcept { return s_gutterTexture; }
	static LPDIRECT3DTEXTURE9 GetFretNumTexture() noexcept { return s_fretNumTexture; }

private:
	static inline std::atomic<bool> s_active = false;
	static inline LPDIRECT3DTEXTURE9 s_notewayTexture = nullptr;
	static inline LPDIRECT3DTEXTURE9 s_gutterTexture = nullptr;
	static inline LPDIRECT3DTEXTURE9 s_fretNumTexture = nullptr;
};
