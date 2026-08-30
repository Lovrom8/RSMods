using System.ComponentModel;
using System.Diagnostics;
using Avalonia.Threading;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RS2014_Mod_Installer.Core;
using RS2014_Mod_Installer.Payload;

namespace RS2014_Mod_Installer.ViewModels;

internal sealed partial class InstallerViewModel(EmbeddedInstallerResources resources) : ObservableObject
{
    private bool _initialized;

    public Func<Task<string?>>? BrowseForRocksmithFolderAsync { get; set; }

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(InstallCommand))]
    private string _rocksmithFolder = string.Empty;

    [ObservableProperty]
    private string _statusMessage = "Locating Rocksmith 2014…";

    [ObservableProperty]
    private string _errorMessage = string.Empty;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(InstallCommand))]
    [NotifyCanExecuteChangedFor(nameof(BrowseCommand))]
    private bool _isBusy;

    [ObservableProperty]
    private bool _createDesktopShortcut = true;

    [ObservableProperty]
    private bool _launchConfigurator = true;

    [ObservableProperty]
    private bool _installationComplete;

    public void Initialize()
    {
        if (_initialized)
            return;

        _initialized = true;
        RocksmithLocationResult result = RocksmithLocator.Locate();
        RocksmithFolder = result.Path;
        StatusMessage = result.Status switch
        {
            RocksmithLocationStatus.Found => "Rocksmith 2014 was detected. Ready to install.",
            RocksmithLocationStatus.CachePsarcMissing => "A possible Steam folder was found, but cache.psarc is missing. Choose the active Rocksmith 2014 folder.",
            _ => "Rocksmith 2014 was not detected automatically. Choose its installation folder."
        };
    }

    private bool CanBrowse() => !IsBusy;

    [RelayCommand(CanExecute = nameof(CanBrowse))]
    private async Task BrowseAsync()
    {
        if (BrowseForRocksmithFolderAsync is null)
            return;

        string? selected = await BrowseForRocksmithFolderAsync();
        if (string.IsNullOrWhiteSpace(selected))
            return;

        RocksmithFolder = selected;
        ErrorMessage = string.Empty;
        StatusMessage = RocksmithLocator.IsRocksmithFolder(selected)
            ? "Rocksmith 2014 folder selected. Ready to install."
            : "That folder does not look like a complete Rocksmith 2014 installation.";
    }

    private bool CanInstall() => !IsBusy && !string.IsNullOrWhiteSpace(RocksmithFolder);

    [RelayCommand(CanExecute = nameof(CanInstall))]
    private async Task InstallAsync()
    {
        ResetInstallationState();

        if (!TryGetRocksmithFolder(out string rocksmithFolder))
            return;

        IsBusy = true;
        try
        {
            StatusMessage = "Checking the Rocksmith installation…";

            CompatibilityResult compatibility = await Task.Run(() => RocksmithCompatibility.Evaluate(rocksmithFolder));
            if (!compatibility.IsCompatible)
            {
                ShowCompatibilityFailure(compatibility);
                return;
            }

            await Task.Run(() => InstallPayloads(rocksmithFolder));

            string configuratorPath = ConfiguratorPaths.ExecutablePath(rocksmithFolder);
            ShowInstallationSuccess();
            RunPostInstallActions(configuratorPath);
        }
        catch (Exception ex)
        {
            ShowInstallationFailure(ex);
        }
        finally
        {
            IsBusy = false;
        }
    }

    private void ResetInstallationState()
    {
        ErrorMessage = string.Empty;
        InstallationComplete = false;
    }

    private bool TryGetRocksmithFolder(out string rocksmithFolder)
    {
        rocksmithFolder = string.Empty;
        if (!RocksmithLocator.IsRocksmithFolder(RocksmithFolder))
        {
            ErrorMessage = "Choose the Rocksmith 2014 folder containing cache.psarc and a non-empty dlc folder.";
            StatusMessage = "Installation was not started.";
            return false;
        }

        try
        {
            rocksmithFolder = Path.GetFullPath(RocksmithFolder);
            return true;
        }
        catch (Exception ex) when (ex is ArgumentException or IOException or NotSupportedException)
        {
            ErrorMessage = "The selected Rocksmith folder path is invalid: " + ex.Message;
            StatusMessage = "Installation was not started.";
            return false;
        }
    }

    private void ShowCompatibilityFailure(CompatibilityResult compatibility)
    {
        ErrorMessage = compatibility.ExecutableUnreadable
            ? "Rocksmith2014.exe is in use. Close Rocksmith, then try again."
            : "This installer supports the current Steam versions of Rocksmith 2014 only.\n" +
              string.Join(Environment.NewLine, compatibility.Reasons);
        StatusMessage = "Installation was not started.";
    }

    private void ShowInstallationSuccess()
    {
        InstallationComplete = true;
        StatusMessage = "RSMods was installed successfully.";
    }

    private void RunPostInstallActions(string configuratorPath)
    {
        var warnings = new List<string>();

        if (CreateDesktopShortcut)
            TryCreateDesktopShortcut(configuratorPath, warnings);

        if (LaunchConfigurator)
            TryLaunchConfigurator(configuratorPath, warnings);

        if (warnings.Count > 0)
            ErrorMessage = "RSMods was installed, but:\n" + string.Join(Environment.NewLine, warnings);
    }

    private void TryCreateDesktopShortcut(string configuratorPath, List<string> warnings)
    {
        try
        {
            StatusMessage = "Creating the desktop shortcut…";
            string desktop = Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
            DesktopShortcut.WriteUrlShortcut(Path.Combine(desktop, "RSMods.url"), configuratorPath);
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
        {
            warnings.Add("the desktop shortcut could not be created: " + ex.Message);
        }
        finally
        {
            StatusMessage = "RSMods was installed successfully.";
        }
    }

    private static void TryLaunchConfigurator(string configuratorPath, List<string> warnings)
    {
        try
        {
            Process.Start(new ProcessStartInfo(configuratorPath) { UseShellExecute = true });
        }
        catch (Exception ex) when (ex is InvalidOperationException or Win32Exception)
        {
            warnings.Add("the configurator could not be opened: " + ex.Message);
        }
    }

    private void ShowInstallationFailure(Exception exception)
    {
        ErrorMessage = exception switch
        {
            PayloadInstallException => exception.Message,
            UnauthorizedAccessException =>
                "The installer could not write to the Rocksmith folder. Run it as administrator or choose a writable installation.\n" + exception.Message,
            IOException => exception.Message,
            _ => "Unexpected installer error: " + exception.Message
        };

        StatusMessage = exception is PayloadInstallException
            ? "Installation failed. Fix the problem and try again."
            : "Installation failed. You can try again.";
    }

    private void InstallPayloads(string rocksmithFolder)
    {
        ReportProgress("Installing the native mod…");
        NativeModInstallStatus nativeStatus = NativeModInstaller.Install(rocksmithFolder, resources.ReadNativeDll(), resources.ReadNativePdb());

        if (nativeStatus == NativeModInstallStatus.GameRunning)
            throw new IOException("Rocksmith is open and the native mod files cannot be replaced. Close the game and try again.");

        PayloadManifest manifest = resources.ReadManifest();
        using Stream payload = resources.OpenPayload();

        string folder = ConfiguratorPaths.RsModsFolder(rocksmithFolder);
        ConfiguratorPayloadInstaller.Install(folder, payload, manifest, ReportProgress);
    }

    private void ReportProgress(string message) => Dispatcher.UIThread.Post(() => StatusMessage = message);

    partial void OnRocksmithFolderChanged(string value)
    {
        InstallationComplete = false;
        ErrorMessage = string.Empty;
    }
}
