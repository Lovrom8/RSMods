using System.IO;
using System.Linq;
using RSMods_WPF.Pages.ModPages;

namespace RSMods_WPF
{
    public class Mod
    {
        /// <summary>
        /// Name to display in the "Mods" tab.
        /// </summary>
        public string UIName { get; }

        /// <summary>
        /// What section of the Settings File should this mod be placed in?
        /// </summary>
        public string Section { get; }

        /// <summary>
        /// What should the mod be called in the Settings File?
        /// </summary>
        public string SettingName { get; }

        /// <summary>
        /// A short description on what the mod is / does.
        /// </summary>
        public string Description { get; }

        /// <summary>
        /// PRIVATE Value used for {get;} so we can run commands on {set;}
        /// </summary>
        private object _value;

        /// <summary>
        /// <para>PUBLIC Value used to determine what the mod state is.</para>
        /// <para>When Mod State is changed, we will save it to the Settings File.</para>
        /// </summary>
        public object Value
        {
            get { return _value; }
            set {
                
                _value = value;

                if (!AlreadyInit)
                {
                    if (Settings.HasValidSettingsFile())
                    {
                        if (WhereSettingName(SettingName).Value != Value)
                            WhereSettingName(SettingName).Value = Value;

                        LoadMods.WriteSettingsFile();
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
        /// The mods default "off" value.
        /// </summary>
        public object DefaultValue { get; }

        /// <summary>
        /// Page to display when "More Info" is pressed.
        /// </summary>
        public ModPage ModPage { get; }

        /// <summary>
        /// Is the setting a mod itself (true), or just a setting for another mod (false)
        /// </summary>
        public bool ShowInModsTab { get; }


        /// <summary>
        /// Create New Mod
        /// </summary>
        /// <param name="_UIName"> - Name shown in "Mods" tab.</param>
        /// <param name="_Section"> - What section of the Settings File do you want this mod to be located in?</param>
        /// <param name="_SettingName"> - What should the setting name in the Settings File be?</param>
        /// <param name="_Description"> - A short description on what the mod is / does.</param>
        /// <param name="_DefaultValue"> - If we can't find the value, what should we default to?</param>
        /// <param name="_ShowMoreInfo"> - Is there more than just a toggle switch required?</param>
        /// <param name="_ModPage"> - Mod Page for if _ShowMoreInfo is set to true.</param>
        public Mod(string _UIName, string _Section, string _SettingName, string _Description, object _DefaultValue, ModPage _ModPage = null, bool _ShowInModsTab = true)
        {
            AlreadyInit = true;
            UIName = _UIName;
            Section = _Section;
            SettingName = _SettingName;
            Description = _Description;
            DefaultValue = _DefaultValue;
            ModPage = _ModPage;
            ShowInModsTab = _ShowInModsTab;

            if (File.Exists(Settings.SettingsFile) && File.ReadAllText(Settings.SettingsFile).Length > 0)
            {
                if (ReadPreviousSetting(_SettingName, _DefaultValue) == null) // Value not found
                {
                    Value = DefaultValue;
                    InitialValue = DefaultValue;
                }   
                else // Value found
                {
                    Value = ReadPreviousSetting(_SettingName, _DefaultValue);
                    InitialValue = Value;
                }   
            }

            LoadMods.Mods.Add(this);
        }

        /// <summary>
        /// Read Setting From Settings File
        /// </summary>
        /// <param name="SettingName"> - Name of setting in the Settings File.</param>
        /// <returns>INT (if int), STRING (if string), NULL (if not found)</returns>
        private static object ReadPreviousSetting(string SettingName, object @default)
        {
            if (LoadMods.SettingsFile_Cache.Count == 0)
            {
                bool settingExistsInSettingsFile = false;
                foreach (string line in File.ReadAllLines(Settings.SettingsFile))
                {
                    if (line.Length == 0 || line[0] == '[') // Don't cache sections
                        continue;

                    int equals = line.IndexOf(" = ");
                    LoadMods.SettingsFile_Cache.Add(line[..equals], line[(equals + " = ".Length)..]);

                    if (line[..equals] == SettingName)
                        settingExistsInSettingsFile = true;
                }

                if (!settingExistsInSettingsFile) // Mod doesn't exist in Settings File.
                    LoadMods.SettingsFile_Cache.Add(SettingName, @default);

                return ReadPreviousSetting(SettingName, @default);
            }
            else
            {
                if (!LoadMods.SettingsFile_Cache.ContainsKey(SettingName)) // Mod doesn't exist in Settings File.
                    LoadMods.SettingsFile_Cache.Add(SettingName, @default);

                object output = LoadMods.SettingsFile_Cache[SettingName];
                try
                {
                    if (output != null && int.TryParse(output.ToString(), out int intOutput))
                        return intOutput;
                    else
                        return output;
                }
                catch // INI Error. Best to just return that we don't know what the mod is.
                {
                    return null;
                }
            }
        }

        /// <summary>
        /// Get instance of mod by the Setting Name.
        /// </summary>
        /// <param name="SettingName"> - Name of setting in the Settings File.</param>
        /// <returns>Mod</returns>
        public static Mod WhereSettingName(string SettingName) => LoadMods.Mods.Where(mod => mod.SettingName == SettingName).FirstOrDefault();

        /// <summary>
        /// Get instance of mod by the UI Name.
        /// </summary>
        /// <param name="UIName"> - Name of setting in the "Mods" tab.</param>
        /// <returns>Mod</returns>
        public static Mod WhereUIName(string UIName) => LoadMods.Mods.Where(mod => mod.UIName == UIName).FirstOrDefault();
    }
}
