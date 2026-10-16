using System;
using System.Collections.Generic;
using System.IO;

namespace RS2014_Mod_Installer.Core
{
    public enum NativeModInstallStatus
    {
        Success,
        GameRunning  // A file could not be written because Rocksmith is open and holds a lock.
    }

    // The native files an install replaced, kept beside them until the rest of the install has gone in.
    public sealed class NativeModBackup
    {
        private const string BackupSuffix = ".rsmods-previous";

        private readonly string _rocksmithLocation;
        private readonly List<(string Path, bool Existed)> _files = [];

        internal NativeModBackup(string rocksmithLocation) => _rocksmithLocation = rocksmithLocation;

        // Copied, not moved, so the original stays in place until its replacement is swapped in.
        internal void Replace(string path, string replacement)
        {
            bool existed = File.Exists(path);
            if (existed)
                File.Copy(path, path + BackupSuffix, overwrite: true);

            try
            {
                File.Move(replacement, path, overwrite: true);
            }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
            {
                // The original is still in place, so its copy isn't needed.
                if (existed)
                    TryDelete(path + BackupSuffix);
                throw;
            }

            _files.Add((path, existed));
        }

        // Puts every replaced file back, newest first. False if any of it failed, which leaves that file's backup beside it.
        public bool Restore()
        {
            bool complete = true;
            for (int i = _files.Count - 1; i >= 0; i--)
            {
                (string path, bool existed) = _files[i];
                try
                {
                    if (existed)
                        File.Move(path + BackupSuffix, path, overwrite: true);
                    else
                        File.Delete(path);
                }
                catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
                {
                    complete = false;
                }
            }

            _files.Clear();
            return complete;
        }

        // The install went in: drop the backups and the legacy injector. Best-effort, nothing here can undo the install.
        public void Commit()
        {
            foreach ((string path, bool existed) in _files)
            {
                if (existed)
                    TryDelete(path + BackupSuffix);
            }

            _files.Clear();

            // A large D3DX9_42.dll is a leftover injector from older installs; remove it so ours loads.
            string legacyProxy = Path.Combine(_rocksmithLocation, "D3DX9_42.dll");
            if (File.Exists(legacyProxy) && new FileInfo(legacyProxy).Length >= 300000)
                TryDelete(legacyProxy);
        }

        private static void TryDelete(string path)
        {
            try { File.Delete(path); } catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { }
        }
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
            NativeModInstallStatus status = Install(rocksmithLocation, xinput1_3Dll, xinput1_3Pdb, out NativeModBackup backup);
            backup?.Commit();
            return status;
        }

        // As above, but keeps the replaced files: the caller commits the backup once the rest of its install is in, or
        // restores it. A failure here has already put both files back. The backup is null unless Success.
        public static NativeModInstallStatus Install(string rocksmithLocation, byte[] xinput1_3Dll, byte[] xinput1_3Pdb, out NativeModBackup backup)
        {
            backup = null;
            string dll = Path.Combine(rocksmithLocation, "xinput1_3.dll");

            // The game holds the DLL from launch until it exits. Asked first, so neither file changes while it runs.
            try
            {
                if (IsLocked(dll))
                    return NativeModInstallStatus.GameRunning;
            }
            catch (IOException ex) when (IsSharingViolation(ex))
            {
                return NativeModInstallStatus.GameRunning;
            }

            var saved = new NativeModBackup(rocksmithLocation);
            try
            {
                ReplaceFile(dll, xinput1_3Dll, saved);
                ReplaceFile(Path.Combine(rocksmithLocation, "xinput1_3.pdb"), xinput1_3Pdb, saved);
            }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
            {
                // Never leave a new DLL beside the old PDB, or a half-done install the caller can't undo.
                saved.Restore();
                if (ex is IOException io && IsSharingViolation(io))
                    return NativeModInstallStatus.GameRunning;

                throw;
            }

            backup = saved;
            return NativeModInstallStatus.Success;
        }

        // Written beside the target and swapped in, so a write that fails partway never leaves a truncated DLL that would
        // stop the game from starting.
        private static void ReplaceFile(string path, byte[] contents, NativeModBackup backup)
        {
            string temporary = path + ".new";
            try
            {
                File.WriteAllBytes(temporary, contents);
                backup.Replace(path, temporary);
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
        internal static bool IsSharingViolation(IOException ex) => (ex.HResult & 0xFFFF) is 32 or 33;
    }
}
