# ASIO Drop Pedal

Shifts the guitar's pitch before the game receives it, so songs in any tuning
can be played without touching a tuning peg. Range is -24 to +24 semitones. To
play an Eb song on a guitar in E standard, set the pedal to -1. Works for
guitar and for emulated bass.

This guide covers the ASIO engine. For setups without RS_ASIO, see the
[Cable Drop Pedal guide](cable-drop-pedal.md).

## Requirements

- [RS_ASIO](https://github.com/mdias/rs_asio) with an ASIO audio interface.
- No in-game setup. The pedal operates on every tone, stock or custom.

The engine is selected at launch and announced beside the pedal readout, then
fades after a few seconds:

![Pedal readout and engine notice at launch](images/overlay-asio-drop-pedal-engine.png)

The same line is written to the debug log (`Drop pedal engine: ...`). If the
notice reads `Cable Drop Pedal` instead, the ASIO chain did not initialize; see
[troubleshooting](#troubleshooting). The engine cannot change without
relaunching, since the hooks bind the audio driver for the session.

To require this engine, set `[Drop Pedal] Engine = asio` in `RSMods.ini` or the
settings app. The default `automatic` setting uses ASIO when the chain appears
and otherwise uses the Cable Drop Pedal. Forced `asio` logs an error and stays
inactive if the ASIO chain never appears.

## How it works

The mod hooks the ASIO driver underneath RS_ASIO and pitch shifts the raw
guitar signal before the game receives it. The shifter uses period-synchronous
splicing. Rocksmith receives a shifted input signal, so detection, the tuner and
tone processing operate on the same pitch-shifted audio. No tuning-reference
redirect is required.

The shifter trails the input by up to one pitch period of the note being
played (roughly 1-13 ms depending on the string), on top of the interface's
normal round trip (~15 ms at 256 frames / 48 kHz on a typical interface).

## Controls

| Action | Key |
|---|---|
| Pitch down / up | `,` / `.` |
| Toggle on / off | `F8` |
| Base tuning down / up | `F9` / `F10` |

- Keys register only while Rocksmith is the focused window.
- While the pedal is toggled off, every key except `F8` is ignored.
- These are defaults. Rebind them in the settings app or edit
  `[Keybinds] DropPedalPitchDownKey`, `DropPedalPitchUpKey`,
  `DropPedalToggleKey`, `DropPedalBaseTuningDownKey` and
  `DropPedalBaseTuningUpKey`.

The overlay shows the current state and turns green whenever a shift is
applied. Nothing is saved between sessions: the pedal starts enabled, at no
shift, base E standard, every launch.

### Base tuning

`F9` / `F10` tell the mod what the guitar is physically tuned to. It changes
how tunings are named, nothing else: names are computed relative to the base,
so with a base of D standard, one semitone down reads `Db standard (-1)`.
Leave it at E standard unless the guitar really is tuned differently.

## Playing

- The tuner reads the *shifted* pitch: with the pedal at -2, an E standard
  guitar registers as D. Set the pedal to the song's tuning and standard
  fingering registers.
- Changing the pedal mid-song moves audio and detection together; they cannot
  drift apart. This includes the tuner screen, because the game only sees the
  already-shifted input.

## Emulated bass

Emulated bass works normally with this engine. If Rocksmith is in emulated-bass
mode, use the song offset only: for an Eb song, set the pedal to `-1`.

For the lowest-latency guitar-to-bass setup, tell Rocksmith you are playing
bass and let the ASIO pedal supply the octave before the game hears the signal.
On an E-standard guitar, set the pedal to `-12` for E-standard bass, `-13` for
Eb bass, and so on. This avoids Rocksmith's emulated-bass post-processing path.

## Troubleshooting

| Symptom | Cause |
|---|---|
| Engine notice reads `Cable Drop Pedal` | In `automatic`, the ASIO chain did not initialize and the mod is using Cable. Check `RS_ASIO.ini` names the interface under `[Asio.Input.0]`, and see the next row |
| Forced `asio` logs inactive | `[Drop Pedal] Engine = asio` was selected, but the ASIO input chain never appeared. Check `RS_ASIO.ini` and relaunch |
| Game reports "no audio output device" on launch | Another program changed the interface's sample rate (DAWs and amp sims do this silently). Set it back to 48000 Hz in the interface's control panel and relaunch |
| Tuner reads a different tuning than the guitar is in | The shift, working as designed |
| Pitch keys do nothing | Pedal toggled off (`F8`), or Rocksmith is not the focused window |

The debug log is `RSMods_debug.txt`, next to `Rocksmith2014.exe`, not in the
`RSMods` subfolder. It is overwritten on every launch and locked while the
game runs, so quit before copying it.
