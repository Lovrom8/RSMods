# HUD registry

A decoupling-only surface for on-screen text, so mods stop poking `GameOverlay::` globals directly. It
is the first slice of the "clean, contributor-facing extension surface" the framework exists for, minus the binary-DLL plumbing.

## What it is (and isn't)

- **Is:** an owner-scoped registry mirroring `CommandRouter`. A mod publishes a text element through
  `ctx.Hud()`; the registry holds the latest snapshot; the renderer draws it. The element is torn down
  with the mod, so no orphaned globals survive a fault or a disable.
- **Isn't:** an enforcement boundary. An in-tree mod can still write `GameOverlay::` globals or draw its
  own way. Review is the trust boundary (see `abi-shim` decision) — the registry just makes the clean
  path the obvious one and shrinks the review diff.

## Threading: push, not pull

The renderer runs on the D3D render thread (`Hook_EndScene`); mod ticks run on MainThread. To keep mod
code off the render thread entirely, publishing is a **push snapshot**, not a pull callback:

- MainThread: `ctx.Hud().Set(id, { anchor, order }, HudText{visible, text, colorHex, fontHeight})` each tick.
- Render thread: `GameOverlay::DrawModHud` calls `Hud().SnapshotVisible()`, which copies the visible
  elements out under the lock, then draws from the copy. The lock is never held across a DX9 draw, and
  `HudElement::owner` is an opaque key that is **never dereferenced** on the render thread — so there is
  no lifetime coupling to the mod object.

Anchors (`TopLeft` / `TopCenter` / `TopRight`) resolve to a stack position from the live window size on
the render side, so mods stay out of pixel math and text lands in the same spot at every resolution. The
insets match the hand-written overlays (TopLeft = volume popup + mixer, TopRight = song timer, TopCenter
= RR speed), so ports are pixel-for-pixel. Multiple elements at one anchor stack in `order` — see *Stacked
anchors* below.

## Lifecycle

Elements are cleared on **every** deactivation (disabled / suppressed / faulted) and on shutdown, in
`ModRegistry::BeginTeardown` / `Fault` / `Shutdown`. This differs from command bindings, which persist
while a mod is Inactive so `Initialized`-availability keys keep working: a HUD element is meaningless
once the mod stops ticking, and the mod simply re-publishes on its next active tick (`Set` upserts).

## Consumer: the whole volume overlay

`VolumeDisplayMod` owns the entire volume overlay — both the transient "current volume" popup **and** the
held-key full mixer. It was the clean first consumer because the mod is always Active (so it ticks every
frame and can re-publish), and it now demonstrates the full surface: a single anchored line (popup) and a
multi-line stack (mixer). `GameOverlay::DisplayMixer` is gone; the render side is just `DrawModHud`. The
popup is suppressed while the mixer is held (the mixer already lists every channel), matching the old
`if mixer … else if popup` draw order.

## Consumer: the song timer (second mod, second anchor)

`ShowSongTimerMod` publishes the top-right song timer as a single `TopRight` element, so two different
mods now share `DrawModHud` and the registry is the only writer of both the `TopLeft` and `TopRight`
bands. `GameOverlay::DisplaySongTimer` and the `D3DHooks::showSongTimerOnScreen` global are gone; the
show flag is a private member. The port surfaced one lifecycle wrinkle worth knowing for future ports:

- The old mod was Active only in **automatic** mode and let its keybinding (`Availability::Initialized`)
  flip the global while Inactive in manual mode. That does not work with a HUD element, which is cleared on
  deactivation and only re-published by a ticking mod. So the mod is now Active whenever the timer is
  **enabled**, in both modes; automatic-vs-manual is a member it consults on the song edges (and on a
  mode flip in `OnSettingsChanged`), and the key binding became `Availability::Active`. Rule of thumb:
  **a mod that owns a HUD element must be Active for as long as the element can be visible.**
- The `h:m:s` formatter moved from `D3DHooks` to `SongTimer::FormatTime`, so the mod includes only
  `Framework.hpp` + `SongTimer.hpp` — no D3D headers, same template shape as `VolumeDisplayMod`.

The mod also owns all of its **state**. The channel table, the selected-channel index, and the
show/expiry flags used to live as `GameOverlay::` globals that the mod reached back into; they are now
private members (see *Decoupling from GameOverlay* below), so `VolumeDisplayMod` includes only
`Framework.hpp` + `VolumeControl.hpp`, touches no `GameOverlay` and no Wwise, and is the honest template
an external mod would follow.

## Stacked anchors

Single-occupancy was a property of the PoC, not the design. The mixer is the first anchor with more than
one element, and it turns an anchor from *a fixed rect drawn once* into *a container* — a semantic region
whose elements are laid out in order. This is a render-side layout change plus one ordering field on the
element. The registry stays a flat bag and the threading model is untouched: the payload grew, the
concurrency model did not.

### Element model

`HudText` gained an optional per-line height; publishing takes a `HudPlacement` (anchor + stacking key):

```cpp
struct HudText {
    bool visible = false;
    std::string text;
    int colorHex = static_cast<int>(0xFFFFFFFF);
    int fontHeight = 0;   // 0 = shared default font; >0 = a custom-sized line (feeds DX9DrawText)
};

struct HudPlacement {
    HudAnchor anchor = HudAnchor::TopLeft;
    int order = 0;        // stacking key within the anchor; lower sits nearer the anchor edge
};

// HudBinder
void Set(std::string id, HudPlacement placement, HudText snapshot) const;
```

`order` defaults to 0 and `HudPlacement` brace-inits from a bare anchor, so a single-occupant consumer
publishes as `Set("current-volume", { HudAnchor::TopLeft }, snapshot)` and renders byte-for-byte as
before.

### Layout (render side)

`AnchorRect` (one fixed rect) became `AnchorStart` (where a stack begins + how it aligns), and
`DrawModHud` walks the elements:

1. `SnapshotVisible()` → the visible elements.
2. Sort by `(anchor, order, id, owner)` so groups are contiguous and deterministically ordered.
3. Walk each anchor group from its start inset, advancing a vertical cursor by each line's height
   (`fontHeight` if set, else the legacy line unit `WindowSize.height / 54.0f`). Top anchors grow
   **downward**; direction is a property of the anchor, so future `Bottom*` anchors grow up without
   touching callers.

The first line of any group starts at exactly the old `AnchorRect` inset, so single-occupant stacks are
pixel-for-pixel identical and the mixer reproduces its old `offset += h/54` spacing.

### Decisions taken

- **Tie-break:** the sort key is `(anchor, order, id, owner-pointer)`. `id` is deterministic; the owner
  pointer is only the final fallback for two *different* mods sharing one `(order, id)` at an anchor
  (stable within a run). Storing an `ownerId = owner->Id()` string for fully content-deterministic order
  was considered and **not** adopted — the pointer fallback is visually fine and nothing needs it yet.
- **TopLeft is shared** by the popup (`order 0`) and the mixer lines (`order 10 + index`) — distinct
  bands, so the one frame where both are visible stacks cleanly instead of colliding.
- **Dynamic line counts:** `Set` upserts and never removes, so the mixer publishes its hidden lines as
  `{ .visible = false }` each tick (the channel set is fixed, so ids are stable). A per-element
  `Remove(id)` was left unbuilt — no consumer needs true retraction.

### What this did not change

Push-snapshot publishing, copy-out-under-lock, teardown-clear-on-deactivation, and owner opacity on the
render thread all stand. Stacking is layout plus one field — no part of the concurrency or lifetime model
moved.

## Decoupling from GameOverlay

The framework *core* was never tied to `GameOverlay` (it builds standalone in the test harness); the tie
lived in the *mods* that reached into `GameOverlay::` globals for state. That is now gone for the volume
overlay:

- **View state → the mod.** `mixerChannels`, `currentVolumeIndex`, `displayMixer`, `displayCurrentVolume`,
  and `displayVolumeStartTime` moved out of `GameOverlay` into `VolumeDisplayMod` private members. The old
  `mixerChannels` table and the mod's parallel key-binding array merged into one `channels` table (the row
  index is the selected channel), dropping a duplicated 7-entry array and its `overlayIndex` field.
- **The volume query → `VolumeControl`.** Reading a channel's live Wwise volume was a render-thread call
  inside `DisplayMixer`; it is now `VolumeControl::CurrentVolume(channel)`, called on MainThread where the
  other volume ops live. The mod no longer touches Wwise directly.

The result: `GameOverlay` is back to being purely the DX9 render backend (device, font cache,
`DX9DrawText`, `WindowSize`, and `DrawModHud`), and all mods are self-contained. All bespoke `Display*`
functions have been retired, making `DrawModHud` the single overlay draw path in the entire codebase.

## Completed ports & consumers

| Overlay / Feature | Mod Owner | Anchor | Order |
|---|---|---|---|
| Volume popup | `VolumeDisplayMod` | `TopLeft` | 0 |
| Volume mixer | `VolumeDisplayMod` | `TopLeft` | 10 + index |
| RR song speed | `RiffRepeaterMod` | `TopCenter` | 0 |
| Looping display | `RiffRepeaterMod` | `TopCenter` | 10 |
| Song timer | `ShowSongTimerMod` | `TopRight` | 0 |
| Song accuracy | `SongAccuracyMod` | `TopRight` | 10 |
| Auto-tune tuning | `MidiMod` | `TopTuning` | 0 |
| In-song current note | `ShowCurrentNoteMod` | `HighwayLeft` | 0 |
| In-menu current note | `ShowCurrentNoteMod` | `MenuBanner` | 0 |

In addition, `HandleLooping` audio seeking (`Wwise::SoundEngine::SeekOnEvent`) and grey note timer manipulation
(`SongTimer::SetGreyNoteTimer`) were evicted from `RenderOverlay` on the D3D render thread to `RiffRepeaterMod::OnSongTick`
on the MainThread.

## Natural next steps

1. **Second declarative element type.** Text + color + font height covers every current overlay. The next
   contributor ask (a bar/meter, a coloured selection highlight in the mixer) is a new `HudText`-sibling
   variant drawn from shared resources — still push-snapshot, still off the render thread. Add it when a
   real consumer needs it, not before.
2. **`MenuBinder`** — the menu-registration sibling of `HudBinder`/`CommandBinder`. Deliberately deferred
   until a real menu consumer (likely DropPedal) exists; do not build it speculatively.
3. **Owner-scoped D3D callbacks** stay deleted (see `render-hooks.md`). They are structurally excluded
   from the external mod API (a raw device handle can't cross the ABI shim, and it forfeits the
   off-render-thread invariant) and revive **only** for an in-tree mod that owns device resources with an
   enable/disable lifecycle. The declarative surface above is the drawing API for everyone else.
