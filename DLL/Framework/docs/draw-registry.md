# Draw-Interception Registry (`DrawRegistry`)

This document specifies the `DrawRegistry` rework: turning `Hook_DIP` / `Hook_DP` in
`DLL/D3D/D3DHooks.cpp` from a hardcoded switchboard of mod logic into a thin walk over
per-mod draw interceptors. It is the render-thread sibling of `HudRegistry` and the final
large piece of the D3D decoupling effort.

It is written to be implemented without the design conversation that produced it. Read
`docs/texture-utilities.md` (generation-side decoupling, already done) and
`docs/hud-registry.md` (the registry/binder/snapshot pattern this follows) first.

---

## 1. Motivation

`D3D.cpp/.hpp` is already a clean, mod-agnostic utility layer (see `texture-utilities.md`),
and texture *generation* is already owned per-mod. The remaining monolith is texture
*binding and mesh interception*:

- **`Hook_DIP`** (`D3DHooks.cpp`) inlines every rendering mod: ExtendedRange/CustomColors,
  CustomHighwayColors, RainbowNotes, five Twitch effects, Greenscreen, Fretless,
  RemoveInlays, RemoveLaneMarkers, RemoveLyrics, Headstock removal, Skyline removal,
  Fingerprints. Adding a mod means editing D3D core — the opposite of pluggable.
- **`Hook_DP`** duplicates a subset (note-tails) of that logic.
- **`D3DHooks.hpp`** still owns mod state flags (`RainbowNotes`, `PrideMode`,
  `RemoveLyrics`, `GreenScreenWall`, `toggleSkyline`, `twitchUserDefinedTexture`,
  `randomTextures`, headstock caches, ...).
- **`CheckRecreateTextures`** and **`RegenerateTwitchNoteColors`** hardcode the list of
  mods to regenerate — a second, smaller switchboard.

Goal: after this rework, adding or removing a rendering mod never touches D3D core, and
`D3DHooks` becomes the mechanical-hook mirror of what `D3D.cpp` already achieved.

> **Scope note.** This is draw-call/texture interception. It is distinct from the shelved
> "RenderHooks" overlay concept (which the DropPedal shared-HUD overlay settled as having
> no tenant). Interception has ~a dozen real tenants; the overlay concern is unrelated and
> already served by `HudRegistry` + `GameOverlay`.

---

## 2. The Contract

New header `DLL/Framework/DrawRegistry.hpp`. Keep it free of `Settings.hpp` and mod
includes, exactly as `HudRegistry.hpp` is (opaque `IMod` forward declaration only).

```cpp
enum class DrawPath   { Indexed, Primitive, Both };     // DIP, DP, or register for both
enum class DrawOutcome { Pass, Show, Hide, ReplaceTexture };

struct DrawResult {
    DrawOutcome outcome = DrawOutcome::Pass;
    DWORD stage = 1;                       // for ReplaceTexture
    LPDIRECT3DTEXTURE9 texture = nullptr;  // for ReplaceTexture
};

// Render-thread. MUST NOT query Settings by string — enablement is pre-resolved (Section 4).
using DrawInterceptor = std::function<DrawResult(DrawContext&)>;
```

### Outcome semantics (preserving current behavior)

| Outcome          | Equivalent to today                       | Terminal? |
|------------------|-------------------------------------------|-----------|
| `Pass`           | block did not apply, fall through         | no        |
| `Hide`           | `return REMOVE_TEXTURE`                    | **yes**   |
| `Show`           | `return SHOW_TEXTURE`                      | **yes**   |
| `ReplaceTexture` | `pDevice->SetTexture(stage, tex)` + continue | no     |

`ReplaceTexture` being non-terminal is what lets a later, higher-layering interceptor
override an earlier `SetTexture` — this is how CustomHighwayColors and RainbowNotes
currently layer *over* ExtendedRange. That ordering is encoded as **priority** (Section 3),
not source position.

Note: today some Twitch blocks do `SetTexture(nonexistentTexture)` then `return
SHOW_TEXTURE`. Model that as an interceptor returning `ReplaceTexture{nonexistentTexture}`
at a low priority, and a separate `Show` if the block is terminal in the original. When a
block both sets a texture *and* returns terminally in the original, emit two behaviors from
one interceptor by returning `Hide`/`Show` after the registry has already applied a prior
`ReplaceTexture` from the same mod at higher priority, OR (simpler) let the single
interceptor perform the `SetTexture` on `ctx.device` itself and return `Show`/`Hide`.
Interceptors may call `ctx.device->SetTexture` directly; `ReplaceTexture` is a convenience,
not the only channel.

### DrawContext

Carries what the two hooks already compute, plus a **shared lazy stage-CRC cache** — the
single biggest efficiency win, since today each CRC block independently does
`GetTexture` + `D3D::CRCForTexture`.

```cpp
struct DrawContext {
    IDirect3DDevice9* device = nullptr;
    const Mesh&      mesh;    // Stride / PrimCount / NumVertices           (see D3DHelper.hpp)
    const ThiccMesh& thicc;   // full 9-field signature                      (see D3DHelper.hpp)
    DrawPath path = DrawPath::Indexed;   // Indexed (DIP) or Primitive (DP)
    bool inSong = false;                 // cached GameState::IsInSong() for this draw

    // Computes GetTexture(stage) + D3D::CRCForTexture once per (stage) per draw and caches.
    // Returns std::nullopt if the stage has no bound texture. Interceptors matching on CRC
    // MUST go through this rather than calling CRCForTexture themselves.
    std::optional<DWORD> StageCRC(DWORD stage);

private:
    // Cache keyed by stage; the getter releases the base texture it retrieves, matching the
    // GetTexture()/Release() discipline already in Hook_DIP.
};
```

The existing draw classifiers (`NOTE_STEMS`, `NOTE_TAILS`, `OPEN_NOTE_ACCENTS`,
`IsToBeRemoved(sevenstring, mesh)`, `IsExtraRemoved(noteModifiers, thicc)`, etc.) stay
where they are; interceptors call them against `ctx.mesh` / `ctx.thicc`. The CRC tag values
live as immutable constants in `namespace D3D::Crc` (`D3D::Crc::StemsAccents`,
`D3D::Crc::Skyline*`, `D3D::Crc::Headstock*`, `D3D::Crc::NoteLanes`, ...); interceptors compare
against them through `DrawMesh::StageMatches` / `DrawMesh::StageMatchesAny`.

---

## 3. Registration and Binder

Follow the `CommandBinder` / `HudBinder` / `MenuBinder` pattern in `ModContext.hpp`.

```cpp
// In DrawRegistry.hpp
class DrawRegistry {
public:
    DrawRegistry();
    ~DrawRegistry();
    DrawRegistry(const DrawRegistry&) = delete;
    DrawRegistry& operator=(const DrawRegistry&) = delete;

    // MainThread: register an interceptor owned by `owner`. Lower `priority` runs earlier
    // in the walk. Ties break on owner Id() to stay deterministic.
    void Register(const IMod* owner, std::string id, int priority, DrawPath path,
                  DrawInterceptor fn);
    // MainThread: drop every interceptor owned by this mod.
    void RemoveMod(const IMod* owner);

    // MainThread: rebuild the active snapshot from currently-enabled owners. Called by the
    // ModRegistry whenever the enabled-mod set or settings change (Section 4).
    void RebuildActive(/* predicate: is owner enabled now? */);

    // Render thread: lock-free load of the immutable, priority-sorted active list for a path.
    [[nodiscard]] std::shared_ptr<const std::vector<ActiveEntry>> ActiveSnapshot(DrawPath) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

DrawRegistry& Draw();   // accessor, like Framework::Hud()
```

```cpp
// In ModContext.hpp, alongside HudBinder
struct DrawBinder {
    DrawRegistry& draw;
    const IMod* mod;
    void Register(std::string id, int priority, DrawPath path, DrawInterceptor fn) const {
        draw.Register(mod, std::move(id), priority, path, std::move(fn));
    }
};
// ModContext::Draw() const { return { Framework::Draw(), currentMod }; }
```

Mods register their interceptors in `OnInitialize` (same place they bind commands). The
interceptor closure captures the mod's own texture pointers / state. Teardown on
deactivate/fault reuses the owner-keyed `RemoveMod`, exactly like `HudRegistry::RemoveMod`.

`ActiveEntry` is `{ int priority; std::string_view ownerId; DrawInterceptor fn; }`.

---

## 4. The Enablement Snapshot (render-thread constraint)

Interceptors run thousands of times per frame, so **no `Settings::IsOn`/`IsTwitchSettingEnabled`
string lookups may occur in the hot path**. Enablement is resolved on the MainThread into an
immutable snapshot:

1. `DrawRegistry` holds the full registered list `{owner, id, priority, path, fn}`.
2. Whenever the enabled-mod set or settings change — the same trigger points that already
   drive `OnSettingsChanged` and mod enable/disable in `ModRegistry` — call
   `DrawRegistry::RebuildActive(...)`. It filters to interceptors whose owner is currently
   enabled, sorts by `(priority, ownerId)`, and publishes **two** immutable vectors (one for
   `Indexed`, one for `Primitive`; a `Both` registration lands in both).
3. Publish via `std::atomic<std::shared_ptr<const vector>>` (C++20 atomic shared_ptr). The
   render thread does one relaxed load per hook call and walks the copy. No lock, no
   per-call `IsOwnerAvailable`.

An interceptor present in the active snapshot is therefore *guaranteed its owning mod is
enabled* — its body needs only cheap per-frame runtime guards: `if (!ctx.inSong) return
{DrawOutcome::Pass};`, mesh matches, and `ctx.StageCRC(...)`. All the expensive string-keyed
settings resolution has been hoisted to `RebuildActive`.

Some current guards are per-frame *game state*, not settings (e.g. "Fretless only in song",
Skyline's `DrawSkylineInMenu`, Headstock's per-menu cache). Those stay in the interceptor
body as cheap bool checks against `ctx` / `GameState`.

---

## 5. The Hooks After

`Hook_DIP` body reduces to (keeping the settings-missing reload guard and the dev
mesh-logging block, which are host infra, not mods):

```cpp
Mesh current(Stride, PrimCount, NumVertices);
ThiccMesh currentThicc(Stride, PrimCount, NumVertices, StartIndex, StartRegister,
                       PrimType, decl->Type, VectorCount, NumElements);

DrawContext ctx{ pDevice, current, currentThicc, DrawPath::Indexed, GameState::IsInSong() };
auto active = Framework::Draw().ActiveSnapshot(DrawPath::Indexed);
for (const auto& e : *active) {
    DrawResult r = e.fn(ctx);
    switch (r.outcome) {
        case DrawOutcome::Hide: return REMOVE_TEXTURE;
        case DrawOutcome::Show: return SHOW_TEXTURE;
        case DrawOutcome::ReplaceTexture: pDevice->SetTexture(r.stage, r.texture); break;
        case DrawOutcome::Pass: break;
    }
}
return SHOW_TEXTURE;   // KEEP: "Display Graphics"
```

`Hook_DP` gets the identical walk with `DrawPath::Primitive`. Note-tail interceptors
register as `DrawPath::Both` so DIP and DP cannot drift.

---

## 6. Migration Tiers

Migrate one tier at a time. After each tier, delete the corresponding flags from
`D3DHooks.hpp` and confirm a frame-time and visual parity check before the next.

### Tier A — PoC: pure mesh-signature `Hide` [COMPLETED]
`Fretless`, `RemoveInlays`, `RemoveLaneMarkers`, `GreenScreenWall`, `RemoveLyrics`.
Migrated to DrawRegistry interceptors returning `DrawOutcome::Hide` or `Pass`.
Legacy blocks in `Hook_DIP` deleted.

### Tier B — CRC-based single-texture ops [COMPLETED]
`RemoveFingerprints`, `RemoveSkyline`, `CustomHighwayColors` (stage-1 `ReplaceTexture`),
and `RemoveHeadstock`. State moved into owning mods. All CRC reads go through `ctx.StageCRC`.

### Tier C — ordering-sensitive note cluster [COMPLETED]
`RainbowNotes` (stems at priority 10, heads/tails at 40), `ExtendedRange` (priority 20),
Twitch note mods (`RemoveNotes`, `TransparentNotes`, `SolidNotes` at priority 30).
`Hook_DP` note tails migrated via `DrawPath::Both`. All legacy note blocks deleted from `Hook_DIP` and `Hook_DP`.

### Tier D — non-draw effects riding the DIP hook [COMPLETED]
Twitch `FYourFC` (note streak zeroing) and `DrunkMode` (camera float write) moved out of DIP
into `TwitchMod::RunPerFrameEffects()`, called at the per-frame render boundary in `Hook_EndScene`
gated on `GameState::IsInSong()`.

### State retired from `D3DHooks.hpp` [COMPLETED]
`RainbowNotes`, `PrideMode`, `RemoveLyrics`, `GreenScreenWall`, `toggleSkyline`,
`SkylineOff`, `DrawSkylineInMenu`, `RemoveHeadstockInThisMenu`, `resetHeadstockCache`,
`twitchUserDefinedTexture`, `randomTextures`, `randomTextureColors`, `currentRandomTexture`,
`ToggleOffLoftWhenDoneWithMod`, `DiscoModeEnabled`, `DiscoModeInitialSetting`,
`EnumSliderVal`, `vertexBufferSize`, headstock caches.

---

## 7. Fold In the Generation Switchboard [COMPLETED]

The hardcoded mod lists in `CheckRecreateTextures` were replaced by generic texture-regeneration
fan-out in `DrawRegistry`:
- `DrawRegistry::RegisterTextureRegen(const IMod* owner, TextureRegenCallback fn)`
- `DrawRegistry::RegenerateAllTextures(IDirect3DDevice9* pDevice)`
- `ExtendedRangeMod`, `CustomHighwayColorsMod`, and `TwitchMod` self-register on `OnInitialize`.
- `CheckRecreateTextures` delegates solely to `Framework::Draw().RegenerateAllTextures(pDevice)`.

---

## 8. How to Add a Rendering Mod

This section is a real guide against the shipped code, not a spec. Read it top-to-bottom before writing interceptor logic.

---

### 8.1 The priority ladder

Lower priority number runs earlier. The live ladder:

| Priority | Owner                    | Path     | What it does |
|----------|--------------------------|----------|--------------|
| -10      | `CustomHighwayColorsMod` | Indexed  | Highway texture swap (noteway, fret numbers, gutter) |
| 10       | `RainbowNotesStems`      | Indexed  | Rainbow stems / bend-slide indicators |
| 20       | `ExtendedRangeNotes`     | Both     | ER / custom-color note heads, tails, stems |
| 30       | `TwitchNotes`            | Both     | Remove / transparent / solid-color note overlay |
| 40       | `RainbowNotesHeadsTails` | Both     | Rainbow note heads + tails |

Tie-breaking is alphabetical on owner `Id()`.

---

### 8.2 Outcomes

| `DrawOutcome`    | Effect                                          | Terminal? |
|------------------|-------------------------------------------------|-----------|
| `Pass`           | Do nothing, continue to next interceptor         | no        |
| `Hide`           | `return REMOVE_TEXTURE`                          | **yes**   |
| `Show`           | `return SHOW_TEXTURE`                            | **yes**   |
| `ReplaceTexture` | `pDevice->SetTexture(stage, tex)` and **continue** | no      |

`ReplaceTexture` is non-terminal deliberately — it allows a later interceptor to override it.
Call `ctx.device->SetTexture(stage, tex)` directly if you need to set a texture **and** return a terminal outcome in the same interceptor.

---

### 8.3 Shared mesh/CRC helpers — `DrawMeshTags.hpp`

All mesh-classification predicates and CRC constants live in `DLL/Mods/DrawMeshTags.hpp`.
**Never** duplicate the stride/primcount numbers or raw CRC values in your mod.

```cpp
#include "DrawMeshTags.hpp"

// Mesh geometry
DrawMesh::IsNoteHead(ctx)           // seven-string head or modifier shape
DrawMesh::IsNoteStemOrAccent(ctx)   // stem / bend / slide / accent
DrawMesh::IsNoteTail(ctx)           // Primitive path, Stride 12

// CRC helpers (all internally use ctx.StageCRC — cached once per stage per draw)
DrawMesh::IsNoteStemCrc(ctx)                                 // stage 1 is stem or bend-slide CRC
DrawMesh::StageMatches(ctx, 1, D3D::Crc::NoteLanes)          // single tag check
DrawMesh::StageMatchesAny(ctx, 1, { D3D::Crc::Headstock0,    // any-of tag check
                                    D3D::Crc::Headstock1 })

// Available CRC tag constants (namespace D3D::Crc):
//   StemsAccents, BendSlideIndicators
//   NoteLanes, NotewayFretNumbers, NotewayGutters
//   SkylinePurple, SkylineOrange, SkylineBackground, SkylineShadow
//   Headstock0..4
//   FingerprintNumber, FingerprintIcon
```

---

### 8.4 Texture lifetime rule

> **Never call `ReleaseTextures()` (or any COM `Release()`) on the MainThread.**
> That window overlaps with the render thread's interceptor pointer load → `SetTexture`.

The correct pattern for mods that own procedural textures:

**`OnInitialize`**: use `RegisterTextureLifecycle` (regen + release in one declaration):
```cpp
c.Draw().RegisterTextureLifecycle(
    &MyMod::RegenerateTextures,   // render thread, called by CheckRecreateTextures
    &MyMod::ReleaseTextures);     // render thread, called by RunPendingReleases
```

**`OnDisabled`**: null the interceptor-facing atomic and enqueue a deferred release:
```cpp
void MyMod::OnDisabled(ModContext& c) {
    s_myTexture.store(nullptr, std::memory_order_release); // interceptor self-guards this frame
    c.Draw().RequestTextureRelease();                      // deferred, runs at next EndScene
}
```

**`OnShutdown`**: synchronous `ReleaseTextures()` is fine — the render loop is ending:
```cpp
void MyMod::OnShutdown(ModContext&) {
    ReleaseTextures(); // safe: no render thread in flight
}
```

**`OnEnabled` / `OnSettingsChanged`**: trigger regen by setting `D3DHooks::RecreateTextures = true;`.

---

### 8.5 Worked example A — simple mesh hide (no textures)

`RemoveInlaysMod` (Tier A) hides fretboard inlays while in a song. No textures owned.

```cpp
class RemoveInlaysMod : public Framework::IMod {
public:
    MOD_ID(RemoveInlaysMod)

    bool IsEnabled(const Framework::ModContext& c) const override {
        return c.IsOn(Setting::RemoveInlaysEnabled);
    }

    void OnInitialize(Framework::ModContext& c) override {
        c.Draw().Register("RemoveInlays", 0, Framework::DrawPath::Indexed,
            [](Framework::DrawContext& ctx) -> Framework::DrawResult {
                if (!ctx.inSong) return { Framework::DrawOutcome::Pass };
                if (IsExtraRemoved(inlays, ctx.thicc))
                    return { Framework::DrawOutcome::Hide };
                return { Framework::DrawOutcome::Pass };
            });
        // No RegisterTextureLifecycle — this mod owns no D3D textures.
    }
};
```

---

### 8.6 Worked example B — CRC-matched texture replacement (owns D3D textures)

Pattern derived from `CustomHighwayColorsMod`. Owns a texture pack; replaces highway texture on CRC match.

```cpp
class MyHighwayMod : public Framework::IMod {
public:
    MOD_ID(MyHighwayMod)

    bool IsEnabled(const Framework::ModContext& c) const override {
        return c.IsOn("MyHighwayEnabled");
    }

    void OnInitialize(Framework::ModContext& c) override {
        // -10: runs before the note cluster so the highway is styled first.
        c.Draw().Register("MyHighway", -10, Framework::DrawPath::Indexed,
            [](Framework::DrawContext& ctx) -> Framework::DrawResult {
                auto tex = s_texture.load(std::memory_order_acquire);
                if (!tex) return { Framework::DrawOutcome::Pass };

                // Only match the noteway lane CRC on stage 1:
                if (IsToBeRemoved(noteHighway, ctx.mesh) &&
                    DrawMesh::StageMatches(ctx, 1, D3D::Crc::NoteLanes)) {
                    return { Framework::DrawOutcome::ReplaceTexture, 1, tex };
                }
                return { Framework::DrawOutcome::Pass };
            });

        c.Draw().RegisterTextureLifecycle(&MyHighwayMod::Regenerate, &MyHighwayMod::Release);
    }

    void OnEnabled(Framework::ModContext&)  override { D3DHooks::RecreateTextures = true; }
    void OnDisabled(Framework::ModContext& c) override {
        s_texture.store(nullptr, std::memory_order_release);
        c.Draw().RequestTextureRelease();
    }
    void OnShutdown(Framework::ModContext&) override { Release(); }

    static void Regenerate(IDirect3DDevice9* dev) { /* D3D::GenerateGradientTexture(...) */ }
    static void Release() { D3D::ReleaseTexture(&s_rawTexture); s_texture.store(nullptr, std::memory_order_release); }

private:
    static inline IDirect3DTexture9*            s_rawTexture = nullptr;
    static inline std::atomic<IDirect3DTexture9*> s_texture  = nullptr;
};
```

---

## 9. Known Risks

- **Frame time.** `std::function` + `shared_ptr` load per draw × thousands/frame. Mitigate by
  ordering cheap mesh-only interceptors ahead of CRC ones, and rely on the shared
  `StageCRC` cache — at most one `CRCForTexture` per stage per draw regardless of tenant
  count.
- **Ordering regressions.** The above/below-ER relationships are load-bearing. Encode them as
  explicit priorities. Never migrate Tier C mods piecemeal — interceptor and legacy-block deletion
  must land in the same commit.
- **DIP/DP drift.** Register note-tail interceptors as `DrawPath::Both`.
- **Snapshot staleness.** `RebuildActive` must fire on every enable/disable/fault and
  settings change. Miss one and a toggled mod keeps or loses effect until the next rebuild.
- **Texture UAF.** Always use `RegisterTextureLifecycle` + `RequestTextureRelease` for mods
  that own D3D textures. Calling COM `Release()` on the MainThread (in `OnDisabled`) is a UAF.
