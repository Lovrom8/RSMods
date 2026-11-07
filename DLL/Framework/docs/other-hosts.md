# Other hosts: the plan

> **Status: the framework side is ready, no host exists yet.** The first host would be the macOS port for RS Re-ARMed
> (PR #256 and its `docs/macos.md`), which closed by agreement so integration can follow once the UI and architecture
> are settled. Until then the Mac mods live outside this repo. What a host has to provide is in the
> [README](../README.md#other-hosts).

## The goal

One core and one `RSMods.ini`, not a second framework to keep in sync. Mod bodies differ per platform, because a
recompiled game has nothing to patch and no D3D9. Everything around them is shared: mod lifecycle, conflict rules, key
bindings, settings and fault isolation.

## Done

- **Portable core:** the framework builds and passes its tests under MSVC and g++, without `windows.h` or `stdafx.h`.
  `Tests/BuildAndRun.sh` is the non-MSVC entry point.
- **Settings:** `SettingsStore` holds the values mods read and loads them through any `CSimpleIniA`-shaped reader.
  `KeyNames` maps `RSMods.ini` key names to virtual-key codes.
- **Shared declarations:** `SettingsManifest` reads `mods.manifest.json` back into `SettingDefs`, so a mod on another
  host doesn't declare its settings again. `Conflicts()` catches a shared key that drifts.
- **A host's own manifest:** the GUI reads a `mods.manifest.json` next to it, and off Windows refuses to fall back to
  the built-in Windows list.
- **Smallest host:** `Tests/HeadlessHostTests.cpp` runs a mod through the whole lifecycle with no game or window.
- **Guards:** `.github/workflows/framework-portable.yml` tests Ubuntu and macOS, `.gitignore` covers manifests away from
  the root, CI fails on a force-added one, and the pre-commit hook blocks a manifest change it can't verify.
- **The configurator on macOS:** `GUI.Core` and `GUI.Avalonia` build for plain `net10.0` there (`RsModsMac` in
  `Directory.Build.props`), and on Windows nothing changes. Windows-only parts are guarded where they start: window
  messages, the registry, the ASIO, MIDI and WASAPI lists, WMI, DPAPI (Twitch tokens are never stored unencrypted) and
  the crash reporter's box. `MacLocations` finds RS Re-ARMed's folders. `.github/workflows/configurator-macos.yml` keeps
  it building and tested. To run it on a Mac, build `GUI.Avalonia` and put a copy of `mods.manifest.json` beside the
  output: until a host ships its own, that's the Windows list.

## Decisions

- **Each build ships its own manifest, with no platform flag.** The file lists what that build has, so the GUI shows
  a host's own mods and none of the Windows-only ones. A flag would make Windows declare mods it doesn't have.
- **Windows declarations stay the only source for shared settings.** A host's manifest is build output, never
  committed. Labels and hints may differ per platform; INI location, type, default and choices may not.
- **A mod on another host keeps the Windows mod's `Id()`.** That's how it finds its settings, and a rename fails loudly
  (`ModRegistry` rejects the mod) rather than losing them.
- **Key binds stay Windows virtual-key codes on every platform,** because that's what `RSMods.ini` stores. A host maps
  its own key codes to them.
- **The framework stays in `DLL/Framework` for now.** See step 6.

## Next

1. **Check it in game:** a toggle, a key bind and an INI reload, since settings loading moved.
2. **See macOS build it:** a draft PR into `avalonia-consolidated` starts the workflow, the first run under clang and
   Apple's standard library. Expect small portability fixes.
3. **Agree the architecture with the macOS port.** Everything below needs that first.
4. **Write the Mac host layer.** RS Re-ARMed's per-frame hook drives the tick, its key hook feeds `Inbox()`, and an INI
   reload goes through `EnqueueSettingsUpdate`. Mods become `IMod` classes on the shared core.
5. **Add the host's build step:** dump the manifest, fail on anything in `Conflicts()`, and ship the file next to the GUI.
   Keep the framework's path in one variable in the build script, so step 6 is a one-line change there.
6. **Move the shared code out of `DLL/`,** when the first real host lands and it's clear what is shared, as a commit
   containing only the move:

   ```
   Core/   framework, Log, json, settings store and manifest
   DLL/    Windows host, D3D draw code, Windows mods
   Mac/    Mac host and Mac mods
   ```

   `DrawRegistry` and `DeviceEvents` serve only the D3D9 hooks, so they'd stay in `DLL/`, which means taking `Draw()` out
   of `ModContext` and `ModRegistry`. External mods include `"Framework/Framework.hpp"`, so that path needs to keep working.

## Open

- **Audio devices on macOS:** the input device list is empty there until it uses Core Audio.
- **SoundPacks on macOS:** import and export don't work yet. 7-Zip's library is Windows-only, MP3 needs ACM, and
  `SoundPackLocationPrefix` mixes a file path with backslashes into the archive path it's also used for.
- **HUD and draw backend:** `DrawRegistry`'s API is shaped by D3D9. What a Mac backend needs from it is a question for
  whoever writes one.
- **Marking mods as "not on macOS yet":** the GUI hides them for now. Marking them means comparing against the Windows
  manifest, plus UI work.
- **Windows-focused tabs:** only the settings list and key binds follow the manifest, so MIDI, Twitch and the like would
  still show on a Mac.
- **How a host dumps its manifest:** the call is `SettingsSchema().DumpManifestJson()`, but a mods library that needs the
  game loaded to link needs some way to run it during a build.
- **CI coverage:** AppVeyor's `-VerifyManifest` runs on `develop` only, so a stale manifest can sit on a feature branch.
