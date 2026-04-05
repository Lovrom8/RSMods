# RSMods QC Fork Release Notes Template

## Highlights

- `<feature/change 1>`
- `<feature/change 2>`
- `<bugfix/stability item>`

## What To Download

Use one of these release assets:

- `RSMods-QC-<tag>-win32.zip` (full tester bundle)
- `RSMods-QC-<tag>-installer.exe` (installer only, when provided)
- `RSMods-QC-<tag>-portable.zip` (manual install path)
- `SHA256SUMS.txt` (integrity hashes)

## Installation

### Installer path

1. Run `RSMods-QC-<tag>-installer.exe`.
2. Point it to Rocksmith 2014 folder if not auto-detected.

### Manual path

1. Extract `RSMods-QC-<tag>-portable.zip`.
2. Copy `xinput1_3.dll` into Rocksmith root (same folder as `Rocksmith2014.exe`).
3. Copy the `RSMods` folder into Rocksmith root.

## Quad Cortex Setup (Tester Quick Start)

- Set QC Automation MIDI output to Quad Cortex MIDI USB port.
- Confirm MIDI channel matches QC receive channel.
- Ensure QC preset targets exist in your My Presets bank.
- For transpose v1, scene map is `A=0, B=-1, C=-2, D=-3, E=-4, F=-5, G=-6, H=-7`.

## Known Limitations

- `<limitation 1>`
- `<limitation 2>`
- `<limitation 3>`

## Report Issues

Please open issues in this fork repository and include:

- RSMods QC release tag (`qc-vX.Y.Z`)
- Rocksmith song/arrangement context
- Quad Cortex preset/scene setup
- relevant `RSMods` log lines

## Upstream Base Reference

- Upstream repository: `https://github.com/Lovrom8/RSMods`
- Upstream base commit: `<commit_sha>`
