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
            {
                Constants.RSFolder = detected;
                return detected;
            }

            await dialogs.ShowErrorAsync(
                "It looks like your current Rocksmith2014 install folder cannot be found. Please tell us where it is located!",
                "Error: Rocksmith Location Not Found");

            string picked = await PromptForRSFolderAsync(dialogs);
            if (string.IsNullOrEmpty(picked))
            {
                await dialogs.ShowErrorAsync(
                    "We cannot detect where you have Rocksmith located. Please try reinstalling your game on Steam.",
                    "Error: Rocksmith Location Not Found");
                environment.RequestShutdown();
                return string.Empty;
            }

            Constants.RSFolder = picked;
            return picked;
        }

        /// <summary>
        /// Resolves the Rocksmith save folder. Auto-detects first; if that fails, asks the user. Unlike the
        /// install folder, the save folder is optional — cancelling records the decline (Profile Edits stays
        /// disabled) instead of shutting down. Pass <paramref name="forcePrompt"/> to prompt even when the
        /// user previously declined (the "Set Save Path" button).
        /// </summary>
        public static async Task<string> ResolveSaveFolderAsync(IDialogService dialogs, bool forcePrompt = false)
        {
            // GetSaveFolder also refreshes Constants.SavePathDeclined from the persisted settings.
            string detected = GenUtil.GetSaveFolder();

            if (!string.IsNullOrEmpty(detected) && detected.IsSavePath())
            {
                Constants.SavePath = detected;
                Constants.SavePathDeclined = false;
                return detected;
            }

            if (Constants.SavePathDeclined && !forcePrompt)
                return string.Empty;

            // Registry-based detection as a last automatic attempt before bothering the user.
            string fromRegistry = GenUtil.GetSaveDirectory(true);
            if (!string.IsNullOrEmpty(fromRegistry) && fromRegistry.IsSavePath())
            {
                Constants.SavePath = fromRegistry;
                Constants.SavePathDeclined = false;
                return fromRegistry;
            }

            string picked = await PromptForSaveFolderAsync(dialogs);
            if (string.IsNullOrEmpty(picked))
            {
                Constants.SavePathDeclined = true;
                return string.Empty;
            }

            Constants.SavePath = picked;
            Constants.SavePathDeclined = false;
            return picked;
        }

        private static async Task<string> PromptForRSFolderAsync(IDialogService dialogs)
        {
            while (true)
            {
                string? picked = await dialogs.PickFolderAsync("Select your Rocksmith 2014 installation folder");

                if (string.IsNullOrEmpty(picked)) // user cancelled
                    return string.Empty;

                if (picked.IsRSFolder())
                    return picked!;

                await dialogs.ShowErrorAsync(
                    "We cannot verify your installation of Rocksmith 2014. The folder you selected doesn't contain a cache.psarc, which is REQUIRED for Rocksmith 2014 to boot. Please select the correct folder.",
                    "Invalid Rocksmith Folder");
            }
        }

        private static async Task<string> PromptForSaveFolderAsync(IDialogService dialogs)
        {
            await dialogs.ShowInfoAsync(
                "It looks like your Rocksmith 2014 save folder cannot be found. Please tell us where it is located!\n" +
                "This can be found in your Steam install folder.\n<Path To Steam Install>/userdata/#/221680/remote",
                "SavePath Not Found");

            while (true)
            {
                string? picked = await dialogs.PickFolderAsync("Select your Rocksmith 2014 save folder");

                if (string.IsNullOrEmpty(picked)) // user cancelled — allowed, disables Profile Edits
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
