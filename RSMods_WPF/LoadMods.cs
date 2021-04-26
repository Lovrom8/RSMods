using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows.Controls;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace RSMods_WPF
{
    class LoadMods
    {
        /// <summary>
        /// Load Mods into a list, title LoadMods.Mods;
        /// </summary>
        public LoadMods()
        {
            Mods.Clear();

            Mods.Add(new Mod("Toggle Loft", "Toggle Switches", "ToggleLoft", "Disable the background of the game", "on", true, new Pages.ModPages.ToggleLoft()));
            Mods.Add(new Mod("Toggle Loft When", "Toggle Switches", "ToggleLoftWhen", "When should we toggle the background of the game?", "manual", _ShowInModsTab: false));
            Mods.Add(new Mod("Disable Headstock", "Toggle Switches", "Headstock", "Removes the headstock", "off"));


            WriteSettingsFile();
        }

        /// <summary>
        /// <para>A dictionary of what the Settings File says the mod should be.</para>
        /// <para>Use this for caching the Settings File so we aren't wasting time re-reading the same file over, and over again.</para>
        /// </summary>
        public static Dictionary<string, object> SettingsFile_Cache = new();

        /// <summary>
        /// A list of all the Mods.
        /// </summary>
        public static List<Mod> Mods = new();

        public static void WriteSettingsFile(Mod changedMod = null)
        {
            using (StreamWriter sw = File.CreateText(Settings.SettingsFile))
            {
                Dictionary<string, List<Mod>> splitModsIntoSections = new();

                foreach (Mod mod in Mods)
                {
                    if (splitModsIntoSections.ContainsKey(mod.Section))
                        splitModsIntoSections[mod.Section].Add(mod);
                    else
                        splitModsIntoSections.Add(mod.Section, new() { mod });
                }

                if (changedMod != null)
                    splitModsIntoSections[changedMod.Section][splitModsIntoSections[changedMod.Section].FindIndex(mod => mod.SettingName == changedMod.SettingName)] = changedMod;

                foreach (string section in splitModsIntoSections.Keys)
                {
                    sw.WriteLine("[" + section + "]");

                    foreach (Mod mod in splitModsIntoSections[section])
                    {
                        if (mod.Value == null)
                            sw.WriteLine(mod.SettingName + " = " + mod.DefaultValue);
                        else
                            sw.WriteLine(mod.SettingName + " = " + mod.Value);
                    }
                }
            }
        }
    }
}
