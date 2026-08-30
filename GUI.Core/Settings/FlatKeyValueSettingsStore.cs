using System;
using System.Collections.Generic;
using System.IO;

namespace RSMods
{
    /// <summary>
    /// Round-trip-safe store for GUI_Settings.ini, whose legacy format is a flat list of
    /// key/value pairs rather than a sectioned INI file. Unknown lines, comments, blank lines,
    /// and ordering are preserved when named values are updated.
    /// </summary>
    public sealed class FlatKeyValueSettingsStore
    {
        private static readonly object FileLock = new object();

        private readonly string _filePath;
        private readonly List<string> _lines;

        public FlatKeyValueSettingsStore(string filePath)
        {
            if (string.IsNullOrWhiteSpace(filePath))
                throw new ArgumentException("A settings path is required.", nameof(filePath));

            _filePath = filePath;
            _lines = File.Exists(filePath)
                ? new List<string>(File.ReadAllLines(filePath))
                : [];
        }

        public bool TryGetString(string key, out string value)
        {
            value = string.Empty;
            bool found = false;

            foreach (string line in _lines)
            {
                if (TryParseLine(line, out string parsedKey, out string parsedValue) &&
                    string.Equals(parsedKey, key, StringComparison.OrdinalIgnoreCase))
                {
                    value = parsedValue;
                    found = true;
                }
            }

            return found;
        }

        public string GetString(string key, string defaultValue = "") => TryGetString(key, out string value) ? value : defaultValue;

        public void SetString(string key, string value)
        {
            if (string.IsNullOrWhiteSpace(key))
            {
                throw new ArgumentException("A settings key is required.", nameof(key));
            }

            int lastIndex = -1;
            for (int i = 0; i < _lines.Count; i++)
            {
                if (TryParseLine(_lines[i], out string parsedKey, out _) && string.Equals(parsedKey, key, StringComparison.OrdinalIgnoreCase))
                {
                    lastIndex = i;
                }
            }

            string replacement = $"{key} = {value ?? string.Empty}";
            if (lastIndex >= 0)
            {
                _lines[lastIndex] = replacement;
            }
            else
            {
                _lines.Add(replacement);
            }
        }

        public bool Remove(string key)
        {
            bool removed = false;
            for (int i = _lines.Count - 1; i >= 0; i--)
            {
                if (TryParseLine(_lines[i], out string parsedKey, out _) && string.Equals(parsedKey, key, StringComparison.OrdinalIgnoreCase))
                {
                    _lines.RemoveAt(i);
                    removed = true;
                }
            }

            return removed;
        }

        public void Save()
        {
            string directory = Path.GetDirectoryName(_filePath);
            if (!string.IsNullOrEmpty(directory))
            {
                Directory.CreateDirectory(directory);
            }

            string tempPath = _filePath + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try
            {
                File.WriteAllLines(tempPath, _lines);
                ReplaceFile(tempPath, _filePath);
            }
            finally
            {
                if (File.Exists(tempPath))
                {
                    File.Delete(tempPath);
                }
            }
        }

        public static void UpdateFile(string filePath, Action<FlatKeyValueSettingsStore> update)
        {
            if (update == null)
            {
                throw new ArgumentNullException(nameof(update));
            }

            lock (FileLock)
            {
                var store = new FlatKeyValueSettingsStore(filePath);
                update(store);
                store.Save();
            }
        }

        internal static bool TryParseLine(string line, out string key, out string value)
        {
            key = string.Empty;
            value = string.Empty;

            if (string.IsNullOrWhiteSpace(line))
                return false;

            string trimmed = line.TrimStart();
            if (trimmed.StartsWith(";", StringComparison.Ordinal) ||
                trimmed.StartsWith("#", StringComparison.Ordinal) ||
                trimmed.StartsWith("[", StringComparison.Ordinal))
            {
                return false;
            }

            int separator = line.IndexOf('=');
            if (separator <= 0)
                return false;

            key = line.Substring(0, separator).Trim();
            value = line.Substring(separator + 1).Trim();
            return key.Length > 0;
        }

        private static void ReplaceFile(string tempPath, string destinationPath)
        {
            if (File.Exists(destinationPath))
            {
                string backupPath = destinationPath + ".bak";
                try
                {
                    if (File.Exists(backupPath))
                    {
                        File.Delete(backupPath);
                    }

                    File.Replace(tempPath, destinationPath, backupPath);

                    if (File.Exists(backupPath))
                    {
                        File.Delete(backupPath);
                    }

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
