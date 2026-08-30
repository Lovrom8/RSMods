# Set-and-Forget migration plan

> **Status: complete.** All slices below have shipped and the WinForms GUI has been removed. The Core
> `SetAndForgetMods` facade this plan produced was subsequently split into injected `CachePsarcService`,
> `TuningService`, `ProfileToneService`, and `FastLoadService` — see the "Set-and-Forget service split"
> entry in `docs/avalonia-migration.md`. This document is retained for historical context.

A step-by-step plan for porting the WinForms Set-and-Forget tab (`GUI/UI.SetAndForget.cs`, backed by
`GUI/SetAndForgetMods.cs`) to Avalonia, following the same shared-Core pattern used for SoundPacks and
Profiles. Written as a handoff so a future session can execute it slice by slice.

## Why this is the thorny one

1. **Toolkit-type leak (the key constraint).** `SetAndForgetMods` exposes `ArrangementTuning`
   (`Rocksmith2014PsarcLib`) on its public surface (`ToArrangementTuning`, `GetUnknownTunings` →
   `SortedDictionary<string, ArrangementTuning>`, `GetSongsWithTuning(..., ArrangementTuning)`,
   `GetDefinedTunings`, `IsTuningStandard/Drop`). `GUI.Avalonia` does **not** reference the toolkit, so
   naming `ArrangementTuning` at an Avalonia call site is a CS0012 compile error. A POCO facade is required.
   - Good news: `SongData` (`GUI.Core/Rocksmith/SongManager.cs`) and `TuningDefinitionInfo`/`TuningDefinitionList`
     (`GUI.Core/SetAndForget/Models/TuningDefinition.cs`, plain `UIName` + `Dictionary<string,int> Strings`)
     are already Core POCOs and can be used from Avalonia directly. Only `ArrangementTuning` must be hidden.
2. **WMI drive detection.** `GetDriveType`/`GetDriveSpecifications` use `System.Management` (net48 in-box;
   needs a package on net8.0).
3. **Embedded resources live in the WinForms assembly** and must move to `GUI.Core` (as
   `original.rs_soundpack` did): `tuning.database.json`, `maingame.csv`, `introsequence_{mid,max,original}.gfx`,
   `sltsv1_aggregategraph.nt`, `ui_menu_pillar_{main,mission,startup}.database.json`, `init.bnk`.
4. **`ZipUtils.cs` has a WinForms `MessageBox`** (`GUI/Util/ZipUtils.cs:135`) that must be removed when it
   moves to Core (throw/return; the frontend presents the error).
5. **Big multi-panel UI** → split into 4 slices behind one Core-prep step.

## Pieces already in place to reuse

- `GUI.Core`: `TuningService`, `TuningDefinitionInfo`/`List`, `SongManager`/`SongData`, `GenUtil`,
  `Constants`, `StringColors` (`GUI.Core/Settings/ModsSettings.cs`), `GuitarSpeak.GuitarSpeakNoteOctaveMath`,
  `SevenZipSharp` (added for SoundPacks), `RocksmithToolkitLib.DLCPackage.Packer` +
  `RocksmithToolkitLib.Extensions.GlobalProgress` (the psarc plumbing the SoundPack slice established).
- Avalonia patterns: SoundPacks' action-oriented VM (`IsBusy` gate, off-thread `Task.Run`, a
  `RunModActionAsync`-style helper, `Process.Start(UseShellExecute)` for external links) and Profiles'
  Load-songs flow (`SongManager.ExtractSongDataAsync` + determinate progress + `DataGrid`), the
  `HexToBrushConverter` (Colors screen) for the string swatches, and the DI/nav registration pattern
  (`App.axaml.cs`, `MainWindowViewModel`, `MainWindow.axaml`, `IDialogService`).

---

## Step 0 — Core prep (no UI; do this first)

1. **Move `GUI/Util/ZipUtils.cs` → `GUI.Core/Util/`.** Drop `using System.Windows.Forms;` and the
   `MessageBox.Show` in `InjectFile`; let it throw (callers already sit inside try/catch that surface
   errors through `IDialogService`).
2. **Move `GUI/SetAndForgetMods.cs` → `GUI.Core/` (namespace stays `RSMods`).** Make the class remain
   usable from both frontends. Replace `Assembly.GetExecutingAssembly()` in `LoadDefaultFiles`,
   `RestoreDefaults`, `RepackCachePsarc` with `typeof(SetAndForgetMods).Assembly` and resource location
   `"RSMods.Core.Resources"`.
3. **Move + embed the resources** (step-3 list above) into `GUI.Core/Resources/` and add
   `<EmbeddedResource>` entries to `GUI.Core.csproj`; remove them from `GUI/GUI.csproj` and delete the
   physical copies (same mechanics as `original.rs_soundpack`).
4. **Add `System.Management` for net8.0** in `GUI.Core.csproj`:
   `<PackageReference Include="System.Management" Version="8.0.0" Condition="'$(TargetFramework)'=='net8.0'" />`
   (net48 has it in-framework — keep the existing `System.Management` reference for that leg).
5. **Introduce the POCO tuning facade** (removes `ArrangementTuning` from the surface the frontend touches).
   Keep the existing `ArrangementTuning`-typed methods **internal**, and add distinct-named public
   POCO/string methods (distinct names, not overloads, to avoid CS0012):
   - a small POCO for six string offsets, e.g. `public sealed class TuningStrings { int String0..String5 }`
     (or `int[6]`);
   - `IReadOnlyList<string> GetUnknownTuningKeys(IReadOnlyList<SongData> songs)` — wraps
     `GetUnknownTunings`, exposing only the display keys (keep the `ArrangementTuning` dict internal);
   - `TuningStrings GetUnknownTuningStrings(string key)` — the string offsets for a "shows as Custom" entry
     (drives "load custom tuning from song");
   - `List<string> GetSongsWithSelectedTuning(string internalTuningName, IReadOnlyList<SongData> songs)` —
     internally `ToArrangementTuning(TuningsCollection[name])` + `GetSongsWithTuning`.
   `TuningsCollection` (`TuningDefinitionList`) is already POCO and can stay public for the editor.
6. **Repoint `GUI/UI.SetAndForget.cs`** to the moved Core types (it keeps `using RSMods;`), and **build all
   three** (GUI.Core net48+net8.0, GUI.Avalonia, WinForms) green before writing any Avalonia UI.

---

## Step 1 — Screen shell + one-click cache mods (simplest slice; land the nav here)

New `SetAndForgetViewModel`/`SetAndForgetView`; register in DI + `MainWindowViewModel` + `MainWindow.axaml`
(copy the SoundPacks wiring). Action-oriented, everything off-thread behind `IsBusy`, with a shared
`RunModActionAsync(action, successMessage, errorPrefix)` helper (port of the WinForms one).

- **Cache mods** (each repacks `cache.psarc`): Add Custom Tunings (`AddCustomTunings`), "Turn it up to
  eleven" (`AddIncreasedVolumeWwiseBank`), Add exit-game menu (`AddExitGameMenuOption`), Add Direct Connect
  mode (`AddDirectConnectModeOption`), and Fast Load (`ApplyFastLoadMod`) driven by the drive-prompt flow
  (`GetFastLoadDrivePrompt` → `FastLoadDrivePrompt` enum → confirm dialogs; the enum is already POCO).
- **Cache management:** Restore Defaults (`RestoreDefaults` — it already takes `IDialogService`; either pass
  it or refactor to return a result and prompt in the VM), Reset Cache (`Process.Start("steam://validate/221680")`
  + `RemoveTempFolders`, behind a warning confirm), Unpack Again (`CleanUnpackedCache`), Remove Temp Folders
  (`RemoveTempFolders`), Import Existing Settings (`ImportExistingSettings`).

## Step 2 — Custom Tunings editor

- Tunings list (with a `<New>` sentinel), six string numerics, `UIName` + UI index, internal name — bound
  to `TuningDefinitionInfo` via `TuningsCollection`.
- Live note-letter labels per string: per-string MIDI base `{40,45,50,55,59,64}` + string offset →
  `GuitarSpeak.GuitarSpeakNoteOctaveMath` (small VM helper).
- String swatches from `StringColors.GetStringColor(i, !extendedRange)`, with the extended-range coloring
  (`Toggles.ExtendedRange` + `ModSettings.ExtendedRangeModeAt`); reuse `HexToBrushConverter`.
- Add / Save / Remove (`SaveTuningsJSON`); surface the blank-name and duplicate-name validation via dialogs.
  (Injecting into the game is the step-1 "Add Custom Tunings" button.)

## Step 3 — Songs-with-tuning lists

- "Load songs" reuses `SongManager.ExtractSongDataAsync` + `Progress<int>` + `IsBusy` (as in Profiles).
- Songs-with-selected-tuning list ← `GetSongsWithSelectedTuning(internalName, songs)`.
- "Shows up as Custom Tuning" list ← `GetUnknownTuningKeys(songs)`.
- "Load custom tuning from song" → `GetUnknownTuningStrings(key)` populates the step-2 numerics.

## Step 4 — Default & Guitarcade tones

- Load profile tones: `GetSteamProfilesTones()` → `List<string>`.
- Default tones: rhythm/lead/bass → `SetDefaultTones(name, 0|1|2)` → `(bool, string)` → dialog.
- Guitarcade tones: 10 named targets → `SetGuitarArcadeTone(name, 0..9)` → `(bool, string)` → dialog.
  Both repack `cache.psarc`; run off-thread behind `IsBusy`.

---

## Cross-cutting rules

- Keep the WinForms Set-and-Forget tab building and working after every slice.
- Each slice: build GUI.Core (both TFMs), GUI.Avalonia, WinForms — 0 new warnings.
- Update `docs/avalonia-migration.md` per slice; leave commits to the user.
- Do **not** name `ArrangementTuning` (or any `Rocksmith2014PsarcLib`/`RocksmithToolkitLib` type) in the
  Avalonia project — always go through the Core POCO facade.

## Verification

- Build all three with the commands in `docs/avalonia-migration.md`.
- Run the Avalonia app, open the new tab; smoke-test one-click mods (safe — `Restore Defaults`/backup exists),
  the tuning editor add/save, the load-songs lists, and tone assignment. Confirm the WinForms tab still works.
