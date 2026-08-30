using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Core;
using RSMods.Data;

namespace RSMods.ViewModels;

/// <summary>
/// The default landing page: startup status plus the resolved Rocksmith install and save folders.
/// The shell populates it once startup resolution completes.
/// </summary>
internal sealed partial class StatusViewModel : ObservableObject
{
    private readonly IDialogService _dialogs;

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
    [NotifyCanExecuteChangedFor(nameof(SetSavePathCommand))]
    private bool _isReady;

    public string SavePathDisplay => SavePathAvailable
        ? SavePath!
        : "No save folder set. Profile Edits stays disabled until one is selected.";

    public string CoreAssembly => typeof(RsModsSettings).Assembly.GetName().Name ?? "GUI.Core";

    public StatusViewModel(IDialogService dialogs)
    {
        _dialogs = dialogs;
    }

    [RelayCommand(CanExecute = nameof(IsReady))]
    private async Task SetSavePathAsync()
    {
        // Force the prompt even if a previous run recorded that the user declined.
        string picked = await RSLocationResolver.ResolveSaveFolderAsync(_dialogs, forcePrompt: true);

        SavePath = picked;
        SavePathAvailable = !string.IsNullOrEmpty(picked);
        Constants.SaveBaseSettings();

        StatusMessage = SavePathAvailable
            ? "Save folder updated."
            : "Save folder selection cancelled.";
    }
}
