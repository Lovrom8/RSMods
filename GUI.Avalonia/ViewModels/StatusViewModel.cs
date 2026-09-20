using System;
using System.Diagnostics;
using System.Reflection;
using System.Threading.Tasks;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Core;
using RSMods.Core.Update;
using RSMods.Data;

namespace RSMods.ViewModels;

/// <summary>
/// The default landing page: startup status, the resolved Rocksmith install and save folders,
/// and the latest GitHub release notes.
/// </summary>
internal sealed partial class StatusViewModel : ObservableObject
{
    private readonly IDialogService _dialogs;
    private readonly IGitHubReleaseService _releases;

    public StatusViewModel(IDialogService dialogs, IGitHubReleaseService releases)
    {
        _dialogs = dialogs;
        _releases = releases;

        Version? running = Assembly.GetExecutingAssembly().GetName().Version;
        CurrentVersion = running is null ? null : running.ToString();

        // Fetch once at launch; failures degrade to a quiet retry link rather than blocking the page.
        _ = LoadLatestReleaseAsync();
    }

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

    public string SavePathDisplay => SavePathAvailable ? SavePath! : "No save folder set. Profile Edits stays disabled until one is selected.";

    public string CoreAssembly => typeof(RsModsSettings).Assembly.GetName().Name ?? "GUI.Core";

    // ---- Latest release ----

    [ObservableProperty]
    private bool _isLoadingRelease;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(IsUpToDate))]
    private bool _hasRelease;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(IsUpToDate))]
    private bool _updateAvailable;

    /// <summary>The running build's version, e.g. "1.2.8.4".</summary>
    [ObservableProperty]
    private string? _currentVersion;

    /// <summary>True once the release is known and it is not newer than the running build.</summary>
    public bool IsUpToDate => HasRelease && !UpdateAvailable;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(HasReleaseError))]
    private string? _releaseError;

    public bool HasReleaseError => !string.IsNullOrEmpty(ReleaseError);

    [ObservableProperty]
    private string? _releaseTitle;

    [ObservableProperty]
    private string? _releaseVersion;

    [ObservableProperty]
    private string? _releasePublished;

    [ObservableProperty]
    private string? _releaseNotes;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(OpenReleaseCommand))]
    private string? _releaseUrl;

    [RelayCommand(CanExecute = nameof(IsReady))]
    private async Task SetSavePathAsync()
    {
        // Force the prompt even if a previous run recorded that the user declined.
        string picked = await RSLocationResolver.ResolveSaveFolderAsync(_dialogs, forcePrompt: true);

        SavePath = picked;
        SavePathAvailable = !string.IsNullOrEmpty(picked);
        Constants.SaveBaseSettings();

        StatusMessage = SavePathAvailable ? "Save folder updated." : "Save folder selection cancelled.";
    }

    [RelayCommand]
    private async Task LoadLatestReleaseAsync()
    {
        if (IsLoadingRelease)
            return;

        IsLoadingRelease = true;
        ReleaseError = null;
        try
        {
            ReleaseInfo release = await _releases.GetLatestReleaseAsync();

            ReleaseTitle = release.Title;
            ReleaseVersion = release.TagName;
            ReleasePublished = release.PublishedAt?.ToLocalTime().ToString("d MMM yyyy");
            ReleaseNotes = release.Notes;
            ReleaseUrl = release.HtmlUrl;
            HasRelease = true;

            // The badge shows only when the published release is strictly newer than this build; if the
            // tag has no parseable version we stay quiet rather than nagging.
            Version? running = Assembly.GetExecutingAssembly().GetName().Version;
            UpdateAvailable =
                running is not null
                && ReleaseVersioning.TryExtractVersion(release.TagName, out Version latest)
                && latest > running;
        }
        catch (Exception)
        {
            // Offline, rate-limited, or a schema change: keep the page usable, offer a retry.
            HasRelease = false;
            ReleaseError = "Couldn't reach GitHub to load the latest release.";
        }
        finally
        {
            IsLoadingRelease = false;
        }
    }

    private bool CanOpenRelease => !string.IsNullOrEmpty(ReleaseUrl);

    [RelayCommand(CanExecute = nameof(CanOpenRelease))]
    private void OpenRelease() =>
        Process.Start(new ProcessStartInfo(ReleaseUrl!) { UseShellExecute = true });
}
