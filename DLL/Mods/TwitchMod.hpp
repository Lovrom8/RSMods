#pragma once

#include <atomic>
#include <d3d9.h>
#include <vector>

#include "../Framework/Framework.hpp"
#include "../RSColor.h"

class TwitchMod : public Framework::IMod {
public:
	MOD_ID(TwitchMod)

	void OnInitialize(Framework::ModContext& c) override;
	void OnSongTick(Framework::ModContext& c) override;
	void OnDisabled(Framework::ModContext& c) override;
	void OnSettingsChanged(Framework::ModContext& c) override;
	void OnShutdown(Framework::ModContext& c) override;

	static void RegenerateTextures(IDirect3DDevice9* pDevice);
	static void GenerateRandomTextures(IDirect3DDevice9* pDevice);
	static void RegenerateUserDefinedTexture(IDirect3DDevice9* pDevice);
	static void ReleaseTextures();
	static void SyncState();
	static void RunPerFrameEffects(IDirect3DDevice9* pDevice = nullptr);

	static constexpr int randomTextureCount = 10;
	static inline LPDIRECT3DTEXTURE9 twitchUserDefinedTexture = nullptr;
	static inline std::vector<LPDIRECT3DTEXTURE9> randomTextures = std::vector<LPDIRECT3DTEXTURE9>(randomTextureCount, nullptr);
	static inline ColorList randomTextureColors = ColorList(randomTextureCount);
	static inline int currentRandomTexture = 0;

private:
	static inline std::atomic<bool> s_removeNotes = false;
	static inline std::atomic<bool> s_transparentNotes = false;
	static inline std::atomic<bool> s_solidNotes = false;
	static inline std::atomic<bool> s_fYourFC = false;
	static inline std::atomic<bool> s_drunkMode = false;
	static inline std::atomic<LPDIRECT3DTEXTURE9> s_activeSolidTexture = nullptr;
};
