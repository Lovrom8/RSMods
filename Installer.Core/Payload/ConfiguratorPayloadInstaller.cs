using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Security.Cryptography;

namespace RS2014_Mod_Installer.Payload
{
    public enum PayloadInstallFailure
    {
        None = 0,
        UnsafeArchivePath,   // a zip entry or manifest path tried to escape the install directory
        CorruptPayload,      // a file was missing, wrong length, or failed its hash after extraction
        ConfiguratorRunning, // the installed RSMods.exe is locked (the app is open)
        IOError              // filesystem failure while deploying
    }

    // Thrown for every failure the installer can report. Carries a user-facing Message and a machine-readable Reason the tests assert on.
    public sealed class PayloadInstallException(PayloadInstallFailure reason, string message, Exception inner = null) : Exception(message, inner)
    {
        public PayloadInstallFailure Reason { get; } = reason;
    }

    // Deploys the Avalonia configurator payload into <Rocksmith>/RSMods. 
    public static class ConfiguratorPayloadInstaller
    {
        // Files at the root of the RSMods folder that belong to the user and must survive every upgrade.
        private static readonly string[] PreservedFiles =
        [
            "GUI_Settings.ini",
            "TwitchAuth.dat",
            "TwitchEnabledEffects.xml"
        ];

        // Directories at the root of the RSMods folder that must never be touched during an upgrade.
        private static readonly string[] PreservedDirectories =
        [
            "CustomMods"
        ];

        // The manifest of the currently installed payload, written beside the executable for the next upgrade.
        public const string InstalledManifestName = "RSModsGUI.manifest.json";

        // One-time list of files shipped only by the old WinForms GUI. They are removed
        // on upgrade once confirmed absent from the new payload. Exact names plus the TwitchLib.* family.
        private static readonly string[] ObsoleteLegacyFiles =
        [
            "NAudio.WinForms.dll",
            "Ookii.Dialogs.dll",
            "PSTaskDialog.dll",
            "RSMods.exe.config",
            "RSMods.pdb"
        ];

        private static readonly string[] ObsoleteLegacyPrefixes =
        [
            "TwitchLib."
        ];

        /// <summary>
        /// Installs (fresh) or upgrades the configurator under <paramref name="rsModsFolder"/>.
        /// </summary>
        /// <exception cref="PayloadInstallException">On any failure; the prior install is left usable.</exception>
        public static void Install(string rsModsFolder, Stream payloadZip, PayloadManifest manifest, Action<string> progress = null)
        {
            using StagedConfiguratorPayload staged = Stage(rsModsFolder, payloadZip, manifest, progress);
            staged.Deploy(progress);
        }

        /// <summary>
        /// Extracts and verifies the payload into a sibling staging folder without touching the install, so a caller
        /// can check it before changing anything else. Disposing the result deletes the staging folder.
        /// </summary>
        /// <exception cref="PayloadInstallException">On any failure; nothing under <paramref name="rsModsFolder"/> changed.</exception>
        public static StagedConfiguratorPayload Stage(string rsModsFolder, Stream payloadZip, PayloadManifest manifest, Action<string> progress = null)
        {
            ValidateInstallRequest(rsModsFolder, payloadZip, manifest);

            string installFolder = Path.GetFullPath(rsModsFolder);
            string stagingFolder = CreateStagingFolderPath(installFolder);

            bool staged = false;
            try
            {
                Report(progress, "Extracting configurator...");
                Directory.CreateDirectory(stagingFolder);
                ExtractZipGuarded(payloadZip, stagingFolder);

                Report(progress, "Verifying configurator...");
                VerifyAgainstManifest(stagingFolder, manifest);

                // Written with the payload so Deploy swaps it in with everything else and can roll it back too.
                File.WriteAllText(Path.Combine(stagingFolder, InstalledManifestName), manifest.ToJson());

                staged = true;
                return new StagedConfiguratorPayload(installFolder, stagingFolder, manifest);
            }
            catch (Exception ex) when (ex is UnauthorizedAccessException or IOException)
            {
                throw CreateFileSystemFailure(ex);
            }
            finally
            {
                if (!staged)
                    TryDeleteDirectory(stagingFolder);
            }
        }

        // Deploys a verified staging folder; see Stage.
        internal static void DeployStaged(string stagingFolder, string installFolder, PayloadManifest manifest, Action<string> progress)
        {
            try
            {
                ThrowIfConfiguratorRunning(installFolder, manifest);

                Report(progress, "Installing configurator...");
                Deploy(stagingFolder, installFolder, manifest, stagingFolder + "_previous");
            }
            catch (Exception ex) when (ex is UnauthorizedAccessException or IOException)
            {
                throw CreateFileSystemFailure(ex);
            }
        }

        internal static void DeleteStaging(string stagingFolder) => TryDeleteDirectory(stagingFolder);

        private static void ValidateInstallRequest(string rsModsFolder, Stream payloadZip, PayloadManifest manifest)
        {
            ArgumentNullException.ThrowIfNull(rsModsFolder);
            ArgumentNullException.ThrowIfNull(payloadZip);
            ArgumentNullException.ThrowIfNull(manifest);

            if (manifest.Files == null || manifest.Files.Count == 0)
                throw new PayloadInstallException(PayloadInstallFailure.CorruptPayload, "The installer payload manifest lists no files.");
        }

        private static string CreateStagingFolderPath(string installFolder)
        {
            string trimmedInstallFolder = installFolder.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
            string parentFolder = Directory.GetParent(trimmedInstallFolder)?.FullName
                                  ?? Path.GetDirectoryName(trimmedInstallFolder);

            if (string.IsNullOrEmpty(parentFolder))
            {
                throw new PayloadInstallException(
                    PayloadInstallFailure.IOError,
                    "The installer could not determine where to create its staging folder.");
            }

            return Path.Combine(parentFolder, "RSMods._install_" + Guid.NewGuid().ToString("N"));
        }

        private static PayloadInstallException CreateFileSystemFailure(Exception exception)
        {
            string message = exception is UnauthorizedAccessException
                ? "The installer could not write to the RSMods folder: " + exception.Message
                : "A file error occurred while installing the configurator: " + exception.Message;

            return new PayloadInstallException(PayloadInstallFailure.IOError, message, exception);
        }

        // --- Extraction -----------------------------------------------------------------------------

        private static void ExtractZipGuarded(Stream payloadZip, string destinationRoot)
        {
            string rootFull = WithTrailingSeparator(Path.GetFullPath(destinationRoot));

            using var archive = new ZipArchive(payloadZip, ZipArchiveMode.Read, leaveOpen: true);
            foreach (ZipArchiveEntry entry in archive.Entries)
            {
                // Directory entries have an empty name; skip - files below create their own directories.
                if (entry.FullName.Length == 0 || entry.FullName.EndsWith('/') || entry.FullName.EndsWith('\\'))
                    continue;

                string target = Path.GetFullPath(Path.Combine(destinationRoot, entry.FullName));
                if (!target.StartsWith(rootFull, StringComparison.OrdinalIgnoreCase))
                {
                    throw new PayloadInstallException(PayloadInstallFailure.UnsafeArchivePath,
                        "The installer payload contains an unsafe path and was rejected: " + entry.FullName);
                }

                Directory.CreateDirectory(Path.GetDirectoryName(target));
                entry.ExtractToFile(target, overwrite: true);
            }
        }

        // --- Verification ---------------------------------------------------------------------------

        private static void VerifyAgainstManifest(string root, PayloadManifest manifest)
        {
            foreach (PayloadFile file in manifest.Files)
            {
                if (!IsSafeRelativePath(file.Path))
                {
                    throw new PayloadInstallException(PayloadInstallFailure.UnsafeArchivePath,
                        "The installer payload manifest contains an unsafe path: " + file.Path);
                }

                string full = Path.Combine(root, ToNativePath(file.Path));
                if (!File.Exists(full))
                {
                    throw new PayloadInstallException(PayloadInstallFailure.CorruptPayload,
                        "The installer payload is incomplete (missing " + file.Path + ").");
                }

                var info = new FileInfo(full);
                if (info.Length != file.Length)
                {
                    throw new PayloadInstallException(PayloadInstallFailure.CorruptPayload,
                        "The installer payload is corrupt (wrong size for " + file.Path + ").");
                }

                if (!string.Equals(ComputeSha256(full), file.Sha256, StringComparison.OrdinalIgnoreCase))
                {
                    throw new PayloadInstallException(PayloadInstallFailure.CorruptPayload,
                        "The installer payload is corrupt (hash mismatch for " + file.Path + ").");
                }
            }
        }

        // --- Running-exe guard ----------------------------------------------------------------------

        /// <summary>
        /// Throws <see cref="PayloadInstallException"/> (<see cref="PayloadInstallFailure.ConfiguratorRunning"/>) when the
        /// installed configurator is open. The installer checks this before touching anything else, so an open
        /// configurator can't leave a new native mod beside the old configurator.
        /// </summary>
        public static void ThrowIfConfiguratorRunning(string rsModsFolder, PayloadManifest manifest)
        {
            ArgumentNullException.ThrowIfNull(rsModsFolder);
            ArgumentNullException.ThrowIfNull(manifest);

            string exeRelative = string.IsNullOrEmpty(manifest.Executable) ? "RSMods.exe" : ToNativePath(manifest.Executable);
            string exePath = Path.Combine(rsModsFolder, exeRelative);
            if (!File.Exists(exePath))
            {
                return; // fresh install - nothing to lock
            }

            // A read-only exe can't be opened for write, yet Deploy's move replaces it fine; lift the flag just for the probe.
            FileAttributes attributes = File.GetAttributes(exePath);
            bool readOnly = attributes.HasFlag(FileAttributes.ReadOnly);
            try
            {
                if (readOnly)
                    File.SetAttributes(exePath, attributes & ~FileAttributes.ReadOnly);

                // If the configurator is open, the OS holds an exclusive lock on the image; opening it for write throws.
                using (new FileStream(exePath, FileMode.Open, FileAccess.ReadWrite, FileShare.None))
                {
                }
            }
            // Only a lock means it's open; access denied or a disk error is thrown for the caller to report as it is.
            catch (IOException ex) when (Core.NativeModInstaller.IsSharingViolation(ex))
            {
                throw new PayloadInstallException(PayloadInstallFailure.ConfiguratorRunning,
                    "RSMods is currently open. Please close it and run the installer again.", ex);
            }
            finally
            {
                if (readOnly)
                {
                    try { File.SetAttributes(exePath, attributes); }
                    catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { }
                }
            }
        }

        // --- Deployment -----------------------------------------------------------------------------

        // Copies the validated payload over the top. Each file it replaces is moved to backupFolder first, so a copy that
        // fails partway (a file in use, a full disk) puts the prior install back exactly as it was.
        private static void Deploy(string staging, string rsModsFolder, PayloadManifest manifest, string backupFolder)
        {
            Directory.CreateDirectory(rsModsFolder);

            // Worked out before the new installed manifest replaces the one naming the prior install's files.
            List<string> stalePaths = FindStaleFiles(rsModsFolder, manifest);

            var replaced = new List<(string Destination, string Backup)>();
            var added = new List<string>();
            try
            {
                foreach (string relativePath in DeployedFiles(manifest))
                {
                    string native = ToNativePath(relativePath);
                    string source = Path.Combine(staging, native);
                    string destination = Path.Combine(rsModsFolder, native);

                    // The player's own files stay as they are, even if a payload ships one.
                    if (IsPreserved(relativePath) && File.Exists(destination))
                        continue;

                    Directory.CreateDirectory(Path.GetDirectoryName(destination));

                    if (File.Exists(destination))
                    {
                        string backup = Path.Combine(backupFolder, native);
                        Directory.CreateDirectory(Path.GetDirectoryName(backup));
                        File.Move(destination, backup);
                        replaced.Add((destination, backup));
                    }
                    else
                    {
                        added.Add(destination);
                    }

                    File.Copy(source, destination);
                }

                // The configurator expects its user mods directory to exist.
                Directory.CreateDirectory(Path.Combine(rsModsFolder, "CustomMods"));
            }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
            {
                if (!RollBack(replaced, added))
                {
                    throw new PayloadInstallException(PayloadInstallFailure.IOError,
                        "Installing the configurator failed and it couldn't be fully undone. The previous files are in " +
                        backupFolder + ": " + ex.Message, ex);
                }

                TryDeleteDirectory(backupFolder);
                throw;
            }

            TryDeleteDirectory(backupFolder);

            // Only now that every new file is in: a failed upgrade above still has all of the prior install's files.
            foreach (string relativePath in stalePaths)
                TryDeleteOwnedFile(rsModsFolder, relativePath);
        }

        // The payload's files, then the installed manifest Stage wrote beside them (always ours, even if a payload lists one).
        private static IEnumerable<string> DeployedFiles(PayloadManifest manifest)
        {
            foreach (PayloadFile file in manifest.Files)
            {
                if (!string.Equals(NormalizeRelative(file.Path), InstalledManifestName, StringComparison.OrdinalIgnoreCase))
                    yield return file.Path;
            }

            yield return InstalledManifestName;
        }

        // Newest first: removes what the failed deploy added and moves each replaced file back. False if any of it failed,
        // which leaves those files in the backup folder.
        private static bool RollBack(List<(string Destination, string Backup)> replaced, List<string> added)
        {
            bool complete = true;

            for (int i = added.Count - 1; i >= 0; i--)
                complete &= TryDeleteFile(added[i]);

            for (int i = replaced.Count - 1; i >= 0; i--)
            {
                (string destination, string backup) = replaced[i];
                try
                {
                    File.Move(backup, destination, overwrite: true);
                }
                catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
                {
                    complete = false;
                }
            }

            return complete;
        }

        private static bool TryDeleteFile(string path)
        {
            try
            {
                File.Delete(path);
                return true;
            }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
            {
                return false;
            }
        }

        private static List<string> FindStaleFiles(string rsModsFolder, PayloadManifest newManifest)
        {
            var newPaths = new HashSet<string>(newManifest.Files.Select(f => NormalizeRelative(f.Path)), StringComparer.OrdinalIgnoreCase)
            {
                InstalledManifestName
            };

            return EnumerateStalePaths(rsModsFolder, newPaths).Distinct(StringComparer.OrdinalIgnoreCase).ToList();
        }

        private static IEnumerable<string> EnumerateStalePaths(string rsModsFolder, HashSet<string> newPaths)
        {
            PayloadManifest previousManifest = TryReadInstalledManifest(rsModsFolder);
            if (previousManifest?.Files != null)
            {
                foreach (PayloadFile file in previousManifest.Files)
                {
                    if (file != null && !newPaths.Contains(NormalizeRelative(file.Path)))
                        yield return file.Path;
                }
            }

            foreach (string legacyPath in EnumerateObsoleteLegacyFiles(rsModsFolder))
            {
                if (!newPaths.Contains(NormalizeRelative(legacyPath)))
                    yield return legacyPath;
            }
        }

        private static PayloadManifest TryReadInstalledManifest(string rsModsFolder)
        {
            string manifestPath = Path.Combine(rsModsFolder, InstalledManifestName);
            if (!File.Exists(manifestPath))
                return null;

            try
            {
                return PayloadManifest.Parse(File.ReadAllText(manifestPath));
            }
            catch
            {
                // Cleanup metadata is optional. A damaged prior manifest must not block an otherwise valid
                // install; it only disables manifest-driven stale-file cleanup for this run.
                return null;
            }
        }

        private static void TryDeleteOwnedFile(string rsModsFolder, string relativePath)
        {
            // The installed manifest lives in a user-writable directory, so treat every recorded path as
            // untrusted even though manifests generated by this installer contain only safe paths.
            if (!IsSafeRelativePath(relativePath))
                return;

            string normalized = NormalizeRelative(relativePath);
            if (IsPreserved(normalized))
                return;

            try
            {
                string installRoot = WithTrailingSeparator(Path.GetFullPath(rsModsFolder));
                string fullPath = Path.GetFullPath(Path.Combine(installRoot, ToNativePath(normalized)));
                if (!fullPath.StartsWith(installRoot, StringComparison.OrdinalIgnoreCase))
                    return;

                File.Delete(fullPath);
            }
            catch (IOException)
            {
                // Stale-file cleanup is best-effort and must not make an otherwise valid upgrade fail.
            }
            catch (UnauthorizedAccessException)
            {
            }
            catch (ArgumentException)
            {
                // Ignore malformed paths from a damaged or edited installed manifest.
            }
            catch (NotSupportedException)
            {
            }
        }

        private static IEnumerable<string> EnumerateObsoleteLegacyFiles(string rsModsFolder)
        {
            foreach (string name in ObsoleteLegacyFiles)
                yield return name;

            if (Directory.Exists(rsModsFolder))
            {
                foreach (string path in Directory.GetFiles(rsModsFolder, "*", SearchOption.TopDirectoryOnly))
                {
                    string name = Path.GetFileName(path);
                    foreach (string prefix in ObsoleteLegacyPrefixes)
                    {
                        if (name.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
                        {
                            yield return name;
                            break;
                        }
                    }
                }
            }
        }

        private static bool IsPreserved(string relative)
        {
            string normalized = NormalizeRelative(relative);
            foreach (string file in PreservedFiles)
            {
                if (string.Equals(normalized, file, StringComparison.OrdinalIgnoreCase))
                {
                    return true;
                }
            }

            foreach (string dir in PreservedDirectories)
            {
                if (normalized.StartsWith(dir + "/", StringComparison.OrdinalIgnoreCase) || string.Equals(normalized, dir, StringComparison.OrdinalIgnoreCase))
                {
                    return true;
                }
            }

            return false;
        }

        // --- Helpers --------------------------------------------------------------------------------

        private static bool IsSafeRelativePath(string path)
        {
            if (string.IsNullOrEmpty(path)) return false;
            if (Path.IsPathRooted(path)) return false;

            string normalized = path.Replace('\\', '/');
            return normalized != ".." && !normalized.StartsWith("../") && !normalized.EndsWith("/..") && !normalized.Contains("/../");
        }

        private static string NormalizeRelative(string path)
        {
            return (path ?? string.Empty).Replace('\\', '/').Trim('/');
        }

        private static string ToNativePath(string relative)
        {
            return relative.Replace('/', Path.DirectorySeparatorChar).Replace('\\', Path.DirectorySeparatorChar);
        }

        private static string WithTrailingSeparator(string path)
        {
            if (path.EndsWith(Path.DirectorySeparatorChar.ToString()) || path.EndsWith(Path.AltDirectorySeparatorChar.ToString()))
                return path;

            return path + Path.DirectorySeparatorChar;
        }

        private static string ComputeSha256(string path)
        {
            using var sha = SHA256.Create();
            using FileStream stream = File.OpenRead(path);

            byte[] hash = sha.ComputeHash(stream);
            var builder = new System.Text.StringBuilder(hash.Length * 2);
            foreach (byte b in hash) builder.Append(b.ToString("x2"));

            return builder.ToString();
        }

        private static void Report(Action<string> progress, string message)
        {
            progress?.Invoke(message);
        }

        private static void TryDeleteDirectory(string path)
        {
            try
            {
                if (Directory.Exists(path))
                {
                    Directory.Delete(path, recursive: true);
                }
            }
            catch (UnauthorizedAccessException)
            {
                // A read-only file moved aside from the prior install blocks the delete; clear the flag and try once more.
                try
                {
                    foreach (string file in Directory.EnumerateFiles(path, "*", SearchOption.AllDirectories))
                        File.SetAttributes(file, FileAttributes.Normal);

                    Directory.Delete(path, recursive: true);
                }
                catch (IOException) { }
                catch (UnauthorizedAccessException) { }
            }
            catch (IOException) { }
        }
    }
}
