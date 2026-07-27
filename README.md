# RSModsPlus

A fork of [RSMods](https://github.com/Lovrom8/RSMods) that adds a drop pedal to
Rocksmith 2014.

It does in software what a DigiTech Drop or similar hardware pedal does. It
shifts your guitar's pitch, in this case inside the game, and moves the pitch the
game expects by the same amount, so note detection still agrees with what you're
playing. Range is -24 to +24 semitones.

![The drop pedal set to Eb standard](docs/images/overlay-eb-standard.png)

To play an Eb song on a guitar in E standard, set the pedal to -1. Works for
guitar and for emulated bass.

**[Setup and usage guide](docs/drop-pedal.md)**

---

## What this fork adds

- A drop pedal, -24 to +24 semitones, with note detection and the in-game tuner
  following the shift.
- A base tuning setting, so the pedal offsets from whatever your guitar is
  physically in rather than from E.
- An on-screen readout of the current state.

Everything else comes from RSMods 1.2.8.2 and behaves as upstream documents it:
extended range mode, custom song list titles, toggle loft, force re-enumeration,
GuitarSpeak, and the rest. See
[upstream's README](https://github.com/Lovrom8/RSMods#readme) for that list and
for the full `RSMods.ini` reference.

---

## Installing

This is built from RSMods 1.2.8.2 and uses the same filename, so it replaces
RSMods' own `xinput1_3.dll` rather than sitting beside it. Only one of the two
can be loaded at a time.

**If you already have RSMods installed, back up the existing `xinput1_3.dll`
first.** Copy it somewhere outside the game folder, or rename it. That copy is
how you get back to plain RSMods later.

Then download `xinput1_3.dll` from the
[latest release](https://github.com/Cheesewizard/RSModsPlus/releases) and put it
in your Rocksmith 2014 folder, next to `Rocksmith2014.exe`, overwriting the file
that is already there.

Your `RSMods` folder, the settings app and `RSMods.ini` are untouched and carry
on working, since this is the same 1.2.8.2 codebase with the drop pedal added.

To uninstall, put your backup back. If you had no RSMods before this, deleting
the file is enough.

Requirements are upstream's: Steam Rocksmith 2014 Remastered on Windows, and the
MS Visual C++ 2015-2019 redistributable.

The pedal needs one piece of in-game setup, a tone containing a MultiPitch pedal.
That's covered in [setup](docs/drop-pedal.md#1-setup).

---

## How it works

Rocksmith's note detection reads the raw signal from your guitar, upstream of the
tone chain, so anything in the tone chain is invisible to it. Shifting the audio
alone would leave you playing in Eb while the game still marked you against E.

A hardware pedal sidesteps this by sitting between the guitar and the input: the
game receives an already-shifted signal, and detection works natively with
nothing to correct. This mod shifts inside the game, downstream of detection, so
it also redirects the reference frequency the game derives its expected pitch
from. That is the same value CDLC charters set as an arrangement's tuning pitch.
Shift the audio down a semitone, move the expectation up one, and the two agree.

Reverse-engineering notes, including everything that was ruled out along the way,
are in [docs/wwise-plugin-internals.md](docs/wwise-plugin-internals.md).

---

## Roadmap

V1 shifts my guitar inside the game and corrects what the game expects to hear.
One DLL, no extra hardware or software, and it works with every input method
Rocksmith supports.

Two things I want to try next:

- Shifting the game's backing track instead of the guitar, so the music moves to
  the tuning I'm already in.
- A companion app that pitches the input before Rocksmith gets it. That's where a
  hardware pedal sits, so detection works natively and nothing needs redirecting.
  In-game hotkeys would still drive it.

I'm keeping the in-game path either way. It's the only one that works on a Real
Tone cable with nothing else in the chain.

---

## Documentation

| Document | Covers |
|---|---|
| [docs/drop-pedal.md](docs/drop-pedal.md) | Setup, controls, limitations, troubleshooting |
| [docs/wwise-plugin-internals.md](docs/wwise-plugin-internals.md) | How the Wwise pitch shifter is reached, and what was ruled out |

---

## Issues

Drop pedal problems go
[on this repository](https://github.com/Cheesewizard/RSModsPlus/issues), not on
upstream's tracker. This feature isn't theirs to support.

A bug in an inherited RSMods feature that reproduces on a stock upstream build
belongs [upstream](https://github.com/Lovrom8/RSMods/issues).

A debug log is written to `RSMods_debug.txt` beside `Rocksmith2014.exe`. It is
overwritten on every launch and locked while the game runs, so quit before
copying it. Attaching it makes a bug report far easier to act on.

---

## Credits

RSMods is the work of **Lovrom8** and **ffio1**, with contributions from
ZagatoZee, Kokolihapihvi and L0fka. This fork is the drop pedal on top of their
project. If you find the rest of the mod suite useful, thank them.

The reference-frequency technique the drop pedal relies on is the same one CDLC
charters have long used to move a chart's expected notes by setting an
arrangement's tuning pitch.
