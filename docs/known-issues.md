# Known Issues and Limits

This page is for testers using the QC-enabled fork of RSMods.

## Current QC Automation Limits

### Transpose / Scene Automation
- v1 transpose uses a **song-level scene model**.
- Scene map:
  - `A = 0`
  - `B = -1`
  - `C = -2`
  - `D = -3`
  - `E = -4`
  - `F = -5`
  - `G = -6`
  - `H = -7`
- No true-tuning / cent automation.
- No expression-based transpose automation.
- Unsupported or non-uniform tunings may be skipped or clamped depending on current settings/logic.

### Tone Classification
- Classification is based on case-insensitive substring matching.
- Results depend on the authored tone names in the song data.
- Some songs will classify better than others.
- Bass guard and classifier priorities can affect behavior.

### Asset / Timeline Resolution
- QC tone-following depends on installed song asset metadata and timeline resolution.
- Some songs may fall back to more limited behavior if timeline metadata is missing or ambiguous.
- Compiled SNG data can contain ambiguity, especially around some tone ID mappings.

## Expected Tester Checks

When testing a beta build, try to verify at least:
1. one normal E-standard song
2. one semitone-down or lower song
3. one song with authored auto tone changes
4. manual override using Rocksmith tone slots `2`, `3`, and `4`
5. return-to-auto behavior when applicable
6. song exit / re-enter behavior
7. arrangement changes if the song supports them

## What to Include in Bug Reports

Please include:
- release tag tested
- song name
- arrangement tested
- whether the problem happened on:
  - song start
  - auto tone change
  - manual override
  - song restart
  - arrangement change
  - song exit / re-enter
- QC preset mapping used
- MIDI output device name
- MIDI channel
- relevant RSMods log lines

## What Is Probably Not a QC Bug

Before reporting a bug, double-check:
- the QC preset target actually exists at the configured `MyPresets:<bank><slot>` location
- the correct MIDI output device is selected
- the QC is receiving USB MIDI from Windows
- the QC MIDI channel matches the configured channel in RSMods

## Scope Reminder

This fork is for **Rocksmith 2014 Remastered on Windows**.
It is not for Rocksmith+ / “Rocksmith 2024”.
