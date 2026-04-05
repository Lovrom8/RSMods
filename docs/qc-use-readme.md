# QC Automation Use Guide

This guide covers the **QC Automation** tab in `RSMods.exe`.

## What QC Automation Does

- Chooses QC preset targets from Rocksmith authored tone names.
- Supports manual Rocksmith tone slot overrides (`2`, `3`, `4`).
- Supports an idle/default QC target when not in-song.
- Supports song-level transpose scene automation (v1 semitone model).

## Basic Setup

1. Open `RSMods.exe` and go to **QC Automation**.
2. Enable **QC Automation**.
3. Select your **MIDI Out Device** (Quad Cortex MIDI OUT).
4. Set **MIDI Channel (1-16)** to match QC.
5. Set targets in **QC Target Mapping** using format like `MyPresets:32A`.

## Classifier Behavior

- Matching is case-insensitive substring matching.
- Priority order is configurable (`1 = highest`) in the UI.
- Bass guard can skip bass arrangements when enabled.

## Transpose (v1)

- Scene map: `A=0, B=-1, C=-2, D=-3, E=-4, F=-5, G=-6, H=-7`.
- Transpose scene is song-level state, not per-tone-change state.

## Preview Tools

- **Classifier Preview**: test a tone name against current keywords/priorities.
- **Song Preview**: inspect tones in a selected `.psarc` and see mapped targets.
