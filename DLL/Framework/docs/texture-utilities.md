# Procedural Texture Generation & D3D Utility Architecture

This document describes the decoupled procedural texture generation architecture in RSMods, the role of `namespace D3D`, and how future mods should create, manage, and consume procedural textures.

---

## 1. Architecture Overview

Historically, `DLL/D3D/D3D.cpp` served as a monolithic switchboard: it queried mod settings, derived string and note color palettes, generated guitar string, note, and highway textures, and wrote test bitmaps to disk. This coupled generic graphics routines directly to specific features and caused textures to be regenerated mid-frame inside `Hook_DIP`.

The modern architecture separates concerns into two distinct layers:
1. **Generic Graphics Layer (`D3D/D3D.hpp`, `D3D/D3D.cpp`):** A pure utility layer providing device texture management, GDI+ gradient rasterization, solid-color texture generation, and texture CRC verification. It contains **zero** mod includes, **zero** settings queries, and **zero** game-feature assumptions.
2. **Feature Mod Ownership:** Each mod owns its textures, color mathematics, and generation triggers:
   - **`ExtendedRangeMode`** (`DLL/Mods/ExtendedRangeMode.{hpp,cpp}`): Owns string textures (`customStringColorTexture`), note textures (`customNoteColorTexture`), and animated rainbow textures (`rainbowTextures`), along with HSL color derivations (`GetCustomColors`, `SetCustomColors`).
   - **`CustomHighwayColorsMod`** (`DLL/Mods/CustomHighwayColorsMod.{hpp,cpp}`): Implements `Framework::IMod` and owns the noteway highway lane, gutter, and fret number textures (`GetNotewayTexture`, `GetGutterTexture`, `GetFretNumTexture`).
   - **`TwitchMod`** (`DLL/Mods/TwitchMod.{hpp,cpp}`): Owns the Twitch / CrowdControl Solid Notes textures, random solid note textures (`randomTextures`) and the user-defined note texture (`twitchUserDefinedTexture`).

---

## 2. Generic Graphics Utilities (`namespace D3D`)

All generic rendering helpers live in `namespace D3D` (`DLL/D3D/D3D.hpp`):

### Safe Texture Release
```cpp
void D3D::ReleaseTexture(IDirect3DTexture9** ppTexture);
```
Safely calls `Release()` on non-null Direct3D textures and resets the pointer to `nullptr`. Call this during mod shutdown or before re-allocating a texture.

### Solid Texture Generation
```cpp
HRESULT D3D::GenerateSolidTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppD3Dtex, DWORD colour32);
```
Allocates an 8x8 managed 16-bit texture (`D3DFMT_A4R4G4B4`) filled with a uniform 32-bit ARGB color.

### Gradient Texture Generation
```cpp
void D3D::GenerateGradientTexture(
    IDirect3DDevice9* pDevice,
    IDirect3DTexture9** ppTexture,
    const ColorList& colorSet,
    UINT in_width = 256,
    UINT in_height = 128,
    int in_lineHeight = 8,
    int howManyLines = 16
);
```
Rasterizes a smooth horizontal gradient using GDI+ `LinearGradientBrush` across `howManyLines` and copies the resulting ARGB bitmap directly into a managed `D3DFMT_A8R8G8B8` Direct3D texture.
- Prior texture at `*ppTexture` is automatically released via `D3D::ReleaseTexture`.
- `ColorList` is a `std::vector<RSColor>` defined in `RSColor.h`.

### Texture CRC Calculation
```cpp
bool D3D::CRCForTexture(LPDIRECT3DTEXTURE9 texture, IDirect3DDevice9* pDevice, DWORD& o_crc);
```
Locks the texture surface in read-only mode and computes a quick checksum over the initial byte block to identify in-game meshes/textures during `Hook_DIP`.

---

## 3. Frame Boundary Texture Lifecycle (EndScene vs DIP)

### The Problem with DIP Hook Regeneration
In earlier implementations, textures were regenerated inside `Hook_DIP` in `D3DHooks.cpp`. Because `DrawIndexedPrimitive` executes hundreds or thousands of times per frame in the middle of scene rendering, locking devices and generating 190+ textures mid-draw caused noticeable frame drops and hitching. A secondary counter in `MidiThread` was used as a crude throttle.

### The Modern Frame Boundary Pipeline
1. **Requesting a rebuild:** A mod that needs its textures rebuilt (on enable, on a settings change, or when an effect changes a colour) sets:
   ```cpp
   D3DHooks::RecreateTextures = true;
   ```
2. **Atomic Consumption at `Hook_EndScene`:** At the frame boundary in `dllmain.cpp` after `D3D::LoadTextures(pDevice)`:
   ```cpp
   D3DHooks::CheckRecreateTextures(pDevice);
   ```
3. **Execution:** Inside `D3DHooks::CheckRecreateTextures(pDevice)`, which knows no mods:
   ```cpp
   void D3DHooks::CheckRecreateTextures(IDirect3DDevice9* pDevice) {
       if (!pDevice) return;

       if (RecreateTextures.exchange(false)) {
           Framework::Draw().RegenerateAllTextures(pDevice);
       }
   }
   ```
   Each texture-owning mod registered its regen callback with `ctx.Draw()` (see Step 3 below).
   - Uses `atomic_bool::exchange(false)` for thread-safe consumption without locking.
   - Textures are created while no scene draw calls are in progress.
   - `MidiThread` has been stripped of all graphics flags and only processes MIDI.

---

## 4. Developer Guide: Adding New Procedural Textures

When creating a new mod that requires custom procedural textures, follow this standard pattern:

### Step 1: Declare Texture Ownership in Your Mod
Do not put mod-specific texture pointers into `D3D.hpp`. Keep them inside your mod class or namespace:

```cpp
// In MyTextureMod.hpp
#pragma once

#include <d3d9.h>
#include "../Framework/Framework.hpp"

class MyTextureMod : public Framework::IMod {
public:
    MOD_ID(MyTextureMod);

    bool IsEnabled(const Framework::ModContext& c) const override;
    void OnInitialize(Framework::ModContext& c) override;
    void OnEnabled(Framework::ModContext& c) override;
    void OnDisabled(Framework::ModContext& c) override;
    void OnShutdown(Framework::ModContext& c) override;

    static void RegenerateTextures(IDirect3DDevice9* pDevice);
    static void ReleaseTextures();

    static LPDIRECT3DTEXTURE9 GetTexture() noexcept { return s_myCustomTexture; }

private:
    static inline LPDIRECT3DTEXTURE9 s_myCustomTexture = nullptr;
};
```

### Step 2: Implement Generation Using `D3D::`
```cpp
// In MyTextureMod.cpp
#include "../stdafx.h"
#include "MyTextureMod.hpp"
#include "../D3D/D3D.hpp"
#include "../Settings.hpp"

namespace {
    Framework::ModRegistrar<MyTextureMod> _myTextureModReg;
}

bool MyTextureMod::IsEnabled(const Framework::ModContext& c) const {
    return c.IsOn("MyTextureSetting");
}

void MyTextureMod::OnShutdown(Framework::ModContext&) {
    ReleaseTextures();
}

void MyTextureMod::RegenerateTextures(IDirect3DDevice9* pDevice) {
    if (!pDevice) return;

    ColorList colors = {
        Settings::ConvertHexToColor("FF5500"),
        Settings::ConvertHexToColor("00AAFF")
    };

    // Use D3D utility for generation
    D3D::GenerateGradientTexture(pDevice, &s_myCustomTexture, colors, 256, 32, 16, 2);
}

void MyTextureMod::ReleaseTextures() {
    D3D::ReleaseTexture(&s_myCustomTexture);
}
```

### Step 3: Register Regen and Release With the Draw Registry
In `OnInitialize`, declare both callbacks; both run on the render thread at `Hook_EndScene`. In
`OnDisabled`, queue the release instead of releasing on MainThread (`draw-registry.md` §8.4):

```cpp
void MyTextureMod::OnInitialize(Framework::ModContext& c) {
    c.Draw().RegisterTextureLifecycle(&MyTextureMod::RegenerateTextures, &MyTextureMod::ReleaseTextures);
}

void MyTextureMod::OnEnabled(Framework::ModContext& c) {
    c.Draw().CancelTextureRelease();
    D3DHooks::RecreateTextures = true;
}

void MyTextureMod::OnDisabled(Framework::ModContext& c) {
    c.Draw().RequestTextureRelease();
}
```

### Step 4: Bind the Texture From a Draw Interceptor
No `D3DHooks` edit: register an interceptor in `OnInitialize` that swaps the texture in
(`draw-registry.md` §8 has the matching helpers and worked examples):

```cpp
c.Draw().Register("MyTexture", 0, Framework::DrawPath::Indexed, [](Framework::DrawContext& ctx) -> Framework::DrawResult {
    if (ctx.StageCRC(1) == myTargetCrc && MyTextureMod::GetTexture())
        return { Framework::DrawOutcome::ReplaceTexture, 1, MyTextureMod::GetTexture() };
    return { Framework::DrawOutcome::Pass };
});
```

---

## 5. Summary of Key Guidelines for Future Code

- **Never add mod-specific logic to `D3D.hpp` or `D3D.cpp`:** Keep `D3D` completely agnostic.
- **Always release textures on teardown:** queue a release in `OnDisabled` (`RequestTextureRelease`), and call `D3D::ReleaseTexture(&ptr)` directly only in `OnShutdown`.
- **Never allocate or regenerate textures inside `Hook_DIP` or `Hook_DP`:** Mid-frame generation causes hitching. Always schedule regeneration through `D3DHooks::RecreateTextures = true;` to execute cleanly at `Hook_EndScene`.
- **Target the correct texture pointer:** Verify that string and note color routines write to their distinct texture pointers (`customStringColorTexture` vs `customNoteColorTexture`).
