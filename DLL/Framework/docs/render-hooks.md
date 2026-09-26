# Render hooks: removed (design record)

> **Status: removed.** The framework once shipped a render-callback subsystem
> (`HostHooks.{hpp,cpp}`, `ctx.Render().OnEndScene(...)`). It had **zero tenants** and was deleted
> to simplify the registry. This note preserves *what it was* and *the one condition under which it
> should come back*, so the idea isn't lost to git archaeology if we ever go full on owner-scoped
> rendering.
>
> The real code is a `git revert` away, find the removal commit with:
> `git log --oneline --all -- DLL/Framework/HostHooks.cpp`

> **Update (Ultrawide, PR #240):** render-side tenants now exist: Ultrawide's per-frame state and its
> caches keyed by D3D pointers, plus Twitch's hand-wired `RunPerFrameEffects`. So a narrow form is back as
> `DrawRegistry` frame and device-reset callbacks (`draw-registry.md` §8.7).
> It does **not** bring back `Deactivating` or quiescence. These callbacks follow the interceptors'
> lifetime rule instead: they are gated by the same active snapshot, and device resources are freed
> only on the render thread through `RequestTextureRelease`. So `OnDisabled` never frees anything a
> callback in flight could still touch. If a future tenant needs to free render resources
> synchronously on disable, that is the point to reconsider the machinery below.

## What it was

A way for a mod to own a **per-frame draw callback** with framework-managed lifetime:

```cpp
void OnInitialize(ModContext& ctx) override {
    ctx.Render().OnEndScene([this](IDirect3DDevice9* device) { /* draw */ });
}
```

Properties the subsystem guaranteed:

- **Owner-scoped.** A subscribed callback fired *only while its mod was effectively active*, and was
  dropped on shutdown. The mod never had to gate the callback itself.
- **Snapshot-dispatched.** `DispatchEndScene` (render thread) iterated an immutable published
  snapshot of subscriptions + an active-set snapshot, holding no lock across mod code.
- **Fault-isolated.** A throwing callback was caught (never crossed the D3D hook); the registry tore
  that mod down on the next tick and dropped its subscription.

## Why it needed the machinery it did

Because a callback runs on the **render thread** while lifecycle/tick hooks run on **MainThread**, a
mod leaving `Active` could not simply run `OnDisabled` because an in-flight callback might still
touch state that `OnDisabled` was about to free. To make that safe the registry carried:

- a **`Deactivating`** state: a mod that left `Active` but still had callbacks in flight parked here;
- **quiescence tracking** (`inFlightByMod`) + **deferred teardown**; `OnDisabled` was postponed until
  the render hooks reported the mod quiescent;
- a **render-fault path** and a **shutdown busy-wait** that blocked until every callback drained.

That is the single most intricate part of the old registry, and it existed **entirely** to protect
owner-scoped render callbacks.

## Why it was removed

No mod ever subscribed. Every visual mod (Volume Display, Song Timer, Riff Repeater, GuitarSpeak,
the MIDI auto-tune target, song accuracy, **and DropPedal's own bespoke status overlay**) renders
through the shared `GameOverlay` HUD instead: they borrow `GameOverlay`'s cached `ID3DXFont`, own no
device resources, and (in DropPedal's case) publish their state via `std::atomic`. A HUD element
drawn from process-lifetime shared resources has **nothing to tear down on deactivate**, so the
`Deactivating`/quiescence apparatus guarded a case that never occurred.

Deleting the subsystem collapsed `Deactivating` out of the lifecycle (Active reverts synchronously),
and removed deferred teardown, the render-fault path, and the shutdown busy-wait.

## The one condition to bring it back

Reintroduce owner-scoped render callbacks **only** for a mod that **owns render resources with an
enable/disable lifecycle**. For example, the mod might allocate textures/vertex buffers on enable and
free them on disable, such that a callback firing mid-teardown would touch freed memory. That is the
exact hazard `Deactivating`/quiescence solved, and the only thing that justifies their cost.

If the need is merely "draw some text/state on screen," it is **not** this. Add a HUD element to the
shared `GameOverlay` surface instead (see the shared-HUD overlays, and DropPedal's `Overlay` as the
reference pattern: shared font, atomic state, no owned resources). The contributor-facing render
story should be a clean HUD-element registration, not a raw D3D callback.