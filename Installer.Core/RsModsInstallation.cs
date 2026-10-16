using System;
using System.IO;
using RS2014_Mod_Installer.Payload;

namespace RS2014_Mod_Installer.Core
{
    // Installs the native mod and the configurator as one: if any step fails, the Rocksmith folder is left as it was,
    // so a retry still sees the same existing DLL (and still sets a custom build's manifest aside).
    public static class RsModsInstallation
    {
        public static void Install(string rocksmithLocation, ExistingNativeMod existing, bool replaceNativeMod,
                                   byte[] xinput1_3Dll, byte[] xinput1_3Pdb, Stream payloadZip, PayloadManifest manifest,
                                   Action<string> progress = null)
        {
            ArgumentNullException.ThrowIfNull(existing);
            string rsModsFolder = ConfiguratorPaths.RsModsFolder(rocksmithLocation);

            // Before the native mod: an open configurator or a bad payload would otherwise fail after a new DLL was in.
            ConfiguratorPayloadInstaller.ThrowIfConfiguratorRunning(rsModsFolder, manifest);
            using StagedConfiguratorPayload staged = ConfiguratorPayloadInstaller.Stage(rsModsFolder, payloadZip, manifest, progress);

            if (!replaceNativeMod)
            {
                staged.Deploy(progress);
                return;
            }

            progress?.Invoke("Installing the native mod…");
            if (NativeModInstaller.Install(rocksmithLocation, xinput1_3Dll, xinput1_3Pdb, out NativeModBackup nativeBackup) == NativeModInstallStatus.GameRunning)
                throw new IOException("Rocksmith is open and the native mod files cannot be replaced. Close the game and try again.");

            bool manifestSetAside = false;
            try
            {
                // A custom build's manifest lists mods the official DLL doesn't have.
                if (existing.Kind == ExistingNativeModKind.CustomRsMods)
                    manifestSetAside = ExistingNativeModInspector.SetAsideCustomManifest(rsModsFolder);

                staged.Deploy(progress);
            }
            catch (Exception ex) when (ex is PayloadInstallException or IOException or UnauthorizedAccessException)
            {
                bool restored = nativeBackup.Restore() & (!manifestSetAside || TryRestoreCustomManifest(rsModsFolder));
                if (!restored)
                {
                    throw new IOException(
                        "Installing RSMods failed and the previous native mod couldn't be fully put back. " +
                        "The previous files are beside xinput1_3.dll in " + rocksmithLocation + ": " + ex.Message, ex);
                }

                throw;
            }

            nativeBackup.Commit();
        }

        private static bool TryRestoreCustomManifest(string rsModsFolder)
        {
            try
            {
                ExistingNativeModInspector.RestoreCustomManifest(rsModsFolder);
                return true;
            }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
            {
                return false;
            }
        }
    }
}
