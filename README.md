# RSMods-QC-MIDI

An unofficial public fork of **[Lovrom8/RSMods](https://github.com/Lovrom8/RSMods)** focused on **Neural DSP Quad Cortex MIDI automation for Rocksmith 2014 Remastered**.

This fork is aimed at testers and power users who want:
- automatic QC preset switching from Rocksmith tone changes
- manual QC target overrides from Rocksmith tone slots `2`, `3`, and `4`
- song-level transpose scene automation for supported tunings
- ready-built beta downloads instead of building from source

## Important Scope

This project targets **Rocksmith 2014 Remastered on Windows**.

It is **not** for Rocksmith+ / “Rocksmith 2024”.

## Quick Start

If you just want to test the mod:

1. Go to **[Releases](../../releases)**
2. Download the latest **prerelease / beta** build
3. Follow the **[Install Guide](docs/install-readme.md)**
4. Open `RSMods.exe`
5. Configure the **QC Automation** tab using the **[QC Automation Use Guide](docs/qc-use-readme.md)**

## What This Fork Adds

The main public addition in this fork is **QC Automation**.

QC Automation can:
- map Rocksmith authored tone names to QC preset targets
- send QC preset recalls over USB MIDI
- support manual in-song overrides from Rocksmith tone slots
- keep a song-level transpose scene active for supported semitone-down tunings

For details, see:
- **[QC Automation Use Guide](docs/qc-use-readme.md)**
- **[QC Architecture Notes](docs/qc-architecture.md)**

## Downloads / Tester Artifacts

Tester releases are published as GitHub release assets. Depending on the release, you should expect to find:
- installer `.exe`
- portable `.zip`
- Win32 bundle `.zip`
- `SHA256SUMS.txt`

If you are not building from source, use the release assets instead of cloning the repo.

## Documentation

- **[Install Guide](docs/install-readme.md)**
- **[QC Automation Use Guide](docs/qc-use-readme.md)**
- **[Known Issues and Limits](docs/known-issues.md)**
- **[QC Architecture Notes](docs/qc-architecture.md)**
- **[QC Integration Plan](docs/qc-integration-plan.md)**
- **[Release Checklist](docs/release-checklist.md)**
- **[Release Notes Template](docs/release-notes-template.md)**
- **[Upstream RSMods Repository](https://github.com/Lovrom8/RSMods)**

## Known Limits

Current QC automation limits are documented in **[Known Issues and Limits](docs/known-issues.md)**.

The short version:
- no true-tuning / cent automation in v1
- no transpose expression automation
- transpose is handled as song-level scene state
- classifier quality depends on authored tone naming in the song data

## Reporting Bugs / Tester Feedback

When reporting an issue, include:
- release tag / prerelease tag tested
- song and arrangement tested
- whether the issue happened on auto tone change or manual override
- QC target mapping used
- MIDI channel / device used
- relevant RSMods log lines

Please open issues in this fork for fork-specific QC automation problems.

## Build From Source

If you want to build locally instead of using tester releases, see the existing repo scripts and docs. The current packaging path is documented in the install/release docs.

## Fork Notice / Attribution

This is an **unofficial public fork** of upstream RSMods.

The original upstream project remains:
- **[Lovrom8/RSMods](https://github.com/Lovrom8/RSMods)**

This fork keeps upstream attribution and exists to develop and publish the QC MIDI workflow additions in a tester-friendly form.
