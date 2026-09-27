# Hook watchdog

> **Status: built and active.** Every mod hook dispatched by the registry is timed; a hook that runs
> over budget on `MainThread` logs a throttled warning. Pure decision core in `HookWatchdog.hpp/.cpp`,
> unit-tested in `Tests/HookWatchdogTests.cpp`. A hook or command that never returns is reported by
> the stall monitor instead (see [Stall monitor](#stall-monitor) below).

## The problem

`MainThread` is shared. The same loop that ticks every active mod also drains keybind commands and
settings closures (see `README.md` → *Main-thread inbox*). A mod hook that blocks doesn't just delay
itself: it freezes keybind dispatch and every other mod's tick, **including the command that would undo
the wait**. This is not hypothetical — a rainbow effect that looped on `MainThread` made itself
impossible to toggle off, because the toggle command couldn't dispatch while the hook held the thread.

The framework already *documents* the rule ("hooks must not block; hold a `steady_clock` deadline and
return early") but nothing observed a violation. A slow hook shipped silently until a user noticed the
game had stopped responding to input.

## What it does

`ModRegistry::Impl::Invoke` is the single chokepoint every `void(ModContext&)` hook passes through —
`OnInitialize`, `OnEnabled`, `OnDisabled`, `OnSettingsChanged`, `OnTick`, `OnMenuTick`, `OnSongEnter`,
`OnSongTick`, `OnSongExit`, `OnShutdown`. It wraps each successful call in a `steady_clock` measurement
and hands the elapsed time to a `HookWatchdog`. When a hook exceeds the budget the registry logs a
`WARNING` naming the mod, the hook, and how long it took.

```
[WARNING] [Framework] RainbowMod::OnTick took 812 ms on MainThread (budget 50 ms);
          hooks must not block - keybind dispatch and every other mod's tick stall while it runs
```

Bounds (`ModRegistry.cpp`):

| Setting     | Value  | Why                                                                      |
|-------------|--------|--------------------------------------------------------------------------|
| Budget      | 50 ms  | Far above any honest per-tick hook, so a warning means real trouble.     |
| Cooldown    | 5 s    | A hook slow every 250 ms tick would otherwise flood the log.             |

Repeated breaches at the same site inside the cooldown are counted, not logged; the next warning after
the cooldown reports how many it silenced:

```
[WARNING] [Framework]   (19 more slow runs of RainbowMod::OnTick were suppressed since the last warning)
```

## What it deliberately does not do

- **It cannot catch a hook that never returns.** `Observe` runs *after* the call completes, so a true
  infinite loop (the original rainbow deadlock at its worst) blocks `MainThread` and the watchdog line
  never prints. That case belongs to the [stall monitor](#stall-monitor). The watchdog is the leading
  indicator: it flags the slow-but-returning hooks that are one edit away from the full stall, and the
  bounded-but-long ones (a sleep, a synchronous wait with too generous a deadline, heavy per-tick work).
- **It does not fault or disable the mod.** A slow hook is a warning, not a crash; the mod keeps running.
  Faulting is reserved for hooks that *throw* (see the lifecycle failure policy in `README.md`).
- **It is not a profiler.** One budget, one line. It answers "is a hook stalling MainThread?", not
  "where did the frame time go?".

## Design

Mirrors `ConflictResolver` / `ResourceLedger`: a **pure, host-agnostic decision core** with no timing
and no logging of its own, so it unit-tests without a clock or a game.

- `HookWatchdog::Observe(owner, where, elapsed, now)` returns a `Breach` to log, or `nullopt` when the
  run is under budget or still inside the site's report cooldown. The registry supplies the measured
  `elapsed` and the call's end timestamp as `now`; the watchdog owns only the per-site suppression state.
- A **site** is `(mod pointer, hook-name literal)`, so each hook of each mod throttles independently.
- `Forget(owner)` drops a mod's sites when it faults or shuts down, matching the other registries'
  `RemoveMod`, so a recycled address can't inherit stale suppression state.

Cost is two `steady_clock::now()` reads per hook (QPC, tens of nanoseconds) — negligible against a
250 ms tick, so it stays on in release builds, which is the point: it exists to catch a regression in
the field, not only in a debugger.

## Stall monitor

Covers the case the watchdog can't: a call on `MainThread` that never returns, so nothing is left to
log it. Pure core in `StallMonitor.hpp/.cpp`, unit-tested in `Tests/StallMonitorTests.cpp`; wiring
checked in `Tests/StateMachineTests.cpp`.

- `MainThread` brackets every mod call with a `StallMonitor::Scope`: each hook in
  `ModRegistry::Impl::Invoke` and each command in `CommandRouter::Impl::InvokeBinding`. Enter records
  the mod Id, hook or command name and entry time under a mutex; Leave clears them.
- `StallMonitorThread` (`dllmain.cpp`, host code) calls `Stalls().Poll(now)` once a second and logs
  what it returns. A call running 5 s or longer is reported once:

  ```
  [ERROR] [Framework] RainbowMod::OnTick has been running for 5 s; MainThread is stuck - keybinds and
          every other mod are frozen until it returns
  ```

  If it later returns, one more line closes the incident:

  ```
  [WARNING] [Framework] RainbowMod::OnTick returned after 12034 ms; MainThread is running again
  ```

- It only names the culprit; it doesn't interrupt the call or fault the mod. A native hook can't be
  safely interrupted from another thread.
- `Poll` copies the names when it reports, since the views passed to Enter die with the call.
- Host code on `MainThread` outside the registry (`ModManager`, the settings closures) isn't covered;
  it has no mod to name.
- Cost is two uncontended mutex locks per mod call, next to the watchdog's clock reads.
