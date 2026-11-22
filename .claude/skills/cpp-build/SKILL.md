---
name: cpp-build
description: Fix a failing build or test run in this repository (MSBuild/MSVC for the DLL, dotnet for the configurator and installer) one error at a time with minimal changes. Use when the DLL, the framework tests or the managed tests fail to build.
---

# Fixing a failing build

Incremental procedure for a red build: run it, group the errors, fix one cause at a time, rebuild
after each fix. `AGENTS.md` is the authority and the `cpp-code` skill owns style; this file adds
only the build mechanics. Do not refactor while fixing.

## The toolchain

- The native DLL is Win32 only, C++20 (`stdcpp20`), `/W4` with `TreatWarningAsError`, toolset
  `v143`, Windows SDK `10.0`. All of it is declared in `DLL/DLL.vcxproj`; do not override it.
- The configurator and installer are .NET 10 (`dotnet`). The portable framework tests use g++ or
  clang++ and run on macOS and Linux too.

## Commands

Native DLL, the same command CI runs (from the repo root):

```sh
msbuild DLL/DLL.vcxproj /t:Build /p:Configuration=Release /p:Platform=Win32 /p:SolutionDir="<repo root>\" /v:minimal /nologo
```

The trailing backslash on `SolutionDir` matters. Replace `Release` with `Debug` only when the task
is about Debug.

Tests:

```sh
pwsh DLL/Framework/Tests/BuildAndRun.ps1         # MSVC framework tests, Windows only
sh DLL/Framework/Tests/BuildAndRun.sh            # portable framework tests (g++/clang++)
dotnet test GUI.Core.Tests/GUI.Core.Tests.csproj
dotnet test Installer.Core.Tests/Installer.Core.Tests.csproj
```

`BuildAndRun.ps1` finds MSVC through vswhere, builds each test in its `$Tests` list and runs it.
`BuildAndRun.sh` builds every `DLL/Framework/Tests/*Tests.cpp`, so a test that includes `Windows.h`
belongs in `DLL/Framework/Tests/Win32/`.

For a complete local check follow CI's order (`appveyor.yml`, `build_script`): test GUI.Core,
publish the configurator, create the payload, test Installer.Core, test the mod framework, build
the native DLL, verify the manifest, publish the installer.

## Procedure

1. **Run the build** and keep the full output, not just the tail.
2. **Group by file** and read the first error of each file. Later errors in the same file are often
   fallout from it.
3. **Fix one cause at a time**, with the smallest change that removes it.
4. **Rebuild after each fix.** Stop and report if the same error survives three attempts, a fix adds
   more errors than it removes, the fix needs an architectural change, or a dependency is missing.
5. **Run the tests** that cover the touched code once the build is green, then report what was
   fixed and what remains.

## Reading MSVC output

- A diagnostic reads `path(line,col): error C####: text` or `warning C####: text`. The file and
  line come first.
- In a template error the first line is rarely the useful one. Read down to the last
  `see reference to ... being compiled` note: it names your call site, which is where the fix goes.
- Errors are reported per translation unit, so a bad header repeats once per `.cpp` that includes
  it. Fix the header once.

## Warnings are errors

The DLL builds at `/W4 /WX`, so every warning is a build failure. Fix the cause. Never add a
`#pragma warning(disable)`, lower the warning level or drop `TreatWarningAsError` to get a green
build.

C4129 is the known trap: in a path literal `"\*"` compiles to `"*"` with only a warning, so the
separator silently vanishes. Write `"\\*"` or a raw literal (`AGENTS.md`, "Building and testing").

## Linker errors

LNK2019 and LNK2001 (unresolved external) usually mean one of:

- a function declared but never defined, or defined with a different signature or `const`;
- a new `.cpp` that is not listed in `DLL/DLL.vcxproj` (the project uses explicit entries, not
  wildcards), or not in `DLL/DLL.vcxproj.filters`;
- a calling-convention or `extern "C"` mismatch between the declaration and the definition;
- a member of a class template or an `inline` function defined in a `.cpp` instead of the header.

List a new source file in both project files. `Build/New-Mod.ps1 -Name <Name>` does that for a new
mod. LNK2005 (already defined) means a non-`inline` definition in a header.

## Common errors

| Error | Typical fix |
|-------|-------------|
| C2065 undeclared identifier | Add the `#include` or fix the name |
| C2664 cannot convert argument | Fix the argument type or add the overload |
| C2440 cannot convert | Fix the type; do not add a cast to silence it |
| C4100 unreferenced parameter | Leave the parameter unnamed or `[[maybe_unused]]` |
| C4244 / C4267 narrowing | Use the right type or an explicit `static_cast` |
| C2511 / C2039 no member | Match the declaration in the header |
| LNK2019 / LNK2001 | See above |

## Manifest

`mods.manifest.json` is generated, never hand-edited. After a change to any setting declaration,
rebuild the DLL first, then run `pwsh DLL/Framework/Tests/BuildAndRun.ps1 -DumpManifest`. The dump
reads the built DLL, so a stale build gives a stale manifest. CI runs the same script with
`-VerifyManifest` after building the DLL.

## Reporting

Build the configuration the task is about. Do not switch between Debug and Release to make an
error go away, and name the configuration every reported error came from. A green build and test
run do not show that a patch executes; see `AGENTS.md`, "Memory patching".
