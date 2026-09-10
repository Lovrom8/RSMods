# Plugin distribution & the trust boundary

> **Status: direction, not committed.** This records how third-party mods should be allowed to ship and
> what stops replacing maintainer review as the trust boundary — and, more importantly, how to sequence
> that change so it doesn't tax a framework that is still moving. It revisits the `abi-shim` decision
> (reviewed in-tree source) in light of "if people want plugins in their own repos, let them."

## The one question is really three

"Let external devs ship plugins from their own repos" bundles three decisions with very different costs.
Keeping them separate is the whole point of this doc.

1. **Out-of-tree distribution** — a mod lives in its author's repo instead of `DLL/Mods/`.
2. **Runtime binary loading** — the host `LoadLibrary`s separate `.dll` files instead of the mod being
   compiled into `RSMods.dll`.
3. **Trust boundary** — maintainer review of source vs. user-assumed risk.

Only #2 is expensive, and #3 is not the free choice it looks like. Treat them as one ("let them have
it") and you pay #2's cost to get #1's benefit, and back into #3 without deciding it.

## #1 — Out-of-tree distribution: cheap, do it

A mod is one `.cpp` implementing `IMod` plus a `ModRegistrar`. Nothing about that requires it to live in
this repo. Authors can keep mods in their own repos and pull them into a build (a submodule, a
`Mods/external/` glob, a documented drop-in). This delivers most of "their own repo" with **zero contract
commitment**: the mod still compiles against the current `IMod`/`ModContext`, so the framework stays free
to change. The blockers are the same two seams that block clean in-tree contribution — the settings
schema (`settings-schema.md`) and globbing `Mods/*.cpp` into `DLL.vcxproj` — not anything about trust.

## #2 — Runtime binary loading: the expensive one, and the real subject of the old deferral

Loading a separate binary means `IMod`/`ModContext` can no longer be the boundary. They are full of
`std::vector<std::string_view>`, `std::function`, and `ModContext&` — C++ types that do **not** survive a
DLL boundary across different compilers, STL versions, or build flags. The result of pretending they do
is silent ABI mismatch and crashes. A real binary boundary needs a **versioned C function-table shim**,
and some things structurally cannot cross it — the raw D3D device handle, which is why owner-scoped
render callbacks stay deleted (`render-hooks.md`).

Building the shim is not the hard part. **The hard part is that a published binary contract freezes
`ModContext` while it is still being reshaped.** Recent and pending changes make the point: RenderHooks
was deleted, `Registry().Tick` lost its `GameLoopState&`, and `Settings()` is about to be added. Every
one of those is free today and becomes a breaking-change negotiation with strangers the moment an ABI
ships. The original deferral (README, `abi-shim`) was about **timing**, not principle: freeze a contract
that has stopped moving and the freeze is cheap; freeze one mid-flight and it taxes every future refactor.

## #3 — The trust boundary: for native in-process code you can't keep review as *enforcement*

This is the decision hiding inside #2. The honest constraint:

> There is no in-process sandbox for a native plugin. A DLL injected into the game — in a codebase that
> already uses Detours and raw `WriteProcessMemory` — can ignore `ModContext` entirely and do anything
> the process can: patch game code, open sockets, touch the filesystem, all with the user's account
> privileges.

The capabilities layer is decoupling-only for exactly this reason (`abi-shim`): it cannot enforce
against a binary that declines to use it. So **the moment #2 is allowed, review has already stopped being
a technical boundary**, whether or not that was decided. What review can still be is a *social* signal.
The replacement for "we audited this" is necessarily "you chose to trust this author," i.e. user-assumed
risk — the mod-loader model (SKSE, BepInEx, Nexus). That model is fine and proven; it just has to be
entered deliberately, not backed into.

One project-level note that argues against a *silent* free-for-all: this is native code injected into a
game, distributed on modding sites, running with full user privileges — not a marketplace extension with
some sandboxing. "RSMods plugins can silently be malware" is a hit to the **project's** reputation, not
just a risk to one user. That is a reason to keep a visible trust signal, not a reason to refuse #2.

## The move: review as a badge, not a gate

Keep review — stop making it a bottleneck. Concretely, once #2 exists:

- The loader shows the user **who signed** a plugin and whether it is on the project's **verified** list.
- Verified = someone read the source (the old bar), now surfaced as a *badge* instead of a merge gate.
- Default is **load-anything-behind-a-warning**: unverified plugins run, the user is told they are
  unaudited.

This gives independent repos and no maintainer merge gate (what "let them have it" is reaching for),
while not staking the project's name on unaudited native code. Review changes role; it does not disappear.

## Recommended sequence

1. **Now — make the *source* surface zero-core-edit.** Land the settings schema and the `vcxproj` glob so
   an out-of-tree **source** mod (their repo → build) needs no core edits. This is #1 with nothing frozen.
2. **Bake the API on real consumers.** Pull DropPedal and Cheesewizard's RE in *that* way. Two or three
   real external mods is the signal that `ModContext` has stopped moving.
3. **Then cut #2 and #3 together.** Once the API is boring, ship the versioned C ABI shim **and** the
   signed/verified loader model in the same step — because #2 forces #3, so decide them as one. Freezing a
   contract that has already stopped changing is the cheap version of this.

## What this doesn't change

- **In-tree mods stay first-class.** Nothing here removes `DLL/Mods/`; built-ins and verified contributions
  keep compiling straight in. The ABI is an *additional* surface for people who want independence, not a
  replacement for the internal C++ API.
- **`IMod`/`ModContext` remain internal and unfrozen** until step 3. Do not treat them as an ABI before
  then (README design boundary still stands).
- **Capabilities stay decoupling-only.** They organize the surface and shrink review diffs; they are not
  and cannot be a security boundary against a binary (`abi-shim`).
