using System.Threading.Tasks;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
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
    private bool _initialized;

    [ObservableProperty]
    private ObservableObject _currentPage;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(
        nameof(ShowModSettingsCommand), nameof(ShowColorsCommand), nameof(ShowRocksmithCommand),
        nameof(ShowAsioCommand), nameof(ShowThemesCommand), nameof(ShowProfilesCommand),
        nameof(ShowSoundPacksCommand), nameof(ShowSetAndForgetCommand), nameof(ShowTwitchCommand))]
    private bool _sectionsEnabled;

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
        NavigationService navigation)
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
        _currentPage = status;

        _navigation.RegisterHandler(HandleNavigationAsync);
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
        _status.SavePath = result.SavePath;
        _status.SavePathAvailable = result.SavePathAvailable;
        _status.IsReady = true;
        _status.StatusMessage = "Startup complete. Settings loaded and ready to configure.";

        // Settings are loaded now, so the settings screens can build their snapshots.
        _modSettings.Load();
        SectionsEnabled = true;

        // Twitch is application-scoped and starts whether or not its page is opened.
        await _twitch.InitializeAsync();

        await _warnings.PresentAsync(result.Warnings);
    }

    [RelayCommand]
    private void ShowStatus() => CurrentPage = _status;

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private void ShowModSettings() => CurrentPage = _modSettings;

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowColorsAsync()
    {
        // Colours live in the already-loaded RSMods.ini store; the snapshot is built on first navigation.
        await _colors.InitializeAsync();
        CurrentPage = _colors;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowRocksmithAsync()
    {
        // Rocksmith.ini is loaded lazily on first navigation; presents its own validation warnings.
        await _rocksmith.InitializeAsync();
        CurrentPage = _rocksmith;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowAsioAsync()
    {
        // RS_ASIO.ini + device enumeration are loaded lazily on first navigation.
        await _asio.InitializeAsync();
        CurrentPage = _asio;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowThemesAsync()
    {
        // Appearance reads the already-loaded GUI settings; the snapshot is built on first navigation.
        await _themes.InitializeAsync();
        CurrentPage = _themes;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowProfilesAsync()
    {
        // Profiles enumerate from the resolved save folder; the lists are built on first navigation.
        await _profiles.InitializeAsync();
        CurrentPage = _profiles;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowSoundPacksAsync()
    {
        // SoundPacks reads its unpacked state from the resolved Rocksmith folder on first navigation.
        await _soundPacks.InitializeAsync();
        CurrentPage = _soundPacks;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowSetAndForgetAsync()
    {
        // Stock cache-mod files and the tuning database are prepared on first navigation.
        await _setAndForget.InitializeAsync();
        CurrentPage = _setAndForget;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private void ShowTwitch() => CurrentPage = _twitch;
}
