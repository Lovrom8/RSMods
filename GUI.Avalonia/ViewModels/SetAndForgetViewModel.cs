using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Core;
using RSMods.SetAndForget.Models;

namespace RSMods.ViewModels;

/// <summary>
/// Action-oriented Set-and-Forget cache-mod screen. All archive and filesystem work runs away from
/// the UI thread behind a shared busy gate; this view-model owns prompts and result presentation.
/// </summary>
internal sealed partial class SetAndForgetViewModel : ObservableObject
{
    private const string NewTuningName = "<New>";

    private readonly IDialogService _dialogs;
    private bool _initialized;
    private IReadOnlyList<SongData> _songs = [];

    public ObservableCollection<string> TuningNames { get; } = [];
    public ObservableCollection<string> SongsWithSelectedTuning { get; } = [];
    public ObservableCollection<string> UnknownTuningKeys { get; } = [];
    public ObservableCollection<string> ProfileTones { get; } = [];
    public IReadOnlyList<ToneTargetOption> DefaultToneTargets { get; } =
    [
        new("Rhythm", 0),
        new("Lead", 1),
        new("Bass", 2)
    ];
    public IReadOnlyList<ToneTargetOption> GuitarcadeToneTargets { get; } =
    [
        new("Temple of Bends", 0),
        new("Scale Warriors", 1),
        new("String Skip Saloon", 2),
        new("Scale Racer", 3),
        new("Ninja Slide N", 4),
        new("Hurtlin' Hurdles", 5),
        new("Harmonic Heist", 6),
        new("Ducks Redux", 7),
        new("Rainbow Laser", 8),
        new("Gone Wailin'", 9)
    ];
    public ObservableCollection<TuningStringViewModel> TuningStrings { get; } =
    [
        new(0, "Low E", 40),
        new(1, "A", 45),
        new(2, "D", 50),
        new(3, "G", 55),
        new(4, "B", 59),
        new(5, "High E", 64)
    ];

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowIndeterminateBusy))]
    [NotifyCanExecuteChangedFor(nameof(AddCustomTuningsCommand))]
    [NotifyCanExecuteChangedFor(nameof(IncreaseVolumeCommand))]
    [NotifyCanExecuteChangedFor(nameof(AddExitGameCommand))]
    [NotifyCanExecuteChangedFor(nameof(AddDirectConnectCommand))]
    [NotifyCanExecuteChangedFor(nameof(ApplyFastLoadCommand))]
    [NotifyCanExecuteChangedFor(nameof(RestoreDefaultsCommand))]
    [NotifyCanExecuteChangedFor(nameof(ResetCacheCommand))]
    [NotifyCanExecuteChangedFor(nameof(UnpackAgainCommand))]
    [NotifyCanExecuteChangedFor(nameof(RemoveTempFoldersCommand))]
    [NotifyCanExecuteChangedFor(nameof(ImportExistingSettingsCommand))]
    [NotifyCanExecuteChangedFor(nameof(AddTuningCommand))]
    [NotifyCanExecuteChangedFor(nameof(SaveTuningCommand))]
    [NotifyCanExecuteChangedFor(nameof(RemoveTuningCommand))]
    [NotifyCanExecuteChangedFor(nameof(LoadSongsCommand))]
    [NotifyCanExecuteChangedFor(nameof(LoadCustomTuningFromSongCommand))]
    [NotifyCanExecuteChangedFor(nameof(LoadProfileTonesCommand))]
    [NotifyCanExecuteChangedFor(nameof(AssignDefaultToneCommand))]
    [NotifyCanExecuteChangedFor(nameof(AssignGuitarcadeToneCommand))]
    private bool _isBusy;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowIndeterminateBusy))]
    private bool _isLoadingSongs;

    [ObservableProperty]
    private int _songLoadProgress;

    [ObservableProperty]
    private bool _hasLoadedSongs;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(LoadCustomTuningFromSongCommand))]
    private string? _selectedUnknownTuningKey;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(AssignDefaultToneCommand))]
    [NotifyCanExecuteChangedFor(nameof(AssignGuitarcadeToneCommand))]
    private string? _selectedProfileTone;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(AssignDefaultToneCommand))]
    private ToneTargetOption? _selectedDefaultToneTarget;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(AssignGuitarcadeToneCommand))]
    private ToneTargetOption? _selectedGuitarcadeToneTarget;

    [ObservableProperty]
    private string _statusMessage = string.Empty;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(IsNewTuning))]
    [NotifyCanExecuteChangedFor(nameof(AddTuningCommand))]
    [NotifyCanExecuteChangedFor(nameof(SaveTuningCommand))]
    [NotifyCanExecuteChangedFor(nameof(RemoveTuningCommand))]
    private string? _selectedTuningName;

    [ObservableProperty]
    private string _internalTuningName = string.Empty;

    [ObservableProperty]
    private string _uiName = string.Empty;

    [ObservableProperty]
    private decimal _uiIndex;

    public bool IsNewTuning => SelectedTuningName == NewTuningName;
    public bool ShowIndeterminateBusy => IsBusy && !IsLoadingSongs;

    public SetAndForgetViewModel(IDialogService dialogs)
    {
        _dialogs = dialogs;
        _selectedDefaultToneTarget = DefaultToneTargets[1]; // WinForms defaults to Lead.
        _selectedGuitarcadeToneTarget = GuitarcadeToneTargets[0];
    }

    /// <summary>Extracts the stock working files and loads the tuning database on first navigation.</summary>
    public async Task InitializeAsync()
    {
        if (_initialized)
        {
            RefreshTuningStringPresentations();
            return;
        }

        _initialized = true;
        IsBusy = true;
        StatusMessage = "Preparing Set-and-Forget files...";
        try
        {
            await Task.Run(() =>
            {
                SetAndForgetMods.LoadDefaultFiles();
                SetAndForgetMods.LoadTuningsCollection();
            });
            ReloadTuningNames();
            StatusMessage = "Ready.";
        }
        catch (Exception ex)
        {
            _initialized = false;
            StatusMessage = "Preparation failed.";
            await _dialogs.ShowErrorAsync($"Unable to prepare Set-and-Forget files.{Environment.NewLine}Error: {ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    private bool CanRun => !IsBusy && _initialized;
    private bool CanAddTuning => CanRun && IsNewTuning;
    private bool CanModifyTuning => CanRun && SelectedTuningName is not null && !IsNewTuning;
    private bool CanLoadCustomTuningFromSong =>
        CanRun && HasLoadedSongs && !string.IsNullOrEmpty(SelectedUnknownTuningKey);
    private bool CanAssignDefaultTone =>
        CanRun && !string.IsNullOrEmpty(SelectedProfileTone) && SelectedDefaultToneTarget is not null;
    private bool CanAssignGuitarcadeTone =>
        CanRun && !string.IsNullOrEmpty(SelectedProfileTone) && SelectedGuitarcadeToneTarget is not null;

    partial void OnSelectedTuningNameChanged(string? value)
    {
        LoadSelectedTuning(value);
        RefreshSongsWithSelectedTuning();
    }

    private void LoadSelectedTuning(string? tuningName)
    {
        if (string.IsNullOrEmpty(tuningName) || tuningName == NewTuningName)
        {
            InternalTuningName = string.Empty;
            UiName = string.Empty;
            UiIndex = 0;
            foreach (TuningStringViewModel tuningString in TuningStrings)
                tuningString.Offset = 0;
            RefreshTuningStringPresentations();
            return;
        }

        if (!SetAndForgetMods.TuningsCollection.TryGetValue(tuningName, out TuningDefinitionInfo? tuning) ||
            tuning is null)
            return;

        var (index, name) = SetAndForgetMods.SplitTuningUIName(tuning.UIName);
        InternalTuningName = tuningName;
        UiName = name ?? string.Empty;
        UiIndex = int.TryParse(index, out int parsedIndex) ? parsedIndex : 0;

        foreach (TuningStringViewModel tuningString in TuningStrings)
        {
            string key = $"string{tuningString.Index}";
            tuningString.Offset = tuning.Strings is not null && tuning.Strings.TryGetValue(key, out int offset)
                ? offset
                : 0;
        }

        RefreshTuningStringPresentations();
    }

    private void ReloadTuningNames(string? preferredSelection = null)
    {
        string selection = preferredSelection ?? SelectedTuningName ?? NewTuningName;

        TuningNames.Clear();
        TuningNames.Add(NewTuningName);
        foreach (string tuningName in SetAndForgetMods.TuningsCollection.Keys)
            TuningNames.Add(tuningName);

        SelectedTuningName = null;
        SelectedTuningName = TuningNames.Contains(selection) ? selection : NewTuningName;

        if (HasLoadedSongs)
            RefreshUnknownTuningKeys();
    }

    private void RefreshTuningStringPresentations()
    {
        foreach (TuningStringViewModel tuningString in TuningStrings)
            tuningString.RefreshPresentation();
    }

    private TuningDefinitionInfo CreateTuningDefinition()
    {
        Dictionary<string, int> strings = TuningStrings.ToDictionary(
            tuningString => $"string{tuningString.Index}",
            tuningString => (int)tuningString.Offset);

        return new TuningDefinitionInfo
        {
            UIName = $"$[{(int)UiIndex}]{UiName}",
            Strings = strings
        };
    }

    [RelayCommand(CanExecute = nameof(CanRun))]
    private async Task LoadSongsAsync()
    {
        IsLoadingSongs = true;
        IsBusy = true;
        SongLoadProgress = 0;
        StatusMessage = "Loading songs from your library...";
        try
        {
            var progress = new Progress<int>(value => SongLoadProgress = value);
            _songs = await SongManager.ExtractSongDataAsync(progress);

            HasLoadedSongs = true;
            RefreshSongTuningLists();
            StatusMessage = $"Loaded {_songs.Count} songs and their arrangements.";
        }
        catch (Exception ex)
        {
            _songs = [];
            HasLoadedSongs = false;
            SongsWithSelectedTuning.Clear();
            UnknownTuningKeys.Clear();
            SelectedUnknownTuningKey = null;
            StatusMessage = "Failed to load songs.";
            await _dialogs.ShowErrorAsync($"Failed to load songs:{Environment.NewLine}{ex.Message}");
        }
        finally
        {
            IsBusy = false;
            IsLoadingSongs = false;
        }
    }

    private void RefreshSongTuningLists()
    {
        RefreshUnknownTuningKeys();
        RefreshSongsWithSelectedTuning();
    }

    private void RefreshUnknownTuningKeys()
    {
        UnknownTuningKeys.Clear();
        SelectedUnknownTuningKey = null;

        if (!HasLoadedSongs)
            return;

        foreach (string key in SetAndForgetMods.GetUnknownTuningKeys(_songs))
            UnknownTuningKeys.Add(key);
    }

    private void RefreshSongsWithSelectedTuning()
    {
        SongsWithSelectedTuning.Clear();
        if (!HasLoadedSongs || string.IsNullOrEmpty(SelectedTuningName) || IsNewTuning)
            return;

        foreach (string song in SetAndForgetMods.GetSongsWithSelectedTuning(SelectedTuningName, _songs))
            SongsWithSelectedTuning.Add(song);
    }

    [RelayCommand(CanExecute = nameof(CanLoadCustomTuningFromSong))]
    private async Task LoadCustomTuningFromSongAsync()
    {
        try
        {
            RSMods.SetAndForget.Models.TuningStrings strings =
                SetAndForgetMods.GetUnknownTuningStrings(SelectedUnknownTuningKey!);
            SelectedTuningName = NewTuningName;
            TuningStrings[0].Offset = strings.String0;
            TuningStrings[1].Offset = strings.String1;
            TuningStrings[2].Offset = strings.String2;
            TuningStrings[3].Offset = strings.String3;
            TuningStrings[4].Offset = strings.String4;
            TuningStrings[5].Offset = strings.String5;
            StatusMessage = "Loaded the selected song tuning into a new tuning.";
        }
        catch (Exception ex)
        {
            await _dialogs.ShowErrorAsync(
                $"Unable to load the selected custom tuning.{Environment.NewLine}Error: {ex.Message}");
        }
    }

    [RelayCommand(CanExecute = nameof(CanRun))]
    private async Task LoadProfileTonesAsync()
    {
        IsBusy = true;
        StatusMessage = "Loading tones from Steam profiles...";
        try
        {
            List<string> tones = await Task.Run(SetAndForgetMods.GetSteamProfilesTones);

            ProfileTones.Clear();
            foreach (string tone in tones.OrderBy(tone => tone, StringComparer.OrdinalIgnoreCase))
                ProfileTones.Add(tone);

            SelectedProfileTone = ProfileTones.FirstOrDefault();
            StatusMessage = ProfileTones.Count == 0
                ? "No profile tones were found."
                : $"Loaded {ProfileTones.Count} profile tones.";

            if (ProfileTones.Count == 0)
            {
                await _dialogs.ShowInfoAsync(
                    "No tones were found in the available Rocksmith profiles.", "No profile tones");
            }
        }
        catch (Exception ex)
        {
            StatusMessage = "Failed to load profile tones.";
            await _dialogs.ShowErrorAsync(
                $"Unable to load tones from Steam profiles.{Environment.NewLine}Error: {ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    [RelayCommand(CanExecute = nameof(CanAssignDefaultTone))]
    private async Task AssignDefaultToneAsync()
    {
        string toneName = SelectedProfileTone!;
        int targetIndex = SelectedDefaultToneTarget!.Index;
        await AssignToneAsync(
            () => SetAndForgetMods.SetDefaultTones(toneName, targetIndex),
            $"Assigning {toneName} as the default {SelectedDefaultToneTarget.Label.ToLowerInvariant()} tone...");
    }

    [RelayCommand(CanExecute = nameof(CanAssignGuitarcadeTone))]
    private async Task AssignGuitarcadeToneAsync()
    {
        string toneName = SelectedProfileTone!;
        int targetIndex = SelectedGuitarcadeToneTarget!.Index;
        await AssignToneAsync(
            () => SetAndForgetMods.SetGuitarArcadeTone(toneName, targetIndex),
            $"Assigning {toneName} to {SelectedGuitarcadeToneTarget.Label}...");
    }

    private async Task AssignToneAsync(
        Func<(bool IsSuccess, string Message)> assignTone,
        string busyMessage)
    {
        IsBusy = true;
        StatusMessage = busyMessage;
        try
        {
            (bool IsSuccess, string Message) result = await Task.Run(assignTone);
            StatusMessage = result.Message;
            if (result.IsSuccess)
                await _dialogs.ShowInfoAsync(result.Message, "Assignment result");
            else
                await _dialogs.ShowErrorAsync(result.Message, "Assignment failed");
        }
        catch (Exception ex)
        {
            StatusMessage = "Tone assignment failed.";
            await _dialogs.ShowErrorAsync(
                $"Unable to assign the selected tone.{Environment.NewLine}Error: {ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    [RelayCommand(CanExecute = nameof(CanAddTuning))]
    private async Task AddTuningAsync()
    {
        string internalName = InternalTuningName.Trim();
        if (internalName.Length == 0)
        {
            await _dialogs.ShowErrorAsync("You cannot have a blank internal name.", "Invalid tuning");
            return;
        }

        if (SetAndForgetMods.TuningsCollection.ContainsKey(internalName))
        {
            await _dialogs.ShowErrorAsync(
                "You already have a tuning with the same internal name.", "Duplicate tuning");
            return;
        }

        TuningDefinitionInfo definition = CreateTuningDefinition();
        await RunModActionAsync(
            () =>
            {
                SetAndForgetMods.TuningsCollection.Add(internalName, definition);
                try
                {
                    SetAndForgetMods.SaveTuningsJSON();
                }
                catch
                {
                    SetAndForgetMods.TuningsCollection.Remove(internalName);
                    throw;
                }
            },
            "Adding tuning...",
            "Tuning added. Use Add custom tunings when you are ready to inject it into the game.",
            "Unable to add tuning",
            () => ReloadTuningNames(internalName));
    }

    [RelayCommand(CanExecute = nameof(CanModifyTuning))]
    private async Task SaveTuningAsync()
    {
        string internalName = SelectedTuningName!;
        TuningDefinitionInfo oldDefinition = SetAndForgetMods.TuningsCollection[internalName];
        TuningDefinitionInfo newDefinition = CreateTuningDefinition();

        await RunModActionAsync(
            () =>
            {
                SetAndForgetMods.TuningsCollection[internalName] = newDefinition;
                try
                {
                    SetAndForgetMods.SaveTuningsJSON();
                }
                catch
                {
                    SetAndForgetMods.TuningsCollection[internalName] = oldDefinition;
                    throw;
                }
            },
            "Saving tuning...",
            "Tuning saved. Use Add custom tunings when you are ready to inject it into the game.",
            "Unable to save tuning",
            () => ReloadTuningNames(internalName));
    }

    [RelayCommand(CanExecute = nameof(CanModifyTuning))]
    private async Task RemoveTuningAsync()
    {
        string internalName = SelectedTuningName!;
        TuningDefinitionInfo removedDefinition = SetAndForgetMods.TuningsCollection[internalName];

        await RunModActionAsync(
            () =>
            {
                SetAndForgetMods.TuningsCollection.Remove(internalName);
                try
                {
                    SetAndForgetMods.SaveTuningsJSON();
                }
                catch
                {
                    SetAndForgetMods.TuningsCollection[internalName] = removedDefinition;
                    throw;
                }
            },
            "Removing tuning...",
            "Tuning removed.",
            "Unable to remove tuning",
            () => ReloadTuningNames());
    }

    [RelayCommand(CanExecute = nameof(CanRun))]
    private Task AddCustomTuningsAsync() => RunModActionAsync(
        SetAndForgetMods.AddCustomTunings,
        "Adding custom tunings...",
        "cache.psarc repackaged successfully.",
        "Unable to add custom tunings");

    [RelayCommand(CanExecute = nameof(CanRun))]
    private Task IncreaseVolumeAsync() => RunModActionAsync(
        SetAndForgetMods.AddIncreasedVolumeWwiseBank,
        "Increasing game volume...",
        "cache.psarc repackaged successfully.",
        "Unable to repack cache.psarc");

    [RelayCommand(CanExecute = nameof(CanRun))]
    private Task AddExitGameAsync() => RunModActionAsync(
        SetAndForgetMods.AddExitGameMenuOption,
        "Adding the Exit Game menu option...",
        "cache.psarc repackaged successfully.",
        "Unable to repack cache.psarc");

    [RelayCommand(CanExecute = nameof(CanRun))]
    private Task AddDirectConnectAsync() => RunModActionAsync(
        SetAndForgetMods.AddDirectConnectModeOption,
        "Adding Direct Connect mode...",
        "cache.psarc repackaged successfully.",
        "Unable to repack cache.psarc");

    [RelayCommand(CanExecute = nameof(CanRun))]
    private async Task ApplyFastLoadAsync()
    {
        IsBusy = true;
        StatusMessage = "Detecting the Rocksmith drive type...";
        try
        {
            SetAndForgetMods.FastLoadDrivePrompt prompt =
                await Task.Run(SetAndForgetMods.GetFastLoadDrivePrompt);
            bool useNvmeFastLoad = false;

            switch (prompt)
            {
                case SetAndForgetMods.FastLoadDrivePrompt.ConfirmHddRisk:
                    if (!await _dialogs.ShowConfirmAsync(
                            "It appears as though Rocksmith is installed on a hard disk drive. HDDs are " +
                            "normally too slow to support the fast load mod and will likely result in a " +
                            "crash.\n\nDo you wish to proceed?",
                            "Drive too slow for fast load"))
                    {
                        StatusMessage = "Fast Load cancelled.";
                        return;
                    }
                    break;

                case SetAndForgetMods.FastLoadDrivePrompt.ConfirmNvme:
                    useNvmeFastLoad = await _dialogs.ShowConfirmAsync(
                        "Can you confirm Rocksmith is installed on an NVMe drive?\nIf you are unsure, " +
                        "press No, because Rocksmith is likely to crash if you pick the fastest option!",
                        "Is Rocksmith on an NVMe drive?");
                    break;

                case SetAndForgetMods.FastLoadDrivePrompt.ConfirmUnknown:
                    useNvmeFastLoad = await _dialogs.ShowConfirmAsync(
                        "We were unable to detect the drive type on which Rocksmith is installed.\nIs it " +
                        "on an NVMe drive? If it is not, the fastest loading option is likely to crash your game!",
                        "Fast drive?");
                    break;
            }

            StatusMessage = "Applying Fast Load...";
            await Task.Run(() => SetAndForgetMods.ApplyFastLoadMod(useNvmeFastLoad));
            StatusMessage = "Fast Load applied.";
            await _dialogs.ShowInfoAsync("cache.psarc repackaged successfully.", "Success");
        }
        catch (Exception ex)
        {
            StatusMessage = "Fast Load failed.";
            await _dialogs.ShowErrorAsync(
                $"Unable to modify required files.{Environment.NewLine}Error: {ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    [RelayCommand(CanExecute = nameof(CanRun))]
    private async Task RestoreDefaultsAsync()
    {
        if (!await _dialogs.ShowConfirmAsync(
                "Do you wish to restore your cache.psarc to its original state?",
                "Restore cache.psarc?"))
            return;

        await RunModActionAsync(
            () =>
            {
                if (!SetAndForgetMods.RestoreDefaults())
                    throw new FileNotFoundException("No cache backup was found.");

                SetAndForgetMods.LoadTuningsCollection();
            },
            "Restoring the cache backup...",
            "Cache backup was restored!",
            "Problems restoring backup",
            () => ReloadTuningNames());
    }

    [RelayCommand(CanExecute = nameof(CanRun))]
    private async Task ResetCacheAsync()
    {
        if (!await _dialogs.ShowConfirmAsync(
                "Woah, hang on there!\nHave you tried Restore Cache Backup first? This should be a last " +
                "resort. Steam will redownload all modified game files. This only removes mods from this " +
                "section; your other RSMods settings are unaffected.",
                "Verify game files with Steam?"))
            return;

        await RunModActionAsync(
            () =>
            {
                Process.Start(new ProcessStartInfo("steam://validate/221680") { UseShellExecute = true });
                SetAndForgetMods.RemoveTempFolders();
            },
            "Starting Steam validation...",
            "Steam file validation started.",
            "Unable to start Steam validation");
    }

    [RelayCommand(CanExecute = nameof(CanRun))]
    private Task UnpackAgainAsync() => RunModActionAsync(
        SetAndForgetMods.CleanUnpackedCache,
        "Unpacking cache.psarc again...",
        "cache.psarc unpacked again successfully.",
        "Unable to unpack cache.psarc");

    [RelayCommand(CanExecute = nameof(CanRun))]
    private Task RemoveTempFoldersAsync() => RunModActionAsync(
        SetAndForgetMods.RemoveTempFolders,
        "Removing temporary files...",
        "Temporary folders removed.",
        "Unable to remove temporary folders");

    [RelayCommand(CanExecute = nameof(CanRun))]
    private Task ImportExistingSettingsAsync() => RunModActionAsync(
        () =>
        {
            SetAndForgetMods.ImportExistingSettings();
            SetAndForgetMods.LoadTuningsCollection();
        },
        "Importing settings from the unpacked cache...",
        "Existing cache settings imported.",
        "Unable to import existing settings",
        () => ReloadTuningNames());

    private async Task<bool> RunModActionAsync(
        Action action,
        string busyMessage,
        string successMessage,
        string errorPrefix,
        Action? onSuccess = null)
    {
        IsBusy = true;
        StatusMessage = busyMessage;
        try
        {
            await Task.Run(action);
            onSuccess?.Invoke();
            StatusMessage = successMessage;
            await _dialogs.ShowInfoAsync(successMessage, "Success");
            return true;
        }
        catch (Exception ex)
        {
            StatusMessage = "Operation failed.";
            await _dialogs.ShowErrorAsync($"{errorPrefix}.{Environment.NewLine}Error: {ex.Message}");
            return false;
        }
        finally
        {
            IsBusy = false;
        }
    }
}

/// <summary>A user-facing tone target and the index expected by the shared cache operation.</summary>
internal sealed record ToneTargetOption(string Label, int Index);
