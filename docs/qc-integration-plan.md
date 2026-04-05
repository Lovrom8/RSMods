# Quad Cortex MIDI Integration Plan (Feasibility)

## Scope
- Target: RSMods runtime automation for Neural DSP Quad Cortex over USB MIDI.
- Phase goal: scene switching on tone changes, then transpose automation for tunings that are representable as a single global semitone shift.
- Non-goals (v1): mixed-string offsets, drop/open tunings, per-string pitch logic.

## Current RSMods capabilities (confirmed)
- MIDI transport already exists in `DLL/Mods/Midi.cpp`:
  - Device discovery and selection (`FindMidiOutDevices`, `FindMidiInDevices`).
  - Outbound Program Change and Control Change (`SendProgramChange`, `SendControlChange`).
  - Async send loop in `DLL/dllmain.cpp` (`MidiThread` using `sendPC/sendCC` flags).
- Song state and lifecycle are available:
  - In-song/menu gating in `DLL/GameState.cpp` + `DLL/GameState.hpp`.
  - Song key tracking from `Play_*` preview events (`GameState::GetSongKey`).
  - Per-tick orchestration in `DLL/ModManager.cpp` (`HandlePostGameLoadedMods`, `HandleInSongState`).
- Tuning logic is available in `DLL/SongTuning.cpp`:
  - Current 6-string offsets, true tuning Hz, drop/standard checks.
  - Current MIDI auto-tuning entry point `ModManager::HandleMidiAutoTuningInSong`.
- Config/UI pipeline already supports live updates:
  - `RSMods.ini` parse/write in DLL and GUI (`Settings.cpp`, `ReadSettings.cs`, `WriteSettings.cs`, `UI.cs`).
  - GUI->DLL update channel via `WM_COPYDATA` (`GUI/Util/WinMsgUtil.cs`, `DLL/Keybindings.cpp`).

## Current RSMods gaps for this feature
- No known live tone-change detector in RSMods (no current tone-slot index read, no tone-change callback).
- Existing tone-slot code (`CC/Effects/ChangeToToneSlot.cpp`, `ShuffleTonesEffect.cpp`) sends keypresses to force a tone; it does not observe tone changes.
- Existing MIDI automation path is tuning-focused (Whammy/software pedal), not scene/tone focused.

## RockSniffer capabilities (confirmed)
- Live memory readout includes:
  - `songID`, `arrangementID`, `songTimer`, `gameStage`, note stats (`RSMemoryReadout` + `RSMemoryReader`).
- PSARC metadata parsing includes:
  - Arrangement list with `arrangementID`, type, sections/phrases, tuning (`PSARCUtil`, `ArrangementDetails`, `ArrangementTuning`).
- Model availability for tone-related metadata:
  - Manifest model includes `Tone_A/B/C/D/Base` fields (`SongArrangement` model).
  - SNG model includes tone switch events (`SngAsset.Tones` with `Time` + `ToneId`).
- RockSniffer currently does **not** expose tone names/slot mapping or live current tone in its emitted `SongDetails`/`RSMemoryReadout` payloads by default.

## Feasibility summary
- **Scene switching is feasible** in RSMods, but the key unknown is reliable live tone-source detection.
- **Transpose automation is feasible** in RSMods for single global semitone tunings; tuning primitives already exist.
- If live tone index cannot be read reliably from RS memory quickly, the safest fallback is deriving current tone from arrangement tone timeline (`SNG Tone[]`) + live song timer + arrangement ID (already available in RockSniffer path, not yet in RSMods).

## Recommended v1 architecture
- Add a dedicated runtime module in RSMods (e.g. `Mods/QCAutomation.*`) with a small state machine:
  - Active only when in song.
  - Sends MIDI only on change (debounced/idempotent).
  - Separates scene logic from transpose logic.
- Reuse existing MIDI send path (`Midi::SendControlChange`) with configurable channel/CCs.
- Add a `QCAutomation` settings group in `RSMods.ini` + GUI controls, but keep defaults conservative (disabled by default).

## Phase plan

### Phase 1: QC scene switching
- Add feature flags and MIDI settings:
  - enabled, output device, channel, scene CC (default 43), scene values.
- Implement tone-source abstraction:
  - source A: live slot index (if pointer found).
  - source B fallback: timeline-derived tone index (requires arrangement tone event metadata).
- Map tone source to scene command:
  - Initially by user slot mapping (slot1..4 -> clean/overdrive/distortion/lead), not by assumed semantic tone IDs.

### Phase 2: Transpose automation
- Add transpose settings:
  - enabled, CC selector (1 or 2), semitone min/max mapping.
- Compute a single global semitone offset from current tuning.
- Reject unsupported tunings explicitly:
  - mixed string offsets, drop/open/custom non-uniform tunings.
- Send CC only when offset changes; reset on song exit/menu.

### Phase 3: Mapping/polish
- Add per-song/per-arrangement overrides keyed by song key + arrangement id.
- Optional tone-name-assisted mapping (if manifest tone names are available in runtime path).
- Improve diagnostics:
  - log reason for unsupported transpose.
  - optional on-screen status for current scene + transpose value.

## Candidate RSMods files to modify
- Runtime wiring:
  - `DLL/ModManager.cpp`, `DLL/ModManager.hpp`
  - `DLL/Settings.cpp`, `DLL/Settings.hpp`
  - `DLL/Offsets.hpp`, `DLL/Offsets.cpp` (only if new memory pointers are added)
  - `DLL/Mods/Midi.hpp`, `DLL/Mods/Midi.cpp` (only for generic QC send helpers/reuse, not to entangle with Whammy logic)
- New runtime module:
  - `DLL/Mods/QCAutomation.hpp` (new)
  - `DLL/Mods/QCAutomation.cpp` (new)
- GUI/config:
  - `GUI/ReadSettings.cs`
  - `GUI/WriteSettings.cs`
  - `GUI/UI.cs`
  - `GUI/UI.Designer.cs`
- Optional docs:
  - `README.md` settings table updates.

## Major risks / unknowns
- Reliable live tone source inside RSMods is not confirmed yet.
- Mapping `SNG ToneId` to semantic slots (A/B/C/D/Base) needs validation in practice.
- Menu/state edges (Riff Repeater seeks, pauses, song restarts) can cause duplicate or stale MIDI sends if state transitions are not carefully debounced.
- QC expression mapping ranges vary by user preset setup; must be configurable and documented.

## Recommendation: direct implementation vs helper prototype
- **Recommended path:** implement directly in RSMods first, but start with a contained discovery spike for tone-source acquisition.
- **Fallback path:** if tone-source acquisition is blocked, prototype timeline-based tone resolution using RockSniffer data flow, then port the proven logic into RSMods.

## Recommended first coding task
1. Add a compile-safe `QCAutomation` skeleton module and call site from `ModManager::HandleInSongState`.
2. Add minimal INI settings + GUI toggles for:
   - feature enable
   - MIDI out device + channel
   - scene CC/value map
3. Implement a no-op state machine that only logs candidate tone source and sends a single test scene change once per song start (behind explicit setting).

This de-risks integration plumbing before solving tone-source detection.
