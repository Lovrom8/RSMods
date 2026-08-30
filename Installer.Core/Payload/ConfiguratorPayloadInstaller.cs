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
            ValidateInstallRequest(rsModsFolder, payloadZip, manifest);

            string installFolder = Path.GetFullPath(rsModsFolder);
            string stagingFolder = CreateStagingFolderPath(installFolder);

            try
            {
                Report(progress, "Extracting configurator...");
                Directory.CreateDirectory(stagingFolder);
                ExtractZipGuarded(payloadZip, stagingFolder);

                Report(progress, "Verifying configurator...");
                VerifyAgainstManifest(stagingFolder, manifest);

                EnsureExecutableNotRunning(installFolder, manifest);

                Report(progress, "Installing configurator...");
                Deploy(stagingFolder, installFolder, manifest);

                WriteInstalledManifest(installFolder, manifest);
            }
            catch (Exception ex) when (ex is UnauthorizedAccessException or IOException)
            {
                throw CreateFileSystemFailure(ex);
            }
            finally
            {
                TryDeleteDirectory(stagingFolder);
            }
        }

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

        private static void WriteInstalledManifest(string installFolder, PayloadManifest manifest) =>
            File.WriteAllText(Path.Combine(installFolder, InstalledManifestName), manifest.ToJson());

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

        private static void EnsureExecutableNotRunning(string rsModsFolder, PayloadManifest manifest)
        {
            string exeRelative = string.IsNullOrEmpty(manifest.Executable) ? "RSMods.exe" : ToNativePath(manifest.Executable);
            string exePath = Path.Combine(rsModsFolder, exeRelative);
            if (!File.Exists(exePath))
            {
                return; // fresh install - nothing to lock
            }

            try
            {
                // If the configurator is open, the OS holds an exclusive lock on the image; opening it for write throws.
                using (new FileStream(exePath, FileMode.Open, FileAccess.ReadWrite, FileShare.None))
                {
                }
            }
            catch (IOException ex)
            {
                throw new PayloadInstallException(PayloadInstallFailure.ConfiguratorRunning,
                    "RSMods is currently open. Please close it and run the installer again.", ex);
            }
            catch (UnauthorizedAccessException ex)
            {
                throw new PayloadInstallException(PayloadInstallFailure.ConfiguratorRunning,
                    "RSMods is currently open or read-only. Please close it and run the installer again.", ex);
            }
        }

        // --- Deployment -----------------------------------------------------------------------------

        private static void Deploy(string staging, string rsModsFolder, PayloadManifest manifest)
        {
            Directory.CreateDirectory(rsModsFolder);

            RemoveStaleFiles(rsModsFolder, manifest);

            // Copy the validated payload over the top, creating directories as needed.
            foreach (PayloadFile file in manifest.Files)
            {
                string native = ToNativePath(file.Path);
                string source = Path.Combine(staging, native);
                string destination = Path.Combine(rsModsFolder, native);
                Directory.CreateDirectory(Path.GetDirectoryName(destination));
                File.Copy(source, destination, overwrite: true);
            }

            // The configurator expects its user mods directory to exist.
            Directory.CreateDirectory(Path.Combine(rsModsFolder, "CustomMods"));
        }

        private static void RemoveStaleFiles(string rsModsFolder, PayloadManifest newManifest)
        {
            var newPaths = new HashSet<string>(newManifest.Files.Select(f => NormalizeRelative(f.Path)), StringComparer.OrdinalIgnoreCase);
            var stalePaths = new HashSet<string>(EnumerateStalePaths(rsModsFolder, newPaths), StringComparer.OrdinalIgnoreCase);

            foreach (string relativePath in stalePaths)
            {
                TryDeleteOwnedFile(rsModsFolder, relativePath);
            }
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
            catch (IOException) { }
            catch (UnauthorizedAccessException) { }
        }
    }
}
