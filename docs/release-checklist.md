# RSMods QC Fork Release Checklist

Use this checklist before publishing a new `qc-*` release tag in your fork.

## 1. Source + Branch Readiness

- Confirm local branch is clean: `git status`
- Confirm upstream attribution text is still present in `README.md`
- Confirm release docs are up to date:
  - `docs/install-readme.md`
  - `docs/qc-use-readme.md`
  - `docs/qc-architecture.md`
  - `docs/release-notes-template.md`

## 2. Build + Packaging

Run from repo root:

```bat
build-rsmods-deliverable.bat Release Win32 qc-v1.0.0
```

Required outputs in `dist/`:

- `RSMods-QC-qc-v1.0.0-win32.zip`
- `RSMods-QC-qc-v1.0.0-portable.zip`
- `RSMods-QC-qc-v1.0.0-installer.exe` (if installer build exists)
- `SHA256SUMS.txt`

## 3. Package Content Validation

- `win32.zip` contains:
  - `portable/xinput1_3.dll`
  - `portable/RSMods/RSMods.exe`
  - `docs/INSTALL-README.md`
  - `docs/QC-USE-README.md`
  - `docs/QC-ARCHITECTURE.md`
  - `BUILD-INFO.txt`
- `portable.zip` contains:
  - `xinput1_3.dll`
  - `RSMods/RSMods.exe`
  - docs + `BUILD-INFO.txt`
- `SHA256SUMS.txt` hashes match all emitted artifacts.

## 4. Smoke Test (Tester Focus)

- Install using installer path and verify game still starts.
- Install using manual copy path and verify game still starts.
- Open `RSMods.exe`, verify QC tab loads, and settings persist.
- Verify MIDI device/channel selection works.
- Verify classifier preview + song preview still operate.
- Verify no startup regressions in non-QC gameplay paths.

## 5. Release Note Prep

- Fill in `docs/release-notes-template.md`
- Include upstream base/reference commit hash.
- Include explicit tester guidance:
  - what to download
  - installer vs manual install
  - Quad Cortex setup requirements
  - known limitations
  - issue reporting location
