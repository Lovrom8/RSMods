using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Windows.Forms;

namespace RSMods.Util
{
    public static class ExeUtil
    {
        private static bool VerifyHash(string installLocation, byte[][] possibleHashes)
        {
            try
            {
                using SHA256 sha256 = SHA256.Create();
                
                string filePath = Path.Combine(installLocation, "Rocksmith2014.exe");

                if (!File.Exists(filePath))
                {
                    throw new FileNotFoundException("The specified file was not found.", filePath);
                }

                using FileStream exeStream = new(filePath, FileMode.Open, FileAccess.Read, FileShare.Read)
                {
                    Position = 0
                };
                
                byte[] computedHash = sha256.ComputeHash(exeStream);

                return possibleHashes.Any(possibleHash => computedHash.SequenceEqual(possibleHash));
            }
            catch (IOException) // Game was open when performing the check
            {
                MessageBox.Show("Please close Rocksmith2014, then re-open this tool!");
                Environment.Exit(1);
                return true;
            }
        }

        public static bool LearnAndPlayExecutableExists(string installLocation) => VerifyHash(installLocation, new byte[][] { HASH_EXE_LP });

        public static bool CheckExecutable(string installLocation) => VerifyHash(installLocation, new byte[][] { HASH_EXE, HASH_EXE_REM, HASH_EXE_LP });

        /// <summary>
        /// Hash for Rocksmith2014.exe for the Remastered Update | SHA256
        /// </summary>
        readonly static byte[] HASH_EXE = { 0xA7, 0x25, 0x84, 0x61, 0x10, 0x1D, 0xA0, 0x20, 0x17, 0x07, 0xF5, 0xC2, 0x72, 0xBA, 0xAA, 0x62, 0xA3, 0xD3, 0xD1, 0x0B, 0x3D, 0x22, 0x13, 0xC0, 0xD0, 0xF2, 0x1C, 0xC8, 0x3B, 0x45, 0x88, 0xDA };
        readonly static byte[] HASH_EXE_REM = { 0x0d, 0x42, 0xe2, 0xff, 0x3c, 0x7a, 0xf6, 0x84, 0x3e, 0xcb, 0x81, 0x25, 0x9c, 0xc6, 0x4f, 0x1d, 0xde, 0xfa, 0x13, 0x97, 0xb7, 0xce, 0x53, 0xfd, 0xcf, 0x0a, 0x05, 0xd0, 0xb6, 0x1a, 0x0d, 0xc3 };
        readonly static byte[] HASH_EXE_LP = { 0xbb, 0x05, 0x69, 0x59, 0xC0, 0xc6, 0x37, 0x1d, 0x4e, 0xcf, 0x78, 0xf8, 0x4c, 0x5D, 0x27, 0xe2, 0xfa, 0xe9, 0x3a, 0x9d, 0x02, 0x58, 0x83, 0x0C, 0x2a, 0x36, 0xf3, 0x3e, 0x6a, 0x27, 0x78, 0xeb };

        public static void CheckInstallIntegrity(string installLocation) // Anti-Piracy Check (False = Real, True = Pirated) || Modified from Beat Saber Mod Assistant
        {
            string reason = string.Empty;
            bool fakeSteamApi = true;
            try
            {
                X509Certificate2 cert = new X509Certificate2(X509Certificate.CreateFromSignedFile(Path.Combine(installLocation, "steam_api.dll")));

                if (cert.GetNameInfo(X509NameType.SimpleName, false) == "Valve" || cert.Verify())
                {
                    fakeSteamApi = false;
                }
                else
                {
                    reason += "Invalid steam_api.dll certificate.";
                }
            }
            catch { } // Fall-through = bad cert.

            bool areCrackIndicationsPresent = File.Exists(Path.Combine(installLocation, "IGG-GAMES.COM.url")) || File.Exists(Path.Combine(installLocation, "SmartSteamEmu.ini")) || File.Exists(Path.Combine(installLocation, "GAMESTORRENT.CO.url")) || File.Exists(Path.Combine(installLocation, "Codex.ini")) || File.Exists(Path.Combine(installLocation, "Skidrow.ini")) || File.Exists(Path.Combine(installLocation, "steamclient.dll"));

            if (areCrackIndicationsPresent)
            {
                reason += "\nParts of game crack are present in the folder.";
            }

            bool isExeInvalid = !ExeUtil.CheckExecutable(installLocation);

            if (isExeInvalid)
            {
                reason += "\nGame executable version doesn't appear to be correct.";
            }

            if (areCrackIndicationsPresent || fakeSteamApi || isExeInvalid)
            {
                MessageBox.Show($"Incompatible Rocksmith version detected! Only the newest RS version is supported - RSMods doesn't support pirated / stolen copies of Rocksmith 2014! {Environment.NewLine}Reason: {reason}", "Incompatible Rocksmith version", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Process.Start("https://store.steampowered.com/app/221680/");
                Environment.Exit(1);
                return;
            }
        }
    }
}
