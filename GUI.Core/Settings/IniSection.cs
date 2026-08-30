using System;
using System.Globalization;
using System.Runtime.CompilerServices;

namespace RSMods
{
    public class IniSection(IniManager ini, string sectionName, bool numericBools = false)
    {
        private readonly IniManager _ini = ini;
        private readonly string _sectionName = sectionName;
        private readonly bool _numericBools = numericBools;

        public string GetString(string defaultValue = "", [CallerMemberName] string key = "")
            => _ini.GetString(_sectionName, key, defaultValue);

        public void SetString(string value, [CallerMemberName] string key = "")
            => _ini.SetString(_sectionName, key, value);

        public int GetInt(int defaultValue = 0, [CallerMemberName] string key = "")
            => _ini.GetInt(_sectionName, key, defaultValue);

        public void SetInt(int value, [CallerMemberName] string key = "")
            => _ini.SetInt(_sectionName, key, value);

        public decimal GetDecimal(decimal defaultValue = 0, [CallerMemberName] string key = "")
            => _ini.GetDecimal(_sectionName, key, defaultValue);

        public void SetDecimal(decimal value, [CallerMemberName] string key = "")
            => SetString(value.ToString(CultureInfo.InvariantCulture), key);

        public bool GetBool(bool defaultValue = false, [CallerMemberName] string key = "")
            => _ini.GetBool(_sectionName, key, defaultValue, _numericBools);

        public void SetBool(bool value, [CallerMemberName] string key = "")
            => _ini.SetBool(_sectionName, key, value, _numericBools);

        public T GetEnum<T>(T defaultValue = default, [CallerMemberName] string key = "") where T : struct, Enum
            => _ini.GetEnum(_sectionName, key, defaultValue);

        public void SetEnum<T>(T value, [CallerMemberName] string key = "") where T : struct, Enum
            => _ini.SetString(_sectionName, key, value.ToString().ToLowerInvariant());

        public void Save() => _ini.Save();

        public T GetEnumInt<T>(T defaultValue = default, [CallerMemberName] string key = "") where T : struct, Enum
            => _ini.GetEnumInt(_sectionName, key, defaultValue);

        public void SetEnumInt<T>(T value, [CallerMemberName] string key = "") where T : struct, Enum
            => _ini.SetInt(_sectionName, key, Convert.ToInt32(value));
    }
}
