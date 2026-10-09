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
  `ExtendedRangeMod`); to do real work, own a thread and join it in `OnShutdown`. The framework times
  every hook and logs a throttled warning when one runs over budget on `MainThread`, so a slow hook is
  surfaced instead of shipping silently, and a monitor thread names any hook or command that has been
  running for 5 s (see [`docs/hook-watchdog.md`](docs/hook-watchdog.md)).
- **No raw input/WndProc surface.** `Keybindings` remains the Win32 adapter and never exposes
  consumable window messages to mods. It snapshots key events into non-consumable `KeyEvent`s;
  mods register named commands through `ModContext`.

## Host wiring (`dllmain.cpp`)

- `MainThread` - `InstantiatePending()` → `ModManager::InitializeMods` → `ApplyStartupMods` →
  `DispatchInitialize()`, then each loop `DispatchCommands` on every wake and `Registry().Tick(phase)`
  on the maintenance tick, and `Registry().Shutdown()` after the loop. `InstantiatePending` comes first
  because `InitializeMods` loads settings from the schemas it registers.
- `WndProc` `WM_COPYDATA` - `Keybindings::UpdateSettingsOnGUIChange` posts the settings mutation
  as a closure onto the `MainThreadInbox` instead of applying it on the message thread.
- `WndProc` key messages - snapshot modifiers/repeat state and post a `KeyEvent` to the same
  `MainThreadInbox`, the single main-thread work queue (see below). Commands are delivered promptly
  on `MainThread`; key input doesn't bring the maintenance tick forward (it runs every 250 ms, or
  33 ms while a mod calls `ctx.RequestFastTick()`) and missed deadlines are not replayed as
  catch-up bursts.

## Adding a mod

1. **Scaffold it:** `powershell Build/New-Mod.ps1 -Name ShowBpm` writes `Mods/ShowBpmMod.hpp/.cpp`
   (a mod with one toggle that builds as-is) and lists both in `DLL.vcxproj` and `.filters`.
2. **Write the mod.** Declare every setting and key binding in `Settings()` and read them through
   `ctx`. Keep state as members.
3. **Build `RSMods.sln`** (Release|Win32). A `.cpp` that isn't listed in `DLL.vcxproj` fails the build
   by name: an unlisted file never compiles, so its mod would otherwise silently not exist.
4. **Regenerate the manifest:** `powershell DLL/Framework/Tests/BuildAndRun.ps1 -DumpManifest`. The GUI
   renders its settings screen and keybindings page from it, so there's no GUI code to write.

That's the whole list. A mod doesn't edit `Settings.hpp`/`.cpp`, `ModManager`, `dllmain.cpp`,
`D3DHooks` or the GUI.

### Worked example: a toggle, a hotkey and a HUD line

```cpp
namespace {
	// Every mod's settings share one key space, so keys carry the mod's name.
	constexpr char kEnabled[] = "ShowBpmEnabled";
	constexpr char kToggleKey[] = "ShowBpmKey";
}

SettingDefs ShowBpmMod::Settings() const {
	return {
		SettingDef::Toggle(kEnabled, "Show BPM")
			.Hint("Shows the song's tempo in the top right corner."),     // becomes the GUI tooltip
		Framework::KeyBind(kToggleKey, "Show / Hide BPM", "J"),           // appears on the keybindings page
	};
}

bool ShowBpmMod::IsEnabled(const ModContext& c) const {
	return c.IsOn(kEnabled);   // the registry activates and deactivates the mod from this
}

void ShowBpmMod::OnInitialize(ModContext& c) {
	c.Commands().BindSetting(kToggleKey, KeyEdge::Up, Availability::Active,
		[this](ModContext&, const KeyEvent&) { shown = !shown; });
}

void ShowBpmMod::OnSongTick(ModContext& c) {
	c.Hud().Set("bpm", { HudAnchor::TopRight, 10 },
		{ .visible = shown, .text = std::to_string(CurrentBpm()) + " BPM" });   // CurrentBpm(): your game read
}

void ShowBpmMod::OnSongExit(ModContext& c) {
	c.Hud().Set("bpm", { HudAnchor::TopRight, 10 }, {});   // otherwise the last line stays up in the menus
}
```

Things worth knowing:

- **Setting keys** live in the mod as `constexpr` strings, as above; the schema is the source of truth.
  The `Settings::Setting` constants in `Settings.hpp` are how the older mods were written; new mods don't
  need to add one.
- **Hooks:** `OnTick` runs every active tick in every phase, including `Loading` (guard game memory
  there), before `OnMenuTick`/`OnSongTick`. A mod that must undo its patch when switched off keeps the
  toggle check inside its ticks and reverts in `OnDisabled`; an apply-only mod can gate on `IsEnabled()`.
- **HUD elements** are cleared automatically when the mod deactivates. A mod that owns one must be
  `Active` whenever it can show, so gate the element's visibility, not the mod.
- **Game addresses** can be a `VersioningStruct` in the mod's own `.cpp`; shared ones live in
  `Offsets.cpp`.

`MOD_ID(Type)` makes the internal framework ID match the concrete class name and verifies that
its argument names the containing class. IDs must be **unique**; duplicates are rejected at
registration. They are deliberately separate from settings keys. The
`static Framework::ModRegistrar<T>` line must stay in the `.cpp`, never a header: it only pushes a POD
factory node at load time (loader-lock safe), and the registry constructs the mod later on MainThread.

## Lifecycle state machine

```
Registered ──OnInitialize──▶ Inactive ──OnEnabled──▶ Active
     │                          ▲                       │
     │ OnInitialize throws      └───────OnDisabled──────┘
     ▼
  Faulted ◀──── OnEnabled / tick hook / OnSettingsChanged / command throws
     │
     └── Retry faulted (mod status view) ──▶ Registered, OnInitialize again
```

- **Inactive** means initialized but not effectively active. It covers a mod that never activated,
  one the user disabled, and one **suppressed** by losing a resource conflict; their next valid
  transition is identical (the suppressed-vs-disabled distinction lives in the log line and the
  mod status snapshot, not in the state).
- **Effective activation** = `IsEnabled()` **and** winning every resource it contends for. Only
  `Active` mods get tick hooks. A mod leaving `Active` reverts **synchronously** (there are no
  in-flight callbacks to wait for): its `OnDisabled` runs in the same `Tick` that deselects it.
- **Ordering guarantees** (edges are per-mod, not global phase edges):
  - Activate in a song: `OnEnabled → OnSongEnter → OnTick → OnSongTick`
  - Deactivate in a song: `OnSongExit → OnDisabled`
  - Enable mid-song fires `OnSongEnter`; disable mid-song fires `OnSongExit`; nothing is missed
    (per-mod `inSong` tracked relative to *its own* activation).
- **Failure policy**:
  - `OnInitialize`/`OnEnabled` throw → **Faulted**: it stops running until the user presses *Retry
    faulted* in the mod status view ([`docs/mod-status.md`](docs/mod-status.md)). `OnEnabled` must be
    strongly exception-safe, as `OnDisabled` is *not* called on a failed enable.
  - A command binding throws → the mod's remaining key events are dropped and it faults like a hook.
  - A tick hook (`OnTick`/`OnMenuTick`/`OnSongTick`/`OnSongEnter`/`OnSongExit`) or
    `OnSettingsChanged` throws → the mod is faulted immediately; an `Active` one receives a best-effort
    `OnDisabled` revert first. Only the `OnSongExit`/`OnDisabled` calls made while tearing a mod down
    are best-effort, since the mod is leaving anyway.
- **Shutdown**: `OnSongExit`(if in song) → `OnDisabled`(if active) → `OnShutdown`, then the
  registry drops every registration but keeps the mod objects alive until it is destroyed itself
  (process exit): the game keeps rendering while it closes, and a frame already in flight may still
  call a mod's menu or draw callback.

## Conflicts & resources

Some mods can't run together (e.g. MIDI auto-tune and any other mod that retunes the guitar, like
DropPedal). They express that by claiming the same named exclusive resource:

```cpp
std::vector<std::string_view> ClaimsExclusive() const override { return { "tuning-controller" }; }
int Priority() const override { return 10; }   // one GLOBAL priority per mod
```

`MidiMod` holds `tuning-controller` at priority 0, and only while `AutoTuneForSong` is on, so another tuning
mod contends with it only when the player has turned both on.

Among all *enabled* mods claiming a resource the highest-`Priority()` one wins it; a mod that loses
any resource it claims is suppressed (its `OnDisabled` reverts its game state). The resolver
(`ConflictResolver.hpp`, pure and unit-tested) is deterministic greedy: order enabled mods by
`(Priority desc, Id asc)`, activate each iff none of its resources are already reserved by an
already-activated mod. `Tick` deactivates losers before activating winners; because a loser's
`OnDisabled` reverts synchronously, it releases its resources before any winner is activated, so a
contested handoff can't double-acquire in a single pass.

`Priority()` orders mods **against each other only**. CrowdControl effects claim the same named
resources through the ledger (`docs/resource-ledger.md`), but registry-vs-CC arbitration is
**first-come, not priority**: a running CC effect holding a resource suppresses every mod claiming it
regardless of the mod's priority, and a mod already holding one makes the CC effect retry.

## On-screen display & in-game menus

The framework manages two decoupled UI interaction surfaces for mods:
- **HUD on-screen display (`ctx.Hud()`):** Snapshot-based text rendering anchored to semantic screen locations (`TopLeft`, `TopCenter`, `TopRight`, `TopTuning`, `HighwayLeft`, `MenuBanner`). The renderer copies snapshots on the D3D thread while mods publish on `MainThread`. Anchors lay out in the whole display, or in a narrower band a mod sets with `Hud().SetLayoutAspect()`. See [`docs/hud-registry.md`](docs/hud-registry.md).
- **In-game settings menu (`ctx.Menu()`):** Host-agnostic registry for ImGui settings drawers (`MidiMod`, `CalibrationMod`, `MicrophoneVolumeOverrideMod`, `VoiceOverControlMod`). Dispatched safely during `Hook_EndScene` with per-mod exception isolation. See [`docs/menu-registry.md`](docs/menu-registry.md).
- **Overlay input capture (`Framework::Input()`):** while the menu is open and ImGui wants the mouse, the game's DirectInput mouse reads come back empty, so clicks and scrolls on a panel stop reaching the game. Host-driven; mods don't call it. See [`docs/input-capture.md`](docs/input-capture.md).
- **Procedural texture generation (`D3D::`):** Decoupled graphics utility layer for procedural texture generation (solid, gradient, CRC hashing). Mod-specific textures are owned by their respective mods (`ExtendedRangeMode`, `CustomHighwayColorsMod`) and regenerated at the `Hook_EndScene` frame boundary. See [`docs/texture-utilities.md`](docs/texture-utilities.md).

The framework also publishes a read-only **mod status snapshot** (`Registry().StatusSnapshot()`):
every mod's effective state (`Active` / `Disabled` / `Suppressed` / `Faulted`), surfacing the
disabled-vs-suppressed distinction that `ModState` collapses and the log line otherwise owns alone. The
in-game `RS Mods` window renders it behind an opt-in `Mod status` toggle. See [`docs/mod-status.md`](docs/mod-status.md).

- **In-game overlays (`ctx.Menu().RegisterOverlay`):** an ImGui drawer that runs every frame while the mod is active, menu open or not, for anything beyond a HUD text line. See [`docs/menu-registry.md`](docs/menu-registry.md).
- **Draw interception and render-thread callbacks (`ctx.Draw()`):** per-draw interceptors (optionally changing device state for one draw via `ctx.AfterDraw`), plus per-frame, before-reset and device-reset callbacks for mods with render-side state. See [`docs/draw-registry.md`](docs/draw-registry.md).
- **Audio input (`ctx.Audio()`):** processors that edit the guitar signal before pitch detection, fed by a mod that hooks the audio driver. See [`docs/audio-input.md`](docs/audio-input.md).

> For the history of the retired render-hook subsystem, and why the frame/reset callbacks don't need its
> machinery, see [`docs/render-hooks.md`](docs/render-hooks.md).

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
cadences: key events every command-dispatch pass, settings closures on the maintenance tick.

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

The framework has no game or Windows dependencies, so it is unit-tested in isolation. Each
`Tests/*Tests.cpp` is a standalone console program with its own `main()` that returns non-zero on
failure; a new suite also needs an entry in the `$Tests` list in `BuildAndRun.ps1`.

Build and run them all with:

```powershell
DLL/Framework/Tests/BuildAndRun.ps1
```

The script locates MSVC via `vswhere`, compiles each test against only the framework translation
units it needs, and runs it. AppVeyor runs the same script as a `test_script` step (`appveyor.yml`),
so a failing test fails the build (and skips deploy) and the tests can't rot out of compilation.
