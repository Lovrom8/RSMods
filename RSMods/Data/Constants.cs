using System.IO;
using System.Windows.Forms;

namespace RSMods.Data
{
    public static class Constants
    {
        private static string _rsFolder;

        public static string RSFolder
        {
            get { return _rsFolder; }
            set { _rsFolder = value; }
        }
        public static string SettingsPath { get { return Path.Combine(Path.GetDirectoryName(Application.ExecutablePath), "GUI_Settings.ini"); } }
        public static string CachePsarcPath { get { return Path.Combine(RSFolder, "cache.psarc"); } }
        public static string WorkFolder { get { return Path.Combine(Path.GetDirectoryName(Application.ExecutablePath), "Temp"); } }
        public static string CachePcPath { get { return Path.Combine(WorkFolder, "cache.psarc_RS2014_Pc"); } }
        public static string Cache4_7zPath { get { return Path.Combine(CachePcPath, "cache4.7z"); } }
        public static string Cache7_7zPath { get { return Path.Combine(CachePcPath, "cache7.7z"); } }
        public static string LocalizationCSV_InternalPath { get { return Path.Combine("localization", "maingame.csv"); } }
        public static string TuningsJSON_InternalPath { get { return Path.Combine("manifests", "tuning.database.json"); } }
        public static string CustomModsFolder { get { return Path.Combine(Path.GetDirectoryName(Application.ExecutablePath), "CustomMods"); } }
        public static string LocalizationCSV_CustomPath { get { return Path.Combine(CustomModsFolder, "maingame.csv"); } }
        public static string TuningJSON_CustomPath { get { return Path.Combine(CustomModsFolder, "tuning.database.json"); } }
        public static string IntroGFX_InternalPath { get { return Path.Combine("gfxassets", "views", "introsequence.gfx"); } }
        public static string IntroGFX_CustomPath { get { return Path.Combine(CustomModsFolder, "introsequence.gfx"); } }
        public static string IntroGFX_MaxPath { get { return Path.Combine(CustomModsFolder, "introsequence_max.gfx"); } }
        public static string IntroGFX_MidPath { get { return Path.Combine(CustomModsFolder, "introsequence_mid.gfx"); } }
        public static string CacheBackupPath { get { return Path.Combine(RSFolder, "cache.bak"); } }
        public static string ExtendedMenuJson_CustomPath { get { return Path.Combine(CustomModsFolder, "ui_menu_pillar_mission.database.json"); } }
        public static string ExtendedMenuJson_InternalPath { get { return Path.Combine("manifests", "ui_menu_pillar_mission.database.json"); } }
        public static string MainMenuJson_CustomPath { get { return Path.Combine(CustomModsFolder, "ui_menu_pillar_main.database.json"); } }
        public static string MainMenuJson_InternalPath { get { return Path.Combine("manifests", "ui_menu_pillar_main.database.json"); } }
        public static string ToneManager_InternalPath { get { return Path.Combine("manifests", "tonemanager.database.json"); } }
        public static string ToneManager_CustomPath { get { return Path.Combine(CustomModsFolder, "tonemanager.database.json"); } }
    }
}
