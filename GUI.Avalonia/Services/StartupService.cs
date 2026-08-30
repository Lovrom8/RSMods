using System.Collections.Generic;
using RSMods.Core;
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
    public IReadOnlyList<IniValidationWarning> Warnings { get; init; } = [];
}

/// <summary>
/// Resolve the Rocksmith install/save folders through <see cref="RSLocationResolver"/>,
/// persist them to the shared <c>GUI_Settings.ini</c>, and load <see cref="RsModsSettings"/> so
/// settings-dependent screens have data to bind.
/// </summary>
internal sealed class StartupService(IDialogService dialogs, IAppEnvironment environment)
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
            RsModsSettings.LoadSettingsFromINI();
        }
        finally
        {
            RsModsSettings.ValidationWarning -= Collect;
        }

        return new StartupResult
        {
            Completed = true,
            RocksmithFolder = rsFolder,
            SavePath = savePath,
            Warnings = warnings,
        };
    }
}
