using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace RSMods
{
    public sealed class IniValidationWarning
    {
        public IniValidationWarning(string filePath, string section, string key, string rawValue, string defaultValue, string reason)
        {
            FilePath = filePath;
            Section = section;
            Key = key;
            RawValue = rawValue;
            DefaultValue = defaultValue;
            Reason = reason;
        }

        public string FilePath { get; }
        public string Section { get; }
        public string Key { get; }
        public string RawValue { get; }
        public string DefaultValue { get; }
        public string Reason { get; }
    }

    public class IniManager(string filePath)
    {
        public event Action SettingChanged;
        public event Action<IniValidationWarning> ValidationWarning;

        private readonly Dictionary<string, Dictionary<string, string>> _data = new(StringComparer.OrdinalIgnoreCase);
        private readonly Dictionary<string, Dictionary<string, string>> _commentedData = new(StringComparer.OrdinalIgnoreCase);
        private readonly Dictionary<string, string[]> _sectionComments = new(StringComparer.OrdinalIgnoreCase);

        private int _saveSuspendCount;
        private bool _saveDeferred;

        public void Load()
        {
            _data.Clear();
            _commentedData.Clear();

            try
            {
                if (!File.Exists(filePath)) return;

                string currentSection = string.Empty;

                foreach (var line in File.ReadLines(filePath))
                {
                    var trimmed = line.Trim();
                    if (string.IsNullOrWhiteSpace(trimmed)) continue;

                    if (TryParseSection(trimmed, out string newSection))
                    {
                        currentSection = newSection;
                    }
                    else if (trimmed.StartsWith(";"))
                    {
                        ParseKeyValuePair(trimmed.Substring(1), currentSection, _commentedData);
                    }
                    else
                    {
                        ParseKeyValuePair(trimmed, currentSection, _data);
                    }
                }
            }
            catch (IOException ex)
            {
                Debug.WriteLine($"Failed to load INI file: {ex.Message}");
            }
        }

        /// <summary>
        /// Suspends disk writes until the returned scope is disposed, coalescing a burst of setter-driven
        /// <see cref="Save"/> calls into a single write on dispose. Callers that set many values at once
        /// (for example a settings screen persisting its whole snapshot) use this to avoid rewriting the
        /// file once per property; the default per-set auto-save behaviour is unchanged for callers that
        /// don't opt in. Scopes nest, and a write only happens if at least one <see cref="Save"/> was
        /// requested while suspended.
        /// </summary>
        public IDisposable SuspendSave()
        {
            _saveSuspendCount++;
            return new SaveScope(this);
        }

        private void EndSuspendSave()
        {
            if (_saveSuspendCount == 0)
                return;

            _saveSuspendCount--;
            if (_saveSuspendCount == 0 && _saveDeferred)
            {
                _saveDeferred = false;
                Save();
            }
        }

        public void Save()
        {
            if (_saveSuspendCount > 0)
            {
                _saveDeferred = true;
                return;
            }

            try
            {
                using var sw = new StreamWriter(filePath);

                // _data ordering first, then any section that exists only as commented lines.
                var sectionNames = new List<string>(_data.Keys);
                foreach (var section in _commentedData.Keys)
                    if (!_data.ContainsKey(section))
                        sectionNames.Add(section);

                foreach (var sectionName in sectionNames)
                {
                    if (_sectionComments.TryGetValue(sectionName, out var headers))
                    {
                        foreach (var header in headers)
                            sw.WriteLine(header);
                    }

                    sw.WriteLine(sectionName);

                    _data.TryGetValue(sectionName, out var dataSection);
                    _commentedData.TryGetValue(sectionName, out var commentedSection);

                    if (dataSection != null)
                    {
                        foreach (var kvp in dataSection)
                            WriteKeyValuePair(sw, sectionName, kvp.Key, kvp.Value);
                    }

                    // Preserve commented-only entries that were never mirrored into _data,
                    // so a load/save round-trip doesn't silently drop them.
                    if (commentedSection != null)
                    {
                        foreach (var kvp in commentedSection)
                            if (dataSection == null || !dataSection.ContainsKey(kvp.Key))
                                sw.WriteLine($";{kvp.Key}={kvp.Value}");
                    }

                    sw.WriteLine(); // Blank line for readability
                }
            }
            catch (IOException ex)
            {
                Debug.WriteLine($"Failed to save INI file: {ex.Message}");
            }
        }

        private bool TryParseSection(string line, out string section)
        {
            if (line.StartsWith("[") && line.EndsWith("]"))
            {
                section = line;
                return true;
            }
            section = string.Empty;
            return false;
        }

        private void ParseKeyValuePair(string body, string currentSection, Dictionary<string, Dictionary<string, string>> targetDict)
        {
            if (string.IsNullOrEmpty(currentSection)) return;

            int splitIndex = body.IndexOf('=');
            if (splitIndex > 0)
            {
                var key = body.Substring(0, splitIndex).Trim();
                var val = body.Substring(splitIndex + 1).Trim();

                var sectionDict = GetOrCreateSection(targetDict, currentSection);
                sectionDict[key] = val;
            }
        }

        private void WriteKeyValuePair(StreamWriter sw, string sectionName, string key, string value)
        {
            if (_commentedData.TryGetValue(sectionName, out var commentedSection) && commentedSection.TryGetValue(key, out var commentedValue))
            {
                sw.WriteLine($";{key}={commentedValue}");
            }
            else
            {
                sw.WriteLine($"{key}={value}");
            }
        }

        private Dictionary<string, string> GetOrCreateSection(Dictionary<string, Dictionary<string, string>> target, string section)
        {
            if (!target.TryGetValue(section, out var sectionDict))
            {
                sectionDict = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                target[section] = sectionDict;
            }
            return sectionDict;
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
            var sectionDict = GetOrCreateSection(_data, section);

            bool changed = !sectionDict.TryGetValue(key, out var oldVal) || oldVal != value;
            sectionDict[key] = value;

            if (changed)
                SettingChanged?.Invoke();
        }

        public bool GetBool(string section, string key, bool defaultValue = false, bool forceNumeric = false)
        {
            string def = forceNumeric ? (defaultValue ? "1" : "0") : (defaultValue ? "on" : "off");
            var str = GetString(section, key, def);
            string normalized = str.Trim();

            if (normalized.Equals("on", StringComparison.OrdinalIgnoreCase) ||
                normalized.Equals("true", StringComparison.OrdinalIgnoreCase) ||
                normalized.Equals("yes", StringComparison.OrdinalIgnoreCase) ||
                normalized == "1")
            {
                return true;
            }

            if (normalized.Equals("off", StringComparison.OrdinalIgnoreCase) ||
                normalized.Equals("false", StringComparison.OrdinalIgnoreCase) ||
                normalized.Equals("no", StringComparison.OrdinalIgnoreCase) ||
                normalized == "0")
            {
                return false;
            }

            ReportInvalid(section, key, str, def, "not a valid boolean (expected 0/1/true/false/yes/no)");
            return defaultValue;
        }

        public void SetBool(string section, string key, bool value, bool forceNumeric = false)
            => SetString(section, key, value ? (forceNumeric ? "1" : "on") : (forceNumeric ? "0" : "off"));

        public int GetInt(string section, string key, int defaultValue = 0)
        {
            var str = GetString(section, key, defaultValue.ToString());
            if (int.TryParse(str, out var result))
                return result;

            ReportInvalid(section, key, str, defaultValue.ToString(), "not a valid integer");
            return defaultValue;
        }

        public void SetInt(string section, string key, int value)
            => SetString(section, key, value.ToString());

        public decimal GetDecimal(string section, string key, decimal defaultValue = 0)
        {
            string defaultText = defaultValue.ToString(System.Globalization.CultureInfo.InvariantCulture);
            string raw = GetString(section, key, defaultText);
            if (decimal.TryParse(raw, System.Globalization.NumberStyles.Any, System.Globalization.CultureInfo.InvariantCulture, out decimal result))
                return result;

            ReportInvalid(section, key, raw, defaultText, "not a valid number");
            return defaultValue;
        }

        public T GetEnum<T>(string section, string key, T defaultValue = default) where T : struct, Enum
        {
            string raw = GetString(section, key, defaultValue.ToString().ToLowerInvariant());
            if (Enum.TryParse(raw, ignoreCase: true, out T result) && Enum.IsDefined(typeof(T), result))
                return result;

            ReportInvalid(section, key, raw, defaultValue.ToString().ToLowerInvariant(), $"not a valid {typeof(T).Name} value");
            return defaultValue;
        }

        public T GetEnumInt<T>(string section, string key, T defaultValue = default) where T : struct, Enum
        {
            int defaultNumber = Convert.ToInt32(defaultValue);
            string raw = GetString(section, key, defaultNumber.ToString());
            if (int.TryParse(raw, out int result) && Enum.IsDefined(typeof(T), result))
                return (T)Enum.ToObject(typeof(T), result);

            ReportInvalid(section, key, raw, defaultNumber.ToString(), $"not a valid {typeof(T).Name} value");
            return defaultValue;
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
                GetOrCreateSection(_commentedData, section)[key] = value;
                GetOrCreateSection(_data, section)[key] = value;
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

        public void SetSectionComments(string section, string[] comments)
        {
            _sectionComments[section] = comments;
        }

        private void ReportInvalid(string section, string key, string rawValue, string defaultValue, string reason)
        {
            ValidationWarning?.Invoke(new IniValidationWarning(
                filePath,
                section,
                key,
                rawValue,
                defaultValue,
                reason));
        }

        private sealed class SaveScope : IDisposable
        {
            private IniManager _owner;

            public SaveScope(IniManager owner) => _owner = owner;

            public void Dispose()
            {
                // Guard against a double dispose ending one scope twice.
                IniManager owner = _owner;
                _owner = null;
                owner?.EndSuspendSave();
            }
        }
    }
}
