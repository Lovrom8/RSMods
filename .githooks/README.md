# Git hooks

Shared, version-controlled hooks for this repo. To enable them (one-time, per clone):

```bash
git config core.hooksPath .githooks
```

## pre-commit

Verifies that `mods.manifest.json` is still in sync with the DLL settings schema
whenever a commit touches a mod's setting declarations or the schema core
(`DLL/Mods/*.cpp|*.hpp`, `DLL/Framework/SettingsSchema.*`, `DLL/Settings.*`).

It reuses the CI verifier `DLL/Framework/Tests/BuildAndRun.ps1 -VerifyManifest`,
which dumps the schema from the last-built DLL and compares it to the committed
manifest. It is a fast local guard against "rebuilt the DLL but forgot to
re-dump the manifest"; if no DLL is built it warns and lets the commit through.
Appveyor's "Verify manifest currency" step remains the authoritative gate.

If it blocks a commit, rebuild the DLL and regenerate the manifest:

```bash
powershell DLL/Framework/Tests/BuildAndRun.ps1 -DumpManifest
```

then stage the updated `mods.manifest.json`.

Bypass once with `SKIP_MANIFEST_CHECK=1 git commit ...`, or skip all hooks with
`git commit --no-verify`.
