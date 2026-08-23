# Resource ledger

Shared arbitration surface between the mod registry and CrowdControl effects so neither system can
silently clobber the other's game-state mutations.

## How it works

- The **registry** calls `Ledger().HeldExcluding(kRegistryOwner)` each tick to seed the conflict
  resolver with CC's live claims, then `Ledger().Publish(kRegistryOwner, winnerResources)` to
  advertise its own winners.
- **CC effects** call `Ledger().TryClaim(this, ClaimsExclusive())` on `Start` (via the non-virtual
  `CCEffect::Start` wrapper) and `Ledger().Release(this)` on `Stop` (via `CCEffect::Stop`).
- Cross-boundary: a mod holding `string-colors` blocks a CC effect that wants it; a running CC
  effect blocks the mod (it loses the next resolve and yields). **First holder wins**; no priority
  across the boundary.

## Resource catalog

| Resource        | Claimed by (mods)               | Claimed by (CC effects)               |
|-----------------|---------------------------------|---------------------------------------|
| `string-colors` | ExtendedRangeMod                | rainbowstrings, invertedstrings       |
| `note-visuals`  | -                               | rainbownotes, solid*, removenotes, transparentnotes, big/small noteheads, wavynotes |
| `tone-slot`     | -                               | changetoneslot1–4, shuffletones       |
| `scroll-speed`  | -                               | half/double/triplescrollspeed         |
| `zoom`          | -                               | zoomin, zoomout                       |
| `player-volume` | -                               | killguitarvolume, killmusicvolume     |
| `song-speed`    | RiffRepeaterMod                 | turbospeed                            |
| `instrument`    | -                               | removeinstrument                      |

`removeinstrument` claims both `instrument` and `string-colors`.

`VolumeDisplayMod` intentionally has no claim: it handles all seven Wwise channels via keybinds
and deactivating it entirely would be too broad; the CC effects' own `player-volume` claim is
sufficient to document the contention.

## The one accepted race

Registry does read-seed → compute-unlocked → publish. A CC claim landing mid-compute is missed
until the next tick (≤250 ms). Self-heals; a one-tick overlap in volume/speed/colour is a cosmetic
glitch, not a crash.

## Key files

- `Framework/ResourceLedger.hpp` / `.cpp` - the ledger class and `Ledger()` singleton
- `Framework/Tests/ResourceLedgerTests.cpp` - 14 unit tests (standalone, no `windows.h`)
- `CC/CCEffect.hpp` / `.cpp` - non-virtual `Start`/`Stop` wrappers that own claim/release
- `Framework/ModRegistry.cpp` - `Tick` seeds from ledger; `Shutdown` releases
