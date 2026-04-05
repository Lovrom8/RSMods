# RSMods QC Fork Install Guide

This fork supports two user paths:

- **Tester path (no source build):** use downloadable release artifacts.
- **Developer path:** build from source locally.

## Tester Path (Ready-Built Releases)

From this fork's GitHub Releases page, download either:

- `RSMods-QC-<tag>-win32.zip` (full bundle), or
- `RSMods-QC-<tag>-installer.exe`, or
- `RSMods-QC-<tag>-portable.zip`.

### Installer

1. Run `RSMods-QC-<tag>-installer.exe`.
2. Select your Rocksmith 2014 install folder.
3. Complete install.

### Manual (Portable)

1. Extract `RSMods-QC-<tag>-portable.zip`.
2. Copy `xinput1_3.dll` into Rocksmith root (same folder as `Rocksmith2014.exe`).
3. Copy the extracted `RSMods` folder into Rocksmith root.
4. Launch Rocksmith and open `RSMods\RSMods.exe`.

## Developer Path (Build From Source)

From repo root:

```bat
build-rsmods-deliverable.bat Release Win32 qc-v1.0.0
```

Artifacts are written to `dist/`.

## Notes

- Close Rocksmith before replacing files.
- `xinput1_3.dll` must be in the game root.
- QC usage and configuration are documented in `docs/qc-use-readme.md`.
