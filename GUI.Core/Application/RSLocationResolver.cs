#nullable enable
using System.Threading.Tasks;
using RSMods.Core;
using RSMods.Data;
using RSMods.Util;

namespace RSMods
{
    /// <summary>
    /// Resolves the Rocksmith 2014 install / save folders once at startup, prompting through
    /// <see cref="IDialogService"/> when auto-detection fails. This is the interactive counterpart to
    /// <see cref="GenUtil"/>'s pure detection: it owns every dialog and the shutdown-on-give-up flow that
    /// used to be baked into <c>GetRSDirectory</c> / <c>GetSaveFolder</c>.
    /// </summary>
    public static class RSLocationResolver
    {
        /// <summary>
        /// Resolves the Rocksmith install folder. Auto-detects first; if that fails, asks the user to point
        /// us at it. The install folder is mandatory, so giving up requests application shutdown.
        /// </summary>
        public static async Task<string> ResolveRSFolderAsync(IDialogService dialogs, IAppEnvironment environment)
        {
            string detected = GenUtil.GetRSDirectory();

            if (!string.IsNullOrEmpty(detected) && detected.IsRSFolder())
                return Constants.RSFolder = GenUtil.NormalizePath(detected);

            await dialogs.ShowErrorAsync(
                "It looks like your current Rocksmith2014 install folder cannot be found. Please tell us where it is located!",
                "Error: Rocksmith Location Not Found");

            string picked = await PickRSFolderAsync(dialogs, startPath: null);
            if (string.IsNullOrEmpty(picked))
            {
                await dialogs.ShowErrorAsync(
                    "We cannot detect where you have Rocksmith located. Please try reinstalling your game on Steam.",
                    "Error: Rocksmith Location Not Found");
                environment.RequestShutdown();
                return string.Empty;
            }

            return Constants.RSFolder = GenUtil.NormalizePath(picked);
        }

        /// <summary>
        /// Resolves the Rocksmith save folder. Auto-detects first; if that fails, asks the user. Unlike the
        /// install folder, the save folder is optional: cancelling records the decline (Profile Edits stays
        /// disabled) instead of shutting down.
        /// </summary>
        public static async Task<string> ResolveSaveFolderAsync(IDialogService dialogs)
        {
            // GetSaveFolder also refreshes Constants.SavePathDeclined from the persisted settings.
            string detected = GenUtil.GetSaveFolder();

            if (!string.IsNullOrEmpty(detected) && detected.IsSavePath())
                return UseSaveFolder(detected);

            if (Constants.SavePathDeclined)
                return string.Empty;

            // Registry-based detection as a last automatic attempt before bothering the user.
            string fromRegistry = GenUtil.GetSaveDirectory(true);
            if (!string.IsNullOrEmpty(fromRegistry) && fromRegistry.IsSavePath())
                return UseSaveFolder(fromRegistry);

            string picked = await PromptForSaveFolderAsync(dialogs);
            if (string.IsNullOrEmpty(picked))
            {
                Constants.SavePathDeclined = true;
                return string.Empty;
            }

            return UseSaveFolder(picked);
        }

        /// <summary>
        /// Lets the user pick a different install folder (the Home tab's Change button), starting from the current
        /// one. Only a folder with Rocksmith2014.exe and cache.psarc is accepted. Returns the picked folder, or null
        /// when they cancel. It doesn't switch to it: settings from the current folder are already loaded, so the
        /// caller decides when to.
        /// </summary>
        public static async Task<string?> ChooseRSFolderAsync(IDialogService dialogs)
        {
            string picked = await PickRSFolderAsync(dialogs, Constants.RSFolder);
            return string.IsNullOrEmpty(picked) ? null : GenUtil.NormalizePath(picked);
        }

        /// <summary>
        /// Lets the user pick a different save folder (the Home tab's Change button), starting from the current
        /// one. Returns the new folder, or null when they cancel, which keeps the current one.
        /// </summary>
        public static async Task<string?> ChangeSaveFolderAsync(IDialogService dialogs)
        {
            string picked = await PickSaveFolderAsync(dialogs, Constants.SavePath);
            return string.IsNullOrEmpty(picked) ? null : UseSaveFolder(picked);
        }

        private static string UseSaveFolder(string folder)
        {
            Constants.SavePath = GenUtil.NormalizePath(folder);
            Constants.SavePathDeclined = false;
            return Constants.SavePath;
        }

        private static async Task<string> PickRSFolderAsync(IDialogService dialogs, string? startPath)
        {
            while (true)
            {
                string? picked = await dialogs.PickFolderAsync("Select your Rocksmith 2014 installation folder", startPath);

                if (string.IsNullOrEmpty(picked)) // user canceled
                    return string.Empty;

                if (picked.IsRSFolder())
                    return picked!;

                await dialogs.ShowErrorAsync(
                    "We cannot verify your installation of Rocksmith 2014. The folder you selected doesn't contain both Rocksmith2014.exe and cache.psarc, which Rocksmith 2014 needs to boot. Please select the folder that has them.",
                    "Invalid Rocksmith Folder");
            }
        }

        private static async Task<string> PromptForSaveFolderAsync(IDialogService dialogs)
        {
            await dialogs.ShowInfoAsync(
                "It looks like your Rocksmith 2014 save folder cannot be found. Please tell us where it is located!\n" +
                "This can be found in your Steam install folder.\n<Path To Steam Install>/userdata/#/221680/remote",
                "SavePath Not Found");

            return await PickSaveFolderAsync(dialogs, startPath: null);
        }

        private static async Task<string> PickSaveFolderAsync(IDialogService dialogs, string? startPath)
        {
            while (true)
            {
                string? picked = await dialogs.PickFolderAsync("Select your Rocksmith 2014 save folder", startPath);

                if (string.IsNullOrEmpty(picked)) // user canceled — allowed, disables Profile Edits
                    return string.Empty;

                if (picked.IsSavePath())
                    return picked!;

                bool retry = await dialogs.ShowConfirmAsync(
                    "The save folder you selected does not appear to be correct (no LocalProfiles.json).\n" +
                    "It should follow the format: <Where Steam Is Installed>/userdata/#####/221680/remote\n" +
                    "Choosing \"No\" will prevent the usage of the \"Profile Edits\" tab.\nTry again?",
                    "Invalid Save Path");

                if (!retry)
                    return string.Empty;
            }
        }
    }
}
