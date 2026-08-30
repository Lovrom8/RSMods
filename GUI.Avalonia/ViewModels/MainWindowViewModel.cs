using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Services;

namespace RSMods.ViewModels;

internal sealed partial class MainWindowViewModel : ObservableObject
{
    private readonly StartupService _startup;
    private readonly SettingsWarningPresenter _warnings;
    private readonly ThemeService _theme;
    private bool _initialized;

    public StatusViewModel Status { get; }
    public ModSettingsViewModel ModSettings { get; }
    public ColorsViewModel Colors { get; }
    public RocksmithSettingsViewModel Rocksmith { get; }
    public AsioSettingsViewModel Asio { get; }
    public ThemesViewModel Themes { get; }
    public ProfilesViewModel Profiles { get; }
    public SoundPacksViewModel SoundPacks { get; }
    public SetAndForgetViewModel SetAndForget { get; }

    [ObservableProperty]
    private ObservableObject _currentPage;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(ShowModSettingsCommand))]
    [NotifyCanExecuteChangedFor(nameof(ShowColorsCommand))]
    [NotifyCanExecuteChangedFor(nameof(ShowRocksmithCommand))]
    [NotifyCanExecuteChangedFor(nameof(ShowAsioCommand))]
    [NotifyCanExecuteChangedFor(nameof(ShowThemesCommand))]
    [NotifyCanExecuteChangedFor(nameof(ShowProfilesCommand))]
    [NotifyCanExecuteChangedFor(nameof(ShowSoundPacksCommand))]
    [NotifyCanExecuteChangedFor(nameof(ShowSetAndForgetCommand))]
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
        SetAndForgetViewModel setAndForget)
    {
        _startup = startup;
        _warnings = warnings;
        _theme = theme;
        Status = status;
        ModSettings = modSettings;
        Colors = colors;
        Rocksmith = rocksmith;
        Asio = asio;
        Themes = themes;
        Profiles = profiles;
        SoundPacks = soundPacks;
        SetAndForget = setAndForget;
        _currentPage = status;
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
            Status.StatusMessage = "Rocksmith 2014 could not be located. Closing…";
            return;
        }

        // Settings are loaded now, so the saved appearance can replace the default theme.
        _theme.ApplyFromSettings();

        Status.RocksmithFolder = result.RocksmithFolder;
        Status.SavePath = result.SavePath;
        Status.SavePathAvailable = result.SavePathAvailable;
        Status.IsReady = true;
        Status.StatusMessage = "Startup complete. Settings loaded and ready to configure.";

        // Settings are loaded now, so the settings screens can build their snapshots.
        ModSettings.Load();
        SectionsEnabled = true;

        await _warnings.PresentAsync(result.Warnings);
    }

    [RelayCommand]
    private void ShowStatus() => CurrentPage = Status;

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private void ShowModSettings() => CurrentPage = ModSettings;

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowColorsAsync()
    {
        // Colours live in the already-loaded RSMods.ini store; the snapshot is built on first navigation.
        await Colors.InitializeAsync();
        CurrentPage = Colors;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowRocksmithAsync()
    {
        // Rocksmith.ini is loaded lazily on first navigation; presents its own validation warnings.
        await Rocksmith.InitializeAsync();
        CurrentPage = Rocksmith;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowAsioAsync()
    {
        // RS_ASIO.ini + device enumeration are loaded lazily on first navigation.
        await Asio.InitializeAsync();
        CurrentPage = Asio;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowThemesAsync()
    {
        // Appearance reads the already-loaded GUI settings; the snapshot is built on first navigation.
        await Themes.InitializeAsync();
        CurrentPage = Themes;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowProfilesAsync()
    {
        // Profiles enumerate from the resolved save folder; the lists are built on first navigation.
        await Profiles.InitializeAsync();
        CurrentPage = Profiles;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowSoundPacksAsync()
    {
        // SoundPacks reads its unpacked state from the resolved Rocksmith folder on first navigation.
        await SoundPacks.InitializeAsync();
        CurrentPage = SoundPacks;
    }

    [RelayCommand(CanExecute = nameof(SectionsEnabled))]
    private async Task ShowSetAndForgetAsync()
    {
        // Stock cache-mod files and the tuning database are prepared on first navigation.
        await SetAndForget.InitializeAsync();
        CurrentPage = SetAndForget;
    }
}
