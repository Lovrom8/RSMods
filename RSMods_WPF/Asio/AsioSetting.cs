using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace RSMods_WPF.Asio
{
    public class AsioSetting
    {

        /// <summary>
        /// What section of the Settings File should this AsioSetting be placed in?
        /// </summary>
        public string Section { get; }

        /// <summary>
        /// What should the AsioSetting be called in the Settings File?
        /// </summary>
        public string SettingName { get; }

        /// <summary>
        /// A short description on what the AsioSetting does.
        /// </summary>
        public string Description { get; }

        /// <summary>
        /// PRIVATE Value used for {get;} so we can run commands on {set;}
        /// </summary>
        private object _value;

        /// <summary>
        /// <para>PUBLIC Value used to determine what the AsioSetting state is.</para>
        /// <para>When AsioSetting State is changed, we will save it to the Settings File.</para>
        /// </summary>
        public object Value
        {
            get { return _value; }
            set
            {

                _value = value;

                if (!AlreadyInit)
                {
                    if (Settings.HasValidAsioSettingsFile())
                    {
                        if (WhereSettingName(SettingName, Section).Value != Value)
                            WhereSettingName(SettingName, Section).Value = Value;

                        LoadAsio.WriteSettingsFile();
                    }
                }
                else
                    AlreadyInit = false;
            }
        }

        /// <summary>
        /// <para>Value used to detach DataGridCheckbox from the actual value when loading the UI.</para>
        /// <para>This should fix the checkbox getting changed twice.</para>
        /// </summary>
        public object InitialValue { get; set; }

        /// <summary>
        /// Prevents stack overflow loop.
        /// </summary>
        private bool AlreadyInit = false;

        /// <summary>
        /// The AsioSetting default "off" value.
        /// </summary>
        public object DefaultValue { get; }

        /// <summary>
        /// Create New AsioSetting
        /// </summary>
        /// <param name="_Section"> - What section of the Settings File do you want this AsioSetting to be located in?</param>
        /// <param name="_SettingName"> - What should the setting name in the Settings File be?</param>
        /// <param name="_Description"> - A short description on what the AsioSetting is / does.</param>
        /// <param name="_DefaultValue"> - If we can't find the value, what should we default to?</param>
        public AsioSetting(string _Section, string _SettingName, string _Description, object _DefaultValue)
        {
            AlreadyInit = true;
            Section = _Section;
            SettingName = _SettingName;
            Description = _Description;
            DefaultValue = _DefaultValue;

            if (Settings.HasValidAsioSettingsFile())
            {
                if (ReadPreviousSetting(_SettingName, _Section, _DefaultValue) == null) // Value not found
                {
                    Value = DefaultValue;
                    InitialValue = DefaultValue;
                }
                else // Value found
                {
                    Value = ReadPreviousSetting(_SettingName, _Section, _DefaultValue);
                    InitialValue = Value;
                }
            }

            LoadAsio.AsioSettings.Add(this);
        }

        /// <summary>
        /// Read Setting From Settings File
        /// </summary>
        /// <param name="SettingName"> - Name of setting in the Settings File.</param>
        /// <returns>INT (if int), STRING (if string), NULL (if not found)</returns>
        private static object ReadPreviousSetting(string SettingName, string SectionName, object @default)
        {
            if (LoadAsio.SettingsFile_Cache.Count == 0)
            {
                bool settingExistsInSettingsFile = false;
                string section = string.Empty;
                Dictionary<string, object> currentSection = new Dictionary<string, object>();

                foreach (string line in File.ReadAllLines(Settings.AsioSettingsFile))
                {
                    string currentLine = line;
                    if (currentLine.Length == 0)
                        continue;

                    if (currentLine[0] == '[')
                    {
                        if (currentSection.Count > 0)
                        {
                            LoadAsio.SettingsFile_Cache.Add(section, currentSection);
                            currentSection = new Dictionary<string, object>();
                        }
                        
                        section = currentLine[..currentLine.IndexOf(']')].Replace("[", "");
                        continue;
                    }

                    if (currentLine[0] == ';') // Commented out.
                    {
                        if (!currentLine.Contains("="))
                        {
                            continue;
                        }
                        else if (currentLine.Contains("Driver"))
                        {
                            LoadAsio.DisabledDevices.Add(section);
                            currentLine = currentLine.Remove(0, 1); // Remove the comment so we read the setting.
                        }
                    }

                    int equals = currentLine.IndexOf("=");
                    currentSection.Add(currentLine[..equals], currentLine[(equals + "=".Length)..]);

                    if (currentLine[..equals] == SettingName && section == SectionName)
                        settingExistsInSettingsFile = true;
                }

                LoadAsio.SettingsFile_Cache.Add(section, currentSection); // Get last section into our cache.

                bool couldHaveDeviceDisabled = SettingName != "Driver" && LoadAsio.DisabledDevices.Contains(SectionName);
                if (!settingExistsInSettingsFile && !couldHaveDeviceDisabled) // AsioSetting doesn't exist in Settings File.
                    LoadAsio.SettingsFile_Cache[SectionName].Add(SettingName, @default);

                return ReadPreviousSetting(SettingName, SectionName, @default);
            }
            else
            {
                if (!LoadAsio.SettingsFile_Cache.ContainsKey(SectionName)) // Asio Section does not exist in the Settings File.
                    LoadAsio.SettingsFile_Cache.Add(SectionName, new Dictionary<string, object>());

                if (!LoadAsio.SettingsFile_Cache[SectionName].ContainsKey(SettingName)) // AsioSetting doesn't exist in Settings File.
                    LoadAsio.SettingsFile_Cache[SectionName].Add(SettingName, @default);

                object output = LoadAsio.SettingsFile_Cache[SectionName][SettingName];
                try
                {
                    return output;
                }
                catch // INI Error. Best to just return that we don't know what the AsioSetting is.
                {
                    return null;
                }
            }
        }

        /// <summary>
        /// Get instance of AsioSetting by the Setting Name.
        /// </summary>
        /// <param name="SettingName"> - Name of setting in the Settings File.</param>
        /// <param name="SectionName"> - Name of the section that contains the setting</param>
        /// <returns>AsioSetting</returns>
        public static AsioSetting WhereSettingName(string SettingName, string SectionName) => LoadAsio.AsioSettings.Where(asioSetting => asioSetting.SettingName == SettingName && asioSetting.Section == SectionName).FirstOrDefault();
    }
}
