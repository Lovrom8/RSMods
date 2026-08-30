using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.Serialization;

namespace RSMods.Twitch
{
    public sealed class TwitchRewardRepository
    {
        private static readonly XmlSerializer Serializer = new XmlSerializer(typeof(List<TwitchReward>));
        private readonly string _filePath;

        public TwitchRewardRepository(string filePath)
        {
            if (string.IsNullOrWhiteSpace(filePath))
                throw new ArgumentException("A reward file path is required.", nameof(filePath));

            _filePath = filePath;
        }

        public List<TwitchReward> Load()
        {
            if (!File.Exists(_filePath))
                return new List<TwitchReward>();

            using var stream = File.OpenRead(_filePath);
            return (List<TwitchReward>)Serializer.Deserialize(stream);
        }

        public Task<List<TwitchReward>> LoadAsync()
            => Task.Run(() => Load());

        public void Save(IReadOnlyCollection<TwitchReward> rewards)
        {
            if (rewards == null)
                throw new ArgumentNullException(nameof(rewards));

            string directory = Path.GetDirectoryName(_filePath);
            if (!string.IsNullOrEmpty(directory))
                Directory.CreateDirectory(directory);

            string tempPath = _filePath + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try
            {
                var snapshot = new List<TwitchReward>(rewards);
                var settings = new XmlWriterSettings
                {
                    Encoding = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false),
                    Indent = true
                };

                using (var stream = File.Create(tempPath))
                using (XmlWriter writer = XmlWriter.Create(stream, settings))
                    Serializer.Serialize(writer, snapshot);

                ReplaceFile(tempPath, _filePath);
            }
            finally
            {
                if (File.Exists(tempPath))
                    File.Delete(tempPath);
            }
        }

        public Task SaveAsync(IReadOnlyCollection<TwitchReward> rewards)
            => Task.Run(() => Save(rewards));

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
