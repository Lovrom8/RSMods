#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include <d3d9.h>
#include "../Framework/GamePhase.hpp"
#include "../Framework/Framework.hpp"

class RemoveHeadstockMod : public Framework::IMod {
public:
    MOD_ID(RemoveHeadstockMod)

    bool IsEnabled(const Framework::ModContext& c) const override;
    Framework::SettingDefs Settings() const override;

    void OnInitialize(Framework::ModContext& c) override;
    void OnEnabled(Framework::ModContext& c) override;
    void OnDisabled(Framework::ModContext& c) override;
    void OnSettingsChanged(Framework::ModContext& c) override;
    void OnSongEnter(Framework::ModContext& c) override;
    void OnSongExit(Framework::ModContext& c) override;
    void OnMenuTick(Framework::ModContext& c) override;

    void UpdateHeadstockCacheForMenu();

private:
    void SyncState(Framework::ModContext& c);

    void AddHeadstockTexture(LPDIRECT3DTEXTURE9 tex);
    bool IsHeadstockTextureCached(LPDIRECT3DTEXTURE9 tex) const;
    void ClearCachedTextures();
    size_t GetCachedTextureCount() const;

    bool active = false;
    std::string previousMenu;

    std::atomic<bool> removeHeadstockInThisMenu = false;
    std::atomic<bool> resetHeadstockCache = true;
    std::atomic<bool> calculatedHeadstocks = false;
    std::atomic<std::shared_ptr<const std::vector<LPDIRECT3DTEXTURE9>>> cachedTextures;
};