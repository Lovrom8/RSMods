using Newtonsoft.Json;
using System;
using System.IO;
using System.Security.Cryptography;
using System.Text;

namespace RSMods.Twitch
{
    public sealed class TwitchTokenSet
    {
        public string AccessToken { get; set; }
        public string RefreshToken { get; set; }
        public DateTimeOffset? ExpiresAtUtc { get; set; }
        public string ClientId { get; set; }
        public string[] Scopes { get; set; } = new string[0];
    }

    /// <summary>Stores Twitch OAuth material encrypted for the current Windows user.</summary>
    public sealed class TwitchTokenStore
    {
        private static readonly byte[] Entropy = Encoding.UTF8.GetBytes("RSMods.Twitch.TokenStore.v1");
        private readonly string _filePath;

        public TwitchTokenStore(string filePath)
        {
            if (string.IsNullOrWhiteSpace(filePath))
                throw new ArgumentException("A token file path is required.", nameof(filePath));

            _filePath = filePath;
        }

        public TwitchTokenSet Load()
        {
            // The tokens are encrypted with DPAPI, which is Windows-only; nothing is stored elsewhere (Twitch effects
            // need the Windows DLL anyway)
            if (!OperatingSystem.IsWindows() || !File.Exists(_filePath))
                return null;

            try
            {
                byte[] encrypted = File.ReadAllBytes(_filePath);
                byte[] plaintext = ProtectedData.Unprotect(encrypted, Entropy, DataProtectionScope.CurrentUser);
                string json = Encoding.UTF8.GetString(plaintext);
                return JsonConvert.DeserializeObject<TwitchTokenSet>(json);
            }
            catch (CryptographicException)
            {
                return null;
            }
            catch (JsonException)
            {
                return null;
            }
        }

        public void Save(TwitchTokenSet tokens)
        {
            if (tokens == null)
                throw new ArgumentNullException(nameof(tokens));
            if (!OperatingSystem.IsWindows())
                return; // never stored unencrypted

            string directory = Path.GetDirectoryName(_filePath);
            if (!string.IsNullOrEmpty(directory))
                Directory.CreateDirectory(directory);

            string json = JsonConvert.SerializeObject(tokens);
            byte[] plaintext = Encoding.UTF8.GetBytes(json);
            byte[] encrypted = ProtectedData.Protect(plaintext, Entropy, DataProtectionScope.CurrentUser);

            string tempPath = _filePath + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try
            {
                File.WriteAllBytes(tempPath, encrypted);
                ReplaceFile(tempPath, _filePath);
            }
            finally
            {
                if (File.Exists(tempPath))
                    File.Delete(tempPath);
            }
        }

        public TwitchTokenSet LoadOrImportLegacy(string settingsPath, string clientId = "")
        {
            var settings = new FlatKeyValueSettingsStore(settingsPath);
            bool hasLegacyLine = settings.TryGetString("AccessToken", out string legacyAccessToken);

            TwitchTokenSet stored = Load();
            if (stored != null)
            {
                // An earlier import saved the token but couldn't remove the plaintext copy; finish that now.
                if (hasLegacyLine)
                    TryRemoveLegacyToken(settingsPath);
                return stored;
            }

            if (string.IsNullOrWhiteSpace(legacyAccessToken))
                return null;

            var imported = new TwitchTokenSet
            {
                AccessToken = legacyAccessToken,
                ClientId = clientId ?? string.Empty
            };
            Save(imported);

            TryRemoveLegacyToken(settingsPath);
            return imported;
        }

        // A locked settings file must not stop Twitch from loading; the next start tries again.
        private static void TryRemoveLegacyToken(string settingsPath)
        {
            try
            {
                FlatKeyValueSettingsStore.UpdateFile(settingsPath, file => file.Remove("AccessToken"));
            }
            catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException)
            {
            }
        }

        public void Clear()
        {
            if (File.Exists(_filePath))
                File.Delete(_filePath);
        }

        private static void ReplaceFile(string tempPath, string destinationPath)
        {
            if (File.Exists(destinationPath))
            {
                string backupPath = destinationPath + ".bak";
                try
                {
                    if (File.Exists(backupPath))
                        File.Delete(backupPath);
                    File.Replace(tempPath, destinationPath, backupPath);
                    if (File.Exists(backupPath))
                        File.Delete(backupPath);
                    return;
                }
                catch (PlatformNotSupportedException)
                {
                }
            }

            if (File.Exists(destinationPath))
                File.Delete(destinationPath);
            File.Move(tempPath, destinationPath);
        }
    }
}
