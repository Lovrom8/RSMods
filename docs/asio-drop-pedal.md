# ASIO Drop Pedal

Shifts the guitar's pitch before the game receives it, so songs in another
tuning can be played without touching a tuning peg. For example, to play an Eb
song with a guitar in E standard, enable Drop Pedal and press `,` once. The
readout becomes `Drop: E -> Eb (-1)`.

It supports guitar, emulated bass, physical bass and two-player arrangements.

This guide covers the ASIO engine. For setups without RS_ASIO, see the
[Cable Drop Pedal guide](cable-drop-pedal.md).

## Before you start

- Set up [RS_ASIO](https://github.com/mdias/rs_asio) first and confirm that the
  unshifted guitar works in Rocksmith.
- Enable Drop Pedal on the settings app's Tuning tab. There is no special
  in-game tone to install; the pedal works with stock and custom tones.
- Input selection is automatic. A single-player setup needs no extra channel
  settings in `RSMods.ini`.
- For two players, configure both Rocksmith inputs in `RS_ASIO.ini` as normal.
  Both inputs must use the same audio interface.

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

At 48 kHz with 128-frame callbacks, the production-shifter harness measures
roughly 6-20 ms of observable content delay depending on the note and shift.
This is added to the interface's normal round trip, which also depends on its
driver and buffer configuration.

For context, DigiTech's published Drop pedal specifications list its 44.1 kHz
sample rate but do not give a latency figure. Independent waveform measurements
report approximately
[12-17 ms](https://www.thefretboard.co.uk/discussion/107282/digitech-drop-tune/p2),
and a separate documented burst test measured roughly
[16 ms](https://www.reddit.com/r/audioengineering/comments/r3mecr/analyzing_the_digitech_drop_pedal/). Those tests are not method-for-method
identical to this project's envelope-transition harness, but they show that
the mod's 6-20 ms result is in the same broad range as a respected dedicated
drop-tuning pedal. End-to-end feel also depends on the interface's round trip,
so use the lowest stable ASIO buffer size for the most responsive setup.

At a zero-semitone target or while disabled, a shifter bypasses detection and
splicing and returns that input unshifted, adding no pitch-shifter delay. The
lightweight format conversion and history update still run so engaging the
pedal starts from live input instead of an empty delay line. Two players with
non-zero targets run two complete shifter workloads.

## Controls

| Action | Player 1 | Player 2 |
|---|---|---|
| Pitch down / up | `,` / `.` | `Control+,` / `Control+.` |
| Base tuning (cycles E, Eb, D, ...) | `F9` | `Control+F9` |
| Toggle both players on / off | `F7` | `F7` |

- Keys register only while Rocksmith is the focused window.
- While the pedal is toggled off, every key except `F7` is ignored.
- Keys are rebindable in the settings app (Tuning tab), or under `[Keybinds]`
  in `RSMods.ini` (`DropPedalPitchDownKey`, `DropPedalPitchUpKey`,
  `DropPedalToggleKey`, `DropPedalBaseTuningKey`). The table above shows the
  defaults. The same modifier rule applies after rebinding.

The overlay shows one line in single player. When Rocksmith activates
multiplayer, Player 2 appears directly below Player 1; the rows are intentionally
not numbered because their order is unambiguous:

```text
Drop: E -> Eb (-1)
Drop: E
```

Each shifted row turns green. Every row shows its own player's configured
state, like a physical pedal: routes and signal levels do not change what is
displayed. Nothing is saved between sessions: the pedal starts enabled, at no
shift, base E standard, every launch.

### Base tuning

`F9` cycles what Player 1's instrument is physically tuned to, one name per
press: `E -> Eb -> D -> ... -> F -> E`. Hold Control for Player 2. This changes
how tunings are named, nothing else. Names are computed relative to that
player's base, so with a base of D standard, one semitone down reads
`D -> Db (-1)`. A guitar physically tuned to C# standard reaches E with a
`+3` shift and reads `C# -> E (+3)`. Leave the base at E standard unless that
physical instrument is tuned differently.

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
| Pedal engages but the guitar's pitch never changes | The automatic route may have selected another input, such as a microphone. Follow [Wrong ASIO input selected](#wrong-asio-input-selected) below |
| Game reports "no audio output device" on launch | Another program changed the interface's sample rate (DAWs and amp sims do this silently). Set it back to 48000 Hz in the interface's control panel and relaunch |
| Tuner reads a different tuning than the guitar is in | The shift, working as designed |
| Pitch keys do nothing | Pedal toggled off (`F7`), or Rocksmith is not the focused window |
| Player 2 controls report that Input 1 is unavailable | `[Asio.Input.1]` is not configured; signal level is not part of this check |

### Wrong ASIO input selected

Check the `uses ASIO channel` line in `RSMods_debug.txt`. If it does not match
the guitar's `Channel =` value in `RS_ASIO.ini`, correct that player's channel
on the GUI's **RS_ASIO Settings** tab, then restart the game.

Logging is opt-in: enable it from the settings app before reproducing the
issue, or the log will not exist. The log is `RSMods_debug.txt`, next to
`Rocksmith2014.exe`, not in the `RSMods` subfolder. It is overwritten on every
launch and locked while the game runs, so quit before copying it.
