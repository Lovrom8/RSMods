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

The tests are part of the build, not a later step: CI runs every suite inside its build script, so a
build with a failing test is a failed build. Three Windows-only suites are not in `BuildAndRun.ps1`'s
list and have their own scripts in `DLL/Framework/Tests/Win32/`. Run them by hand when the change
touches what they cover:

```sh
pwsh DLL/Framework/Tests/Win32/BuildMemoryProtectionTests.ps1   # MemUtil, a patch site or a hook
pwsh DLL/Framework/Tests/Win32/BuildCacheStoreTests.ps1         # the cache file or its sections
pwsh DLL/Framework/Tests/Win32/BuildEnumerationTests.ps1        # the DLC scan
```

`BuildMemoryProtectionTests.ps1` executes the production stubs with stack canaries. Any change to
`MemUtil`, a patch site or a hook runs it before the change is considered done.

The repo is developed on both Windows and macOS. On macOS the configurator builds for plain `net10.0` without the
Windows-only parts, and the native DLL itself cannot be built. Keep code that only works on Windows behind
`[WindowsFact]`/`[WindowsTheory]` in tests (not an early `return`, which reports a pass that never ran) and out of
the portable framework test set: `BuildAndRun.sh` builds every `DLL/Framework/Tests/*Tests.cpp`, so Windows-only
framework tests go in `DLL/Framework/Tests/Win32/`.

The DLL builds at `/W4 /WX`; fix warnings instead of silencing them. In path literals write `\\`: `"\*"` compiles
to `"*"` with only warning C4129.

`.claude/skills/cpp-build/SKILL.md` is the procedure for fixing a failing build: the MSBuild command, MSVC
and linker errors, and the manifest rebuild.

## Mods and the manifest

- Scaffold a new mod with `Build/New-Mod.ps1 -Name <Name>`; it also lists the files in `DLL.vcxproj` and
  `.filters`, which use explicit entries, not wildcards.
- Declare a mod's settings and keybinds in its `Settings()`. Don't edit `ModManager`, `Settings.*`, `dllmain.cpp`
  or the GUI to add one.
- Mod hooks run on `MainThread` and must never block or sleep.
- Never edit `mods.manifest.json` by hand. Only the DLL writes it: regenerate it with
  `pwsh DLL/Framework/Tests/BuildAndRun.ps1 -DumpManifest` after changing any setting declaration. The pre-commit
  hook in `.githooks/` and CI both check it.
- Every key is a keybind declared in `Settings()` that the user can rebind; no fixed keys, and debug-only code is
  compiled out of Release. Don't default to keys Windows, Steam or the game already use: F11, F12, Enter, Escape,
  Space, Delete, PageUp, PageDown. The one fixed key is Alt+Enter, which forwards to the game's own F11 fullscreen
  toggle.

## Memory patching

- All code writes go through `MemUtil::PreparedPatchSet`. No raw `memcpy` or `VirtualProtect` in a mod.
- Every patch byte-checks its whole site and does nothing on a mismatch. An unknown game executable gets no patches.
- Learn & Play is relocated: its addresses are RVAs in `LnpSites`, expanded from the live module base, never a fixed
  image base.
- The game is already running when `DllMain` runs; keep loader-lock work out of it.
- Nothing allocates, logs, throws or takes a lock while other threads are suspended: prepare first, report after the
  resume. Pausing threads is opt-in (`RequireThreadExclusion()`), not the default.
- Never free a hook gateway the game can still call, even after the original bytes are restored.
- A green build and test run do not show that a patch works. Check a game boot's `RSMods_debug.txt` for the
  committed groups and for mismatch lines.

## Threads, settings and evidence

- State shared between a worker thread and `MainThread` needs synchronisation or a hand-off through
  `MainThreadInbox`; a flag plus a container read on both sides is a race.
- When a setting is renamed with `migratesFrom`, check that an old "off" cannot override the new default and
  silently disable a feature.
- Fixes are on by default. Add a toggle only when users really need the choice, never as a hotkey or environment
  variable gate.
- Resolve files relative to the game executable, not the current directory.
- A timing or race claim needs repeated alternating runs of the old and new build; one clean launch is not proof.
- Build the smallest design the current behaviour needs: no extra switches, caches or frameworks for a future use.
- Never commit binaries, logs, local profiles, game files or machine-specific paths. Keep each file's existing line
  endings.

## Code style

Write code for a human reader who follows it from top to bottom.

- Separate the logical steps inside a function with a blank line, so each step reads as a paragraph.
- Do not split a function into many one-line helpers. A reader should not have to jump between definitions to follow one idea. Extract a helper when it names a real concept or is used in more than one place.
- Do not let a function grow to hundreds of lines either. When it does several distinct jobs, split it along those jobs.
- Match the surrounding code's conventions rather than introducing new ones.
- Keep comments short and about the code: no investigation history, tester names or timings.
- Put a comment above the code it explains, on its own line, not trailing at the end of one.

`.claude/skills/cpp-code/SKILL.md` is the C++ standard for `DLL/`: the Core Guidelines plus the house rules
that override them here (naming, function size, and what hook and patch code may not do). Read it before
non-trivial C++ work. The rules in this file win if the two ever disagree.

## Commits

Write the subject as a short imperative sentence describing the behaviour change, without a type prefix
(e.g. "Defer the DLC installed checks at startup"). Branch off `develop`, the main branch.

Most commits need no body. Add one only for what a reader of the diff cannot see: why the change is shaped
this way, a limitation that survives it, or a compatibility trap. Keep it to a few plain sentences.

A commit message is not a report. Keep research notes, evidence, validation diaries, review requests and
pending work out of it: those belong in `docs/`, a pull request or an issue, where they can be corrected
after the fact. Do not format a body with markdown headings, bullet lists or backticks.
