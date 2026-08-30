using System;
using System.Collections.Generic;
using System.ComponentModel;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Core;
using RSMods.Rocksmith;
using RSMods.Services;

namespace RSMods.ViewModels;

/// <summary>
/// Edits Rocksmith.ini (the game's own settings) through the path-based <see cref="RocksmithSettings"/>
/// instance. Holds an editable snapshot loaded from the store and writes it back on save; the store's
/// round-trip-safe backend preserves unknown sections, comments, and commented-out values.
/// </summary>
internal sealed partial class RocksmithSettingsViewModel(RocksmithSettingsService service, SettingsWarningPresenter warnings) : ObservableObject
{
    private readonly SettingsWarningPresenter _warnings = warnings;
    private bool _loading;
    private bool _initialized;

    // --- Audio ---
    [ObservableProperty] private bool _enableMicrophone;
    [ObservableProperty] private bool _exclusiveMode;
    [ObservableProperty] private decimal _latencyBuffer;
    [ObservableProperty] private string _forceDefaultPlaybackDevice = string.Empty;
    [ObservableProperty] private bool _forceWdm;
    [ObservableProperty] private bool _forceDirectXSink;
    [ObservableProperty] private bool _dumpAudioLog;
    [ObservableProperty] private decimal _maxOutputBufferSize;
    [ObservableProperty] private bool _realToneCableOnly;
    [ObservableProperty] private bool _win32UltraLowLatencyMode;

    public static decimal LatencyBufferMin => RocksmithSettings.AudioSettings.MinLatencyBuffer;
    public static decimal LatencyBufferMax => RocksmithSettings.AudioSettings.MaxLatencyBuffer;

    // --- Renderer.Win32 ---
    [ObservableProperty] private bool _showGamepadUi;
    [ObservableProperty] private decimal _screenWidth;
    [ObservableProperty] private decimal _screenHeight;
    [ObservableProperty] private FullscreenMode _fullscreen;
    [ObservableProperty] private VisualQualityMode _visualQuality;
    [ObservableProperty] private decimal _renderingWidth;
    [ObservableProperty] private decimal _renderingHeight;
    [ObservableProperty] private bool _enablePostEffects;
    [ObservableProperty] private bool _enableShadows;
    [ObservableProperty] private bool _enableHighResScope;
    [ObservableProperty] private bool _enableDepthOfField;
    [ObservableProperty] private bool _enablePerPixelLighting;
    [ObservableProperty] private MsaaMode _msaaSamples;
    [ObservableProperty] private bool _disableBrowser;

    public static FullscreenMode[] FullscreenModes { get; } = (FullscreenMode[])Enum.GetValues(typeof(FullscreenMode));

    public static VisualQualityMode[] VisualQualityModes { get; } = (VisualQualityMode[])Enum.GetValues(typeof(VisualQualityMode));

    public static MsaaMode[] MsaaModes { get; } = (MsaaMode[])Enum.GetValues(typeof(MsaaMode));

    // --- Net ---
    [ObservableProperty] private bool _useProxy;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(SaveCommand), nameof(RevertCommand))]
    private bool _isDirty;

    [ObservableProperty]
    private string _statusMessage = string.Empty;

    /// <summary>
    /// First-time load: captures validation warnings raised while reading the snapshot and presents
    /// them. Idempotent; navigating back reuses the already-loaded snapshot.
    /// </summary>
    public async Task InitializeAsync()
    {
        if (_initialized)
            return;
        _initialized = true;

        var settings = service.Get();
        var warnings = new List<IniValidationWarning>();
        void Collect(IniValidationWarning warning) => warnings.Add(warning);

        settings.ValidationWarning += Collect;
        try
        {
            Load();
        }
        finally
        {
            settings.ValidationWarning -= Collect;
        }

        if (!settings.SettingsExist)
            StatusMessage = "Rocksmith.ini was not found; showing defaults. Saving will create it.";

        await _warnings.PresentAsync(warnings);
    }

    private void Load()
    {
        var s = service.Get();
        _loading = true;
        try
        {
            EnableMicrophone = s.Audio.EnableMicrophone;
            ExclusiveMode = s.Audio.ExclusiveMode;
            LatencyBuffer = s.Audio.LatencyBuffer;
            ForceDefaultPlaybackDevice = s.Audio.ForceDefaultPlaybackDevice;
            ForceWdm = s.Audio.ForceWDM;
            ForceDirectXSink = s.Audio.ForceDirectXSink;
            DumpAudioLog = s.Audio.DumpAudioLog;
            MaxOutputBufferSize = s.Audio.MaxOutputBufferSize;
            RealToneCableOnly = s.Audio.RealToneCableOnly;
            Win32UltraLowLatencyMode = s.Audio.Win32UltraLowLatencyMode;

            ShowGamepadUi = s.RendererWin32.ShowGamepadUI;
            ScreenWidth = s.RendererWin32.ScreenWidth;
            ScreenHeight = s.RendererWin32.ScreenHeight;
            Fullscreen = s.RendererWin32.Fullscreen;
            VisualQuality = s.RendererWin32.VisualQuality;
            RenderingWidth = s.RendererWin32.RenderingWidth;
            RenderingHeight = s.RendererWin32.RenderingHeight;
            EnablePostEffects = s.RendererWin32.EnablePostEffects;
            EnableShadows = s.RendererWin32.EnableShadows;
            EnableHighResScope = s.RendererWin32.EnableHighResScope;
            EnableDepthOfField = s.RendererWin32.EnableDepthOfField;
            EnablePerPixelLighting = s.RendererWin32.EnablePerPixelLighting;
            MsaaSamples = s.RendererWin32.MsaaSamples;
            DisableBrowser = s.RendererWin32.DisableBrowser;

            UseProxy = s.Net.UseProxy;
        }
        finally
        {
            _loading = false;
            IsDirty = false;
            StatusMessage = string.Empty;
        }
    }

    private bool CanSaveOrRevert => IsDirty;

    [RelayCommand(CanExecute = nameof(CanSaveOrRevert))]
    private async Task SaveAsync()
    {
        var s = service.Get();

        // The store's setters persist to disk; run the write off the UI thread. SuspendSave coalesces
        // the per-property auto-saves into a single file write on scope dispose instead of one per setter.
        await Task.Run(() =>
        {
            using var _ = s.SuspendSave();

            s.Audio.EnableMicrophone = EnableMicrophone;
            s.Audio.ExclusiveMode = ExclusiveMode;
            s.Audio.LatencyBuffer = (int)LatencyBuffer;
            s.Audio.ForceDefaultPlaybackDevice = ForceDefaultPlaybackDevice;
            s.Audio.ForceWDM = ForceWdm;
            s.Audio.ForceDirectXSink = ForceDirectXSink;
            s.Audio.DumpAudioLog = DumpAudioLog;
            s.Audio.MaxOutputBufferSize = (int)MaxOutputBufferSize;
            s.Audio.RealToneCableOnly = RealToneCableOnly;
            s.Audio.Win32UltraLowLatencyMode = Win32UltraLowLatencyMode;

            s.RendererWin32.ShowGamepadUI = ShowGamepadUi;
            s.RendererWin32.ScreenWidth = (int)ScreenWidth;
            s.RendererWin32.ScreenHeight = (int)ScreenHeight;
            s.RendererWin32.Fullscreen = Fullscreen;
            s.RendererWin32.VisualQuality = VisualQuality;
            s.RendererWin32.RenderingWidth = (int)RenderingWidth;
            s.RendererWin32.RenderingHeight = (int)RenderingHeight;
            s.RendererWin32.EnablePostEffects = EnablePostEffects;
            s.RendererWin32.EnableShadows = EnableShadows;
            s.RendererWin32.EnableHighResScope = EnableHighResScope;
            s.RendererWin32.EnableDepthOfField = EnableDepthOfField;
            s.RendererWin32.EnablePerPixelLighting = EnablePerPixelLighting;
            s.RendererWin32.MsaaSamples = MsaaSamples;
            s.RendererWin32.DisableBrowser = DisableBrowser;

            s.Net.UseProxy = UseProxy;
        });

        IsDirty = false;
        StatusMessage = "Settings saved.";
    }

    [RelayCommand(CanExecute = nameof(CanSaveOrRevert))]
    private void Revert()
    {
        Load();
        StatusMessage = "Reverted to the last saved values.";
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
