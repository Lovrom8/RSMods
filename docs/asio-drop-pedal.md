# ASIO Drop Pedal

Shifts the guitar's pitch before the game receives it, so songs in any tuning
can be played without touching a tuning peg. Range is -24 to +24 semitones. To
play an Eb song on an E-standard instrument, set that player to -1. It supports
guitar, emulated bass, physical bass and two-player arrangements.

This guide covers the ASIO engine. For setups without RS_ASIO, see the
[Cable Drop Pedal guide](cable-drop-pedal.md).

## Requirements

- [RS_ASIO](https://github.com/mdias/rs_asio) with an ASIO audio interface.
- Input formats `ASIOSTFloat32LSB`, `ASIOSTInt32LSB`, `ASIOSTInt24LSB` and
  `ASIOSTInt16LSB` are supported.
- No in-game tone setup. The pedal operates on every tone, stock or custom.
- `[Asio.Input.0]` is Player 1 and `[Asio.Input.1]` is Player 2. Both inputs
  must use the same ASIO driver; they may use different channels.
- A silent second hardware jack is still ready. Availability is determined by
  the configured ASIO route, buffer and format, not by whether a guitar is
  producing a signal.

The engine is selected at launch and announced beside the pedal readout, then
fades after a few seconds:

![Pedal readout and engine notice at launch](images/overlay-asio-drop-pedal-engine.png)

The same line is written to the debug log (`Drop pedal engine: ...`). In
automatic mode the pedal starts with Cable ownership and promotes to ASIO when
every configured input route is ready. If the notice remains `Cable Drop
Pedal`, the ASIO chain did not initialize; see
[troubleshooting](#troubleshooting).

## How it works

The mod hooks the ASIO driver underneath RS_ASIO and gives each configured
Rocksmith input its own persistent pitch shifter. Each route is shifted before
the game receives it. The shifter uses period-synchronous splicing. Rocksmith
therefore receives each player's shifted input signal, so detection, the tuner
and tone processing operate on the same pitch-shifted audio. The pedal does not
modify Rocksmith's tuning reference. Arrangements authored for A != 440 retain
that reference, so each instrument must be true-tuned as Rocksmith normally
requires before applying the semitone shift.

The shifter trails the input by up to one pitch period of the note being
played (roughly 1-13 ms depending on the string), on top of the interface's
normal round trip (~15 ms at 256 frames / 48 kHz on a typical interface).
At a zero-semitone target or while disabled, a shifter bypasses detection and
splicing and returns that input unshifted, adding no pitch-shifter delay. The
lightweight format conversion and history update still run so engaging the
pedal starts from live input instead of an empty delay line. Two players with
non-zero targets run two complete shifter workloads: two 96-frame routes
measure 0.046 ms per callback at -2/-2 and 0.063 ms at -12/-12 in an x86
Release build, against a 2 ms callback budget.

## Controls

| Action | Player 1 | Player 2 |
|---|---|---|
| Pitch down / up | `,` / `.` | `Control+,` / `Control+.` |
| Base tuning down / up | `F9` / `F10` | `Control+F9` / `Control+F10` |
| Toggle both players on / off | `F7` | `F7` |

- Keys register only while Rocksmith is the focused window.
- While the pedal is toggled off, every key except `F7` is ignored.
- Keys are rebindable in the settings app (Tuning tab), or under `[Keybinds]`
  in `RSMods.ini` (`DropPedalPitchDownKey`, `DropPedalPitchUpKey`,
  `DropPedalToggleKey`, `DropPedalBaseTuningDownKey`,
  `DropPedalBaseTuningUpKey`). The table above shows the defaults. The same
  modifier rule applies after rebinding.

The overlay shows one line in single player. When Rocksmith activates
multiplayer, Player 2 appears directly below Player 1; the rows are intentionally
not numbered because their order is unambiguous:

```text
Drop: E -> Eb (-1)
Drop: E
```

Each shifted row turns green. Every row shows its own player's configured
state, like a physical pedal: routes and signal levels do not change what is
displayed. Nothing is saved between sessions: both players start enabled, at
no shift, base E standard, every launch.

### Base tuning

`F9` / `F10` tell the mod what Player 1's instrument is physically tuned to;
hold Control for Player 2. This changes how tunings are named, nothing else.
Names are computed relative to that player's base, so with a base of D
standard, one semitone down reads `D -> Db (-1)`. Leave it at E standard unless
that physical instrument is tuned differently.

## Playing

- Each tuner reads that player's *shifted* pitch: with Player 1 at -2, an E
  standard guitar registers as D. Set each player to their arrangement's
  tuning; a song can author Lead and Bass in different tunings.
- Changing either target mid-song moves that player's audio and detection
  together. This includes the tuner screen, because the game only sees the
  already-shifted input.

## Emulated bass

Player identity is the Rocksmith input slot, not the selected arrangement.
Changing Player 1 from Lead to Emulated Bass or Physical Bass keeps using
`[Asio.Input.0]` and its existing shifter.

- **Emulated Bass from a guitar:** use the song arrangement offset only;
  Rocksmith supplies the octave.
- **Physical Bass from a bass:** use the song arrangement offset only; the
  instrument already supplies the octave.
- **Physical Bass from a guitar:** explicitly include the octave in the target,
  such as `-12` for E-standard bass. Rocksmith should be configured for a
  physical bass input so it does not add another emulated-bass octave shift.

## Troubleshooting

| Symptom | Cause |
|---|---|
| Engine notice reads `Cable Drop Pedal` | A configured ASIO route did not initialize. Check the driver and channel under both `[Asio.Input.0]` and `[Asio.Input.1]` |
| Game reports "no audio output device" on launch | Another program changed the interface's sample rate (DAWs and amp sims do this silently). Set it back to 48000 Hz in the interface's control panel and relaunch |
| Tuner reads a different tuning than the guitar is in | The shift, working as designed |
| Pitch keys do nothing | Pedal toggled off (`F7`), or Rocksmith is not the focused window |
| Player 2 controls report that Input 1 is unavailable | `[Asio.Input.1]` is not configured; signal level is not part of this check |

Logging is opt-in: enable it from the settings app before reproducing the
issue, or the log will not exist. The log is `RSMods_debug.txt`, next to
`Rocksmith2014.exe`, not in the `RSMods` subfolder. It is overwritten on every
launch and locked while the game runs, so quit before copying it.
