using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Services;

namespace RSMods.ViewModels;

internal sealed partial class MainWindowViewModel(
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
    TwitchViewModel twitch) : ObservableObject
{
    private bool _initialized;

    [ObservableProperty]
    private ObservableObject _currentPage = status;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(
        nameof(ShowModSettingsCommand), nameof(ShowColorsCommand), nameof(ShowRocksmithCommand),
        nameof(ShowAsioCommand), nameof(ShowThemesCommand), nameof(ShowProfilesCommand),
        nameof(ShowSoundPacksCommand), nameof(ShowSetAndForgetCommand), nameof(ShowTwitchCommand))]
    private bool _sectionsEnabled;

    /// <summary>
    /// Runs the one-time startup resolution once the window is shown so its dialogs have an owner.
    /// </summary>
    public async Task InitializeAsync()
    {
        if (_initialized)
            return;
        _initialized = true;

        var result = await startup.RunAsync();
        if (!result.Completed)
        {
            // The resolver already requested shutdown; leave the status page in its resolving state.
            status.StatusMessage = "Rocksmith 2014 could not be located. Closing…";
            return;
        }

        // Settings are loaded now, so the saved appearance can replace the default theme.
        theme.ApplyFromSettings();

        status.RocksmithFolder = result.RocksmithFolder;
        status.SavePath = result.SavePath;
        status.SavePathAvailable = result.SavePathAvailable;
        status.IsReady = true;
        status.StatusMessage = "Startup complete. Settings loaded and ready to configure.";

        // Settings are loaded now, so the settings screens can build their snapshots.
        modSettings.Load();
        SectionsEnabled = true;

        // Twitch is application-scoped and starts whether or not its page is opened.
        await twitch.InitializeAsync();

        await warnings.PresentAsync(result.Warnings);
    }

    [RelayCommand]
    private void ShowStatus() => CurrentPage = status;

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private void ShowModSettings() => CurrentPage = modSettings;

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowColorsAsync()
    {
        // Colours live in the already-loaded RSMods.ini store; the snapshot is built on first navigation.
        await colors.InitializeAsync();
        CurrentPage = colors;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowRocksmithAsync()
    {
        // Rocksmith.ini is loaded lazily on first navigation; presents its own validation warnings.
        await rocksmith.InitializeAsync();
        CurrentPage = rocksmith;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowAsioAsync()
    {
        // RS_ASIO.ini + device enumeration are loaded lazily on first navigation.
        await asio.InitializeAsync();
        CurrentPage = asio;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowThemesAsync()
    {
        // Appearance reads the already-loaded GUI settings; the snapshot is built on first navigation.
        await themes.InitializeAsync();
        CurrentPage = themes;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowProfilesAsync()
    {
        // Profiles enumerate from the resolved save folder; the lists are built on first navigation.
        await profiles.InitializeAsync();
        CurrentPage = profiles;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowSoundPacksAsync()
    {
        // SoundPacks reads its unpacked state from the resolved Rocksmith folder on first navigation.
        await soundPacks.InitializeAsync();
        CurrentPage = soundPacks;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowSetAndForgetAsync()
    {
        // Stock cache-mod files and the tuning database are prepared on first navigation.
        await setAndForget.InitializeAsync();
        CurrentPage = setAndForget;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private void ShowTwitch() => CurrentPage = twitch;
}
