using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Threading;

namespace RSMods
{
    public sealed class IniValidationWarning(string filePath, string section, string key, string rawValue, string defaultValue, string reason, bool valueKept = false)
    {
        public string FilePath { get; } = filePath;
        public string Section { get; } = section;
        public string Key { get; } = key;
        public string RawValue { get; } = rawValue;
        public string DefaultValue { get; } = defaultValue;
        public string Reason { get; } = reason;

        /// <summary>True when the file keeps the raw value (only the default is shown) rather than being corrected.</summary>
        public bool ValueKept { get; } = valueKept;
    }

    /// <summary>
    /// Reads and writes one INI file, keeping the file's own lines: comments, unknown sections and keys,
    /// duplicates and spacing are written back as they were, and only a line whose value changes is rewritten.
    /// </summary>
    /// <param name="fillDefaults">
    /// When true, reading a missing key adds its default to the file and an invalid value is corrected to its
    /// default. When false (a file another program owns), both only show the default: the file changes only
    /// where a value is set to something other than what was shown.
    /// </param>
    public class IniManager(string filePath, bool fillDefaults = true)
    {
        public event Action<IniValidationWarning> ValidationWarning;

        private sealed class Line
        {
            public string Raw;      // Null once the value changes, so the line is regenerated.
            public string Key;      // Null for comments, blank lines and anything else that isn't key=value.
            public string Value;
            public bool Commented;

            public string Text => Raw ?? $"{(Commented ? ";" : "")}{Key}={Value}";
        }

        private sealed class Block
        {
            public string Name;     // Null for the lines before the first section.
            public string Header;   // The header as the file wrote it; null for a section this class created.
            public readonly List<Line> Lines = [];
        }

        private readonly List<Block> _blocks = [new Block()];

        // The line each key reads from: the last active one, and separately the last commented-out one.
        private readonly Dictionary<string, Dictionary<string, Line>> _data = new(StringComparer.OrdinalIgnoreCase);
        private readonly Dictionary<string, Dictionary<string, Line>> _commentedData = new(StringComparer.OrdinalIgnoreCase);
        private readonly Dictionary<string, string[]> _sectionComments = new(StringComparer.OrdinalIgnoreCase);

        // Without fillDefaults: the default shown for a missing or invalid key, so setting that same value back isn't a change.
        private readonly Dictionary<string, string> _shownDefaults = new(StringComparer.OrdinalIgnoreCase);

        private string _newLine = Environment.NewLine;

        // Screens save from the thread pool while the UI thread reads and writes values (a read can seed a
        // default), so every access goes through this. Events fire after it's released.
        private readonly object _gate = new();

        private int _saveSuspendCount;
        private bool _saveDeferred;
        private bool _changedSinceSave; // A value was set to something new; reading or seeding a default doesn't count.
        private bool _fileOutOfDate;    // Anything the file on disk lacks, seeded defaults and corrections included.

        public void Load()
        {
            lock (_gate)
                LoadLocked();
        }

        private void LoadLocked()
        {
            _blocks.Clear();
            _data.Clear();
            _commentedData.Clear();
            _shownDefaults.Clear();
            _newLine = Environment.NewLine;
            _changedSinceSave = false;
            _fileOutOfDate = false;

            var block = new Block();
            _blocks.Add(block);

            try
            {
                if (!File.Exists(filePath)) return;

                string text = File.ReadAllText(filePath);
                if (text.Contains('\n') && !text.Contains("\r\n"))
                    _newLine = "\n";

                using var reader = new StringReader(text);
                for (string line = reader.ReadLine(); line != null; line = reader.ReadLine())
                {
                    var trimmed = line.Trim();

                    if (TryParseSection(trimmed, out string newSection))
                    {
                        block = new Block { Name = newSection, Header = line };
                        _blocks.Add(block);
                        continue;
                    }

                    var entry = new Line { Raw = line };
                    block.Lines.Add(entry);

                    // Key lines before the first section belong to no section, so they're only kept, never read.
                    if (block.Name == null || trimmed.Length == 0)
                        continue;

                    entry.Commented = trimmed.StartsWith(';');
                    if (TrySplitKeyValue(entry.Commented ? trimmed.Substring(1) : trimmed, out entry.Key, out entry.Value))
                        GetOrCreateSection(entry.Commented ? _commentedData : _data, block.Name)[entry.Key] = entry;
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
        /// file once per property; the default per-set auto-save behavior is unchanged for callers that
        /// don't opt in. Scopes nest, and a write only happens if at least one <see cref="Save"/> was
        /// requested while suspended.
        /// </summary>
        public IDisposable SuspendSave()
        {
            lock (_gate)
                _saveSuspendCount++;
            return new SaveScope(this);
        }

        private void EndSuspendSave()
        {
            lock (_gate)
            {
                if (_saveSuspendCount == 0)
                    return;

                _saveSuspendCount--;
                if (_saveSuspendCount == 0 && _saveDeferred)
                {
                    _saveDeferred = false;
                    WriteLocked(); // Leaves _changedSinceSave for the next Save() to report.
                }
            }
        }

        /// <summary>
        /// Writes the file. Throws <see cref="IOException"/> when it can't, so the caller can report it.
        /// Returns whether any value changed since the last save, so a caller can skip telling the game to reload.
        /// </summary>
        public bool Save()
        {
            lock (_gate)
            {
                if (_saveSuspendCount > 0)
                {
                    _saveDeferred = true;
                    return false;
                }

                WriteLocked();

                bool changed = _changedSinceSave;
                _changedSinceSave = false;
                return changed;
            }
        }

        /// <summary>True when memory holds something the file doesn't, even if it's only a seeded default.</summary>
        public bool FileOutOfDate
        {
            get
            {
                lock (_gate)
                    return _fileOutOfDate;
            }
        }

        // The DLL rereads RSMods.ini whenever it likes, and a crash mid-write used to leave it truncated, so the
        // new contents go to a temporary file that's swapped in whole.
        private void WriteLocked()
        {
            string temporary = filePath + ".tmp";
            try
            {
                using (var stream = new FileStream(temporary, FileMode.Create, FileAccess.Write, FileShare.None))
                using (var sw = new StreamWriter(stream))
                {
                    WriteContents(sw);
                    sw.Flush();
                    stream.Flush(flushToDisk: true);
                }
            }
            catch (UnauthorizedAccessException)
            {
                // A folder that lets us edit the file but not create one next to it: write in place as before.
                using (var sw = new StreamWriter(filePath))
                    WriteContents(sw);
                _fileOutOfDate = false;
                return;
            }

            try
            {
                ReplaceWithRetry(temporary);
                _fileOutOfDate = false;
            }
            catch
            {
                try { File.Delete(temporary); } catch { }
                throw;
            }
        }

        private void ReplaceWithRetry(string temporary)
        {
            for (int attempt = 1; ; attempt++)
            {
                try
                {
                    if (File.Exists(filePath))
                        File.Replace(temporary, filePath, null);
                    else
                        File.Move(temporary, filePath);
                    return;
                }
                catch (IOException) when (attempt < 5)
                {
                    Thread.Sleep(50); // The game opens the file without allowing a replace, but only while it reads it.
                }
            }
        }

        private void WriteContents(StreamWriter sw)
        {
            sw.NewLine = _newLine;

            foreach (var block in _blocks)
            {
                if (block.Name != null)
                {
                    // A file's own section keeps whatever comments it has; only a section created here gets ours.
                    if (block.Header == null && _sectionComments.TryGetValue(block.Name, out var headers))
                    {
                        foreach (var header in headers)
                            sw.WriteLine(header);
                    }

                    sw.WriteLine(block.Header ?? block.Name);
                }

                foreach (var line in block.Lines)
                    sw.WriteLine(line.Text);
            }
        }

        private static bool TryParseSection(string line, out string section)
        {
            if (line.StartsWith('[') && line.EndsWith(']'))
            {
                section = line;
                return true;
            }
            section = string.Empty;
            return false;
        }

        private static bool TrySplitKeyValue(string body, out string key, out string value)
        {
            int splitIndex = body.IndexOf('=');
            if (splitIndex > 0)
            {
                key = body.Substring(0, splitIndex).Trim();
                value = body.Substring(splitIndex + 1).Trim();
                return key.Length > 0;
            }

            key = value = null;
            return false;
        }

        private static Dictionary<string, Line> GetOrCreateSection(Dictionary<string, Dictionary<string, Line>> target, string section)
        {
            if (!target.TryGetValue(section, out var sectionDict))
            {
                sectionDict = new Dictionary<string, Line>(StringComparer.OrdinalIgnoreCase);
                target[section] = sectionDict;
            }
            return sectionDict;
        }

        private static Line Find(Dictionary<string, Dictionary<string, Line>> source, string section, string key)
            => source.TryGetValue(section, out var sec) && sec.TryGetValue(key, out var line) ? line : null;

        private Block GetOrCreateBlock(string section)
        {
            for (int i = _blocks.Count - 1; i >= 0; i--)
            {
                if (string.Equals(_blocks[i].Name, section, StringComparison.OrdinalIgnoreCase))
                    return _blocks[i];
            }

            // Keep a blank line between the previous section and this one, as the file would have been written.
            var last = _blocks[^1];
            if ((last.Name != null || last.Lines.Count > 0) && (last.Lines.Count == 0 || last.Lines[^1].Text.Trim().Length > 0))
                last.Lines.Add(new Line { Raw = "" });

            var block = new Block { Name = section };
            block.Lines.Add(new Line { Raw = "" });
            _blocks.Add(block);
            return block;
        }

        private Line AddLine(string section, string key, string value, bool commented)
        {
            var block = GetOrCreateBlock(section);

            // After the section's last key, so the comments and blank lines that lead into the next section stay there.
            int index = block.Lines.FindLastIndex(l => l.Key != null) + 1;
            var line = new Line { Key = key, Value = value, Commented = commented };
            block.Lines.Insert(index, line);
            GetOrCreateSection(commented ? _commentedData : _data, section)[key] = line;
            return line;
        }

        private void MarkChanged()
        {
            _changedSinceSave = true;
            _fileOutOfDate = true;
        }

        private static string ShownDefaultId(string section, string key) => section + "\n" + key;

        public string GetString(string section, string key, string defaultValue = "")
        {
            lock (_gate)
            {
                var line = Find(_data, section, key);
                if (line != null)
                    return line.Value;

                if (fillDefaults)
                {
                    // Seed the default so Save() persists it, but a read must never count as a
                    // change (that would trigger a spurious save and game reload).
                    AddLine(section, key, defaultValue, commented: false);
                    _fileOutOfDate = true;
                }
                else
                {
                    _shownDefaults[ShownDefaultId(section, key)] = defaultValue;
                }

                return defaultValue;
            }
        }

        /// <summary>Reads a key's active value without adding its default.</summary>
        public bool TryGetString(string section, string key, out string value)
        {
            lock (_gate)
            {
                value = Find(_data, section, key)?.Value;
                return value != null;
            }
        }

        public void SetString(string section, string key, string value)
        {
            lock (_gate)
            {
                string id = ShownDefaultId(section, key);
                if (_shownDefaults.TryGetValue(id, out string shown))
                {
                    if (shown == value)
                        return; // The screen is saving back the default it showed; the file keeps what it has.
                    _shownDefaults.Remove(id);
                }

                SetActiveLocked(section, key, value);
            }
        }

        private void SetActiveLocked(string section, string key, string value)
        {
            var line = Find(_data, section, key);
            if (line == null)
            {
                AddLine(section, key, value, commented: false);
                MarkChanged();
            }
            else if (line.Value != value)
            {
                line.Value = value;
                line.Raw = null;
                MarkChanged();
            }
        }

        /// <summary>Deletes every active line of the key in the section; commented-out ones stay.</summary>
        public void RemoveKey(string section, string key)
        {
            lock (_gate)
            {
                if (_data.TryGetValue(section, out var sec) && sec.Remove(key))
                {
                    foreach (var block in _blocks)
                    {
                        if (string.Equals(block.Name, section, StringComparison.OrdinalIgnoreCase))
                            block.Lines.RemoveAll(l => !l.Commented && string.Equals(l.Key, key, StringComparison.OrdinalIgnoreCase));
                    }
                    MarkChanged();
                }
                _shownDefaults.Remove(ShownDefaultId(section, key));
            }
        }

        public bool GetBool(string section, string key, bool defaultValue = false, bool forceNumeric = false)
        {
            string def = forceNumeric ? (defaultValue ? "1" : "0") : (defaultValue ? "on" : "off");
            var str = GetString(section, key, def);
            string normalized = str.Trim();

            if (normalized.Length == 0)
                return defaultValue;

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
            if (string.IsNullOrWhiteSpace(str))
                return defaultValue;

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
            if (string.IsNullOrWhiteSpace(raw))
                return defaultValue;

            ReportInvalid(section, key, raw, defaultText, "not a valid number");
            return defaultValue;
        }

        public T GetEnum<T>(string section, string key, T defaultValue = default) where T : struct, Enum
        {
            string raw = GetString(section, key, defaultValue.ToString().ToLowerInvariant());
            if (Enum.TryParse(raw, ignoreCase: true, out T result) && Enum.IsDefined(typeof(T), result))
                return result;
            if (string.IsNullOrWhiteSpace(raw))
                return defaultValue;

            ReportInvalid(section, key, raw, defaultValue.ToString().ToLowerInvariant(), $"not a valid {typeof(T).Name} value");
            return defaultValue;
        }

        public T GetEnumInt<T>(string section, string key, T defaultValue = default) where T : struct, Enum
        {
            int defaultNumber = Convert.ToInt32(defaultValue);
            string raw = GetString(section, key, defaultNumber.ToString());
            if (int.TryParse(raw, out int result) && Enum.IsDefined(typeof(T), result))
                return (T)Enum.ToObject(typeof(T), result);
            if (string.IsNullOrWhiteSpace(raw))
                return defaultValue;

            ReportInvalid(section, key, raw, defaultNumber.ToString(), $"not a valid {typeof(T).Name} value");
            return defaultValue;
        }

        public string GetCommentedString(string section, string key, string defaultValue = "")
        {
            lock (_gate)
                return Find(_commentedData, section, key)?.Value ?? defaultValue;
        }

        /// <summary>
        /// Comments the key out (<paramref name="commented"/> true) or back in, with the given value. The line
        /// changes in place; other commented-out lines of the same key, such as alternatives a user keeps, stay.
        /// </summary>
        public void SetCommentedString(string section, string key, string value, bool commented)
        {
            lock (_gate)
            {
                _shownDefaults.Remove(ShownDefaultId(section, key));

                var active = Find(_data, section, key);
                var inactive = Find(_commentedData, section, key);

                if (commented)
                {
                    if (active != null)
                    {
                        _data[section].Remove(key);
                        GetOrCreateSection(_commentedData, section)[key] = active;
                        active.Commented = true;
                        active.Value = value;
                        active.Raw = null;
                        MarkChanged();
                    }
                    else if (inactive == null)
                    {
                        AddLine(section, key, value, commented: true);
                        MarkChanged();
                    }
                    else if (inactive.Value != value)
                    {
                        inactive.Value = value;
                        inactive.Raw = null;
                        MarkChanged();
                    }
                }
                else if (active == null && inactive != null)
                {
                    _commentedData[section].Remove(key);
                    GetOrCreateSection(_data, section)[key] = inactive;
                    inactive.Commented = false;
                    inactive.Value = value;
                    inactive.Raw = null;
                    MarkChanged();
                }
                else
                {
                    SetActiveLocked(section, key, value);
                }
            }
        }

        /// <summary>True when the key is only present commented out; an active line of it wins over any comment.</summary>
        public bool IsCommented(string section, string key)
        {
            lock (_gate)
                return Find(_data, section, key) == null && Find(_commentedData, section, key) != null;
        }

        /// <summary>Comment lines written above the section when this class creates it; a file's own section keeps its own.</summary>
        public void SetSectionComments(string section, string[] comments)
        {
            lock (_gate)
                _sectionComments[section] = comments;
        }

        // A blank value isn't reported: it is how RS_ASIO's own RS_ASIO.ini leaves a setting at its default, so the
        // typed getters read it as the default and leave the file as it is.
        private void ReportInvalid(string section, string key, string rawValue, string defaultValue, string reason)
        {
            lock (_gate)
            {
                if (fillDefaults)
                {
                    // Self-heal: overwrite the invalid raw value with the default in memory so the next
                    // Save() persists the correction.
                    var line = Find(_data, section, key);
                    line.Value = defaultValue;
                    line.Raw = null;
                    _fileOutOfDate = true;
                }
                else
                {
                    _shownDefaults[ShownDefaultId(section, key)] = defaultValue;
                }
            }

            ValidationWarning?.Invoke(new IniValidationWarning(
                filePath,
                section,
                key,
                rawValue,
                defaultValue,
                reason,
                valueKept: !fillDefaults));
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
