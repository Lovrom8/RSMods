# External mods

Mods that live in their own repos. Clone or submodule each one into its own folder here:

```
DLL/ExternalMods/DropPedal/DropPedalMod.cpp
DLL/ExternalMods/DropPedal/Audio/AsioTap.cpp
```

Everything in this folder except this file is git-ignored. `DLL/ExternalMods.targets` compiles every
`.cpp` under a mod's folder, at any depth, except under `Tests/` folders.

## Writing one

- Don't include `stdafx.h`: it's force-included. Include framework and game headers from the `DLL` folder,
  e.g. `#include "Framework/Framework.hpp"`, `#include "MemUtil.hpp"`.
- The mod is an ordinary `IMod` with a `ModRegistrar` (`Framework/README.md`, "Adding a mod"). It compiles
  against the framework source, so a breaking framework change fails in your build, not at runtime.
- Prefix setting keys with the mod's name; every mod shares one key space.
- Extra libraries: `#pragma comment(lib, "...")` in your source.

## Building and shipping

Build `RSMods.sln` as usual. The built DLL contains your mods, so its settings no longer match the committed
`mods.manifest.json`: write your build's own with

```
powershell DLL/Framework/Tests/BuildAndRun.ps1 -DumpManifest -ManifestPath <file>
```

and ship it as `mods.manifest.json` next to the GUI exe, which prefers it over the one built in.

The build names your mod folders in the DLL's version info ("SpecialBuild"). The official installer reads it and
asks before replacing a custom build; if the player agrees, it renames your manifest to `mods.manifest.json.bak`.
`-DumpManifest` without `-ManifestPath` and `-VerifyManifest` refuse to run while this folder has mods in it.

Background: `Framework/docs/plugin-distribution.md`.
