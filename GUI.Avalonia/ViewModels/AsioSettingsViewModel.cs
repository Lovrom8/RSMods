using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.ASIO;
using RSMods.Core;
using RSMods.Services;
using RSMods.Util;

namespace RSMods.ViewModels;

/// <summary>
/// Edits RS_ASIO.ini through the path-based <see cref="AsioSettings"/> instance. Preserves the
/// store's two disable conventions (blank driver for Output/Input.0/Input.Mic, commented driver for
/// Input.1) and the tri-state WASAPI output mode. Each change is saved a moment after it's made.
/// </summary>
internal sealed partial class AsioSettingsViewModel : ObservableObject
{
    private readonly AsioSettingsService _service;
    private readonly SettingsWarningPresenter _warnings;
    private readonly DebouncedSaver _saver;
    private bool _loading;
    private bool _initialized;

    public ObservableCollection<string> AvailableDrivers { get; } = [];

    // The drivers found in the registry; AvailableDrivers also lists saved ones that aren't installed.
    private readonly HashSet<string> _installedDrivers = new(StringComparer.OrdinalIgnoreCase);

    public AsioInputViewModel Input0 { get; }
    public AsioInputViewModel Input1 { get; }
    public AsioInputViewModel InputMic { get; }
    public IReadOnlyList<AsioInputViewModel> Inputs { get; }

    public AsioSettingsViewModel(AsioSettingsService service, SettingsWarningPresenter warnings, AutoSaveService autoSave)
    {
        _service = service;
        _warnings = warnings;
        _saver = autoSave.Create(SaveAsync, ex => StatusMessage = $"Couldn't save: {ex.Message}");

        Input0 = new AsioInputViewModel("Input 0 (instrument)", "Player 1's cable, through your audio interface.", AvailableDrivers, IsInstalled);
        Input1 = new AsioInputViewModel("Input 1 (second instrument)", "Player 2's cable, through your audio interface.", AvailableDrivers, IsInstalled);
        InputMic = new AsioInputViewModel("Input Mic (microphone)", "For singing. Needs RS_ASIO 0.5.5 or later.", AvailableDrivers, IsInstalled);
        Inputs = [Input0, Input1, InputMic];

        foreach (var input in Inputs)
            input.PropertyChanged += OnChildChanged;
    }

    // --- Config ---
    [ObservableProperty] private WasapiOutputMode _wasapiOutputs;
    [ObservableProperty] private bool _enableWasapiInputs;
    [ObservableProperty] private bool _enableAsio;

    public static WasapiOutputMode[] WasapiOutputModes { get; } = (WasapiOutputMode[])Enum.GetValues(typeof(WasapiOutputMode));

    // --- Asio (buffer) ---
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(IsCustomBufferSize))]
    private string _bufferSizeMode = RsAsioLimits.BufferModeDriver;

    public bool IsCustomBufferSize => BufferSizeMode == RsAsioLimits.BufferModeCustom;
    [ObservableProperty] private decimal _customBufferSize;

    public static string[] BufferModes { get; } = [RsAsioLimits.BufferModeDriver, RsAsioLimits.BufferModeHost, RsAsioLimits.BufferModeCustom];

    public static decimal CustomBufferSizeMin => RsAsioLimits.CustomBufferSizeMin;
    public static decimal CustomBufferSizeMax => RsAsioLimits.CustomBufferSizeMax;

    // --- Output ---
    [ObservableProperty] private bool _outputEnabled;
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(OutputDriverNotInstalled))]
    private string? _outputDriver;

    public bool OutputDriverNotInstalled => !string.IsNullOrEmpty(OutputDriver) && !IsInstalled(OutputDriver);
    [ObservableProperty] private decimal _outputBaseChannel;
    [ObservableProperty] private decimal _outputAltBaseChannel;
    [ObservableProperty] private bool _outputEnableEndpointVolume;
    [ObservableProperty] private bool _outputEnableMasterVolume;
    [ObservableProperty] private decimal _outputMasterVolumePercent;
    [ObservableProperty] private bool _outputRefCountHack;

    public static decimal ChannelMin => RsAsioLimits.ChannelMin;
    public static decimal ChannelMax => RsAsioLimits.ChannelMax;
    public static decimal VolumePercentMin => RsAsioLimits.VolumePercentMin;
    public static decimal VolumePercentMax => RsAsioLimits.VolumePercentMax;

    [ObservableProperty]
    private string _statusMessage = string.Empty;

    /// <summary>Why RS_ASIO can't work as set up (missing files, no ASIO driver), or empty when nothing is wrong.</summary>
    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(HasSetupWarning))]
    private string _setupWarning = string.Empty;

    public bool HasSetupWarning => SetupWarning.Length > 0;

    /// <summary>
    /// First-time load: enumerates devices, captures validation warnings while reading the snapshot,
    /// and presents them. Idempotent; navigating back reuses the loaded snapshot.
    /// </summary>
    public async Task InitializeAsync()
    {
        if (_initialized)
        {
            // Checked on every visit, so installing what was missing clears the warning without a restart.
            PopulateDrivers(_service.Get());
            CheckSetup();
            OnPropertyChanged(nameof(OutputDriverNotInstalled));
            foreach (var input in Inputs)
                input.RefreshInstalled();
            return;
        }
        _initialized = true;

        var settings = _service.Get();

        var warnings = new List<IniValidationWarning>();
        void Collect(IniValidationWarning warning) => warnings.Add(warning);

        settings.ValidationWarning += Collect;
        try
        {
            PopulateDrivers(settings);
            Load();
        }
        finally
        {
            settings.ValidationWarning -= Collect;
        }

        CheckSetup();
        await _warnings.PresentAsync(warnings);
    }

    private bool IsInstalled(string? driver) => driver is not null && _installedDrivers.Contains(driver);

    private void CheckSetup()
    {
        var problems = new List<string>();

        IReadOnlyList<string> missing = _service.FindMissingFiles();
        if (missing.Count > 0)
        {
            string files = missing.Count == 1 ? missing[0] : string.Join(", ", missing.Take(missing.Count - 1)) + " and " + missing[^1];
            string verb = missing.Count == 1 ? "is" : "are";
            problems.Add(
                $"{files} {verb} missing from your Rocksmith folder, so Rocksmith won't use RS_ASIO and these settings have no effect. " +
                "Install (or reinstall) RS_ASIO to use them." +
                (missing.Contains(AsioSettings.DefaultFileName) ? " Changing a setting here creates RS_ASIO.ini with the values shown." : ""));
        }

        if (_installedDrivers.Count == 0)
            problems.Add("No ASIO drivers are installed. RS_ASIO needs your audio interface's ASIO driver, with the interface plugged in.");

        SetupWarning = string.Join(Environment.NewLine + Environment.NewLine, problems);
    }

    private void PopulateDrivers(AsioSettings settings)
    {
        _installedDrivers.Clear();
        foreach (var name in _service.FindDeviceNames())
            _installedDrivers.Add(name);

        // Added to rather than rebuilt, so a driver box keeps its selection when the list is refreshed.
        foreach (var name in _installedDrivers)
        {
            if (!AvailableDrivers.Contains(name))
                AvailableDrivers.Add(name);
        }

        // Keep any saved driver that is no longer installed, so its ComboBox still shows it.
        foreach (var saved in new[]
                 {
                     settings.Output.Driver, settings.Input0.Driver,
                     settings.Input1.Driver, settings.InputMic.Driver,
                 })
        {
            if (!string.IsNullOrEmpty(saved) && !AvailableDrivers.Contains(saved))
                AvailableDrivers.Add(saved);
        }
    }

    private void Load()
    {
        var s = _service.Get();
        _loading = true;
        try
        {
            WasapiOutputs = s.Config.WasapiOutputs;
            EnableWasapiInputs = s.Config.EnableWasapiInputs;
            EnableAsio = s.Config.EnableAsio;

            BufferSizeMode = NormalizeBufferMode(s.AsioSection.BufferSizeMode);
            CustomBufferSize = s.AsioSection.CustomBufferSize;

            OutputEnabled = !s.Output.Disabled;
            OutputDriver = string.IsNullOrWhiteSpace(s.Output.Driver) ? null : s.Output.Driver;
            OutputBaseChannel = s.Output.BaseChannel;
            OutputAltBaseChannel = s.Output.AltBaseChannel;
            OutputEnableEndpointVolume = s.Output.EnableSoftwareEndpointVolumeControl;
            OutputEnableMasterVolume = s.Output.EnableSoftwareMasterVolumeControl;
            OutputMasterVolumePercent = s.Output.SoftwareMasterVolumePercent;
            OutputRefCountHack = s.Output.EnableRefCountHack;

            LoadInput(Input0, s.Input0.Disabled, s.Input0.Driver, s.Input0.Channel,
                s.Input0.EnableSoftwareEndpointVolumeControl, s.Input0.EnableSoftwareMasterVolumeControl,
                s.Input0.SoftwareMasterVolumePercent, s.Input0.EnableRefCountHack);

            LoadInput(Input1, s.Input1.Disabled, s.Input1.Driver, s.Input1.Channel,
                s.Input1.EnableSoftwareEndpointVolumeControl, s.Input1.EnableSoftwareMasterVolumeControl,
                s.Input1.SoftwareMasterVolumePercent, s.Input1.EnableRefCountHack);

            LoadInput(InputMic, s.InputMic.Disabled, s.InputMic.Driver, s.InputMic.Channel,
                s.InputMic.EnableSoftwareEndpointVolumeControl, s.InputMic.EnableSoftwareMasterVolumeControl,
                s.InputMic.SoftwareMasterVolumePercent, s.InputMic.EnableRefCountHack);
        }
        finally
        {
            _loading = false;
            StatusMessage = string.Empty;
        }
    }

    private static void LoadInput(AsioInputViewModel vm, bool disabled, string driver, int channel,
        bool endpoint, bool master, int percent, bool refHack)
    {
        // No driver means RS_ASIO can't use the input. Input 1 marks "disabled" by commenting its driver out, so a
        // blank uncommented one (a missing RS_ASIO.ini gives that) would otherwise show as enabled.
        vm.Enabled = !disabled && !string.IsNullOrWhiteSpace(driver);
        // No driver is null, as the driver box reports it; a blank string would read as a change and save straight away.
        vm.Driver = string.IsNullOrWhiteSpace(driver) ? null : driver;
        vm.Channel = channel;
        vm.EnableEndpointVolume = endpoint;
        vm.EnableMasterVolume = master;
        vm.MasterVolumePercent = percent;
        vm.RefCountHack = refHack;
    }

    private static string NormalizeBufferMode(string mode) =>
        RsAsioLimits.IsValidBufferMode(mode) ? mode.Trim().ToLowerInvariant() : RsAsioLimits.BufferModeDriver;

    private async Task SaveAsync()
    {
        var s = _service.Get();

        // The store's setters persist to disk; run the write off the UI thread, as one write rather than one per setter.
        await Task.Run(() =>
        {
            using var _ = s.SuspendSave();

            s.Config.WasapiOutputs = WasapiOutputs;
            s.Config.EnableWasapiInputs = EnableWasapiInputs;
            s.Config.EnableAsio = EnableAsio;

            s.AsioSection.BufferSizeMode = BufferSizeMode;
            s.AsioSection.CustomBufferSize = (int)CustomBufferSize;

            // Output disable convention: blank Driver.
            if (OutputEnabled)
            {
                s.Output.Disabled = false;
                if (!string.IsNullOrEmpty(OutputDriver))
                    s.Output.Driver = OutputDriver;
            }
            else
            {
                s.Output.Disabled = true;
            }
            s.Output.BaseChannel = (int)OutputBaseChannel;
            s.Output.AltBaseChannel = (int)OutputAltBaseChannel;
            s.Output.EnableSoftwareEndpointVolumeControl = OutputEnableEndpointVolume;
            s.Output.EnableSoftwareMasterVolumeControl = OutputEnableMasterVolume;
            s.Output.SoftwareMasterVolumePercent = (int)OutputMasterVolumePercent;
            s.Output.EnableRefCountHack = OutputRefCountHack;

            // Input.0 / Input.Mic: blank-driver disable. Input.1: commented-driver disable.
            // Both are expressed through each section's Disabled/Driver API, so the mapping is uniform.
            SaveInput(Input0, v => s.Input0.Disabled = v, v => s.Input0.Driver = v, v => s.Input0.Channel = v,
                v => s.Input0.EnableSoftwareEndpointVolumeControl = v, v => s.Input0.EnableSoftwareMasterVolumeControl = v,
                v => s.Input0.SoftwareMasterVolumePercent = v, v => s.Input0.EnableRefCountHack = v);

            SaveInput(Input1, v => s.Input1.Disabled = v, v => s.Input1.Driver = v, v => s.Input1.Channel = v,
                v => s.Input1.EnableSoftwareEndpointVolumeControl = v, v => s.Input1.EnableSoftwareMasterVolumeControl = v,
                v => s.Input1.SoftwareMasterVolumePercent = v, v => s.Input1.EnableRefCountHack = v);

            SaveInput(InputMic, v => s.InputMic.Disabled = v, v => s.InputMic.Driver = v, v => s.InputMic.Channel = v,
                v => s.InputMic.EnableSoftwareEndpointVolumeControl = v, v => s.InputMic.EnableSoftwareMasterVolumeControl = v,
                v => s.InputMic.SoftwareMasterVolumePercent = v, v => s.InputMic.EnableRefCountHack = v);
        });

        StatusMessage = $"Saved at {DateTime.Now:HH:mm:ss}";
    }

    private static void SaveInput(AsioInputViewModel vm, Action<bool> setDisabled, Action<string> setDriver,
        Action<int> setChannel, Action<bool> setEndpoint, Action<bool> setMaster, Action<int> setPercent,
        Action<bool> setRefHack)
    {
        if (vm.Enabled)
        {
            setDisabled(false);
            if (!string.IsNullOrEmpty(vm.Driver))
                setDriver(vm.Driver);
        }
        else
        {
            setDisabled(true);
        }

        setChannel((int)vm.Channel);
        setEndpoint(vm.EnableEndpointVolume);
        setMaster(vm.EnableMasterVolume);
        setPercent((int)vm.MasterVolumePercent);
        setRefHack(vm.RefCountHack);
    }

    private void OnChildChanged(object? sender, PropertyChangedEventArgs e)
    {
        // DriverNotInstalled is re-raised on every visit; saving on it rewrote RS_ASIO.ini just by opening the page.
        if (!_loading && e.PropertyName is not null && SavedInputProperties.Contains(e.PropertyName))
            _saver.Request();
    }

    protected override void OnPropertyChanged(PropertyChangedEventArgs e)
    {
        base.OnPropertyChanged(e);

        if (!_loading && e.PropertyName is not null && SavedProperties.Contains(e.PropertyName))
            _saver.Request();
    }

    // Listing what's saved, not what isn't, so a display-only property added later can't start saving.
    private static readonly HashSet<string> SavedProperties =
    [
        nameof(WasapiOutputs), nameof(EnableWasapiInputs), nameof(EnableAsio),
        nameof(BufferSizeMode), nameof(CustomBufferSize),
        nameof(OutputEnabled), nameof(OutputDriver), nameof(OutputBaseChannel), nameof(OutputAltBaseChannel),
        nameof(OutputEnableEndpointVolume), nameof(OutputEnableMasterVolume), nameof(OutputMasterVolumePercent),
        nameof(OutputRefCountHack),
    ];

    private static readonly HashSet<string> SavedInputProperties =
    [
        nameof(AsioInputViewModel.Enabled), nameof(AsioInputViewModel.Driver), nameof(AsioInputViewModel.Channel),
        nameof(AsioInputViewModel.EnableEndpointVolume), nameof(AsioInputViewModel.EnableMasterVolume),
        nameof(AsioInputViewModel.MasterVolumePercent), nameof(AsioInputViewModel.RefCountHack),
    ];
}
