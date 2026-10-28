# RSMods on macOS

RSMods works on Apple Silicon Macs through **RS Re-ARMed**, a native arm64 port of Rocksmith 2014 for macOS.

**Status.** Many RSMods features have already been built for macOS using the approach described here. The plan is
to bring them into this repository in steps (see [Plan](#plan)). The first step, this change, makes the configurator
(`GUI.Core`, `GUI.Avalonia`) build and run on macOS and adds this document. It changes nothing on Windows.

### What you need

- An Apple Silicon Mac (M1 or later).
- [RS Re-ARMed](https://github.com/bbelgg/rs-rearmed) **1.2.0 or later**, which builds the game on your Mac from your
  own Steam copy of Rocksmith 2014 Remastered. 1.2.0 is the first version that loads mods.

The Steam copy of the Mac game runs under Rosetta (Intel emulation) and can't load mods, so on macOS RSMods runs only
in RS Re-ARMed.

### Settings

The settings are the same `RSMods.ini` as on Windows, in RS Re-ARMed's game folder
(`~/Library/Application Support/RS Re-ARMed/game/RSMods.ini`): the same sections, keys, values and default keys. An
`RSMods.ini` from Windows works on the Mac and vice versa; settings for mods the Mac doesn't have yet are kept and ignored.
The mods re-read the file while the game runs, so a change takes effect within a second, without a restart.

Keys are written as on Windows (letters, digits, `F1`-`F20`, `VK_` names) and mapped to the Mac keys. Function keys
may need **Fn**. Where RSMods uses Ctrl + a key, use the Control key.

### What works, and how it differs

These mods are already built for macOS and will be added to this repository in a later change.

| Mod | On macOS |
|---|---|
| Riff Repeater speed above 100% | As on Windows, with the speed shown at the top and in `riff_repeater_speed.txt`. |
| Linear Riff Repeater | As on Windows. |
| Looping, rewinding | Work, but RSMods' reset of the "grey note" time (notes you already passed stay active after a pause) is not on macOS yet. |
| Show song timer | The position comes from the audio engine; it appears once the song's audio plays, a few seconds in. |
| Volume control | As on Windows; the changed channel's volume hides again after 3 seconds. |
| Remove song previews | As on Windows. |
| Autoload last used profile | Enters the top profile (the last used one); `ProfileToLoad` (a named profile) is not on macOS yet. |
| Force re-enumeration | Manual and automatic re-enumeration work. The "Enumerate" main-menu entry (a change to the game's files) is not on macOS. |
| Override input volume | Through Core Audio. The device is matched by part of its macOS name (Audio MIDI Setup), e.g. `Rocksmith USB Guitar Adapter`. Unlike on Windows, several devices can be listed, separated by `;`, each optionally with its own percentage. |
| Screenshot scores | RS Re-ARMed runs without the Steam client, so the mod saves the screen itself, as a PNG in `~/Pictures/RS Re-ARMed/`. |
| Bypass 2+ Real Tone cable popup | The popup is gone; the game may still show its warning as a 10-second notification. |
| Play audio in background | As on Windows. |
| Toggle loft | The venue goes black. The Mac build draws the song's backdrop, the venue's props and floor, and the tone's amps separately from the loft, so the mod hides them as well. Lessons keep the loft. The green screen wall is not on macOS yet. |
| Remove headstock, skyline, lyrics | As on Windows. |
| Fretless, remove inlays, remove lane markers | As on Windows. |

The mods that hide parts of the picture find them after each scene loads, so give them a second or two after a song
starts.

**Not on macOS yet:** Extended Range, custom string colors, rainbow strings and notes, custom highway colors, green
screen wall, remove fingerprints, GuitarSpeak, show current note, song accuracy, custom song list titles and the
extra song lists, custom Non-Stop Play timer, prevent buggy tones, alternative sample rates, MIDI auto-tune, start on
a secondary monitor, the in-game settings menu, profile backups, calibration, voice-over control, and the file
tools (Fast Load, custom tunings, default tones, Direct Connect). **Not planned** (Windows only): the
Twitch and Crowd Control effects, the Oculus crash fix and ASIO.

## How it works

### RS Re-ARMed

RSMods on Windows is a DLL the 32-bit Windows game loads (as `xinput1_3.dll`). It patches the game's x86 code at
known addresses and hooks Direct3D 9. None of that carries over to macOS:

- The Mac game is a different program: a 64-bit x86 Mac build of Rocksmith 2014, with its own code and addresses,
  OpenGL instead of Direct3D, and Cocoa instead of Win32.
- RS Re-ARMed doesn't run that x86 code. It **statically recompiles** it, once, at build time: every function is
  translated to C and compiled for arm64. At run time there is no original machine code to patch, and the
  translated functions call each other directly.

Instead, starting with version **1.2.0**, RS Re-ARMed gives mods these ways in:

- **A mods folder.** At startup it loads every `.dylib` in `~/Library/Application Support/RS Re-ARMed/mods/`
  (`RS_LOAD_MODS=0` skips them; `RS_MODS_PATH` loads another folder or library). A reinstall keeps the folder.
- **Hook points.** Some game functions are built so that a mod can run code before or after them, or replace
  them entirely. The list is in RS Re-ARMed's `tools/ws7/recomp/overrides/hooks.skip`; the API is
  `tools/ws7/recomp/runtime/rc_hook.h` (`rc_hook_add`). 1.2.0 has nine: each frame, key and mouse input, Wwise
  `PostEvent` and `SetRTPCValue` by name, the app's focus and UI events, the camera frustum, the Riff Repeater
  speed conversion, and the two-cable check.
- **The runtime, exported.** The game executable exports what a mod needs to work with the game: `rc_call_guest`
  (call any game function), `rc_slide` (where the game image is in memory), the image bounds and `rc_hook_add`.
  Game memory is ordinary memory: a game pointer is a host pointer.

Consequences for RSMods:

- A Mac mod needs **no change to the game** and no rebuild of it, unless it needs a new hook point. A new hook point
  means a new RS Re-ARMed release, which users install by rebuilding the game (the RS Re-ARMed installer does this).
  Hook points are cheap; adding them in batches keeps those releases rare.
- RS Re-ARMed supports one game build, the current Steam Mac release, so the Mac mods' addresses are fixed. If that
  build ever changes, both RS Re-ARMed and the Mac mods need updating.
- RS Re-ARMed is a separate project (MIT licensed); the Mac mods depend on its version, just as the Windows DLL depends on
  the Windows game build.

### The Mac mods library

The Mac mods are a C library (a `.dylib`) with a core and one source file per mod. It reimplements the RSMods
features for the Mac build, keeping RSMods' settings, names and behaviour:

- **Hooks.** In its constructor it registers its hooks: a tick after each game frame (on the game thread, where it
  can call game functions safely), key events before the game sees them (the hotkeys), and Wwise events (the song
  key from the preview event, the song's playing ID).
- **Game functions.** Wwise is statically linked into the game with its symbol names, so the library calls
  `SetRTPCValue`, `SetActorMixerEffect`, `SeekOnEvent`, `GetSourcePlayPosition` and others directly. That covers
  song speed, volume, seeking and the song position.
- **Game state.** The current menu's name is read from the UI manager (as RSMods does on Windows), and the song
  key from the preview event.
- **Hiding parts of the picture.** Where RSMods skips Direct3D draws, the Mac library works with the game's
  Gamebryo scene: it finds objects and render steps by name in memory (from their run-time type information) and
  culls them, collapses them, or switches the render steps off. The loft also uses the camera hook (the loft camera's
  far plane, as on Windows).
- **Text on screen** (song speed, loop, song timer, volume) is drawn with Core Text and OpenGL after the game's
  frame.
- **macOS APIs** where RSMods uses Windows ones: Core Audio for the input volume, ImageIO for screenshots.
- **RSMods.ini** is read with the same keys and defaults, and re-read when it changes.

Each mod's source names the game functions and data it uses, with addresses in the Mac build.

### The configurator on macOS

The Avalonia configurator is mostly cross-platform already. Minor changes make it build and run on macOS:

- **Build.** `Directory.Build.props` sets `RsModsMac` when building on macOS. `GUI.Core` and `GUI.Avalonia` then
  target plain `net10.0` (not `net10.0-windows`), without `NAudio.WinMM` and the forced x64. On Windows nothing
  changes: same target framework, packages and code paths.
- **Windows-only parts** are guarded where they start, so the rest of the code is unchanged:

  | Part | On macOS |
  |---|---|
  | Window messages to and from the game (`WinMsgUtil`, `GameSettingsReceiver`) | Skipped. The Mac mods re-read `RSMods.ini` while the game runs, so saving the file is enough. |
  | Registry lookups (`GenUtil`) | Replaced by `MacLocations`: RS Re-ARMed's game and save folders. |
  | ASIO, MIDI and WASAPI device lists | Empty (a Core Audio device list is planned). |
  | WMI drive types (Fast Load) | Unknown. |
  | Twitch token store (DPAPI) | Nothing is stored (tokens are never saved unencrypted); Twitch effects need the Windows DLL anyway. |
  | Crash reporter's message box | Replaced by the console and the log file. |
  | SoundPack import and export (7-Zip's Windows library, ACM for MP3) | Not on macOS yet. |

- **Folders.** `MacLocations` knows RS Re-ARMed's folders: `~/Library/Application Support/RS Re-ARMed/game` for the
  game and `RSMods.ini`, and `.../steam/<account>/221680/remote` for the saves (`RS_NATIVE_HOME` overrides the base
  folder). A folder with `Rocksmith2014.app` counts as a Rocksmith install on macOS.

To build and run it on a Mac (with the .NET 10 SDK):

```bash
dotnet run --project GUI.Avalonia -c Release
```

It edits the real `RSMods.ini` of RS Re-ARMed; to try it on a copy, set `RS_NATIVE_HOME` to a folder laid out like
RS Re-ARMed's.

For now the configurator shows every mod, including the ones the Mac doesn't have yet; marking them is the next
step.

### Where the Mac code lives in this repository

The proposed layout, next to the Windows parts:

```
DLL/                 the Windows mods (unchanged)
Mac/                 the Mac mods library: src/ (core and one file per mod), include/rs-rearmed/ (RS Re-ARMed's
                     runtime headers, MIT), build.sh
GUI.Core/            shared configurator logic; Windows-only parts guarded, Mac folders in MacLocations
GUI.Avalonia/        the configurator, for Windows and macOS
mods.manifest.json   the settings, with which platforms have each mod
docs/macos.md        this document
```

- **The manifest.** The configurator's settings screens come from `mods.manifest.json`, which the Windows DLL
  generates. The Mac library would report its own list of settings the same way, and the manifest would say which
  platforms have each mod. The configurator then shows the same screens on both and marks what isn't on macOS yet.
  All of the Mac library's `RSMods.ini` keys are already in the manifest, with the same sections and names.
- **Installing.** On macOS the configurator would install the library itself: copy it into RS Re-ARMed's mods folder
  and remove the download's quarantine flag. The configurator app is ad-hoc signed (not
  notarized), so macOS asks to confirm it once, with **Open Anyway** in Privacy & Security.
- **Builds.** A macOS runner builds the library and the configurator as an app bundle.

## Approaches chosen

- **A rewrite for the Mac build, not a port of the DLL.** The DLL's code patches and Direct3D hooks have no
  equivalent in a recompiled game; what carries over is RSMods' design: the features, settings and keys. The Mac
  library keeps those, so users and the configurator see one RSMods.
- **Hook points built into RS Re-ARMed, not patching at run time.** Translated functions call each other directly,
  so a function can only be intercepted reliably where the build provides for it. Patching the compiled arm64 code
  at run time would miss inlined calls and break on every rebuild.
- **The same `RSMods.ini`.** One settings file, one configurator, and settings that move between platforms.
- **The game's scene and render steps, not draw calls.** The Windows mods recognise Direct3D draws by their
  geometry and textures. On macOS, hiding Gamebryo objects and switching render steps off by name is simpler and
  survives scene changes.
- **A mods folder outside the game app.** RS Re-ARMed rebuilds and replaces the game app on every update; mods in
  their own folder survive that.

## Plan

1. **This change:** the configurator builds and runs on macOS; this document.
2. The configurator knows which mods the Mac has (the manifest's platforms) and lists Core Audio input devices.
3. The Mac library is added under `Mac/`, with its build script.
4. The configurator installs and updates the Mac library, and CI builds the configurator as a macOS app for the
   releases.
5. More mods on macOS, starting with the ones that need no new hook point: remove fingerprints, calibration,
   voice-over control, profile backups, then song accuracy, show current note and GuitarSpeak.