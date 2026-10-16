using System;
using System.Threading.Tasks;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Core;
using RSMods.Core.Settings;
using RSMods.Services;

namespace RSMods.ViewModels;

internal sealed partial class MainWindowViewModel : ObservableObject
{
    private readonly StartupService _startup;
    private readonly SettingsWarningPresenter _warnings;
    private readonly ThemeService _theme;
    private readonly StatusViewModel _status;
    private readonly ModSettingsViewModel _modSettings;
    private readonly ColorsViewModel _colors;
    private readonly RocksmithSettingsViewModel _rocksmith;
    private readonly AsioSettingsViewModel _asio;
    private readonly ThemesViewModel _themes;
    private readonly ProfilesViewModel _profiles;
    private readonly SoundPacksViewModel _soundPacks;
    private readonly SetAndForgetViewModel _setAndForget;
    private readonly TwitchViewModel _twitch;
    private readonly NavigationService _navigation;
    private readonly IDialogService _dialogs;
    private readonly AutoSaveService _autoSave;
    private bool _initialized;
    private int _navigationCount; // Bumped on every page change, so a slow page load can't pull the user back
    private bool _profilesBackedUp; // A backup of the current save folder was taken this session
    private bool _backingUpProfiles;

    [ObservableProperty]
    private ObservableObject _currentPage;

    /// <summary>Key of the section currently shown; drives the active sidebar highlight.</summary>
    [ObservableProperty]
    private string _activeSection = "Status";

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(
        nameof(ShowModSettingsCommand), nameof(ShowColorsCommand), nameof(ShowRocksmithCommand),
        nameof(ShowAsioCommand), nameof(ShowThemesCommand), nameof(ShowProfilesCommand),
        nameof(ShowSoundPacksCommand), nameof(ShowSetAndForgetCommand), nameof(ShowTwitchCommand))]
    [NotifyPropertyChangedFor(nameof(ProfilesLockedReason))]
    private bool _sectionsEnabled;

    private BoolSettingFieldViewModel? _backupProfile;

    /// <summary>
    /// As in the old GUI, profiles can only be edited with a backup to fall back on: the Profiles page opens only while
    /// Backup Profile is on and a save folder is set. Null when it's open, else why it's locked (the nav button's tip).
    /// </summary>
    public string? ProfilesLockedReason =>
        !SectionsEnabled ? null
        : !_status.SavePathAvailable ? "Choose your save folder on the Home page to edit profiles."
        : _backupProfile is { Value: false } ? "Turn on Backup Profile in Mod Settings to edit profiles. Edits need a backup to fall back on."
        : _backingUpProfiles ? "Backing up your profiles before they can be edited…"
        : !_profilesBackedUp ? "Your profiles couldn't be backed up, so editing them stays locked. Turn Backup Profile off and on again to retry."
        : null;

    private bool CanShowProfiles => SectionsEnabled && ProfilesLockedReason is null;

    public MainWindowViewModel(
        StartupService startup,
        SettingsWarningPresenter warnings,
        ThemeService theme,
        StatusViewModel status,
        ModSettingsViewModel modSettings,
        ColorsViewModel colors,
        RocksmithSettingsViewModel rocksmith,
        AsioSettingsViewModel asio,
        ThemesViewModel themes,
        ProfilesViewModel profiles,
        SoundPacksViewModel soundPacks,
        SetAndForgetViewModel setAndForget,
        TwitchViewModel twitch,
        NavigationService navigation,
        IDialogService dialogs,
        AutoSaveService autoSave)
    {
        _startup = startup;
        _warnings = warnings;
        _theme = theme;
        _status = status;
        _modSettings = modSettings;
        _colors = colors;
        _rocksmith = rocksmith;
        _asio = asio;
        _themes = themes;
        _profiles = profiles;
        _soundPacks = soundPacks;
        _setAndForget = setAndForget;
        _twitch = twitch;
        _navigation = navigation;
        _dialogs = dialogs;
        _autoSave = autoSave;
        _currentPage = status;

        _navigation.RegisterHandler(HandleNavigationAsync);
        _status.PropertyChanged += (_, e) =>
        {
            if (e.PropertyName == nameof(StatusViewModel.SavePathAvailable))
                ProfilesLockChanged();
            else if (e.PropertyName == nameof(StatusViewModel.SavePath) && SectionsEnabled)
                OnSaveFolderChanged();
        };
    }

    /// <summary>
    /// A new save folder is another account: the loaded profile, its backups and the backup that unlocked editing
    /// all belong to the old one, so they're dropped and the new folder is backed up before edits unlock again.
    /// </summary>
    private void OnSaveFolderChanged()
    {
        _profilesBackedUp = false;
        _profiles.Reset();
        if (CurrentPage == _profiles)
            ShowStatus();
        ProfilesLockChanged();
        _ = BackUpProfilesForEditingAsync();
    }

    /// <summary>
    /// Takes the backup profile edits rely on, as the old GUI did when Backup Profile was switched on, for a save
    /// folder that hasn't been backed up this session. Edits stay locked until it succeeds.
    /// </summary>
    private async Task BackUpProfilesForEditingAsync()
    {
        if (_backingUpProfiles)
            return; // The running backup checks for a folder change when it finishes

        _backingUpProfiles = true;
        ProfilesLockChanged();
        try
        {
            while (!_profilesBackedUp && _status.SavePathAvailable && _backupProfile is { Value: true })
            {
                string savePath = _status.SavePath!;
                bool backedUp = await _startup.BackUpProfilesAsync(_status.RocksmithFolder!, savePath);
                if (savePath != _status.SavePath)
                    continue; // The folder changed meanwhile; back up the new one instead
                _profilesBackedUp = backedUp;
                break;
            }
        }
        finally
        {
            _backingUpProfiles = false;
            ProfilesLockChanged();
        }
    }

    private void ProfilesLockChanged()
    {
        OnPropertyChanged(nameof(ProfilesLockedReason));
        ShowProfilesCommand.NotifyCanExecuteChanged();
    }

    private async Task HandleNavigationAsync(NavigationTarget target, object? parameter)
    {
        if (!SectionsEnabled && target != NavigationTarget.Status)
            return;

        switch (target)
        {
            case NavigationTarget.Status:
                ShowStatus();
                break;
            case NavigationTarget.ModSettings:
                ShowModSettings();
                break;
            case NavigationTarget.Colors:
                await ShowColorsAsync();
                break;
            case NavigationTarget.Rocksmith:
                await ShowRocksmithAsync();
                break;
            case NavigationTarget.Asio:
                await ShowAsioAsync();
                break;
            case NavigationTarget.Themes:
                await ShowThemesAsync();
                break;
            case NavigationTarget.Profiles:
                if (CanShowProfiles)
                    await ShowProfilesAsync();
                break;
            case NavigationTarget.SoundPacks:
                await ShowSoundPacksAsync();
                break;
            case NavigationTarget.SetAndForget:
                await ShowSetAndForgetAsync();
                break;
            case NavigationTarget.Twitch:
                ShowTwitch();
                break;
        }
    }

    /// <summary>
    /// Runs the one-time startup resolution once the window is shown so its dialogs have an owner.
    /// </summary>
    public async Task InitializeAsync()
    {
        if (_initialized)
            return;
        _initialized = true;

        var result = await _startup.RunAsync();
        if (!result.Completed)
        {
            // The resolver already requested shutdown; leave the status page in its resolving state.
            _status.StatusMessage = "Rocksmith 2014 could not be located. Closing…";
            return;
        }

        // Settings are loaded now, so the saved appearance can replace the default theme.
        _theme.ApplyFromSettings();

        _status.RocksmithFolder = result.RocksmithFolder;
        _profilesBackedUp = result.ProfilesBackedUp;
        _status.SavePath = result.SavePath;
        _status.SavePathAvailable = result.SavePathAvailable;
        _status.IsReady = true;
        _status.StatusMessage = "Startup complete. Settings loaded and ready to configure.";

        // Settings are loaded now, so the settings screens can build their snapshots.
        _modSettings.Load();

        // Switching Backup Profile in Mod Settings locks or unlocks the Profiles page straight away.
        _backupProfile = _modSettings.Coordinator.Find<BoolSettingFieldViewModel>("BackupProfile");
        if (_backupProfile is not null)
        {
            _backupProfile.PropertyChanged += (_, e) =>
            {
                if (e.PropertyName != nameof(BoolSettingFieldViewModel.Value))
                    return;
                ProfilesLockChanged();
                if (_backupProfile.Value)
                    _ = BackUpProfilesForEditingAsync();
            };
        }

        SectionsEnabled = true;

        // Twitch is application-scoped and starts whether or not its page is opened.
        await _twitch.InitializeAsync();

        await _warnings.PresentAsync(result.Warnings);
    }

    /// <summary>True when closing has to check something first; see <see cref="ConfirmCloseAsync"/>.</summary>
    public bool HasWorkBeforeClose =>
        IsWritingFiles || _twitch.RulesDirty || _autoSave.HasUnsavedChanges;

    private bool IsWritingFiles => _soundPacks.IsBusy || _setAndForget.IsBusy || _profiles.IsBusy;

    /// <summary>
    /// Run before the window closes: a long job still writing game files, unsaved Twitch reward rules, and settings
    /// still waiting to save (retried if that fails). False keeps the window open.
    /// </summary>
    public async Task<bool> ConfirmCloseAsync()
    {
        // Closing kills the background thread mid-write, which can leave audio.psarc, cache.psarc or a profile cut short.
        if (IsWritingFiles && await _dialogs.ShowChoiceAsync(
                "RSMods is still working on your game files (sound packs, Set and Forget or a profile). " +
                "Closing now can leave a file half-written.\n\nWait for it to finish before closing?",
                "Still working", "Keep RSMods open", "Close anyway"))
            return false;

        if (_twitch.RulesDirty
            && await _dialogs.ShowChoiceAsync(
                "Your Twitch reward rules have changes that aren't saved. Save them before closing?",
                "Unsaved reward rules", "Save", "Discard")
            && !await _twitch.SaveRulesBeforeCloseAsync())
        {
            ShowTwitch(); // The page says why they weren't saved
            return false;
        }

        while (!await _autoSave.FlushAllAsync())
        {
            string reason = _autoSave.LastError?.Message ?? "Unknown error.";
            if (!await _dialogs.ShowChoiceAsync(
                    $"Some settings couldn't be saved:\n{reason}\n\nClosing now loses those changes.",
                    "Couldn't save settings", "Retry", "Close anyway"))
                break;
        }

        return true;
    }

    /// <summary>Shows a page once it has loaded, unless the user went to another one while it did.</summary>
    private async Task ShowPageAsync(ObservableObject page, string section, Func<Task> initialize)
    {
        int navigation = ++_navigationCount;
        await initialize();
        if (navigation != _navigationCount)
            return;

        CurrentPage = page;
        ActiveSection = section;
    }

    private void ShowPage(ObservableObject page, string section)
    {
        _navigationCount++;
        CurrentPage = page;
        ActiveSection = section;
    }

    [RelayCommand]
    private void ShowStatus() => ShowPage(_status, "Status");

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private void ShowModSettings() => ShowPage(_modSettings, "ModSettings");

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private Task ShowColorsAsync() =>
        // Colors live in the already-loaded RSMods.ini store; the snapshot is built on first navigation.
        ShowPageAsync(_colors, "Colors", _colors.InitializeAsync);

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private Task ShowRocksmithAsync() =>
        // Rocksmith.ini is loaded lazily on first navigation; presents its own validation warnings.
        ShowPageAsync(_rocksmith, "Rocksmith", _rocksmith.InitializeAsync);

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private Task ShowAsioAsync() =>
        // RS_ASIO.ini + device enumeration are loaded lazily on first navigation.
        ShowPageAsync(_asio, "Asio", _asio.InitializeAsync);

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private Task ShowThemesAsync() =>
        // Appearance reads the already-loaded GUI settings; the snapshot is built on first navigation.
        ShowPageAsync(_themes, "Themes", _themes.InitializeAsync);

    [RelayCommand(CanExecute = nameof(CanShowProfiles))]
    private Task ShowProfilesAsync() =>
        // Profiles enumerate from the resolved save folder; the lists are built on first navigation.
        ShowPageAsync(_profiles, "Profiles", _profiles.InitializeAsync);

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private Task ShowSoundPacksAsync() =>
        // SoundPacks reads its unpacked state from the resolved Rocksmith folder on first navigation.
        ShowPageAsync(_soundPacks, "SoundPacks", _soundPacks.InitializeAsync);

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private Task ShowSetAndForgetAsync() =>
        // Stock cache-mod files and the tuning database are prepared on first navigation.
        ShowPageAsync(_setAndForget, "SetAndForget", _setAndForget.InitializeAsync);

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private void ShowTwitch() => ShowPage(_twitch, "Twitch");
}
