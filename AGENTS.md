# Agent guidelines

## Project layout

- `DLL/` - the native mod DLL injected into Rocksmith 2014 (C++20, Win32). Mods live in `DLL/Mods/` and register
  through the framework in `DLL/Framework/`; read `DLL/Framework/README.md` before changing either.
- `GUI.Core/` + `GUI.Avalonia/` - the configurator (RSMods.exe), .NET 10, Avalonia UI. `GUI.Core` holds the logic
  and is tested by `GUI.Core.Tests/`.
- `Installer.Core/` + `Installer/` - the installer, tested by `Installer.Core.Tests/`.
- `mods.manifest.json` - the DLL's settings schema, which the GUI renders its settings and keybinding pages from.
- `Build/` - publish and scaffolding scripts; `appveyor.yml` is the CI pipeline and the reference for the full build.
- `docs/` and `DLL/Framework/docs/` - design notes and migration plans.

The product version is set once, in `Directory.Build.props`.

## Building and testing

```sh
dotnet test GUI.Core.Tests/GUI.Core.Tests.csproj
dotnet test Installer.Core.Tests/Installer.Core.Tests.csproj
sh DLL/Framework/Tests/BuildAndRun.sh            # portable framework tests (g++/clang++)
pwsh DLL/Framework/Tests/BuildAndRun.ps1         # MSVC framework tests, Windows only
```

The repo is developed on both Windows and macOS. On macOS the configurator builds for plain `net10.0` without the
Windows-only parts, and the native DLL itself cannot be built. Keep code that only works on Windows behind
`[WindowsFact]`/`[WindowsTheory]` in tests and out of the portable framework test set.

## Mods and the manifest

- Scaffold a new mod with `Build/New-Mod.ps1 -Name <Name>`; it also lists the files in `DLL.vcxproj` and
  `.filters`, which use explicit entries, not wildcards.
- Declare a mod's settings and keybinds in its `Settings()`. Don't edit `ModManager`, `Settings.*`, `dllmain.cpp`
  or the GUI to add one.
- Mod hooks run on `MainThread` and must never block or sleep.
- Never edit `mods.manifest.json` by hand. Only the DLL writes it: regenerate it with
  `pwsh DLL/Framework/Tests/BuildAndRun.ps1 -DumpManifest` after changing any setting declaration. The pre-commit
  hook in `.githooks/` and CI both check it.

## Code style

Write code for a human reader who follows it from top to bottom.

- Separate the logical steps inside a function with a blank line, so each step reads as a paragraph.
- Do not split a function into many one-line helpers. A reader should not have to jump between definitions to follow one idea. Extract a helper when it names a real concept or is used in more than one place.
- Do not let a function grow to hundreds of lines either. When it does several distinct jobs, split it along those jobs.
- Match the surrounding code's conventions rather than introducing new ones.

## Commits

Write the subject as a short imperative sentence describing the behaviour change, without a type prefix
(e.g. "Defer the DLC installed checks at startup"). Branch off `develop`, the main branch.
