# Porting DropPedal (PR #233) onto the framework

> **Shelved (2026-09-26).** We are not porting #233. The fork (Cheesewizard/RSModsPlus) has since
> folded DropPedal into its Audio Bridge stack: the cable engine's `SetParam` hooks were removed in
> `53bf1fb8` and the engine switch in `1eb40754`, so the only standalone version is the August snapshot,
> which nobody maintains upstream. The ASIO tap would also make RSMods an RS_ASIO support desk; l0fka
> suggested that work belongs in their rs-asio fork. Kept because the table below lists what an
> out-of-tree mod needs from the framework, which is useful input for plugin support
> (`plugin-distribution.md`).

PR #233 (branch `pr233`, merge-base `ac1702c`) predates the mod framework: DropPedal hangs off
`ModManager` and edits a handful of core files. This note records which of those edits the framework
now absorbs, what is still missing, and the one product decision the port needs. It was written after
the device-event and key-binding work (September 2026), so "now" means that state.

## What the PR touches in core

| PR edit in core | What replaces it |
|---|---|
| `D3DOverlay.cpp` renders `DropPedal::Overlay` with GameOverlay's font | `c.Hud().Set(...)`: a tuning line plus the engine notice. The notice fades: republish it with a lower alpha each tick and call `c.RequestFastTick()` (~30 Hz) while it fades. |
| `dllmain.cpp` adds a 15 ms hotkey thread, because the 250 ms loop missed key taps | Not needed. Key events wake MainThread through the inbox, so `c.Commands().BindSetting(...)` sees every tap. `DropPedalInput`'s `GetAsyncKeyState` polling goes away; its 150 ms debounce before pushing pitch stays in the mod. |
| `ModManager::InitializeMods` loads settings, installs the ASIO tap and the cable hooks | `OnInitialize`. It runs at the same point in startup (after `InstantiatePending`, before the first tick). |
| `ModManager` tick: `DropPedal::Poll`, `AsioHook::Poll`, engine arbitration, pre-song tuner handling | `OnTick` / `OnMenuTick` |
| `Settings.cpp`: `EnableDropPedal`, `Engine` | Schema `Toggle` and `EnumChoice` (`automatic`, `asio`, `cable`) in section `Drop Pedal`. The GUI renders them. |
| `Settings.cpp`: five key defaults (`VK_OEM_COMMA`, `VK_OEM_PERIOD`, `VK_F7`, `VK_F9`, `VK_F10`) | `KeyBind(...)` in the mod's `Settings()`; see `settings-schema.md`. Both name tables already know the OEM keys. |
| `GUI/*.cs` (WinForms) | Deleted with WinForms. The manifest drives the settings screen and the keybindings page. |
| `stdafx.h` gains `audioclient.h`, `mmreg.h` etc. | Include them in the mod's and `Audio/` files. |
| Cable engine detours (`SetParam` vtable spy, `SpyTerm`, reference-builder detour) | Stay mod-owned hooks, like `UltrawideRRDim`. No framework involvement. |

## Still missing

### 1. ASIO input tap: take it in as a registry

`DLL/Audio/` (`AsioHook`, `ComVTable`, `CaptureFormat`, `IInputProcessor`, `DelayLinePitchShifter`) is
generic and belongs in core. But `AsioHook::SetProcessor(IInputProcessor*)` takes exactly one
processor, so a second audio mod would have to edit it. That is the D3DHooks problem again, and it is
cheapest to solve before the first tenant lands.

Proposed shape:

- `c.Audio().AddInputProcessor(IInputProcessor&, int order)` from `OnInitialize`. The host installs
  `AsioHook` on the first registration. It must be installed before RS_ASIO loads the driver module;
  `OnInitialize` is the same stage the PR used.
- **Freeze the chain when `createBuffers` fires** (about 40 s after install). `Prepare(CaptureFormat)`
  runs for every processor then, and the audio callback walks a fixed array with no atomics, locks or
  refcounts. A registration after the freeze is logged and ignored. DeviceChannel's
  `atomic<shared_ptr>` snapshot is not suitable here: MSVC implements it with a spinlock, which has no
  place on the ASIO thread.
- **Latency is fixed at freeze.** Rocksmith calibrates input latency once, so the chain's total
  `GetLatencyFrames()` must not change. A processor that turns off bypasses itself (its own atomic
  flag) and keeps reporting the same latency. It never unregisters.
- Readiness: the host polls `AsioHook::Poll()` from its own tick, and mods read
  `c.Audio().InputTapLive()` instead of calling `AsioHook` directly.
- DropPedal's cable/ASIO arbitration (`SetInputShifterActive` ordering, `inputShifterActive`) stays in
  the mod. Only the tap is shared.
- As with draw interceptors, nothing catches a processor that throws. On the audio thread that means a
  crash, so the rules in `IInputProcessor.hpp` (no allocations, locks, logging or blocking in
  `Process()`) are the contract.

### 2. True tuning vs MIDI auto-tune: needs a decision

The cable engine writes the game's true-tuning value so note detection expects the player's physical
pitch. The PR therefore changed core `SongTuning::GetTrueTuning()` to ask DropPedal for the authored
value first. Its only callers are MIDI auto-tune (`Midi.cpp`, song and tuner paths).

- **Option A: exclusive.** DropPedal also claims `"tuning-controller"`, which `MidiMod` already holds,
  so they never run together and the `SongTuning` edit is unnecessary. This is probably right, since
  both at once shift pitch twice (software plus the pedal), and `ConflictResolverTests` already models
  this exact contest. Prerequisite: split `MidiMod`. Its device scan and MIDI-in listener don't touch
  tuning but would be suppressed along with it when DropPedal wins.
- **Option B: coexist.** Core gets a small generic slot for the authored true tuning, which DropPedal
  publishes before writing its own value and `GetTrueTuning()` returns. That is still one core edit,
  just one that doesn't name a mod.

### 3. Small items

- `Offsets.cpp` gains `func_tuningReferenceBuilder`. `VersioningStruct` works from the mod's own file,
  so central vs mod-local is a policy choice. Central matches how CRCs are kept.
- New files still mean editing `DLL.vcxproj`/`.filters`. Globbing `Mods/**/*.cpp` there would remove it.
- `SongTuning::TryGetTrueTuning` (the PR's validated read) is a reasonable core helper on its own merits
  and can land independently.

## Suggested order

1. Decide #2 (A or B). For A, split `MidiMod` first.
2. Land `DLL/Audio/` as the input-tap registry (#1), with the pitch shifter as the first processor.
3. Rewrite DropPedal as an `IMod`: its `Settings()` holds the toggle, engine and five keys; commands
   replace `DropPedalInput`; `c.Hud()` replaces `DropPedalOverlay`; the cable hooks go in `OnInitialize`.
