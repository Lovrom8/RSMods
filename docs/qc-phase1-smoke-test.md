# QC Phase 1 Smoke Test

This smoke test validates the current `QCAutomation` Phase 1 skeleton only:
- in-song lifecycle wiring
- one-time test CC send on song start
- guard behavior when device/config is missing

It does **not** validate tone detection, timeline logic, or transpose automation.

## Required INI keys

Set these in `RSMods.ini`:

```ini
[Toggle Switches]
QCAutomationEnabled = on
QCAutomationTestOnSongStart = on
QCAutomationDevice = <part of your QC MIDI OUT device name>

[Mod Settings]
QCAutomationMidiChannel = 0
QCAutomationSceneCC = 43
QCAutomationTestSceneValue = 0
```

Notes:
- `QCAutomationMidiChannel` is `0-15` (MIDI channel 1 is `0` in this code path).
- `QCAutomationSceneCC` and `QCAutomationTestSceneValue` are clamped to `0-127`.

## Expected behavior

When entering a song:
- exactly one QC test CC is sent for that song session
- no repeated sends every frame while staying in that song
- after returning to menus and starting another song, one send is allowed again

If `QCAutomationDevice` is empty:
- no MIDI send occurs
- one warning is logged per song-entry cycle

If `QCAutomationDevice` does not match a real MIDI OUT device:
- no MIDI send occurs
- one warning is logged per song-entry cycle

## Expected log messages

Success path:
- `(QC) Sent test scene CC on song start. CC=<n> Value=<n> Channel=<n>`
- `Sending Raw MIDI Message: CC<n>: <controller> <value>`

Missing device guard:
- `(QC) QCAutomation is enabled, but QCAutomationDevice is empty. Skipping MIDI send.`

Configured device not found guard:
- `(QC) QCAutomationDevice did not match any MIDI OUT device. Skipping MIDI send.`

## Manual test steps

1. Configure the INI keys above and start Rocksmith with RSMods loaded.
2. Enter any song once.
3. Verify one QC CC action happens and success logs appear once.
4. Stay in song for at least 10 seconds; verify no repeated `(QC) Sent test scene...` logs.
5. Exit to menu and enter a song again.
6. Verify one new test send occurs for the new song session.
7. Set `QCAutomationDevice =` (empty), re-test, and verify warning path with no MIDI send.
8. Set `QCAutomationDevice =` to a non-existent name, re-test, and verify no send with the not-found warning.

## Troubleshooting

- If no MIDI send occurs and no warning appears:
  - verify both toggles are `on`
  - verify RSMods is loading the updated `RSMods.ini`
- If device is configured but no send occurs:
  - verify the substring in `QCAutomationDevice` matches an actual MIDI OUT device name
  - check available device logs from MIDI initialization output
