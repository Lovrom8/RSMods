using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;

namespace RS2014_Mod_Installer.Core
{
    public enum ExecutableCheckStatus
    {
        Valid,
        Invalid,
        Unreadable
    }

    // The outcome of the anti-piracy / supported-version check.
    public sealed class CompatibilityResult(bool isCompatible, bool executableUnreadable, IReadOnlyList<string> reasons)
    {
        public bool IsCompatible { get; } = isCompatible;

        // True when the executable could not be hashed because the game is running.
        public bool ExecutableUnreadable { get; } = executableUnreadable;

        public IReadOnlyList<string> Reasons { get; } = reasons ?? [];
    }

    // Supported-executable and certificate checks (was ExeUtil + GUI.IsVoid).
    public static class RocksmithCompatibility
    {
        public static ExecutableCheckStatus CheckExecutable(string installLocation) => VerifyHash(installLocation, HASH_EXE, HASH_EXE_REM, HASH_EXE_LP);

        public static bool LearnAndPlayExecutableExists(string installLocation) => VerifyHash(installLocation, HASH_EXE_LP) == ExecutableCheckStatus.Valid;

        // Aggregates the executable, steam_api.dll certificate, and crack-indicator checks. Modified from
        // Beat Saber Mod Assistant. IsCompatible is false when any check fails.
        public static CompatibilityResult Evaluate(string installLocation)
        {
            var reasons = new List<string>();

            bool fakeSteamApi = !SteamApiCertificateValid(installLocation);
            if (fakeSteamApi)
                reasons.Add("Invalid steam_api.dll certificate.");

            bool crackPresent = CrackIndicationsPresent(installLocation);
            if (crackPresent)
                reasons.Add("Parts of game crack are present in the folder.");

            ExecutableCheckStatus exe = CheckExecutable(installLocation);
            if (exe == ExecutableCheckStatus.Unreadable)
                return new CompatibilityResult(isCompatible: false, executableUnreadable: true, reasons);

            bool exeInvalid = exe != ExecutableCheckStatus.Valid;
            if (exeInvalid)
                reasons.Add("Game executable version doesn't appear to be correct.");

            bool compatible = !(fakeSteamApi || crackPresent || exeInvalid);
            return new CompatibilityResult(compatible, executableUnreadable: false, reasons);
        }

        private static ExecutableCheckStatus VerifyHash(string installLocation, params byte[][] possibleHashes)
        {
            string filePath = Path.Combine(installLocation, "Rocksmith2014.exe");
            if (!File.Exists(filePath))
                return ExecutableCheckStatus.Invalid;

            try
            {
                using SHA256 sha256 = SHA256.Create();
                using FileStream exeStream = new(filePath, FileMode.Open, FileAccess.Read, FileShare.Read);

                byte[] computedHash = sha256.ComputeHash(exeStream);
                return possibleHashes.Any(possibleHash => computedHash.SequenceEqual(possibleHash))
                    ? ExecutableCheckStatus.Valid
                    : ExecutableCheckStatus.Invalid;
            }
            catch (IOException)
            {
                // The game was open when we tried to hash the executable.
                return ExecutableCheckStatus.Unreadable;
            }
        }

        private static bool SteamApiCertificateValid(string installLocation)
        {
            try
            {
                var cert = new X509Certificate2(X509Certificate.CreateFromSignedFile(Path.Combine(installLocation, "steam_api.dll")));
                return cert.GetNameInfo(X509NameType.SimpleName, false) == "Valve" || cert.Verify();
            }
            catch
            {
                return false; // unsigned / missing / bad certificate
            }
        }

        private static bool CrackIndicationsPresent(string installLocation) =>
            File.Exists(Path.Combine(installLocation, "IGG-GAMES.COM.url")) ||
            File.Exists(Path.Combine(installLocation, "SmartSteamEmu.ini")) ||
            File.Exists(Path.Combine(installLocation, "GAMESTORRENT.CO.url")) ||
            File.Exists(Path.Combine(installLocation, "Codex.ini")) ||
            File.Exists(Path.Combine(installLocation, "Skidrow.ini")) ||
            File.Exists(Path.Combine(installLocation, "steamclient.dll"));

        /// <summary>Hash for Rocksmith2014.exe for the Remastered Update | SHA256</summary>
        private static readonly byte[] HASH_EXE = [0xA7, 0x25, 0x84, 0x61, 0x10, 0x1D, 0xA0, 0x20, 0x17, 0x07, 0xF5, 0xC2, 0x72, 0xBA, 0xAA, 0x62, 0xA3, 0xD3, 0xD1, 0x0B, 0x3D, 0x22, 0x13, 0xC0, 0xD0, 0xF2, 0x1C, 0xC8, 0x3B, 0x45, 0x88, 0xDA];
        private static readonly byte[] HASH_EXE_REM = [0x0d, 0x42, 0xe2, 0xff, 0x3c, 0x7a, 0xf6, 0x84, 0x3e, 0xcb, 0x81, 0x25, 0x9c, 0xc6, 0x4f, 0x1d, 0xde, 0xfa, 0x13, 0x97, 0xb7, 0xce, 0x53, 0xfd, 0xcf, 0x0a, 0x05, 0xd0, 0xb6, 0x1a, 0x0d, 0xc3];
        private static readonly byte[] HASH_EXE_LP = [0xbb, 0x05, 0x69, 0x59, 0xC0, 0xc6, 0x37, 0x1d, 0x4e, 0xcf, 0x78, 0xf8, 0x4c, 0x5D, 0x27, 0xe2, 0xfa, 0xe9, 0x3a, 0x9d, 0x02, 0x58, 0x83, 0x0C, 0x2a, 0x36, 0xf3, 0x3e, 0x6a, 0x27, 0x78, 0xeb];
    }
}
