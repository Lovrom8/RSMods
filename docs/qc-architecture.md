# QC Automation Architecture Notes

This note describes how QC Automation is wired in RSMods today.

## Runtime Model

The QC layer tracks three effective intent modes:

- `AutoAuthored(name)` from authored tone timeline/tone base resolution.
- `ManualSlot(2|3|4)` from live manual tone-slot override detection.
- `IdleDefaultSlot(2)` when not in-song.

Manual slot `1` is treated as return-to-auto (clear manual override), not as a manual target.

## Main Flow

1. RSMods song state enters in-song.
2. Active song/arrangement metadata is resolved from installed assets.
3. Baseline authored tone name is derived from playback time.
4. Classifier maps authored tone name to QC target role.
5. MIDI preset recall is sent only when effective QC target changes.
6. Transpose scene (v1 semitone model) is song-level and sent separately from tone role.

## Key Components

- `DLL/Mods/QCAutomation.*`
  - state machine, classifier, target mapping, MIDI dispatch.
- `DLL/Mods/ToneSource.*`
  - runtime tone-source hooks and manual override signal handling.
- `DLL/Mods/Midi.*`
  - MIDI device/session and raw message send helpers.
- `DLL/GameState.*`
  - song/arrangement/time context used by QC logic.

## Mapping + Classifier

- Classifier uses case-insensitive substring matching.
- Bucket priority is configurable in the GUI and stored in `[QCAutomation]`.
- Bass guard can short-circuit guitar auto classification.
- QC target strings use `MyPresets:<bank><slot>` format (for example `MyPresets:32A`).

## Transpose v1

- Song-level semitone -> scene mapping:
  - `A=0, B=-1, C=-2, D=-3, E=-4, F=-5, G=-6, H=-7`
- Scene MIDI uses `CC#43` values `0..7`.
- Scene logic is decoupled from frequent tone-change preset recalls.

## Known Boundaries

- No true-tuning/cent automation in v1.
- No RockSniffer runtime dependency in core live switching path.
- Manual override path is slot-based; tone-name semantics remain on authored auto path.
