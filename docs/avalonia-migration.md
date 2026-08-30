# RSMods GUI to Avalonia migration

This document describes the current state of the configurator migration from the existing
WinForms application to Avalonia. It is intended to be the handoff document for continuing the
work on the `avalonia-migration` branch.

Last verified: 2026-08-11 (completed Twitch migration Step 4 code cleanup; live acceptance pending).

## Current branch state

The branch contains the latest `develop` changes through merge commit `a9445ff1`, followed by
three focused migration commits:

1. `7e5fe21c` - Make ASIO and Rocksmith settings path-based instances.
2. `7e682660` - Move shared GUI services and models into `GUI.Core`.
3. `8ac1c8fe` - Add the Avalonia GUI shell.

The earlier cleanup and migration history is intentionally retained. The merge from `develop`
was resolved in favour of the typed, round-trip-safe settings architecture while incorporating
the newer validation, limits, DLL, and documentation changes from `develop`.

## Build status

All three application layers currently build:

```powershell
dotnet build GUI.Core/GUI.Core.csproj --no-restore
dotnet build GUI.Avalonia/GUI.Avalonia.csproj --no-restore
MSBuild GUI/GUI.csproj /t:Build /p:Configuration=Debug /p:Platform=x64 /p:PostBuildEvent= /v:minimal
```

- `GUI.Core` builds for both `net48` and `net8.0` with no warnings or errors.
- `GUI.Avalonia` builds for `net8.0-windows` with no warnings or errors.
- The WinForms GUI builds successfully with no warnings or errors.

The migration is Windows-first. `GUI.Avalonia` currently targets `net8.0-windows`, and
`GUI.Core` still uses Windows registry access and legacy Rocksmith libraries. Avalonia removes
the WinForms dependency; it does not make the application cross-platform by itself.

## Solution layout

### `DLL/`

The injected native mod and the actual runtime core of RSMods. It is not being migrated to
Avalonia.

### `GUI.Core/`

An SDK-style, UI-agnostic class library targeting `net48;net8.0`. It is shared by the WinForms
and Avalonia frontends and contains no WinForms or Avalonia references.

It currently owns:

- Application abstractions: `IDialogService`, `IAppEnvironment`, and `AppServices`.
- INI infrastructure: `IniManager`, `IniSection`, validation warnings, enums, and constants.
- Settings models: `RsModsSettings`, `RocksmithSettings`, and `AsioSettings`.
- Explicit-path ASIO and Rocksmith settings instances, allowing each frontend to supply its own
  installation path without relying on WinForms-oriented global path discovery.
- Shared path and key logic: `GenUtil`, `RSLocationResolver`, `KeyConversion`, and
  `RocksmithKeys`.
- Frontend-neutral numeric ranges and defaults in `RsModsLimits` and `RsAsioLimits`, so the WinForms
  and Avalonia configurators clamp identically instead of each hardcoding limits.
- Frontend-neutral ASIO device enumeration (`RSMods.ASIO.Devices`), moved out of the WinForms GUI so
  both frontends share one registry-scanning source.
- Frontend-neutral audio capture-device enumeration (`RSMods.Audio.InputDevices`) over WASAPI, added
  for the override-input-volume feature. It references only `NAudio.Wasapi` (not the `NAudio`
  meta-package, which pulls in WinForms and would force a `-windows` TFM), so `GUI.Core` stays
  UI-framework-free while both frontends can share one capture-device source.
- Frontend-neutral MIDI out/in device enumeration (`RSMods.Audio.MidiDevices`) over winmm, added for the
  auto-tune/tuning-pedal feature. It lives in the `RSMods.Audio` namespace (alongside `InputDevices`)
  rather than a new `RSMods.Midi` namespace on purpose: the WinForms GUI still has a `RSMods.Midi` *type*
  (its live-listen class), and a `RSMods.Midi` namespace would collide with it (CS0437). It covers only
  device enumeration; the WinForms live MIDI-in listening path is a debug diagnostic and is intentionally
  retired from the Avalonia migration scope.
- Shared profile services: `ProfileService`, `ProfileCodec`, `ProfileBackupService`, and
  `ProfileToneImportService`.
- Shared song and tuning logic: `SongCatalogService`, `TuningService`, and `TuningDefinition`.
- Shared SoundPack logic: the `Soundpacks` conversion/import/export/reset class and a `SoundPackService`
  facade over `audio.psarc` unpack/repack (`Packer` + `GlobalProgress`), moved out of the WinForms GUI so
  both frontends share one source. The stock `original.rs_soundpack` is embedded here (under
  `RSMods.Core.Resources`) rather than in the WinForms assembly. This added two sub-package NAudio/native
  dependencies (see below).
- Shared Set-and-Forget cache/tuning/tone logic lives in Core, with the required stock cache files embedded
  under `RSMods.Core.Resources`. The former `SetAndForgetMods` god-facade has been split into injected
  services (see below); the tuning-query facade exposes only `SongData`, strings, and `TuningStrings`, so the
  Avalonia frontend does not need to reference the Rocksmith toolkit assemblies. WMI drive detection uses the
  `System.Management` 8.0.0 package.
- Shared Twitch Step 0 foundation under `GUI.Core/Twitch`: the legacy-compatible reward model, idempotent
  23-effect catalog, normalized trigger model and matcher, atomic polymorphic XML repository, DPAPI-backed
  OAuth token store, and lifecycle-safe localhost Rocksmith effect server. The native bridge preserves the
  null-terminated JSON protocol on `127.0.0.1:45659`, uses unique request IDs, supports RGB/random Solid
  Notes, bounds its queue/retries, and shuts down with the frontend. `FlatKeyValueSettingsStore` now updates
  the unsectioned `GUI_Settings.ini` without dropping unknown keys/comments; both base settings and Twitch
  compatibility persistence use it. A new `GUI.Core.Tests` project covers these boundaries.
- Shared Twitch Step 1 runtime: Device Code Grant for a public client, protected one-use refresh-token
  rotation, startup/hourly token validation, authorized identity lookup, EventSub WebSockets, four Helix
  subscriptions, bounded notification-ID deduplication, keepalive/planned reconnect/unexpected reconnect
  handling, structured state, bounded logs, and application-scoped orchestration through `TwitchService`.
- The `WinMsgUtil` WM_COPYDATA helper (pure user32 P/Invoke), used to ask the running game to play a
  result voice-over.
- Other shared models and helpers such as `Dictionaries`, `GuitarSpeak`, `ColorItem`, and `KeybindItem`.

`GUI.Core` still references the existing libraries under `GUI/Lib`, including
`RocksmithToolkitLib`, `Rocksmith2014PsarcLib`, and (for SoundPacks) `SevenZipSharp`. Those references
build on both current target frameworks, but they are an important constraint for future portability and
maintenance. SoundPack MP3 decoding also pulls in `NAudio.WinMM` (used via `Mp3FileReaderBase` +
`AcmMp3FrameDecompressor`, not the convenience `Mp3FileReader`, which lives in the WinForms-dragging
NAudio meta-package). Like the existing `NAudio.Wasapi`, this is a sub-package, so it does not force a
`-windows` TFM. The native `7z64.dll` and the Wwise/ogg toolchain are not managed references; each
frontend copies them next to its executable (the WinForms GUI via its post-build xcopy of `Lib\`, the
Avalonia app via an equivalent `Copy` target).

### `GUI/`

The existing .NET Framework 4.8 WinForms configurator. It remains the complete production UI
while features are migrated incrementally.

It now acts primarily as a frontend and integration layer:

- `UI.*.cs` partial classes and `UI.Designer.cs` contain WinForms presentation and event wiring.
- `WinFormsDialogService` and `WinFormsAppEnvironment` adapt the shared abstractions.
- `Program.cs` initializes `AppServices` before the form reads shared state.
- WinForms-specific control wiring (`RsModsLimits.ApplyToUiControls`, `RsAsioLimits.ApplyToUiControls`)
  and warning presentation remain in the GUI project; the numeric ranges they apply now come from the
  shared `GUI.Core` `RsModsLimits`/`RsAsioLimits`, which the WinForms classes re-export so existing
  callers stay unchanged. ASIO device enumeration moved to `GUI.Core`.
- The retained WinForms Twitch tab runs entirely over the shared `TwitchService` through a small
  `TwitchRuntime` adapter. The old PubSub/implicit-auth/chatbot implementations and TwitchLib dependencies
  have been removed.
- MIDI live-listen remains WinForms-only by design: it is a debug-output diagnostic, not a user-facing
  migration gap.

The old-style project references the `net48` leg of `GUI.Core` explicitly.

### `GUI.Avalonia/`

The new Avalonia 11.3.12 application targeting `net8.0-windows` and x64. It currently provides:

- Application startup and a compiled Avalonia XAML shell.
- Dependency injection using `Microsoft.Extensions.DependencyInjection`.
- MVVM support through `CommunityToolkit.Mvvm`.
- `AvaloniaDialogService`, including owned message dialogs and folder selection through the
  Avalonia storage provider.
- `AvaloniaAppEnvironment`, including application shutdown and base-directory access.
- `AppServices` initialization with the Avalonia adapters.
- A typed `MainWindowViewModel` and status view.
- A `StartupService` that runs the real startup resolution once the window is shown: it resolves
  the Rocksmith install and save folders through `RSLocationResolver`, persists them to the shared
  `GUI_Settings.ini`, loads `RsModsSettings`, and collects INI validation warnings.
- `ContentControl`-based page navigation: a `StatusViewModel` landing page (startup status and
  resolved locations) and a `ModSettingsViewModel` first settings screen, mapped to their views by
  `DataTemplate`s keyed on view-model type.
- A `SettingsService` boundary over the static `RsModsSettings` store (async save) and a
  `SettingsWarningPresenter` that surfaces `IniValidationWarning`s at the Avalonia edge.
- A `RocksmithSettingsViewModel`/`RocksmithSettingsView` editing `Rocksmith.ini` through the
  path-based `RocksmithSettings` instance (built by `RocksmithSettingsService` from the resolved
  install folder), lazily loaded on first navigation.
- An `AsioSettingsViewModel`/`AsioSettingsView` editing `RS_ASIO.ini` through the path-based
  `AsioSettings` instance (built by `AsioSettingsService`), including the tri-state WASAPI output
  mode, per-channel driver selection from the shared device enumeration, and the two disable
  conventions (blank vs commented driver).
- A `ColorsViewModel`/`ColorsView` editing the string, note, and highway colours in the shared
  `RsModsSettings` store (via `SettingsService`), lazily loaded on first navigation. Each string/note
  palette keeps both a normal and a colour-blind set; the screen holds all of them and persists only
  the cells the user changed, so untouched defaults are never baked into the INI.
- A `SoundPacksViewModel`/`SoundPacksView` driving the shared `SoundPackService`: unpack/repack
  `audio.psarc` (determinate progress), replace any of the 18 result voice lines, import/export a
  `.rs_soundpack`, reset to stock, and preview a result voice-over in the running game. Action-oriented
  like Profiles (an `IsBusy` gate, off-thread `Task.Run`), lazily initialised on first navigation.
- A `SetAndForgetViewModel`/`SetAndForgetView` covering the one-click `cache.psarc` mods and cache
  management: custom tunings, increased volume, Exit Game, Direct Connect, drive-aware Fast Load,
  restore/reset/unpack/cleanup, and importing existing settings. It is action-oriented and lazily prepares
  the stock working files on first navigation; all WMI, archive, filesystem, and process work runs off the
  UI thread behind an `IsBusy` gate. The same screen now includes the custom-tunings editor over the shared
  `TuningDefinitionList`, with live note names and settings-aware string-colour swatches, plus an async song
  scan that lists arrangements using the selected definition and arrangements that show as Custom Tuning.
  The final slice loads tones from Steam profiles and assigns them to the three default or ten Guitarcade
  cache slots.
- All settings and tool navigation entries enable once startup resolution succeeds.

The Avalonia frontend now covers the user-facing configurator screens over the `net8.0` leg of `GUI.Core`.
Installer/publishing integration and the decision to make it the default frontend remain.

### `Installer/`

The existing installer. It is not currently part of the Avalonia work and keeps its independent
helper code where applicable.

## Completed migration work

### UI decoupling

- Split the large WinForms form into feature-oriented `UI.*.cs` partial classes.
- Introduced asynchronous dialog and application-environment abstractions.
- Added WinForms adapters without changing the existing UI behaviour.
- Removed WinForms dialog and application-lifetime decisions from shared path and domain logic.

### Shared Core extraction

- Added the multi-targeted `GUI.Core` project.
- Moved the INI backend, settings models, path resolution, key conversion, profiles, song
  management, tuning, and related services into it.
- Kept existing namespaces where practical to avoid needless frontend churn.
- Removed direct WinForms references from `GUI.Core`.
- Verified both the `net48` and `net8.0` targets.

### Settings integration

- Replaced the old raw read/write settings helpers with typed settings models.
- Made `AsioSettings` and `RocksmithSettings` explicit-path, instance-based services.
- Preserved the newer `develop` defaults and limits, including tri-state WASAPI output mode.
- Connected invalid INI value reporting to the WinForms settings sanitizer.
- Preserved comments and unknown INI entries through the round-trip-safe backend.

### Avalonia foundation

- Added the Avalonia project to the solution.
- Implemented the shared host abstractions for Avalonia.
- Added DI, MVVM, the initial window, status display, folder picker, and message dialog.
- Verified that the Avalonia frontend consumes `GUI.Core` successfully.

### Avalonia startup resolution

- Added `StartupService`, which the main window runs once on `Opened` so its dialogs have an owner.
- Resolves the Rocksmith install folder (mandatory; shutdown on give-up) and save folder (optional)
  through `RSLocationResolver`, storing both in the shared `Constants` state used by WinForms.
- Persists the resolved paths to `GUI_Settings.ini` and loads `RsModsSettings` from `RSMods.ini`.
- Collects `IniValidationWarning`s raised during the load and surfaces a summary dialog.
- Replaced the shell-only folder-picker/dialog-test buttons with the real resolved-location view and
  a "Set save folder" action; navigation entries enable once resolution succeeds.
- Handled cancellation and shutdown through the Avalonia adapters rather than `Application.Exit`.

### Avalonia settings boundary and first screen

- Extracted the numeric ranges/defaults from the WinForms `RsModsLimits` into a shared `GUI.Core`
  `RsModsLimits`; the WinForms class now re-exports them (no caller churn) and keeps its control wiring.
- Added a `SettingsService` (async save through the typed store) and a `SettingsWarningPresenter`
  (Avalonia-owned validation-warning dialog).
- Added `ContentControl`/`DataTemplate` page navigation with a `StatusViewModel` landing page.
- Ported a first Mod Settings screen (`ModSettingsViewModel` + `ModSettingsView`) covering toggle,
  enum, and numeric settings, with dirty tracking, Save, and Revert. It binds an editable snapshot
  loaded from `RsModsSettings`, reuses the shared ranges, and saves through the round-trip-safe
  `IniManager`, preserving comments and unknown entries.
- Extended that screen with the playback & timing group: the millisecond-backed settings
  (`CheckForNewSongsInterval`, `RewindBy`, `RewindLeadup`, `LoopingLeadUp`, `CustomNSPTimeLimit`)
  presented as seconds (`/1000` on load, `*1000` on save, matching WinForms), each gated by its
  parent toggle (`AllowRewind`, `AllowLooping`, `UseCustomNSPTimer`, and re-enumeration mode) so the
  numeric field only shows while the feature is enabled.
- Added the visual toggles group: the plain in-game show/hide booleans (green screen wall, remove
  inlays, remove lane markers, screenshot scores, show current note) plus the "remove headstock" and
  "remove lyrics" toggles paired with their `HeadstockMode`/`LyricsMode` "when" enum and the
  `SkylineMode` "when" for the existing Skyline toggle. Each "when" combo is gated on its parent
  toggle, mirroring the WinForms group-box visibility.
- Added the audio and fixes/misc toggle groups: the plain booleans that need no backing service or
  custom UI (control game volume + its gated `VolumeControlInterval` numeric, audio-in-background,
  linear Riff Repeater, remove previews, fix bass tuning, fix Oculus crash, prevent buggy tones,
  show current accuracy, prevent mid-song pause, remove fingerprints, bypass 2+ RTC message box).
- Added the extended range, secondary monitor, and alternative-output-sample-rate groups (each a
  toggle gating its detail controls):
  - Extended range: moved `ExtendedRangeModeAt` out of the generic numeric section into a named-tuning
    dropdown (`D`..`Octave Down`, mapped to the stored `-index - 2` offset), plus the drop-tuning
    toggle. The WinForms UI convenience of force-enabling custom string colours when extended range is
    turned on was deliberately not ported: the DLL reads `CustomStringColors` independently
    (`ExtendedRangeMode.cpp` handles `CustomStringColors == 0` fine during an ER song), so it is a
    visual preference, not a domain rule. It belongs with the (not-yet-ported) colours screen, where
    the relationship is visible, rather than as a hidden side-effect on an unshown setting here.
  - Secondary monitor: the `OnOffMode` toggle plus a "set to current window position" button that
    captures the Avalonia window's on-screen position (`Window.Position`), matching the WinForms
    workflow (drag the window to the target monitor, then click). The window is passed to the command
    from the view (`$parent[Window]`) so the view-model stays window-agnostic; the captured position
    is shown read-only.
  - Alternative output sample rate: the toggle plus a sample-rate dropdown (44100..192000 Hz).
- Added the profile auto-load group: the `ForceProfileLoad` toggle plus a profile dropdown bound to the
  shared `Profiles.AvailableProfiles()` list (the profiles in the resolved save folder) and a clear
  button. An empty selection maps to `ProfileToLoad = ""`; a stored profile that is no longer present is
  kept in the list so a saved name is never silently dropped. Uses the existing shared profile service,
  so no new enumeration infrastructure or package was needed.
- Added the override-input-volume group: the `OverrideInputVolumeEnabled` toggle gating a capture-device
  dropdown (bound to the new shared `RSMods.Audio.InputDevices` WASAPI enumeration) and the input-volume
  numeric, which moved out of the generic Numeric section into this group to match the WinForms layout.
  An empty/absent device maps to `OverrideInputVolumeDevice = ""`, and a stored device no longer present
  is kept in the list so a saved name is never silently dropped.
- Added the on-screen text font group (`OnScreenFont`): a font-family dropdown enumerated from Avalonia's
  `FontManager.Current.SystemFonts`, the existing `OnScreenFontSize` numeric (moved here from the generic
  Numeric section), and a live preview `TextBlock` rendered in the selected font — mirroring the WinForms
  font tab (which used `InstalledFontCollection` and preview labels). A stored font that is not currently
  installed is kept in the list so a saved name is never dropped. The remaining generic numeric (Riff
  Repeater speed step) now stands in its own small "Riff Repeater" group.
- Added the MIDI auto-tune & tuning-pedal group: the `AutoTuneForSong` master toggle gating a MIDI-out
  device (the pedal) bound to the new shared `RSMods.Audio.MidiDevices` winmm enumeration, the
  `TuningPedalDevice` model dropdown, the `ChordsMode` toggle (shown only for Whammy Bass/Five, matching
  the WinForms group-box visibility), the target `TuningOffset` presented as its 16 tuning labels
  (offset = list index - 3, via the shared `RsModsLimits.MidiOffsetToListIndex`), an optional MIDI-in
  device (`MidiInDevice`), and the `AutoTuneForSongWhen` trigger enum. A stored device or pedal that is
  no longer present is kept in its list so a saved value is never silently dropped. Two deliberate
  divergences from WinForms: the "Enabled" MIDI-in live-listen checkbox was not ported (it only wrote MIDI
  events to `Debug.WriteLine` as a developer diagnostic, with no user-visible output, and needs no
  persisted setting); and the pedal selection round-trips every `TuningPedalDevice` value, including
  `WhammyFour`, which the WinForms load switch silently ignored.
- Added the Guitar Speak group: the `GuitarSpeak` master toggle gating the `GuitarSpeakWhileTuning` toggle
  and a mapping editor. The editor reuses the shared `Dictionaries.GuitarSpeakKeybinds` (15 key-press
  rows) as a small `GuitarSpeakRowViewModel` list; picking a note + octave and clicking "assign" stores
  the MIDI note number on the selected row (`noteIndex + 36 + (octaveIndex - 3) * 12`, the exact WinForms
  math), and each row's note is displayed through the shared `GuitarSpeak.GuitarSpeakNoteOctaveMath`. A
  clear button unsets the selected row. The WinForms "note saved" confirmation popup was dropped as
  redundant — the row list updates in place — keeping the quieter Avalonia snapshot/Save flow.
- Added the keybindings group: the shared mod (12) and audio (10) binds from `Dictionaries`, each a
  `KeybindRowViewModel` showing its current key (via `KeyConversion.VKeyToUI`). Selecting a bind and
  pressing a key — or a middle/side mouse button — in the per-list capture box assigns it. The view's
  code-behind captures the input with tunnelling `KeyDown`/`KeyUp`/`PointerPressed` handlers (so Tab,
  Space, and arrows are captured rather than moving focus or typing) and forwards the framework key name
  plus a capture phase to the view-model, which runs the shared `RocksmithKeys.Classify` policy: key-down
  keys on key-down, key-up-only keys on key-up, the supported mouse buttons on press, and a confirmation
  dialog (`IDialogService`) before accepting a Rocksmith-reserved key — mirroring the WinForms warning.
  Unsupported inputs are ignored. Key-name coverage is whatever the shared `KeyConversion` map provides;
  a few framework-specific names (e.g. Avalonia's `LeftShift`) are unmapped and simply ignored, as in the
  WinForms path. All edits fold into the existing snapshot + Save/Revert + dirty model; the transient
  selection state is excluded from dirty tracking, and child-row edits mark the snapshot dirty. This
  required threading `IDialogService` into `ModSettingsViewModel`.

### Avalonia Rocksmith settings screen

- Added `RocksmithSettingsService`, which lazily constructs the path-based `RocksmithSettings`
  instance from the resolved install folder (`Constants.RSFolder`).
- Ported the full `Rocksmith.ini` screen (`RocksmithSettingsViewModel` + `RocksmithSettingsView`):
  Audio, Renderer.Win32, and Net settings across toggles, enums (`FullscreenMode`,
  `VisualQualityMode`, `MsaaMode`), numerics, and a device string, with the same snapshot + dirty +
  Save/Revert pattern as Mod Settings.
- Loaded lazily on first navigation and surfaced its validation warnings through the (now
  file-name-aware) `SettingsWarningPresenter`; writes run off the UI thread.

### Avalonia RS_ASIO settings screen

- Moved ASIO device enumeration (`Devices`/`DriverInfo`) from the WinForms GUI into `GUI.Core`,
  dropping its WinForms `MessageBox` dependency (best-effort, returns an empty list on failure).
- Extracted the `RsAsioLimits` ranges/defaults/buffer-mode constants and validation helpers into
  `GUI.Core`; the WinForms class re-exports them and keeps its control wiring.
- Added `AsioSettingsService` (lazy `AsioSettings` instance + device-name enumeration) and ported the
  full `RS_ASIO.ini` screen (`AsioSettingsViewModel` + `AsioInputViewModel` + `AsioSettingsView`):
  Config, buffer, Output, and the three inputs (0/1/Mic).
- Preserved the store's semantics: tri-state `WasapiOutputMode`, blank-driver disable for
  Output/Input.0/Input.Mic, and commented-driver disable for Input.1 (value kept). Modeled the two
  software-volume-control settings as distinct fields (the WinForms handlers conflated them).

### Avalonia Custom Colors screen

- Ported the WinForms colours tab into a dedicated `ColorsViewModel`/`ColorsView` navigation entry,
  writing back through the shared `RsModsSettings` store (`SettingsService`), lazily loaded on first
  navigation like the other screens.
- Covered all three colour groups: the six string colours, the six note colours, and the four highway
  colours (numbered/un-numbered frets, noteway sides, fret numbers), each with an enable toggle
  (`CustomStringColors`, `SeparateNoteColors`, `CustomHighwayColors`).
- Modeled the note-colour tri-state faithfully: the master `SeparateNoteColors` toggle plus a "use
  Rocksmith's note colours" toggle map onto `SeparateNoteColorsMode` (`Off`/`RocksmithColors`/`Custom`),
  hiding the custom note swatches while Rocksmith's own colours are in use.
- Kept both the normal and colour-blind palettes for strings and notes in the snapshot (the game
  chooses between them), with a per-section palette switch, and persisted only the cells the user
  changed via a small `ColorSwatchViewModel` (loaded-value baseline) so untouched defaults are never
  materialised into the INI — matching WinForms, which writes a colour only when it is picked.
- Used a dependency-free swatch: an editable 6-digit hex field with a live preview `Border` fed by a
  one-way `HexToBrushConverter`, rather than adding the `Avalonia.Controls.ColorPicker` package (its
  control themes are not bundled with `FluentTheme` and would need separate theme wiring). A visual
  colour-wheel picker is a possible later enhancement.
- Did not port the WinForms convenience of force-enabling `CustomStringColors` when a swatch is edited;
  the toggle stays an explicit user choice, consistent with the extended-range decision that deferred
  this relationship to the colours screen.

### Avalonia Appearance (themes) screen

- Replaced the WinForms tri-colour configurator recolouring (which walked every control setting
  `BackColor`/`ForeColor`) with the idiomatic FluentTheme model: a light/dark/system variant plus an
  optional accent colour, applied live to the running `Application` by a small `ThemeService`.
- Persisted two new keys in the shared `[GUI Settings]` section (`AppThemeVariant`, `AppAccentColor`),
  kept separate from the WinForms tri-colour keys so both frontends round-trip their own appearance.
- Applied the accent by overriding the seven `SystemAccentColor*` shade resources (computed by blending
  toward white/black) so FluentTheme controls pick it up; "Default" removes the overrides.
- Made the shell header/nav chrome theme-aware via `ThemeDictionaries` brushes so a light theme does not
  leave a dark header; the mid-grey card borders read acceptably on both and were left unchanged.
- Applied the saved appearance before the window is shown for returning users (see "Later work" item 8
  for the resulting double-load of `RSMods.ini`); first run falls back to applying after resolution.

### Avalonia Profiles screen (core slice)

- Ported the first slice of the WinForms Profiles tab into a `ProfilesViewModel`/`ProfilesView`
  navigation entry, driving the shared `ProfileService` and `ProfileBackupService` directly.
- Unlike the settings screens this is action-oriented, not a snapshot: selecting a profile decrypts and
  activates it, and each action (add/remove song list, lock/unlock rewards, restore a backup) executes and
  persists immediately — mirroring the WinForms tab, which has no Save/Revert here. File-bound work
  (decrypt, encrypt, restore) runs off the UI thread via `Task.Run` behind an `IsBusy` gate that disables
  the action commands while an operation is in flight.
- Covered by the core slice: profile selection (with song-list count), Add/Remove song list (with the
  6–20 bounds messages), rewards unlock/lock (with the confirmation prompts), and the backups list + Revert
  (with the same date-format/missing-folder error handling as WinForms), all over the existing
  Info/Confirm/Error dialogs.
- Added the tone-import slice: importing CDLC tone manifests (`.json`, `ProcessToneManifests`) and Toolkit
  tone templates (`.tone2014.xml`, `ProcessXmlTonesByName`), with the WinForms "select multiple" toggle and
  the per-tone guitar/bass prompt for XML tones. This needed two `IDialogService` additions —
  `PickFilesAsync` (multi-file open with pattern filters) and `ShowChoiceAsync` (two custom-labelled
  buttons, used for Guitar/Bass) — implemented in both the Avalonia and WinForms adapters. The XML import
  runs off the UI thread via `Task.Run`; each guitar/bass decision marshals back to the UI thread (a
  `Dispatcher.UIThread.Post` + `TaskCompletionSource` that blocks the worker until the user chooses).
  A string-based `Profiles.ProcessXmlTonesByName` was added alongside the existing `Func<Tone2014,bool>`
  overload (a distinct name, not an overload, so the Avalonia frontend needn't reference the legacy
  `RocksmithToolkitLib` just to name `Tone2014`).
- Added the song-list/Favorites grid slice: a "Load songs" action runs the shared
  `SongManager.ExtractSongDataAsync` psarc scan (off-thread, with a determinate progress bar), filters to
  the owned/shipping songs via `GetOwnedRS1DLC` + `ShouldIncludeSong`, and populates a `DataGrid` of
  Artist/Title plus a Favorites checkbox column and one checkbox column per song list (titled from the
  shared song-list titles). Toggling a checkbox writes straight to the in-memory profile
  (`SetSongInFavorites`/`SetSongInList`); a "Save song lists" button persists via `EncryptCurrentProfile`,
  mirroring the WinForms in-memory-then-save model.
  - This needed the `Avalonia.Controls.DataGrid` package (+ its Fluent theme include in `App.axaml`).
    Avalonia builds `DataGrid` columns in code, so the columns are (re)built in the view's code-behind from
    view-model column descriptors (`SongListColumn`); the checkbox columns use reflection bindings to an
    indexer path (`Cells[i].IsChecked`) on a small `SongRowViewModel`/`SongCellViewModel` pair.
  - Because the column set depends on the profile's song-list count, the grid is cleared (and must be
    reloaded) when the selected profile changes or a song list is added/removed. Rewards/import/song-list
    edits are disabled while a load is in flight, and vice versa.
  - The WinForms all-columns-at-once designer quirks (fixed 23-column array, hardcoded Favorites column
    index) were not reproduced; the Avalonia grid derives its columns from the profile's actual list count.
  - This completes the Profiles tab.

### Avalonia SoundPacks screen

- Moved the SoundPack backing logic out of the WinForms GUI into `GUI.Core`: the `Soundpacks` conversion/
  import/export/reset class (now `public`, with `Application.StartupPath`/executing-assembly swapped for
  `AppServices.Environment.BaseDirectory` and the Core assembly) and the `WinMsgUtil` WM_COPYDATA helper.
  The stock `original.rs_soundpack` is embedded in `GUI.Core`. This added the `SevenZipSharp` reference and
  the `NAudio.WinMM` sub-package to Core; MP3 decode uses `Mp3FileReaderBase` + `AcmMp3FrameDecompressor`
  to avoid the WinForms-dragging NAudio meta-package.
- Added a `SoundPackService` facade owning the paths, the unpacked-state check, and `audio.psarc`
  unpack/repack via `Packer`, bridging the toolkit's static `GlobalProgress` events to `IProgress<>` for
  the duration of the work (and forcing the same LOH-compacting GC the WinForms executor did). It also
  exposes the 18-line voice catalogue, the result-VO list, and thin replace/import/export/reset/play
  pass-throughs. All prompts stay in the view-model (the service is dialog-free).
- Ported the tab as `SoundPacksViewModel`/`SoundPacksView`: a two-state layout (Unpack prompt vs the
  replace/repack/manage panel), an `ItemsControl` of labelled voice-line rows each with a Replace button,
  the import/export/reset/remove-unpacked/download-Wwise/report-bug actions, and a result-VO list + Play.
  Unpack/repack show a determinate progress bar + operation label; the other file-bound ops run off-thread
  behind the `IsBusy` gate. External links open via `Process.Start(UseShellExecute)`, not WinForms.
- Export needed a save-file picker, so `IDialogService.PickSaveFileAsync` was added and implemented in both
  the Avalonia (`StorageProvider.SaveFilePickerAsync`) and WinForms (`SaveFileDialog`) adapters.
- Native tooling: the shared code calls `7z64.dll` and the Wwise/ogg toolchain by path relative to the app
  base. The WinForms GUI ships these by xcopying its `Lib\` tree into the output; the Avalonia project now
  does the equivalent via a `Copy` MSBuild target so the same natives sit next to its executable. WAV/OGG
  conversion still requires the user's separately-installed Wwise, exactly as before.
- One faithful correctness fix carried into the shared code: `ImportSoundFile` now extracts under the
  `RSMods` folder (where the unpacked archive lives and where Export reads from) instead of the current
  working directory. The old CWD-relative path only lined up because the WinForms build ran from that
  folder; it would have been wrong for Avalonia.
- This completes the SoundPacks tab.

### Set-and-Forget Core preparation

- Moved `SetAndForgetMods` and `ZipUtilities` from the WinForms project into `GUI.Core`; archive-injection
  errors now propagate as `IOException` instead of opening a WinForms `MessageBox` from shared code.
- Moved the ten cache-mod resources used by Set-and-Forget into `GUI.Core/Resources` and changed all
  extraction sites to resolve the Core assembly and `RSMods.Core.Resources` manifest namespace.
- Added the frontend-neutral tuning facade: unknown-tuning display keys and copied six-string offsets,
  plus song lookup by a tuning's internal name. Updated the WinForms screen to use this facade too.
- Verified `GUI.Core` net48/net8.0 and `GUI.Avalonia` with zero warnings; WinForms also builds, retaining
  only its three pre-existing Twitch `CS4014` warnings. No Avalonia UI was added in this preparation step.

### Avalonia Set-and-Forget screen (cache-mod slice)

- Added the `Set & Forget` navigation entry and a `SetAndForgetViewModel`/`SetAndForgetView`, lazily
  initialised after startup resolution. First navigation extracts the Core-owned stock cache-mod files and
  loads the tuning definitions without eagerly unpacking `cache.psarc`.
- Added the five one-click cache mods: Add Custom Tunings, Turn It Up to Eleven, Exit Game, Direct Connect,
  and Fast Load. The Fast Load flow performs WMI drive detection off-thread and preserves the WinForms HDD,
  NVMe, and unknown-drive confirmation behavior before selecting the mid/max intro asset.
- Added cache management: Restore Cache Backup, Steam file validation plus temporary-cache removal, Unpack
  Again, Remove Temporary Folders, and Import Existing Settings. Steam URIs use
  `ProcessStartInfo.UseShellExecute`; destructive/recovery actions retain confirmation prompts.
- Refactored `SetAndForgetMods.RestoreDefaults` into a dialog-free boolean Core operation. Both frontends now
  own confirmation/result dialogs, and the WinForms handler also performs the restore off-thread.
- Every operation uses one shared `IsBusy` command gate and `Task.Run` boundary, with exceptions surfaced via
  `IDialogService`. Core net48/net8.0 and Avalonia build with zero warnings; WinForms retains only its three
  pre-existing Twitch `CS4014` warnings.

### Avalonia Set-and-Forget screen (custom-tunings editor)

- Added a tuning list with the WinForms-compatible `<New>` sentinel and an editor for the internal key,
  display name, UI localisation index, and all six string offsets (`-24..24`). Existing internal keys are
  read-only because they are dictionary keys; selecting another tuning reloads its shared
  `TuningDefinitionInfo` snapshot.
- Added `TuningStringViewModel` rows with the six MIDI bases (`40, 45, 50, 55, 59, 64`). Offset changes
  immediately recalculate the displayed note through `GuitarSpeakNoteOctaveMath`; the high-E label retains
  the WinForms lowercase convention.
- Each string has a live swatch using `StringColors.GetStringColor`. It switches to the colour-blind/
  extended-range palette when Extended Range is enabled and that string reaches
  `ModSettings.ExtendedRangeModeAt`; revisiting the screen refreshes colours changed elsewhere.
- Add validates blank and duplicate internal names through `IDialogService`; Save updates the selected
  definition; Remove deletes it. Unlike WinForms' in-memory Add/Remove followed by a separate Save click,
  all three Avalonia actions call `SaveTuningsJSON` immediately and roll back the in-memory dictionary if
  persistence fails. The separate Add Custom Tunings action still injects the saved database into the game.

### Avalonia Set-and-Forget screen (songs-by-tuning lists)

- Added a Load Songs action using the shared `SongManager.ExtractSongDataAsync` scan and its `Progress<int>`
  percentage. It uses the screen's existing global command gate while showing a dedicated determinate
  progress bar; scan failures clear both result lists and surface through `IDialogService`.
- The selected-definition list is populated through `GetSongsWithSelectedTuning` and refreshes whenever the
  tuning selection changes or an existing definition is saved. Selecting `<New>` deliberately shows no
  matches.
- The Custom Tuning list is populated through `GetUnknownTuningKeys`. It is recomputed after Add, Save,
  Remove, Restore Backup, or Import Existing Settings so a changed definition set immediately reclassifies
  loaded arrangements.
- Load Selected into `<New>` resolves the facade's cached key through `GetUnknownTuningStrings`, switches the
  editor to `<New>`, and copies all six offsets. The internal/display names stay blank for the user to define;
  no existing tuning is overwritten. No toolkit type is named by the Avalonia project.

### Avalonia Set-and-Forget screen (default and Guitarcade tones)

- Added Load Profile Tones over `GetSteamProfilesTones`, performed off-thread behind the screen-wide busy
  gate. The returned names are sorted for selection; an empty profile scan and scan failures are presented
  through `IDialogService`. The shared loader now keeps the first occurrence of a duplicate tone name across
  multiple profiles instead of aborting the entire load on a dictionary collision.
- Added the three default targets with the shared indices expected by Core (`Rhythm = 0`, `Lead = 1`,
  `Bass = 2`) and retained Lead as the initial selection, matching WinForms.
- Added all ten Guitarcade targets in the exact WinForms/Core index order: Temple of Bends, Scale Warriors,
  String Skip Saloon, Scale Racer, Ninja Slide N, Hurtlin' Hurdles, Harmonic Heist, Ducks Redux, Rainbow
  Laser, and Gone Wailin'.
- Both assignment paths call `SetDefaultTones`/`SetGuitarArcadeTone` off-thread, then present the returned
  success or failure message. The shared tone update now unpacks `cache.psarc` on demand when `cache7.7z` is
  absent and checks extraction success, removing the old reliance on WinForms' eager startup unpack.
- This completes the Avalonia Set-and-Forget screen.

### Set-and-Forget service split (post-migration cleanup)

- Retired the `SetAndForgetMods` static god-facade (five responsibilities plus three unrelated pieces of
  global state) now that WinForms is gone and the screen is its only consumer. It became four injected,
  singleton-registered Core services under `RSMods.SetAndForget`.
- `CachePsarcService` owns the `cache.psarc` lifecycle (unpack, backup, repack, restore, cleanup, import,
  default-file staging) and a single `Modify(cache => cache.Inject(...))` primitive that centralises the
  "ensure unpacked → mutate → repack" round-trip every mod used to repeat. `CacheModification.Inject` throws
  when an injection does not take, so a partially-applied mod can no longer be reported as success. The
  self-contained one-file mods (exit-game, direct-connect, increased-volume) and `AddCustomTunings` are thin
  methods over `Modify`.
- The existing `TuningService` is now injected directly into the view-model instead of being forwarded
  through the facade. The former shared `unknownTunings` dictionary is gone: `GetUnknownTuningLookup` returns
  an immutable `UnknownTuningLookup` the view-model holds, and `Load()`/`Save()` convenience overloads keep
  the custom-mods path out of the frontend.
- `ProfileToneService` owns profile enumeration, the imported `Tone2014` cache (now instance state, not a
  static dictionary), and tone-manager updates through `CachePsarcService`.
- `FastLoadService` owns the drive-prompt decision and intro-asset selection behind an injected
  `IDriveInfoProvider` boundary (`WmiDriveInfoProvider` is the production implementation), covered by
  `FastLoadServiceTests` with a fake provider.
- Fixed `ImportExistingSettings`, which previously always returned `true`: it now reports whether both
  extractions succeeded, and the view-model surfaces a failure instead of silently continuing.
- `GUI.Core`, `GUI.Core.Tests` (32 passing), and `GUI.Avalonia` build with zero warnings.

### Twitch migration (Step 0 shared foundation)

- Moved the polymorphic Twitch reward types and native effect request/response DTOs into `GUI.Core`, then
  replaced repeated singleton catalog appends with `TwitchRewardCatalog.CreateDefaults` and moved enabled
  reward XML loading/saving behind an atomic `TwitchRewardRepository`. Existing
  `TwitchEnabledEffects.xml` files retain their schema and all three derived reward types.
- Added `TwitchTriggerEvent`/`TwitchRewardMatcher` for exact Bits and Channel Point amount matching plus
  subscription fan-out, ready for the EventSub payload adapter in Step 1.
- Added `FlatKeyValueSettingsStore` for the legacy unsectioned `GUI_Settings.ini` format and repointed
  `Constants.SaveBaseSettings`, `GenUtil` parsing, and WinForms Twitch compatibility persistence. Unknown
  settings, comments, blank lines, ordering, and values containing `=` now survive named updates.
- Added a DPAPI current-user `TwitchTokenStore`. WinForms imports the legacy plaintext `AccessToken` once,
  writes it to `TwitchAuth.dat`, and removes the plaintext key. The UI reports only whether authorization is
  stored; copied diagnostics redact the token.
- Replaced the WinForms-local `EffectServerTCP` with the shared `RocksmithEffectServer`: a bounded,
  cancellation-aware, application-lifetime listener with ordered dispatch, null-frame parsing, unique IDs,
  native status-3 retry handling, explicit milliseconds, connection replacement/recovery, and deterministic
  shutdown from `MainForm_FormClosing`.
- Added `GUI.Core.Tests` with 10 passing tests for catalog idempotence, legacy XML round-tripping, reward
  matching, flat-settings preservation, protected token migration/metadata, request construction, fragmented
  TCP responses/retries, and disconnected shutdown. Core builds both targets with zero warnings; Avalonia
  builds with zero warnings; WinForms retains only its two pre-existing legacy Twitch `CS4014` warnings.
- No Avalonia Twitch UI or Twitch API transport was added in this step. Step 1 replaces the permanently
  retired PubSub source with Device Code Grant plus EventSub WebSockets and first proves it through the
  existing WinForms tab.

### Twitch migration (Step 1 shared runtime and WinForms compatibility)

- Added `TwitchOptions`, `TwitchAuthService`, and `TwitchApiClient`. The required public client ID and the
  three scopes live in one options object; Device Code polling honors Twitch's interval and expiry, token
  refresh atomically replaces the one-use refresh token, validation runs at startup/hourly, and identity is
  read from Helix without exposing OAuth material.
- Added `TwitchEventSubClient` over an injectable `ClientWebSocket` transport. It creates the cheer, custom
  reward redemption, subscribe, and subscription-message subscriptions after the welcome frame; normalizes
  payloads into `TwitchTriggerEvent`; deduplicates every notification ID with a bounded TTL cache; enforces
  keepalive deadlines; follows planned reconnect URLs without duplicating subscriptions; and uses capped
  backoff plus fresh subscriptions after unexpected disconnects.
- Added application-scoped `TwitchService` ownership of auth/session state, EventSub, reward matching, the
  effect bridge, validation/retry loops, and a 500-entry structured log. Revoked/invalid authorization stops
  the active session and moves to an explicit reauthorization-required state.
- Replaced the WinForms `TwitchLib.PubSub` singleton with a UI-thread adapter over `TwitchService`. Startup is
  independent of tab navigation, authorization uses Twitch's verification URI and user code, the legacy
  150-second force-reauth/resubscribe timer is disabled, reward tests use bridge connection state, reward
  edits update the running matcher, and form close cancels authorization and disposes the complete runtime.
- Expanded `GUI.Core.Tests` to 20 passing tests, including Device Code pending/success, public refresh without
  a client secret, the exact four EventSub subscription requests, all four payload mappings, bounded
  deduplication, planned reconnect without resubscription, valid startup orchestration, and missing-scope
  reauthorization. Core, Avalonia, and WinForms build with no new warnings. A real-account Twitch smoke test
  is deferred until the RSMods developer application is configured as a public client.

### Twitch migration (Step 2 Avalonia runtime screen)

- Registered the application-scoped Twitch runtime and its existing Core dependencies in Avalonia startup;
  the service starts after normal path/settings resolution even if the Twitch page is never opened and is
  disposed with the application.
- Added a `TwitchViewModel`/`TwitchView` and main-shell navigation. The screen shows account, EventSub, and
  Rocksmith bridge status; authorized identity; authorize/reauthorize, cancel, forget, start, and stop actions;
  the Twitch device code and verification link; and a bounded timestamped log with clear/save actions.
- Browser launch, save-file selection, dialogs, and UI-thread dispatch remain frontend concerns. The
  view-model unsubscribes from Core events on disposal without owning or stopping the application runtime.
- Avalonia and both Core targets build with zero warnings.

### Twitch migration (Step 3 reward editor)

- Added the stock effect catalog and enabled-rule grid with explicit Subscription/Bits/Channel Points
  selection, editable duration and applicable amount, stable legacy IDs, and remove/save/revert actions.
- Added inline Core-backed validation, Solid Notes Random/RGB editing with a live swatch, and selected-rule
  testing through the existing Rocksmith effect bridge.
- Kept the existing XML schema and moved TurboSpeed enable/disable handling into the shared Core rule-update
  path so Avalonia and WinForms stay consistent.
- The Core suite now has 21 passing tests; Avalonia and both Core targets build with zero warnings.

### Twitch migration (Step 4 cleanup and hardening)

- Removed the retired PubSub, implicit-auth, localhost auth-server, chatbot, response-model, TwitchLib, and
  `System.Web` code; renamed the retained WinForms compatibility adapter to `TwitchRuntime`.
- Removed the WinForms reveal/copy-token controls and obsolete reauthorization timer. The remaining
  authorization indicator reports only whether protected credentials are stored.
- Documented plaintext-token migration and “Forget authorization” behavior, and added a regression assertion
  that runtime logs contain neither access nor refresh tokens.
- All 21 Core tests pass, and Core (`net48`/`net8.0`), Avalonia, and WinForms build without warnings. The real
  Twitch/Rocksmith smoke checklist remains a release-acceptance task because credentials and a running game
  are not available in this workspace.

## The DLL seam

Everything else in this document measures the migration against WinForms. This section is the other
contract, and it is the one that does not show up in a feature-parity sweep: what the configurator owes
the mod DLL. Both defects found the first time it was audited (a setting written to the wrong section, and
the live-reload message going missing entirely) were invisible to every screen-by-screen parity check,
because the GUI looked and behaved correctly — the game just never saw the result.

Since the DLL's modular mod framework landed on `develop`, it groups settings by *owning mod*, while the
configurator groups them by *INI section*. Nothing in either codebase ties a key to its consumer, so the
rules below plus the guard test are what hold the seam together.

### Telling a running game to reload

`RSMods.ini` is not polled. The DLL re-reads it only on a WM_COPYDATA message (`dwData == 1`) handled by
`Keybindings::UpdateSettingsOnGUIChange`:

- `update all` — full reload via `Settings::UpdateSettings()`. This is what the configurator sends.
- `update <custom|setting> <entry> <value>` — single-setting update via `Settings::ParseSettingUpdate`.
- `WwiseEvent <name>`, `enable`/`disable <effect>`, `Reconnect` — non-settings traffic (SoundPacks, Twitch).

The DLL routes settings messages through `Registry().EnqueueSettingsUpdate`, which posts them to the
framework's `MainThreadInbox`; the next registry tick drains the batch and notifies each mod via
`OnSettingsChanged` before re-resolving activation. **Skipping the message means none of that runs**, so a
saved setting sits inert until the next game launch.

`SettingsService.SaveAsync` is the single place this happens, because `RsModsSettings.Save()` is the only
path that writes the file — the `IniSection` setters mutate memory and raise `SettingChanged` without
persisting. A new screen that saves mod settings should go through `SettingsService`, not the static store.

### Adding or moving a setting

- **The section is part of the contract, not just the key.** The DLL names both (`reader.GetValue("Mod
  Settings", "OnScreenFontSize", 24)`), and this store is section-aware, so the same key under the wrong
  heading reads as absent and the DLL silently falls back to its default. The retired WinForms reader was
  a section-blind line scan, which is why this class of bug could not happen before and can now.
- **Put a new property in the nested class matching the DLL's section**, not the one matching the screen
  it appears on. `OnScreenFont` and `OnScreenFontSize` sit on the same slider pair but live in different
  sections, because that is what the DLL reads.
- **Keybinds seed empty, never the DLL's default.** The DLL's hardcoded defaults (`"T"`, `"5"`, …) apply
  only when a key is absent from the file entirely; both GUIs have always written `""`, leaving a bind
  unset until the user assigns one.
- **Fix a misplaced key before the build ships, not after.** Round-trip preservation means a key written
  to the wrong section survives in users' files indefinitely, so correcting it later costs a migration
  step and a way to delete keys — neither of which this store has, deliberately. While the Avalonia
  configurator is unreleased, moving a property is just moving a property.

### The guard test

`GUI.Core.Tests/SettingsKeyParityTests.cs` scrapes `DLL/Settings.{cpp,hpp}` for every `(section, key)` the
DLL reads, reflects `RsModsSettings` for every one the GUI writes, and asserts both directions. It covers
declarative keys only — runtime-built names (`string{n}_N`, `SongListTitle_{i}`) drop out symmetrically —
and `[GUI Settings]` is excluded as frontend-only.

Its `KnownUnexposed` allowlist is the live inventory of settings the DLL reads that no GUI exposes. All
three current entries are dead on the DLL side too, so the list should only ever shrink.

## Current migration boundary

The user-facing configurator screens are migrated: Mod Settings, Custom Colors, Rocksmith, RS_ASIO,
Appearance, Profiles, SoundPacks, Set-and-Forget, and Twitch all run through Avalonia and shared Core logic.
The WinForms live MIDI-in checkbox is intentionally retired from migration scope because it only writes
diagnostic messages to the debugger. Twitch's code migration is complete; its real-account/game smoke test
remains a release-acceptance check.

The remaining migration work is installer/publishing support, choosing Avalonia as the default frontend,
and the optional longer-term removal of Windows/native and legacy Rocksmith-library constraints.

## Recommended next work

Integrate the Avalonia executable into installer/publishing, run release acceptance (including the live
Twitch/Rocksmith checklist), and decide when Avalonia becomes the default frontend. The completed slices
below remain as implementation history.

### 1. Complete Avalonia startup resolution — done

- Invoked `RSLocationResolver` asynchronously (via `StartupService`) before loading settings screens.
- Stored the resolved Rocksmith and save paths in the same shared `Constants` state used by WinForms.
- Handled cancellation and shutdown through the Avalonia adapters.
- Replaced the shell-only folder picker with the real startup flow.

### 2. Add an Avalonia settings service/view-model boundary — done

- Loaded `RsModsSettings` only after startup resolution succeeds (`ModSettings.Load()` post-startup).
- Exposed a bindable snapshot (`ModSettingsViewModel`) instead of binding controls to static settings.
- Saved through the existing typed `IniManager`/`IniSection` models via `SettingsService`.
- Surfaced `IniValidationWarning` in the Avalonia-owned `SettingsWarningPresenter`.
- Did not copy the WinForms controls or the lossy INI implementation from older prototype ports.

### 3. Port the first settings screen — done

- Enabled the Mod Settings navigation entry after startup.
- Started with simple toggle, enum, and numeric settings (with dirty tracking, Save, Revert).
- Reused the domain defaults and ranges (shared `RsModsLimits`) rather than duplicating values in XAML.
- Saved through the round-trip-safe `IniManager`, which preserves unknown sections, comments, and
  commented-out values.
- Kept the equivalent WinForms screen working throughout.

Now covered by this screen: the toggle/enum/numeric slice, the playback & timing group (the
ms/seconds settings `CheckForNewSongsInterval`, `RewindBy`, `RewindLeadup`, `CustomNSPTimeLimit`,
`LoopingLeadUp`), the visual toggles group (show/hide booleans plus the headstock/lyrics/skyline
"remove when" enums), the audio + fixes/misc plain-toggle groups, and the extended range, secondary
monitor, and alternative-output-sample-rate groups.

Custom string/note/highway colours are now covered by the dedicated Custom Colors screen (step 5), and
profile auto-load, the override-input-volume device, the on-screen text font, and the MIDI auto-tune &
tuning-pedal group are covered on the Mod Settings tab (via the shared profile service, the shared
`RSMods.Audio.InputDevices` enumeration, Avalonia's system-font enumeration, and the new shared
`RSMods.Audio.MidiDevices` enumeration respectively). Guitar Speak and the mod/audio keybindings are now
covered too (reusing the shared `Dictionaries` binds, `GuitarSpeak` note math, `KeyConversion`, and the
`RocksmithKeys` capture policy), which completes the Mod Settings tab.

### 4. Follow with Rocksmith and RS_ASIO — done

- Constructed `RocksmithSettings` and `AsioSettings` with the paths supplied by startup state and
  ported both full screens, reusing their typed properties and validation events.
- Preserved RS_ASIO's commented-driver behaviour and tri-state WASAPI output mode.
- Moved ASIO device enumeration into `GUI.Core` as a frontend-neutral source before binding it.

### 5. Port the Custom Colors screen — done

- Added `ColorsViewModel`/`ColorsView` (plus a small `ColorSwatchViewModel` and a `HexToBrushConverter`)
  as a fourth settings navigation entry, saving through `SettingsService`/the round-trip-safe store.
- Covered the string, note, and highway colour groups with their enable toggles, the note-colour
  tri-state (`Off`/`RocksmithColors`/`Custom`), and both normal and colour-blind palettes.
- Persisted only user-changed cells so untouched defaults stay out of the INI, and kept the swatch UI
  dependency-free (hex entry + live preview) rather than pulling in `Avalonia.Controls.ColorPicker`.

The core settings screens plus Custom Colors are now in place, and the Mod Settings tab is
feature-complete (through Guitar Speak and the mod/audio keybindings). Remaining work is the "Later work"
list below.

## Architectural rules to keep

- `GUI.Core` must not reference WinForms or Avalonia.
- Frontends own dialogs, navigation, control state, and warning presentation.
- Shared logic should throw or return domain results; the frontend decides how to present them.
- Prefer explicit dependencies and paths over hidden global discovery.
- Keep settings round-trip-safe. Saving must not remove unknown keys, comments, or sections, and the
  store has no way to delete a key — so a key written to the wrong section is effectively permanent
  once released. Get the section right before shipping.
- Settings must match the section the DLL reads them from, and saving mod settings must tell a running
  game to reload. See "The DLL seam" — WinForms parity does not imply DLL parity.
- Add abstractions only when a real caller needs them.
- Keep the Avalonia frontend free of the legacy `RocksmithToolkitLib`/`Rocksmith2014PsarcLib` references.
  When a shared API takes a toolkit type (e.g. `Func<Tone2014,bool>`), expose a distinct-named
  string/POCO-based variant in the facade for the frontend rather than an overload — an overload forces the
  call site to resolve against the toolkit type and drags in the reference (CS0012).
- Keep the WinForms application buildable until the corresponding Avalonia feature is complete.
- Treat `DLL/` as the mod core and `GUI.Core/` as the shared configurator core; their roles are
  intentionally different.

## Later work

After the core settings screens are functional:

1. Port GUI theme settings — done (see "Avalonia Appearance (themes) screen"). Custom colours and
   keybindings are also done (see step 5 and the Mod Settings tab).
2. Port profiles and song-list management using the shared profile services — done (see "Avalonia Profiles
   screen"): the core slice (profile selection, song-list count add/remove, rewards, backups), the
   tone-import slice (JSON + XML import, which added `PickFilesAsync`/`ShowChoiceAsync` to `IDialogService`),
   and the song-list/Favorites grid slice (async psarc loading + a `DataGrid` with dynamic checkbox columns,
   which added the `Avalonia.Controls.DataGrid` package).
3. Port Set-and-Forget tuning and tone workflows — done. Core preparation, one-click cache mods/cache
   management, the custom-tunings editor, song tuning lists, and default/Guitarcade tone assignment are all
   available from the Avalonia screen.
4. Port SoundPacks — done (see "Avalonia SoundPacks screen"): moved the `Soundpacks`/`WinMsgUtil` backing
   code into `GUI.Core` behind a `SoundPackService` facade (added the `SevenZipSharp`/`NAudio.WinMM` Core
   deps and `IDialogService.PickSaveFileAsync`), ported the tab, and copied the native tooling next to the
   Avalonia executable. Live MIDI-in listening is intentionally retired, and the Twitch UI migration is done.
5. Evaluate replacing the legacy Rocksmith libraries with the modern Rocksmith2014.NET stack.
6. Decide whether to remain Windows-only or remove the remaining registry, native-library, and
   packaging constraints required for broader platform support.
7. Add installer/publishing support and decide when Avalonia becomes the default frontend.
8. Unify the double load of `RSMods.ini` for returning users. To avoid a flash of the default theme
   before the saved appearance applies, `App.OnFrameworkInitializationCompleted` now loads settings and
   applies the appearance early (`TryApplySavedAppearanceEarly`) whenever the Rocksmith folder is already
   known — the common case — while `StartupService.RunAsync` still loads `RSMods.ini` a second time so it
   can collect INI validation warnings inside its subscription window. The extra read/probe is harmless
   and the two loads resolve to the same folder, but the warning-collection flow could be restructured so
   settings load exactly once. First-run users (folder not yet resolved through dialogs) still fall back
   to the post-resolution apply and may briefly see the default theme.

## Definition of completion

The migration is complete when:

- Every supported WinForms feature has an Avalonia equivalent or an explicit retirement decision.
- The DLL contract holds, not just WinForms parity: `SettingsKeyParityTests` passes, and saving mod
  settings takes effect in a running game without restarting it. Parity with a retired frontend says
  nothing about the seam the DLL actually reads — see "The DLL seam".
- Settings files round-trip without data loss.
- Startup, dialogs, errors, and shutdown are fully asynchronous at the Avalonia edge.
- `GUI.Core`, WinForms, and Avalonia builds remain green during the transition.
- Packaging produces a runnable Avalonia configurator with the required native and managed
  dependencies.
- The WinForms frontend can be removed without moving domain logic back into a UI project.
