using System;
using System.Collections.Generic;
using System.IO;

namespace RSMods
{
    public class IniManager(string filePath)
    {
        public event Action SettingChanged;

        private readonly Dictionary<string, Dictionary<string, string>> _data = new(StringComparer.OrdinalIgnoreCase);
        private readonly Dictionary<string, Dictionary<string, string>> _commentedData = new(StringComparer.OrdinalIgnoreCase);

        public void Load()
        {
            _data.Clear();
            _commentedData.Clear();
            if (!File.Exists(filePath)) return;

            string currentSection = "";
            foreach (var line in File.ReadLines(filePath))
            {
                var trimmed = line.Trim();
                if (string.IsNullOrWhiteSpace(trimmed)) continue;

                // Capture commented key=value lines (e.g. ";Driver=MyDevice") separately
                if (trimmed.StartsWith(";"))
                {
                    var commentBody = trimmed.Substring(1).Trim();
                    var commentSplit = commentBody.IndexOf('=');
                    if (commentSplit > 0 && !string.IsNullOrEmpty(currentSection))
                    {
                        var key = commentBody.Substring(0, commentSplit).Trim();
                        var val = commentBody.Substring(commentSplit + 1).Trim();
                        if (!_commentedData.ContainsKey(currentSection))
                            _commentedData[currentSection] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                        _commentedData[currentSection][key] = val;
                    }
                    continue;
                }

                if (trimmed.StartsWith("[") && trimmed.EndsWith("]"))
                {
                    currentSection = trimmed;
                    if (!_data.ContainsKey(currentSection))
                        _data[currentSection] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                    continue;
                }

                var splitIndex = trimmed.IndexOf('=');
                if (splitIndex > 0)
                {
                    var key = trimmed.Substring(0, splitIndex).Trim();
                    var val = trimmed.Substring(splitIndex + 1).Trim();

                    if (!_data.ContainsKey(currentSection))
                        _data[currentSection] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

                    _data[currentSection][key] = val;
                }
            }
        }

        public void Save()
        {
            using (var sw = new StreamWriter(filePath))
            {
                foreach (var section in _data)
                {
                    sw.WriteLine(section.Key);
                    foreach (var kvp in section.Value)
                    {
                        if (_commentedData.TryGetValue(section.Key, out var commented) && commented.ContainsKey(kvp.Key))
                            sw.WriteLine($";{kvp.Key}={commented[kvp.Key]}");
                        else
                            sw.WriteLine($"{kvp.Key}={kvp.Value}");
                    }
                    sw.WriteLine(); // Blank line for readability
                }
            }
        }

        public string GetString(string section, string key, string defaultValue = "")
        {
            if (_data.TryGetValue(section, out var sec) && sec.TryGetValue(key, out var val))
                return val;

            // Seed the default so Save() persists it, but a read must never count as a
            // change (that would fire SettingChanged and trigger a spurious save/update).
            GetOrCreateSection(_data, section)[key] = defaultValue;
            return defaultValue;
        }

        public void SetString(string section, string key, string value)
        {
            if (!_data.ContainsKey(section))
                _data[section] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            bool changed = !_data[section].TryGetValue(key, out var oldVal) || oldVal != value;
            _data[section][key] = value;

            if (changed)
                SettingChanged?.Invoke();
        }

        public bool GetBool(string section, string key, bool defaultValue = false, bool forceNumeric = false)
        {
            string def = forceNumeric ? (defaultValue ? "1" : "0") : (defaultValue ? "on" : "off");
            var str = GetString(section, key, def);
            return str.Equals("on", StringComparison.OrdinalIgnoreCase) || str == "1";
        }

        public void SetBool(string section, string key, bool value, bool forceNumeric = false)
            => SetString(section, key, value ? (forceNumeric ? "1" : "on") : (forceNumeric ? "0" : "off"));

        public int GetInt(string section, string key, int defaultValue = 0)
        {
            var str = GetString(section, key, defaultValue.ToString());
            return int.TryParse(str, out var result) ? result : defaultValue;
        }

        public void SetInt(string section, string key, int value)
        {
            SetString(section, key, value.ToString());
        }

        public string GetCommentedString(string section, string key, string defaultValue = "")
        {
            if (_commentedData.TryGetValue(section, out var sec) && sec.TryGetValue(key, out var val))
                return val;
            return defaultValue;
        }

        public void SetCommentedString(string section, string key, string value, bool commented)
        {
            if (commented)
            {
                if (!_commentedData.ContainsKey(section))
                    _commentedData[section] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                _commentedData[section][key] = value;

                if (!_data.ContainsKey(section))
                    _data[section] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                _data[section][key] = value;
            }
            else
            {
                if (_commentedData.TryGetValue(section, out var sec))
                    sec.Remove(key);
                SetString(section, key, value);
            }
        }

        public bool IsCommented(string section, string key)
            => _commentedData.TryGetValue(section, out var sec) && sec.ContainsKey(key);
    }
}