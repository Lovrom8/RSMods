using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.ASIO;
using RSMods.Core;
using RSMods.Services;

namespace RSMods.ViewModels;

/// <summary>
/// Edits RS_ASIO.ini through the path-based <see cref="AsioSettings"/> instance. Preserves the
/// store's two disable conventions (blank driver for Output/Input.0/Input.Mic, commented driver for
/// Input.1) and the tri-state WASAPI output mode, and reuses the frontend-neutral device enumeration.
/// </summary>
internal sealed partial class AsioSettingsViewModel : ObservableObject
{
    private readonly AsioSettingsService _service;
    private readonly SettingsWarningPresenter _warnings;
    private bool _loading;
    private bool _initialized;

    public ObservableCollection<string> AvailableDrivers { get; } = [];

    public AsioInputViewModel Input0 { get; }
    public AsioInputViewModel Input1 { get; }
    public AsioInputViewModel InputMic { get; }
    public IReadOnlyList<AsioInputViewModel> Inputs { get; }

    public AsioSettingsViewModel(AsioSettingsService service, SettingsWarningPresenter warnings)
    {
        _service = service;
        _warnings = warnings;

        Input0 = new AsioInputViewModel("Input 0 (instrument)", AvailableDrivers);
        Input1 = new AsioInputViewModel("Input 1 (second instrument)", AvailableDrivers);
        InputMic = new AsioInputViewModel("Input Mic (microphone)", AvailableDrivers);
        Inputs = [Input0, Input1, InputMic];

        foreach (var input in Inputs)
            input.PropertyChanged += OnChildChanged;
    }

    // --- Config ---
    [ObservableProperty] private WasapiOutputMode _wasapiOutputs;
    [ObservableProperty] private bool _enableWasapiInputs;
    [ObservableProperty] private bool _enableAsio;

    public static WasapiOutputMode[] WasapiOutputModes { get; } =
        (WasapiOutputMode[])Enum.GetValues(typeof(WasapiOutputMode));

    // --- Asio (buffer) ---
    [ObservableProperty] private string _bufferSizeMode = RsAsioLimits.BufferModeDriver;
    [ObservableProperty] private decimal _customBufferSize;

    public static string[] BufferModes { get; } =
        [RsAsioLimits.BufferModeDriver, RsAsioLimits.BufferModeHost, RsAsioLimits.BufferModeCustom];

    public static decimal CustomBufferSizeMin => RsAsioLimits.CustomBufferSizeMin;
    public static decimal CustomBufferSizeMax => RsAsioLimits.CustomBufferSizeMax;

    // --- Output ---
    [ObservableProperty] private bool _outputEnabled;
    [ObservableProperty] private string? _outputDriver;
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
    [NotifyCanExecuteChangedFor(nameof(SaveCommand))]
    [NotifyCanExecuteChangedFor(nameof(RevertCommand))]
    private bool _isDirty;

    [ObservableProperty]
    private string _statusMessage = string.Empty;

    /// <summary>
    /// First-time load: enumerates devices, captures validation warnings while reading the snapshot,
    /// and presents them. Idempotent; navigating back reuses the loaded snapshot.
    /// </summary>
    public async Task InitializeAsync()
    {
        if (_initialized)
            return;
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

        if (!settings.SettingsExist)
            StatusMessage = "RS_ASIO.ini was not found; showing defaults. Saving will create it.";

        await _warnings.PresentAsync(warnings);
    }

    private void PopulateDrivers(AsioSettings settings)
    {
        AvailableDrivers.Clear();
        foreach (var name in _service.FindDeviceNames())
            AvailableDrivers.Add(name);

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
            OutputDriver = s.Output.Driver;
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
            IsDirty = false;
            StatusMessage = string.Empty;
        }
    }

    private static void LoadInput(AsioInputViewModel vm, bool disabled, string driver, int channel,
        bool endpoint, bool master, int percent, bool refHack)
    {
        vm.Enabled = !disabled;
        vm.Driver = driver;
        vm.Channel = channel;
        vm.EnableEndpointVolume = endpoint;
        vm.EnableMasterVolume = master;
        vm.MasterVolumePercent = percent;
        vm.RefCountHack = refHack;
    }

    private static string NormalizeBufferMode(string mode) =>
        RsAsioLimits.IsValidBufferMode(mode) ? mode.Trim().ToLowerInvariant() : RsAsioLimits.BufferModeDriver;

    private bool CanSaveOrRevert => IsDirty;

    [RelayCommand(CanExecute = nameof(CanSaveOrRevert))]
    private async Task SaveAsync()
    {
        var s = _service.Get();

        // The store's setters persist to disk; run the write off the UI thread.
        await Task.Run(() =>
        {
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

        IsDirty = false;
        StatusMessage = "Settings saved.";
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

    [RelayCommand(CanExecute = nameof(CanSaveOrRevert))]
    private void Revert()
    {
        Load();
        StatusMessage = "Reverted to the last saved values.";
    }

    private void OnChildChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!_loading)
            IsDirty = true;
    }

    protected override void OnPropertyChanged(PropertyChangedEventArgs e)
    {
        base.OnPropertyChanged(e);

        if (_loading)
            return;

        if (e.PropertyName is nameof(IsDirty) or nameof(StatusMessage))
            return;

        IsDirty = true;
    }
}
