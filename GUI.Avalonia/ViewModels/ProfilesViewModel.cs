using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Avalonia.Threading;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Core;
using RSMods.Util;

namespace RSMods.ViewModels;

/// <summary>
/// The Profiles screen. Unlike the settings screens, this is action-oriented rather than a snapshot:
/// selecting a profile decrypts it and makes it active, and each action (add/remove song list, lock/unlock
/// rewards, restore a backup) executes and persists immediately through the shared <see cref="ProfileService"/> services.
/// </summary>
internal sealed partial class ProfilesViewModel(
    IDialogService dialogs,
    ProfileService profiles,
    ProfileToneImportService toneImports,
    SongCatalogService songCatalog) : ObservableObject
{
    private bool _initialized;

    public ObservableCollection<string> AvailableProfiles { get; } = [];
    public ObservableCollection<string> Backups { get; } = [];
    public ObservableCollection<SongRowViewModel> SongRows { get; } = [];

    /// <summary>Checkbox column descriptors for the grid (Favorites + the profile's song lists). The view
    /// rebuilds the grid's dynamic columns from this whenever <see cref="SongColumnsChanged"/> fires.</summary>
    public IReadOnlyList<SongListColumn> SongListColumns { get; private set; } = [];

    /// <summary>Raised after the song grid's rows and columns have been rebuilt, so the view can regenerate
    /// its dynamic checkbox columns (Avalonia's DataGrid columns are created in code-behind).</summary>
    public event EventHandler? SongColumnsChanged;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(HasSelectedProfile))]
    [NotifyCanExecuteChangedFor(
        nameof(UnlockRewardsCommand), nameof(LockRewardsCommand), nameof(AddSongListCommand),
        nameof(ImportJsonTonesCommand), nameof(ImportXmlTonesCommand), nameof(LoadSongsCommand), nameof(RemoveSongListCommand)
    )]
    private string? _selectedProfile;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(RevertToBackupCommand))]
    private string? _selectedBackup;

    [ObservableProperty]
    private int _songListCount;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(
        nameof(UnlockRewardsCommand), nameof(LockRewardsCommand), nameof(AddSongListCommand),
        nameof(RemoveSongListCommand), nameof(RevertToBackupCommand), nameof(ImportJsonTonesCommand),
        nameof(ImportXmlTonesCommand), nameof(LoadSongsCommand), nameof(SaveSongListsCommand)
    )]
    private bool _isBusy;

    [ObservableProperty]
    private string _statusMessage = string.Empty;

    [ObservableProperty]
    private bool _importTonesBulk;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(
        nameof(UnlockRewardsCommand), nameof(LockRewardsCommand), nameof(AddSongListCommand),
        nameof(RemoveSongListCommand), nameof(ImportJsonTonesCommand), nameof(ImportXmlTonesCommand),
        nameof(LoadSongsCommand), nameof(SaveSongListsCommand)
    )]
    private bool _isLoadingSongs;

    [ObservableProperty]
    private int _loadProgress;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(SaveSongListsCommand))]
    private bool _hasLoadedSongs;

    public bool HasSelectedProfile => !string.IsNullOrEmpty(SelectedProfile);

    /// <summary>Builds the profile and backup lists on first navigation; the save folder is resolved by startup.</summary>
    public Task InitializeAsync()
    {
        if (_initialized)
            return Task.CompletedTask;
        _initialized = true;

        LoadAvailableProfiles();
        LoadBackups();
        return Task.CompletedTask;
    }

    private void LoadAvailableProfiles()
    {
        AvailableProfiles.Clear();
        try
        {
            foreach (string profileName in profiles.GetAvailableProfiles().Keys)
                AvailableProfiles.Add(profileName);
        }
        catch
        {
            // A corrupt/missing profile list is presented as empty, matching the shared service's behaviour.
        }
    }

    private void LoadBackups()
    {
        Backups.Clear();
        try
        {
            foreach (string name in ProfileBackupService.GetFormattedBackupNames(GenUtil.GetRSDirectory()))
                Backups.Add(name);
        }
        catch
        {
            // No backups (or an unreadable backup folder) simply leaves the list empty.
        }
    }

    partial void OnSelectedProfileChanged(string? value)
    {
        // The loaded grid belongs to the previous profile, so drop it until the user reloads.
        ClearSongGrid();

        if (string.IsNullOrEmpty(value))
        {
            SongListCount = 0;
            return;
        }

        // Selection kicks off an async decrypt; exceptions are surfaced inside the task.
        _ = SelectProfileAsync(value);
    }

    private void ClearSongGrid()
    {
        SongRows.Clear();
        SongListColumns = [];
        HasLoadedSongs = false;
        SongColumnsChanged?.Invoke(this, EventArgs.Empty);
    }

    private async Task SelectProfileAsync(string profileName)
    {
        IsBusy = true;
        StatusMessage = $"Loading profile '{profileName}'…";
        try
        {
            // Decrypting the profile is file-bound; keep it off the UI thread.
            await Task.Run(() => profiles.SelectProfile(profileName));

            SongListCount = profiles.SongListCount;
            StatusMessage = $"Loaded profile '{profileName}'.";
        }
        catch (Exception ex)
        {
            SongListCount = 0;
            StatusMessage = "Failed to load the profile.";
            await dialogs.ShowErrorAsync($"Could not load the profile:\n{ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    private bool CanEditProfile => HasSelectedProfile && !IsBusy && !IsLoadingSongs;

    [RelayCommand(CanExecute = nameof(CanEditProfile))]
    private Task UnlockRewardsAsync() => ChangeRewardsAsync(unlock: true);

    [RelayCommand(CanExecute = nameof(CanEditProfile))]
    private Task LockRewardsAsync() => ChangeRewardsAsync(unlock: false);

    private async Task ChangeRewardsAsync(bool unlock)
    {
        string prompt = unlock
            ? "Are you sure you want to unlock all rewards?\nThat defeats the grind for in-game rewards."
            : "Are you sure you want to lock all rewards?\nThis will remove all access to in-game rewards.";

        if (!await dialogs.ShowConfirmAsync(prompt, "Are you sure?"))
            return;

        await RunBusyAsync(async () =>
        {
            await Task.Run(() =>
            {
                profiles.ChangeRewardStatus(unlock);
                profiles.SaveActiveProfile();
            });
            StatusMessage = unlock ? "All rewards unlocked and saved." : "All rewards locked and saved.";
        }, "Failed to change rewards");
    }

    [RelayCommand(CanExecute = nameof(CanEditProfile))]
    private Task AddSongListAsync() => RunBusyAsync(async () =>
    {
        // AddSongList persists immediately; false means the 20-list ceiling was hit.
        bool added = await Task.Run(profiles.AddSongList);
        if (!added)
        {
            await dialogs.ShowInfoAsync("Having more than 20 song lists is not supported.", "Cannot add song list");
            return;
        }

        SongListCount = profiles.SongListCount;
        // The grid's columns no longer match the profile's list count; reload to rebuild them.
        ClearSongGrid();
        StatusMessage = "Added a new song list.";
    }, "Failed to add a song list");

    [RelayCommand(CanExecute = nameof(CanEditProfile))]
    private Task RemoveSongListAsync() => RunBusyAsync(async () =>
    {
        // RemoveSongList persists immediately; false means the 6-list floor was hit.
        bool removed = await Task.Run(profiles.RemoveSongList);
        if (!removed)
        {
            await dialogs.ShowInfoAsync(
                "There are no more song lists that can be removed.", "Cannot remove song list");
            return;
        }

        SongListCount = profiles.SongListCount;
        // The grid's columns no longer match the profile's list count; reload to rebuild them.
        ClearSongGrid();
        StatusMessage = "Removed the newest song list.";
    }, "Failed to remove a song list");

    [RelayCommand]
    private void RefreshBackups()
    {
        LoadBackups();
        StatusMessage = "Backup list refreshed.";
    }

    private bool CanRevert => !string.IsNullOrEmpty(SelectedBackup) && !IsBusy;

    [RelayCommand(CanExecute = nameof(CanRevert))]
    private async Task RevertToBackupAsync()
    {
        if (SelectedBackup is not string name)
            return;

        if (!await dialogs.ShowConfirmAsync($"Revert your save data to the backup '{name}'?\nThis overwrites your current save profiles.", "Revert to backup?"))
            return;

        string? sourceDir = ProfileBackupService.GetBackupSourceDirectory(GenUtil.GetRSDirectory(), name);
        if (string.IsNullOrEmpty(sourceDir))
        {
            await dialogs.ShowErrorAsync("Could not identify the backup date format.");
            return;
        }

        if (!Directory.Exists(sourceDir))
        {
            await dialogs.ShowErrorAsync("The selected backup folder could not be found.");
            return;
        }

        await RunBusyAsync(async () =>
        {
            await Task.Run(() => ProfileBackupService.RestoreBackup(sourceDir, GenUtil.GetSaveDirectory()));
            StatusMessage = $"Reverted to the backup: {name}";
        }, "Failed to restore the backup");
    }

    [RelayCommand(CanExecute = nameof(CanEditProfile))]
    private async Task ImportJsonTonesAsync()
    {
        IReadOnlyList<string> files = await dialogs.PickFilesAsync("Import tone manifest(s)", "JSON", ["*.json"], ImportTonesBulk);
        if (files.Count == 0)
            return;

        await RunBusyAsync(async () =>
        {
            var (imported, errors) = await Task.Run(() => toneImports.ImportManifests([.. files]));
            await ReportToneImportAsync(imported, errors);
        }, "Failed to import tones");
    }

    [RelayCommand(CanExecute = nameof(CanEditProfile))]
    private async Task ImportXmlTonesAsync()
    {
        IReadOnlyList<string> files = await dialogs.PickFilesAsync("Import toolkit XML tone(s)", "Toolkit tone", ["*.tone2014.xml"], ImportTonesBulk);
        if (files.Count == 0)
            return;

        await RunBusyAsync(async () =>
        {
            // The import loop runs off the UI thread; each tone's guitar/bass choice marshals back to it.
            var (imported, errors) = await Task.Run(() => toneImports.ImportXmlTonesByName([.. files], PromptGuitarOrBass));
            await ReportToneImportAsync(imported, errors);
        }, "Failed to import tones");
    }

    /// <summary>
    /// Prompts for a tone's guitar/bass assignment. Called from the background import loop, so it marshals
    /// the modal onto the UI thread and blocks the worker until the user chooses (true = guitar).
    /// Must only ever be invoked from a background thread: it blocks on the UI dispatcher, so calling it
    /// from the UI thread would deadlock.
    /// </summary>
    private bool PromptGuitarOrBass(string toneName)
    {
        var choice = new TaskCompletionSource<bool>();
        Dispatcher.UIThread.Post(async () =>
        {
            try
            {
                bool guitar = await dialogs.ShowChoiceAsync($"Save '{toneName}' as a guitar tone or a bass tone?", "Tone assignment", "Guitar", "Bass");
                choice.SetResult(guitar);
            }
            catch (Exception ex)
            {
                choice.SetException(ex);
            }
        });

        return choice.Task.GetAwaiter().GetResult();
    }

    private async Task ReportToneImportAsync(int importedCount, List<string> errorMessages)
    {
        if (errorMessages.Count > 0)
        {
            string errors = string.Join("\n", errorMessages);
            await dialogs.ShowErrorAsync($"Import completed with some errors:\n\n{errors}", "Import warnings");
        }

        if (importedCount > 0)
        {
            StatusMessage = $"Added {importedCount} tone(s) to the profile.";
            await dialogs.ShowInfoAsync($"Added {importedCount} tone(s) to the profile.", "Import complete");
        }
        else if (errorMessages.Count == 0)
        {
            StatusMessage = "No tones were found to import.";
            await dialogs.ShowInfoAsync("No tones were found to import.", "Import complete");
        }
        else
        {
            StatusMessage = "No tones were imported.";
        }
    }

    private bool CanLoadSongs => HasSelectedProfile && !IsBusy && !IsLoadingSongs;

    [RelayCommand(CanExecute = nameof(CanLoadSongs))]
    private async Task LoadSongsAsync()
    {
        IsLoadingSongs = true;
        LoadProgress = 0;
        StatusMessage = "Loading songs from your library…";
        try
        {
            // The psarc scan is the slow part and already runs on a background thread inside the service.
            var progress = new Progress<int>(value => LoadProgress = value);
            SongCatalogResult catalog = await songCatalog.GetSongsAsync(forceRefresh: HasLoadedSongs, progress: progress);

            HashSet<string> ownedRs1Dlc = profiles.GetOwnedRs1Dlc();
            List<List<string>> lists = profiles.GetProfileSongListsWithFavorites();
            int songListCount = lists.Count - 1; // lists[0] is Favorites; the rest are the numbered lists.

            SongListColumns = BuildSongColumns(songListCount);

            SongRows.Clear();
            foreach (SongRowViewModel row in BuildSongRows(catalog.Songs, lists, ownedRs1Dlc))
                SongRows.Add(row);

            HasLoadedSongs = true;
            SongColumnsChanged?.Invoke(this, EventArgs.Empty);
            string warning = catalog.Warnings.Count == 0
                ? string.Empty
                : $" Skipped {catalog.Warnings.Count} unreadable archive(s).";
            StatusMessage = $"Loaded {SongRows.Count} songs. Toggle Favorites/song lists, then Save song lists.{warning}";
        }
        catch (Exception ex)
        {
            ClearSongGrid();
            await dialogs.ShowErrorAsync($"Failed to load songs:\n{ex.Message}");
            StatusMessage = "Failed to load songs.";
        }
        finally
        {
            IsLoadingSongs = false;
        }
    }

    /// <summary>Favorites (cell 0) followed by one column per numbered song list (cells 1..N), titled from settings.</summary>
    private static List<SongListColumn> BuildSongColumns(int songListCount)
    {
        var columns = new List<SongListColumn> { new("Favorites", 0) };
        
        for (int listNumber = 1; listNumber <= songListCount; listNumber++)
        {
            columns.Add(new SongListColumn(RsModsSettings.GetSongListTitle(listNumber), listNumber));
        }

        return columns;
    }

    /// <summary>
    /// Builds one grid row per included song, each carrying a Favorites cell plus one cell per numbered list.
    /// Every cell closes over its DLC key and list index so toggling it writes straight back to the profile.
    /// </summary>
    private List<SongRowViewModel> BuildSongRows(IReadOnlyList<SongData> songs, List<List<string>> lists, HashSet<string> ownedRs1Dlc)
    {
        int songListCount = lists.Count - 1;
        var rows = new List<SongRowViewModel>();

        foreach (SongData song in songs)
        {
            if (!ProfileService.ShouldIncludeSong(song, ownedRs1Dlc))
                continue;

            string dlcKey = song.DLCKey;
            var cells = new SongCellViewModel[songListCount + 1];
            cells[0] = new SongCellViewModel(lists[0].Contains(dlcKey), add => profiles.SetSongInFavorites(dlcKey, add));

            for (int listNumber = 1; listNumber <= songListCount; listNumber++)
            {
                int listIndex = listNumber - 1; // SetSongInList indexes the numbered lists from 0.
                cells[listNumber] = new SongCellViewModel(lists[listNumber].Contains(dlcKey), add => profiles.SetSongInList(dlcKey, listIndex, add));
            }

            rows.Add(new SongRowViewModel(song.Artist, song.Title, cells));
        }

        return rows;
    }

    private bool CanSaveSongLists => HasLoadedSongs && !IsBusy && !IsLoadingSongs;

    [RelayCommand(CanExecute = nameof(CanSaveSongLists))]
    private Task SaveSongListsAsync() => RunBusyAsync(async () =>
    {
        // Cell toggles already updated the in-memory profile; this persists it (encrypts + writes).
        await Task.Run(profiles.SaveActiveProfile);
        StatusMessage = "Song lists and favorites saved.";
    }, "Failed to save song lists");

    /// <summary>
    /// Runs a busy-gated action behind the shared <see cref="IsBusy"/> flag, turning any exception into an
    /// error dialog and matching status message. The action sets its own success status; confirmation prompts
    /// and file pickers belong before the call, not inside it.
    /// </summary>
    private async Task RunBusyAsync(Func<Task> action, string failureMessage)
    {
        IsBusy = true;
        try
        {
            await action();
        }
        catch (Exception ex)
        {
            await dialogs.ShowErrorAsync($"{failureMessage}:\n{ex.Message}");
            StatusMessage = $"{failureMessage}.";
        }
        finally
        {
            IsBusy = false;
        }
    }
}

/// <summary>A dynamic checkbox column in the song-list grid: its header and the row cell index it binds to.</summary>
internal sealed record SongListColumn(string Header, int CellIndex);
