# RSMods GUI to Avalonia migration

This document describes the current state of the configurator migration from the existing
WinForms application to Avalonia. It is intended to be the handoff document for continuing the
work on the `avalonia-migration` branch.

Last verified: 2026-08-10 (Mod Settings extended range / secondary monitor / sample rate slice).

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
- The WinForms GUI builds successfully. Its three existing `CS4014` warnings are in Twitch code
  and are unrelated to this migration.

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
- Shared profile services: `Profiles`, `ProfileService`, `ProfileCodec`,
  `ProfileBackupService`, and `ProfileToneImportService`.
- Shared song and tuning logic: `SongManager`, `TuningService`, and `TuningDefinition`.
- Other shared models and helpers such as `Dictionaries`, `GuitarSpeak`, `MemoryStream`,
  `ColorItem`, and `KeybindItem`.

`GUI.Core` still references the existing libraries under `GUI/Lib`, including
`RocksmithToolkitLib` and `Rocksmith2014PsarcLib`. Those references build on both current target
frameworks, but they are an important constraint for future portability and maintenance.

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
- UI-bound features such as ASIO device enumeration, SoundPacks, MIDI, Twitch, themes, and
  WinForms dialogs remain here until their Avalonia slices are implemented.

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
- All three settings navigation entries (Mod Settings, Rocksmith, RS_ASIO) enable once startup
  resolution succeeds.

The shell proves that Avalonia, DI, MVVM, dialogs, startup resolution, settings loading, navigation,
and a real settings screen compose over the `net8.0` leg of `GUI.Core` and run together. It is not
yet a full replacement for the WinForms configurator.

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

## Current migration boundary

The domain and settings foundation is shared, Avalonia startup resolves the Rocksmith folders and
loads settings, and the Mod Settings, Rocksmith, and RS_ASIO screens all load and save through the
shared stores. The main remaining gap on these three screens is the Mod Settings tab, which now
covers the toggle/enum/numeric slice, the playback & timing (ms/seconds) group, the visual toggles
group, the audio + fixes/misc plain toggles, and the extended range / secondary monitor / sample
rate groups, but still omits the service-backed feature toggles (colours, MIDI, tuning pedal,
profiles, font picker, override-input-volume device), keybinds, and Guitar Speak; the more
specialised tools (profiles, tuning, SoundPacks, MIDI, Twitch, themes) are still WinForms-only.

The most important remaining frontend dependencies are:

- WinForms control creation, designer layout, validation display, and event wiring.
- Startup orchestration that currently lives in `MainForm`.
- ASIO device enumeration and UI-specific selection state.
- MIDI and Twitch integration.
- SoundPack workflows and other features that still present dialogs directly from the WinForms
  edge.
- Installer and packaging decisions for shipping the Avalonia executable.

## Recommended next vertical slice

Implement startup and the basic Mod Settings screen before porting the more specialised tools.

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

Not yet covered (remaining Mod Settings work): the feature toggles that need a backing service or
richer custom UI — custom string/note/highway colours (colour pickers), MIDI/auto-tune and tuning
pedal (device enumeration), profile auto-load (profile list), on-screen font picker, and the
override-input-volume device — plus keybinds and Guitar Speak.

### 4. Follow with Rocksmith and RS_ASIO — done

- Constructed `RocksmithSettings` and `AsioSettings` with the paths supplied by startup state and
  ported both full screens, reusing their typed properties and validation events.
- Preserved RS_ASIO's commented-driver behaviour and tri-state WASAPI output mode.
- Moved ASIO device enumeration into `GUI.Core` as a frontend-neutral source before binding it.

The three core settings screens are now in place. Remaining work is the "Later work" list below,
plus completing the Mod Settings tab (see the note under step 3).

## Architectural rules to keep

- `GUI.Core` must not reference WinForms or Avalonia.
- Frontends own dialogs, navigation, control state, and warning presentation.
- Shared logic should throw or return domain results; the frontend decides how to present them.
- Prefer explicit dependencies and paths over hidden global discovery.
- Keep settings round-trip-safe. Saving must not remove unknown keys, comments, or sections.
- Add abstractions only when a real caller needs them.
- Keep the WinForms application buildable until the corresponding Avalonia feature is complete.
- Treat `DLL/` as the mod core and `GUI.Core/` as the shared configurator core; their roles are
  intentionally different.

## Later work

After the core settings screens are functional:

1. Port keybindings and colour/theme settings.
2. Port profiles and song-list management using the shared profile services.
3. Port Set-and-Forget tuning and tone workflows.
4. Port SoundPacks, MIDI, and Twitch features.
5. Evaluate replacing the legacy Rocksmith libraries with the modern Rocksmith2014.NET stack.
6. Decide whether to remain Windows-only or remove the remaining registry, native-library, and
   packaging constraints required for broader platform support.
7. Add installer/publishing support and decide when Avalonia becomes the default frontend.

## Definition of completion

The migration is complete when:

- Every supported WinForms feature has an Avalonia equivalent or an explicit retirement decision.
- Settings files round-trip without data loss.
- Startup, dialogs, errors, and shutdown are fully asynchronous at the Avalonia edge.
- `GUI.Core`, WinForms, and Avalonia builds remain green during the transition.
- Packaging produces a runnable Avalonia configurator with the required native and managed
  dependencies.
- The WinForms frontend can be removed without moving domain logic back into a UI project.
