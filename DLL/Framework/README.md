# Mod Framework

Generalizes the `CCEffect` pattern to *all* mods so that a mod owns its own state, activation, and lifecycle 
instead of `ModManager` doing it, and adding a mod is adding one `.cpp` rather than editing the orchestrator.

> **Status: mod-suite migrated.** Every built-in namespace mod now registers through the framework and its
> logic has been removed from `ModManager` (MIDI — the last and hardest — ported the auto-tune / pedal-revert
> cluster and retired the transitional `GameLoopState`). `ModManager` retains only host plumbing: startup/init,
> per-tick game-state sync, and settings. The next layer — a versioned C plugin ABI over `IMod`/`ModContext`
> for third-party mods — is deliberately *not* built yet.

## Design boundaries (read first)

- **Two layers, kept separate.** `IMod`/`ModContext` are an **internal C++ API** for built-in
  mods. They are **not** the third-party plugin boundary and must not be used as an ABI. 
- **Activation ownership lives in the framework, not in each mod.** A mod says *what* it wants
  (enabled? which resources?); the registry decides *whether* and *when* it runs. All mod hooks
  run on `MainThread`.
- **Hooks must not block.** `MainThread` also drains keybind commands, so a sleeping hook freezes
  every keybind and every other mod's tick, including the one that would undo the wait. To wait,
  hold a `steady_clock` deadline and return early until a later tick passes it (see
  `ExtendedRangeMod`); to do real work, own a thread and join it in `OnShutdown`.
- **No raw input/WndProc surface.** `Keybindings` remains the Win32 adapter and never exposes
  consumable window messages to mods. It snapshots key events into non-consumable `KeyEvent`s;
  mods register named commands through `ModContext`.

## Host wiring (`dllmain.cpp`)

- `MainThread` - `InstantiatePending()` → `ApplyStartupMods` → `DispatchInitialize()`, then
  `Registry().Tick(phase)` each loop, and `Registry().Shutdown()` after the loop.
- `WndProc` `WM_COPYDATA` - `Keybindings::UpdateSettingsOnGUIChange` posts the settings mutation
  as a closure onto the `MainThreadInbox` instead of applying it on the message thread.
- `WndProc` key messages - snapshot modifiers/repeat state and post a `KeyEvent` to the same
  `MainThreadInbox`, the single main-thread work queue (see below). Commands are delivered promptly
  on `MainThread`; the 250 ms maintenance tick is not accelerated and missed deadlines are not
  replayed as catch-up bursts.

## Adding a mod

1. `Mods/MyMod.hpp`: a class deriving `Framework::IMod`; put `MOD_ID(MyMod)` in its public
   section, read settings via explicit `Settings::Setting` keys, and keep state as members.
2. `Mods/MyMod.cpp`:
   ```cpp
   #include "stdafx.h"
   #include "MyMod.hpp"
   static Framework::ModRegistrar<MyMod> _myReg;   // MUST be in the .cpp, never a header.
   ```
   The static registrar only pushes a POD factory node at load time (loader-lock safe); the
   registry constructs the mod later from MainThread (`InstantiatePending`).
3. If you migrated logic out of `ModManager`, delete it there. Cross-tick state a mod carried through the
   game loop should become a member of the mod (or, for a signal another mod reads, a flag on the relevant
   namespace helper).

`MOD_ID(Type)` makes the internal framework ID match the concrete class name and verifies that
its argument names the containing class. IDs must be **unique**; duplicates are rejected at
registration. They are deliberately separate from settings keys.

## Lifecycle state machine

```
Registered ──OnInitialize──▶ Inactive ──OnEnabled──▶ Active
     │                          ▲                       │
     │ OnInitialize throws      └───────OnDisabled──────┘
     ▼
  Faulted ◀──────────── OnEnabled / tick hook throws
```

- **Inactive** means initialized but not effectively active. It covers a mod that never activated,
  one the user disabled, and one **suppressed** by losing a resource conflict; their next valid
  transition is identical (the suppressed-vs-disabled distinction survives only in the log line).
- **Effective activation** = `IsEnabled()` **and** winning every resource it contends for. Only
  `Active` mods get tick hooks. A mod leaving `Active` reverts **synchronously** (there are no
  in-flight callbacks to wait for): its `OnDisabled` runs in the same `Tick` that deselects it.
- **Ordering guarantees** (edges are per-mod, not global phase edges):
  - Activate in a song: `OnEnabled → OnSongEnter → OnTick → OnSongTick`
  - Deactivate in a song: `OnSongExit → OnDisabled`
  - Enable mid-song fires `OnSongEnter`; disable mid-song fires `OnSongExit`; nothing is missed
    (per-mod `inSong` tracked relative to *its own* activation).
- **Failure policy**:
  - `OnInitialize`/`OnEnabled` throw → **Faulted** (never runs again). `OnEnabled` must be
    strongly exception-safe, as `OnDisabled` is *not* called on a failed enable.
  - A tick hook (`OnTick`/`OnMenuTick`/`OnSongTick`/`OnSongEnter`) throws → the mod is faulted
    immediately and receives a best-effort `OnDisabled` revert.
- **Shutdown**: `OnSongExit`(if in song) → `OnDisabled`(if active) → `OnShutdown`, then the
  registry destroys the mod objects.

## Conflicts & resources

Some mods can't run together (e.g. **DropPedal** and **MIDI auto-tune**) both drive tuning. They
express that by claiming the same named exclusive resource:

```cpp
std::vector<std::string_view> ClaimsExclusive() const override { return { "tuning-controller" }; }
int Priority() const override { return 10; }   // one GLOBAL priority per mod
```

Among all *enabled* mods claiming a resource the highest-`Priority()` one wins it; a mod that loses
any resource it claims is suppressed (its `OnDisabled` reverts its game state). The resolver
(`ConflictResolver.hpp`, pure and unit-tested) is deterministic greedy: order enabled mods by
`(Priority desc, Id asc)`, activate each iff none of its resources are already reserved by an
already-activated mod. `Tick` deactivates losers before activating winners; because a loser's
`OnDisabled` reverts synchronously, it releases its resources before any winner is activated, so a
contested handoff can't double-acquire in a single pass.

## On-screen display & in-game menus

The framework manages two decoupled UI interaction surfaces for mods:
- **HUD on-screen display (`ctx.Hud()`):** Snapshot-based text rendering anchored to semantic screen locations (`TopLeft`, `TopCenter`, `TopRight`, `TopTuning`, `HighwayLeft`, `MenuBanner`). The renderer copies snapshots on the D3D thread while mods publish on `MainThread`. See [`docs/hud-registry.md`](docs/hud-registry.md).
- **In-game settings menu (`ctx.Menu()`):** Host-agnostic registry for ImGui settings drawers (`MidiMod`, `CalibrationMod`, `MicrophoneVolumeOverrideMod`, `VoiceOverControlMod`). Dispatched safely during `Hook_EndScene` with per-mod exception isolation. See [`docs/menu-registry.md`](docs/menu-registry.md).
- **Procedural texture generation (`D3D::`):** Decoupled graphics utility layer for procedural texture generation (solid, gradient, CRC hashing). Mod-specific textures are owned by their respective mods (`ExtendedRangeMode`, `CustomHighwayColorsMod`) and regenerated at the `Hook_EndScene` frame boundary. See [`docs/texture-utilities.md`](docs/texture-utilities.md).

> For historical context on a retired framework-owned generic render-hook callback subsystem, see
> [`docs/render-hooks.md`](docs/render-hooks.md).

The contributor-facing configuration seam is decoupled via **declarative settings schemas**: mods
declare their configurable settings via `virtual SettingDefs Settings() const`, from which the DLL
drives INI loading and defaults in `Settings.cpp`, and exports `mods.manifest.json` for the GUI.
See [`docs/settings-schema.md`](docs/settings-schema.md).

How third-party mods should be allowed to *ship* — out-of-tree repos vs. runtime binary loading vs. the
trust boundary, and why review becomes a badge rather than a merge gate — is worked through in
[`docs/plugin-distribution.md`](docs/plugin-distribution.md). Short version: the internal C++ API stays
unfrozen; make the source surface zero-core-edit first, freeze a C ABI only once it stops moving.

## Main-thread inbox

`MainThreadInbox` (`Inbox()`) is the single main-thread work queue. Foreign threads post to it -
`WndProc` key input, GUI/`WM_COPYDATA`/Twitch/CrowdControl/render-thread settings writes, and the
window-close wake - and `MainThread` blocks in `WaitUntil`, then drains the two queues at their own
cadences: key events every command-dispatch pass, settings closures on the 250 ms maintenance tick.

The wake semantics mirror those cadences. Key events wake on a non-empty queue, because they are
drained every pass and the predicate self-clears. Settings and the close signal set a **one-shot**
wake flag, because settings are not drained until the next tick and a queue-based predicate would
spin. The `CommandRouter` no longer owns the wait/wake primitive or an event queue: it is pure
binding storage plus dispatch, taking the already-drained event batch and an owner-availability
query as parameters.

**Lifecycle state has a single source: the registry.** The router does not cache per-mod
`initialized`/`active` bits; at dispatch time it asks the registry (`IsOwnerAvailable`), which
answers from its own `records` + `ModState`. The one thing the router still owns is its **fault
set** - a binding that throws mid-batch must suppress that mod's remaining events before the
registry hears about it - which is discovered by the router, not mirrored from the registry.

## Commands & keybindings

`ctx.Commands().BindSetting(...)` attaches a settings-named keybinding to its owning mod;
`BindKey(...)` does the same for a fixed Win32 virtual key. `Keybindings`
still captures Win32 input, but actions and predicates execute on `MainThread`, serialized with mod
lifecycle and tick hooks. `KeyEvent` contains the virtual key, edge, modifier snapshot, and repeat
bit. Modifier-sensitive actions must use the event snapshot; they must not
poll `GetAsyncKeyState` after delivery.

Every command keeps its own predicate. Owner availability is an additional lifecycle gate:

- `Availability::Active` means **strictly `ModState::Active`**. It becomes unavailable the moment
  the mod leaves `Active` (whose `OnDisabled` revert runs synchronously in that `Tick`).
- `Availability::Initialized` remains available after successful initialization while the mod is normally
  inactive or conflict-suppressed, and disappears on fault/shutdown. It is only for uncontended state.
- **A command that mutates any resource returned by its owner's `ClaimsExclusive()` must use
  `Availability::Active`.** Otherwise conflict suppression could be bypassed through input. MIDI
  tuning commands therefore disappear while `tuning-controller` is owned by another mod.

Physical-key collisions are resolved deterministically. Within the normal setting pass, setting
name order matches the old `std::map`; the first physical match owns the event even when its predicate
is false (no fall-through). Volume adjustments are ordinary setting bindings and follow the same
rule. Fixed-key commands use a final internal pass matching the old inline-host-shortcut behavior.
These passes are router implementation details; binding refreshes log physical collisions rather
than rejecting user configuration.

Force Enumeration is owned by `EnumerationMod`, while the fixed Delete auto-tune intent is owned by
`MidiMod`. Only the Ctrl+A settings reload and Backspace debug-menu shortcuts remain host-owned in
`Keybindings`. Input received before `GameLoaded` is discarded at dispatch and is never replayed afterward.

## Settings

All settings writes are serialized onto `MainThread`. GUI/`WM_COPYDATA`, Twitch, CrowdControl,
and render-thread reload requests enqueue closures through `Registry().EnqueueSettingsUpdate`,
which posts them to the `MainThreadInbox`; the next registry `Tick` drains the complete FIFO batch,
then notifies mods via `OnSettingsChanged` before resolving activation. Non-settings GUI and effect
work still runs on its originating thread.

Mods read settings through typed `ModContext` accessors (`IsOn`, `Value`, `Int`, `When`, …) whose
bodies live in `ModContext.cpp`; the framework headers forward-declare the few `Settings` enums
those accessors return and never `#include "Settings.hpp"`. Only that one TU depends on the game's
settings header, which is what keeps the rest of the framework host-agnostic (and unit-testable).

`OnSettingsChanged` is delivered only to mods that are settled `Inactive`/`Active` (a `Registered`
mod isn't initialized yet; a `Faulted` one is terminal). A mod suppressed on the same `Tick` reverts
synchronously, after the notification pass.

The `Settings` maps/vectors are guarded by a single `shared_mutex` in `Settings.cpp`: every
accessor takes a shared lock, every mutator a unique lock, and reads use non-mutating lookups
(the old getters read via `operator[]`, which inserts on a miss and so raced even between two
readers). Worker threads may therefore call the `Settings` getters directly; disk IO in the
reload path stays outside the lock.

## Testing

The framework has no game or Windows dependencies, so it is unit-tested in isolation. `Tests/`
holds seven standalone console test suites (`ConflictResolverTests`, `ResourceLedgerTests`,
`CommandRouterTests`, `MainThreadInboxTests`, `HudRegistryTests`, `MenuRegistryTests`,
`StateMachineTests`), each with its own `main()` that returns non-zero on failure.

Build and run them all with:

```powershell
DLL/Framework/Tests/BuildAndRun.ps1
```

The script locates MSVC via `vswhere`, compiles each test against only the framework translation
units it needs, and runs it. AppVeyor runs the same script as a `test_script` step (`appveyor.yml`),
so a failing test fails the build (and skips deploy) and the tests can't rot out of compilation.
