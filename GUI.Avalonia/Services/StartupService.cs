using System.Collections.Generic;
using RSMods.Core;
using RSMods.Core.Settings;
using RSMods.Data;

namespace RSMods.Services;

/// <summary>
/// Outcome of the one-time startup resolution: the resolved Rocksmith folders and any INI
/// validation warnings raised while loading settings. <see cref="Completed"/> is false when the
/// user could not point us at a Rocksmith install and a shutdown was requested.
/// </summary>
internal sealed class StartupResult
{
    public bool Completed { get; init; }
    public string RocksmithFolder { get; init; } = string.Empty;
    public string SavePath { get; init; } = string.Empty;
    public bool SavePathAvailable => !string.IsNullOrEmpty(SavePath);
    /// <summary>The save folder was backed up this session, which profile edits need.</summary>
    public bool ProfilesBackedUp { get; init; }
    public IReadOnlyList<IniValidationWarning> Warnings { get; init; } = [];
}

/// <summary>
/// Resolve the Rocksmith install/save folders through <see cref="RSLocationResolver"/>,
/// persist them to the shared <c>GUI_Settings.ini</c>, and load <see cref="RsModsSettings"/> so
/// settings-dependent screens have data to bind.
/// </summary>
internal sealed class StartupService(IDialogService dialogs, IAppEnvironment environment, IManifestService manifest)
{
    public async Task<StartupResult> RunAsync()
    {
        // The install folder is mandatory; an empty result means the resolver already requested shutdown.
        string rsFolder = await RSLocationResolver.ResolveRSFolderAsync(dialogs, environment);
        if (string.IsNullOrEmpty(rsFolder))
            return new StartupResult { Completed = false };

        // The save folder is optional; an empty result just disables profile-dependent features.
        string savePath = await RSLocationResolver.ResolveSaveFolderAsync(dialogs);

        // Persist the resolved paths so the next launch can skip detection.
        Constants.SaveBaseSettings();

        var warnings = new List<IniValidationWarning>();
        void Collect(IniValidationWarning warning) => warnings.Add(warning);

        // Subscribe before loading so warnings raised during the initial parse are captured. The
        // handler is stored until LoadSettingsFromINI wires up the underlying IniManager.
        RsModsSettings.ValidationWarning += Collect;
        try
        {
            RsModsSettings.LoadSettingsFromINI(manifest);
        }
        finally
        {
            RsModsSettings.ValidationWarning -= Collect;
        }

        bool backedUp = await BackUpProfilesOnStartupAsync(rsFolder, savePath);

        return new StartupResult
        {
            Completed = true,
            RocksmithFolder = rsFolder,
            SavePath = savePath,
            ProfilesBackedUp = backedUp,
            Warnings = warnings,
        };
    }

    /// <summary>
    /// Backs up the profiles into "Before GUI" before any screen can edit them, so a profile changed here while the
    /// game is closed always has a backup. The DLL makes the in-game backups; both are governed by BackupProfile.
    /// As the old GUI did, the "Before GUI" backups are trimmed to NumberOfBackups on every open, with BackupProfile
    /// off too, so turning backups off doesn't leave an old pile behind. True when a backup was taken.
    /// </summary>
    private async Task<bool> BackUpProfilesOnStartupAsync(string rsFolder, string savePath)
    {
        if (string.IsNullOrEmpty(savePath))
            return false;

        if (RsModsSettings.GUISettings.BackupProfile)
            return await BackUpProfilesAsync(rsFolder, savePath);

        try
        {
            int keep = RsModsSettings.GUISettings.NumberOfBackups;
            await Task.Run(() => ProfileBackupService.DeleteOldBackups(rsFolder, savePath, keep));
        }
        catch (Exception ex)
        {
            await dialogs.ShowErrorAsync($"Could not back up your profiles:\n{ex.Message}");
        }

        return false;
    }

    /// <summary>
    /// Backs up the profiles in <paramref name="savePath"/> into "Before GUI" and trims those backups to
    /// NumberOfBackups. False, after telling the user, when the backup failed; profile edits then stay locked.
    /// </summary>
    public async Task<bool> BackUpProfilesAsync(string rsFolder, string savePath)
    {
        try
        {
            int keep = RsModsSettings.GUISettings.NumberOfBackups;
            await Task.Run(() =>
            {
                ProfileBackupService.CreateBackup(savePath, rsFolder);
                ProfileBackupService.DeleteOldBackups(rsFolder, savePath, keep);
            });
            return true;
        }
        catch (Exception ex)
        {
            await dialogs.ShowErrorAsync($"Could not back up your profiles, so editing them stays locked:\n{ex.Message}");
            return false;
        }
    }
}
