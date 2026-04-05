# QC Automation Tab Guide

This guide covers the **QC Automation** tab in `RSMods.exe` for Rocksmith 2014 + Neural DSP Quad Cortex.

## What It Does

- Auto-selects QC preset targets from authored Rocksmith tone names.
- Supports manual Rocksmith tone slot overrides (`2/3/4`) as dedicated QC targets.
- Supports an idle/default QC target when not in-song.
- Supports song-level transpose scene automation (v1 semitone model).

## General

- **Enable QC Automation**: turns the feature on/off.
- **MIDI Out Device**: select your Quad Cortex MIDI output port.
- **MIDI Channel (1-16)**: UI is 1-based; runtime uses the matching MIDI channel.
- **Ignore Bass Arrangements**: bass tones are ignored by the guitar classifier.

## QC Target Mapping

Format:

- `MyPresets:32A` = My Presets -> Bank 32 -> Slot A

You can edit these target fields:

- Auto Clean / OD / Dist / Mod / Solo
- Manual Slot 2 / 3 / 4
- Idle Target

## Auto Classifier Filters

- Keyword matching is case-insensitive and substring-based.
- Priority is user-configurable with numeric order (**1 = highest**).
- Current default order:
  - Overdrive
  - Distortion
  - Clean
  - Mod
  - Solo

## Transpose (v1)

- Song-level transpose scene automation.
- Scene mapping model:
  - A=0, B=-1, C=-2, D=-3, E=-4, F=-5, G=-6, H=-7 semitones
- Scene changes are tied to song context changes, not per-tone changes.

## Tools

- **Classifier Preview**: type a tone name and preview bucket + keyword match.
- **Song Preview**: open a `.psarc`, choose arrangement, preview tone names and mapped QC targets.

## Notes

- This tab config writes to `RSMods.ini` under `[QCAutomation]`.
- Settings changes in GUI trigger live `"update all"` messaging to RSMods.
