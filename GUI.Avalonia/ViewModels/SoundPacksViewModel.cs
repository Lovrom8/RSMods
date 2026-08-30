using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading.Tasks;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Core;

namespace RSMods.ViewModels;

/// <summary>
/// The SoundPacks screen. Like Profiles, this is action-oriented rather than a snapshot: the user unpacks
/// <c>audio.psarc</c>, replaces result-screen voice lines, and repacks — each action runs immediately
/// through the shared <see cref="SoundPackService"/>. The prompts and progress presentation live here (the
/// service is dialog-free); file-bound work runs off the UI thread behind the <see cref="IsBusy"/> gate.
/// </summary>
internal sealed partial class SoundPacksViewModel : ObservableObject
{
    private const string RepackReminder = "Don't forget to hit \"Repack Audio Psarc\" when you're done.";

    private readonly SoundPackService _service;
    private readonly IDialogService _dialogs;
    private bool _initialized;

    public IReadOnlyList<SoundPackVoiceLine> VoiceLines => _service.VoiceLines;
    public IReadOnlyList<string> ResultVoiceOvers => _service.ResultVoiceOvers;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(ShowUnpackPrompt))]
    private bool _isUnpacked;

    /// <summary>Inverse of <see cref="IsUnpacked"/>, for showing the initial "unpack" prompt.</summary>
    public bool ShowUnpackPrompt => !IsUnpacked;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(UnpackCommand))]
    [NotifyCanExecuteChangedFor(nameof(RepackCommand))]
    [NotifyCanExecuteChangedFor(nameof(RemoveUnpackedCommand))]
    [NotifyCanExecuteChangedFor(nameof(ImportCommand))]
    [NotifyCanExecuteChangedFor(nameof(ExportCommand))]
    [NotifyCanExecuteChangedFor(nameof(ResetCommand))]
    [NotifyCanExecuteChangedFor(nameof(ReplaceVoiceLineCommand))]
    [NotifyCanExecuteChangedFor(nameof(PlayResultVoiceOverCommand))]
    private bool _isBusy;

    /// <summary>True only while unpacking/repacking, so the determinate progress bar shows just for those.</summary>
    [ObservableProperty]
    private bool _isPacking;

    [ObservableProperty]
    private int _progressValue;

    [ObservableProperty]
    private string _progressOperation = string.Empty;

    [ObservableProperty]
    private string _statusMessage = string.Empty;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(PlayResultVoiceOverCommand))]
    private string? _selectedResultVo;

    public SoundPacksViewModel(SoundPackService service, IDialogService dialogs)
    {
        _service = service;
        _dialogs = dialogs;
    }

    /// <summary>Reads the current unpacked state on first navigation.</summary>
    public Task InitializeAsync()
    {
        if (_initialized)
            return Task.CompletedTask;
        _initialized = true;

        IsUnpacked = _service.IsUnpacked;
        return Task.CompletedTask;
    }

    private bool CanRun => !IsBusy;

    [RelayCommand(CanExecute = nameof(CanRun))]
    private async Task UnpackAsync()
    {
        await PerformUnpackAsync();
    }

    /// <summary>Runs the unpack flow (shared by the Unpack button and the auto-unpack step of Repack).</summary>
    private async Task<bool> PerformUnpackAsync()
    {
        if (!await _dialogs.ShowConfirmAsync(
                "For us to do song packs we need to unpack a huge game file. This will take up about 1.3 " +
                "gigabytes on your hard drive.\n\nPress OK if you are fine with that, or Cancel if you are not.",
                "Please Read!!!"))
        {
            return false;
        }

        IsBusy = true;
        IsPacking = true;
        ProgressValue = 0;
        StatusMessage = "Unpacking audio.psarc…";
        try
        {
            await _service.UnpackAsync(
                new Progress<int>(v => ProgressValue = v),
                new Progress<string>(o => ProgressOperation = o));

            IsUnpacked = _service.IsUnpacked;
            StatusMessage = "Unpacked.";
            await _dialogs.ShowInfoAsync(
                "Success! You may now mess around with custom sound packs.", "Unpacked Successfully");
            return true;
        }
        catch (Exception ex)
        {
            StatusMessage = "Unpack failed.";
            await _dialogs.ShowErrorAsync($"An error occurred while unpacking: {ex.Message}");
            return false;
        }
        finally
        {
            IsPacking = false;
            IsBusy = false;
        }
    }

    [RelayCommand(CanExecute = nameof(CanRun))]
    private async Task RepackAsync()
    {
        // The repack source is missing if the user never unpacked (or cleaned it up); recover by unpacking first.
        if (!_service.IsUnpackedPcPresent)
        {
            await _dialogs.ShowInfoAsync(
                "We detect no audio.psarc is decompiled. Give us some time to try to fix that.", "Missing Files");

            if (!await PerformUnpackAsync() || !_service.IsUnpackedPcPresent)
                return;
        }

        await _dialogs.ShowInfoAsync(
            "This will take a couple of minutes depending on your hard drive speed.\nFeel free to minimize " +
            "the app while it works its magic.", "Packing Audio");

        IsBusy = true;
        IsPacking = true;
        ProgressValue = 0;
        StatusMessage = "Repacking audio.psarc…";
        try
        {
            await _service.RepackAsync(
                new Progress<int>(v => ProgressValue = v),
                new Progress<string>(o => ProgressOperation = o));

            StatusMessage = "Repacked.";
            await _dialogs.ShowInfoAsync("Open your game, and see if the sound works!", "Success");
        }
        catch (Exception ex)
        {
            StatusMessage = "Repack failed.";
            await _dialogs.ShowErrorAsync($"An error occurred while packing: {ex.Message}");
        }
        finally
        {
            IsPacking = false;
            IsBusy = false;
        }
    }

    [RelayCommand(CanExecute = nameof(CanRun))]
    private async Task RemoveUnpackedAsync()
    {
        IsBusy = true;
        try
        {
            await Task.Run(_service.RemoveUnpacked);
            await _dialogs.ShowInfoAsync("Unpacked files removed successfully.", "Clean Up");
        }
        catch (Exception ex)
        {
            await _dialogs.ShowErrorAsync(
                $"Could not delete the folder. It might be in use by another program.\nError: {ex.Message}",
                "Delete Error");
        }
        finally
        {
            IsUnpacked = _service.IsUnpacked;
            IsBusy = false;
        }
    }

    [RelayCommand(CanExecute = nameof(CanRun))]
    private async Task ImportAsync()
    {
        IReadOnlyList<string> files = await _dialogs.PickFilesAsync(
            "Import soundpack", "RS2014 Soundpack", ["*.rs_soundpack"], allowMultiple: false);
        if (files.Count == 0)
            return;

        IsBusy = true;
        StatusMessage = "Importing soundpack…";
        try
        {
            await Task.Run(() => SoundPackService.Import(files[0]));
            StatusMessage = "Soundpack imported.";
            await _dialogs.ShowInfoAsync(RepackReminder, "Import complete");
        }
        catch (Exception ex)
        {
            StatusMessage = "Import failed.";
            await _dialogs.ShowErrorAsync($"An error occurred while importing: {ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    [RelayCommand(CanExecute = nameof(CanRun))]
    private async Task ExportAsync()
    {
        string desktop = Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
        string? path = await _dialogs.PickSaveFileAsync(
            "Export soundpack", "MySoundpack.rs_soundpack", "RS2014 Soundpack", "*.rs_soundpack", desktop);
        if (string.IsNullOrEmpty(path))
            return;

        IsBusy = true;
        StatusMessage = "Exporting soundpack…";
        try
        {
            await Task.Run(() => SoundPackService.Export(path));
            StatusMessage = "Soundpack exported.";
            await _dialogs.ShowInfoAsync(
                "You can now share your sound pack with your friends!\nTell them to open this menu and click " +
                "\"Import Soundpack\" button, and point to this file.", "Export complete");
        }
        catch (Exception ex)
        {
            StatusMessage = "Export failed.";
            await _dialogs.ShowErrorAsync($"An error occurred while exporting: {ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    [RelayCommand(CanExecute = nameof(CanRun))]
    private async Task ResetAsync()
    {
        IsBusy = true;
        StatusMessage = "Restoring the stock voice lines…";
        try
        {
            await Task.Run(SoundPackService.ResetToStock);
            StatusMessage = "Stock voice lines restored.";
            await _dialogs.ShowInfoAsync(RepackReminder, "Reset complete");
        }
        catch (Exception ex)
        {
            StatusMessage = "Reset failed.";
            await _dialogs.ShowErrorAsync($"An error occurred while resetting: {ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    [RelayCommand(CanExecute = nameof(CanRun))]
    private async Task ReplaceVoiceLineAsync(SoundPackVoiceLine? line)
    {
        if (line is null)
            return;

        if (!_service.IsUnpacked)
        {
            IsUnpacked = false;
            await _dialogs.ShowInfoAsync("Audio PSARC not unpacked. Please unpack it first.", "Missing Files");
            return;
        }

        IReadOnlyList<string> files = await _dialogs.PickFilesAsync(
            $"Replace “{line.Label}”", "Sound Files",
            ["*.mp3", "*.ogg", "*.wav", "*.wem"], allowMultiple: false);
        if (files.Count == 0)
            return;

        IsBusy = true;
        StatusMessage = $"Replacing “{line.Label}”…";
        try
        {
            await Task.Run(() => _service.ReplaceSound(line.Wem, files[0]));
            StatusMessage = "Sound replaced.";
            await _dialogs.ShowInfoAsync(
                $"Sound replaced successfully!\n{RepackReminder}", "Success");
        }
        catch (Exception ex)
        {
            StatusMessage = "Replace failed.";
            await _dialogs.ShowErrorAsync($"An error occurred: {ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    private bool CanPlayResultVoiceOver => !IsBusy && !string.IsNullOrEmpty(SelectedResultVo);

    [RelayCommand(CanExecute = nameof(CanPlayResultVoiceOver))]
    private void PlayResultVoiceOver()
    {
        if (SelectedResultVo is string vo)
            _service.PlayResultVoiceOver(vo);
    }

    [RelayCommand]
    private async Task DownloadWwiseAsync()
    {
        OpenUrl("https://ignition4.customsforge.com/cfsm/wwise/");
        await _dialogs.ShowInfoAsync(
            "After you download and install Wwise, make sure to open it at least once to ensure the EULA is " +
            "agreed to!", "Wwise EULA");
    }

    [RelayCommand]
    private void ReportBug() => OpenUrl("https://github.com/Lovrom8/RSMods/issues/new");

    private static void OpenUrl(string url) =>
        Process.Start(new ProcessStartInfo(url) { UseShellExecute = true });
}
