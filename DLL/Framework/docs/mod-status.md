# Mod status view

> **Status: built and active.** `ModRegistry` publishes a thread-safe per-mod status snapshot; the
> in-game "RS Mods" window renders it behind an opt-in toggle, with a button to retry faulted mods.
> Snapshot and retry logic are unit-tested in `Tests/StateMachineTests.cpp`.

## The problem

The registry knows a lot the user and a contributor can't see. A mod can be running, disabled by the
user, **suppressed** because it lost a resource conflict, or **faulted** because a hook threw — and
until now the only trace of any of that was a log line scrolling past at the moment it happened. The
`Suppressed`-vs-`Disabled` distinction in particular is thrown away: `ModState` collapses both into
`Inactive`, and the README notes it "survives only in the log line." So "why did my mod do nothing?"
had no answer you could look up.

## What it shows

The view is **opt-in**: a `Mod status` checkbox at the bottom of the `RS Mods` window enables it, off
by default and remembered only for the current game session. When on, it lists every registered mod with:

- **State**, color-coded: `Active` (green), `Suppressed` (amber), `Faulted` (red), `Disabled` (gray),
  `Registered` (blue, transient before init).
- **Detail**: whether it's active in a song, and the exclusive resources it claims — which is what
  explains a `Suppressed` line (it lost that resource to a higher-priority mod).
- A one-line summary of how many mods are active / suppressed / faulted.

## Design

Same split as the rest of the framework: a **host-agnostic data source** plus a thin host renderer.

- **`ModRegistry` owns the data.** A public `ModStatus` / `ModStatusKind` (in `ModRegistry.hpp`) and
  `StatusSnapshot()` expose state without leaking the internal `ModState`. `ModStatusKind` re-derives
  the disabled-vs-suppressed distinction: an `Inactive` mod that `IsEnabled()` this pass but isn't
  `Active` was suppressed; one that isn't enabled is disabled.
- **Published on MainThread, read on the render thread.** `PublishStatus` rebuilds the snapshot under
  a mutex at the end of every `Tick` (with the resolver's requested/selected masks, so the
  suppressed/disabled split is accurate) and once after `DispatchInitialize`. `StatusSnapshot()` takes
  the same mutex and returns a copy, so the D3D `EndScene` thread never touches live registry state —
  the same discipline `MenuRegistry::PublishAvailability` / `GetEntries` uses.
- **The view is host code behind an opt-in toggle.** `Menu.cpp` draws a `Mod status` checkbox in the
  `RS Mods` window (session-only `bool`, off by default) and renders the snapshot with Dear ImGui only
  when it's checked. The framework itself stays free of ImGui and DirectX.

## Retrying faulted mods

The view's one action: when any mod is `Faulted`, a `Retry faulted` button appears. It calls
`Registry().RequestRetryFaulted()`, which sets a flag the next `Tick` consumes on MainThread —
resetting every `Faulted` mod to `Registered` and re-running `OnInitialize`, exactly as
`DispatchInitialize` does at startup. A recovered mod settles into that same tick's resolution and
activates if enabled; a mod that throws again re-faults. Retry is best-effort: a mod that faulted in
`OnEnabled` may have left partial game state that re-initializing does not undo.

## What it deliberately does not do

- **Otherwise read-only.** Beyond retry it reports state; it does not enable, disable, or reconfigure
  mods.
- **Not a substitute for the log.** The log remains the record of *when* a transition happened and its
  exact reason string; the view is the *current* picture.

## Testing

`Test_StatusSnapshotReflectsStates` in `StateMachineTests.cpp` drives a registry through a resource
conflict (winner `Active`, loser `Suppressed`), a disabled mod (`Disabled`), and an `OnEnabled` throw
(`Faulted`), and asserts the snapshot's kinds, `inSong`, priority, claimed resources, and that it
empties after shutdown. `Test_RetryFaultedMods` faults two mods in `OnInitialize`, then after
`RequestRetryFaulted` + a tick asserts the one that stopped throwing recovers to `Active` while the
other re-faults. The ImGui rendering in `Menu.cpp` is host code and is not unit-tested.
