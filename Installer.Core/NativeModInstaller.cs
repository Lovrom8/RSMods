using System;
using System.IO;

namespace RS2014_Mod_Installer.Core
{
    public enum NativeModInstallStatus
    {
        Success,
        GameRunning  // A file could not be written because Rocksmith is open and holds a lock.
    }

    // Deploys the native mod DLL (xinput1_3.dll + .pdb) into the Rocksmith folder. The DLL bytes are
    // supplied by the caller (embedded in the installer resources) so this stays UI- and
    // resource-free and testable. Was DLLStuff.InjectDLL.
    public static class NativeModInstaller
    {
        // Other IO failures (a full disk, a read-only folder) aren't the game holding the file, so they're thrown for the
        // caller to report as they are.
        public static NativeModInstallStatus Install(string rocksmithLocation, byte[] xinput1_3Dll, byte[] xinput1_3Pdb)
        {
            string dll = Path.Combine(rocksmithLocation, "xinput1_3.dll");

            try
            {
                // The game holds the DLL from launch until it exits. Asked first, so neither file changes while it runs.
                if (IsLocked(dll))
                    return NativeModInstallStatus.GameRunning;

                ReplaceFile(dll, xinput1_3Dll);
                ReplaceFile(Path.Combine(rocksmithLocation, "xinput1_3.pdb"), xinput1_3Pdb);
            }
            catch (IOException ex) when (IsSharingViolation(ex))
            {
                return NativeModInstallStatus.GameRunning;
            }

            // A large D3DX9_42.dll is a leftover injector from older installs; remove it so ours loads.
            string legacyProxy = Path.Combine(rocksmithLocation, "D3DX9_42.dll");
            if (File.Exists(legacyProxy) && new FileInfo(legacyProxy).Length >= 300000)
                File.Delete(legacyProxy);

            return NativeModInstallStatus.Success;
        }

        // Written beside the target and swapped in, so a write that fails partway never leaves a truncated DLL that would
        // stop the game from starting.
        private static void ReplaceFile(string path, byte[] contents)
        {
            string temporary = path + ".new";
            try
            {
                File.WriteAllBytes(temporary, contents);
                File.Move(temporary, path, overwrite: true);
            }
            finally
            {
                try { File.Delete(temporary); } catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { }
            }
        }

        private static bool IsLocked(string path)
        {
            if (!File.Exists(path))
                return false;

            try
            {
                using (new FileStream(path, FileMode.Open, FileAccess.ReadWrite, FileShare.None)) { }
                return false;
            }
            catch (IOException ex) when (IsSharingViolation(ex))
            {
                return true;
            }
        }

        // ERROR_SHARING_VIOLATION and ERROR_LOCK_VIOLATION: another process has the file open.
        private static bool IsSharingViolation(IOException ex) => (ex.HResult & 0xFFFF) is 32 or 33;
    }
}
