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
            if (!File.Exists(_filePath))
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
            TwitchTokenSet stored = Load();
            if (stored != null)
                return stored;

            var settings = new FlatKeyValueSettingsStore(settingsPath);
            string legacyAccessToken = settings.GetString("AccessToken");
            if (string.IsNullOrWhiteSpace(legacyAccessToken))
                return null;

            var imported = new TwitchTokenSet
            {
                AccessToken = legacyAccessToken,
                ClientId = clientId ?? string.Empty
            };
            Save(imported);

            FlatKeyValueSettingsStore.UpdateFile(settingsPath, file => file.Remove("AccessToken"));
            return imported;
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
