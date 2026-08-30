using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Core;
using RSMods.Services;

namespace RSMods.ViewModels;

internal sealed partial class MainWindowViewModel : ObservableObject
{
    private readonly IDialogService _dialogs;
    private readonly StartupService _startup;
    private bool _initialized;

    [ObservableProperty]
    private string _statusMessage = "Resolving your Rocksmith 2014 install…";

    [ObservableProperty]
    private string? _rocksmithFolder;

    [ObservableProperty]
    private string? _savePath;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(SavePathDisplay))]
    private bool _savePathAvailable;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(SectionsEnabled))]
    private bool _isReady;

    public bool SectionsEnabled => IsReady;

    public string SavePathDisplay => SavePathAvailable
        ? SavePath!
        : "No save folder set. Profile Edits will stay disabled until one is selected.";

    public string CoreAssembly => typeof(RsModsSettings).Assembly.GetName().Name ?? "GUI.Core";

    public MainWindowViewModel(IDialogService dialogs, StartupService startup)
    {
        _dialogs = dialogs;
        _startup = startup;
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
            // The resolver already requested shutdown; leave the shell in its resolving state.
            StatusMessage = "Rocksmith 2014 could not be located. Closing…";
            return;
        }

        RocksmithFolder = result.RocksmithFolder;
        SavePath = result.SavePath;
        SavePathAvailable = result.SavePathAvailable;
        IsReady = true;
        StatusMessage = "Startup complete. Settings loaded and ready to configure.";

        await ReportWarningsAsync(result);
    }

    private async Task ReportWarningsAsync(StartupResult result)
    {
        if (result.Warnings.Count == 0)
            return;

        int shown = System.Math.Min(result.Warnings.Count, 10);
        var lines = new System.Text.StringBuilder();
        lines.AppendLine(
            $"{result.Warnings.Count} invalid setting(s) in RSMods.ini were reset to their defaults:");
        lines.AppendLine();

        for (int i = 0; i < shown; i++)
        {
            var warning = result.Warnings[i];
            lines.AppendLine($"• {warning.Section} {warning.Key}: \"{warning.RawValue}\" → {warning.DefaultValue}");
        }

        if (result.Warnings.Count > shown)
            lines.AppendLine($"… and {result.Warnings.Count - shown} more.");

        await _dialogs.ShowInfoAsync(lines.ToString().TrimEnd(), "Some settings were reset");
    }

    [RelayCommand]
    private async Task SetSavePathAsync()
    {
        // Force the prompt even if a previous run recorded that the user declined.
        string picked = await RSLocationResolver.ResolveSaveFolderAsync(_dialogs, forcePrompt: true);

        SavePath = picked;
        SavePathAvailable = !string.IsNullOrEmpty(picked);
        Data.Constants.SaveBaseSettings();

        StatusMessage = SavePathAvailable
            ? "Save folder updated."
            : "Save folder selection cancelled.";
    }
}
