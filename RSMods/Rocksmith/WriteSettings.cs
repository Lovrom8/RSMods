using System.IO;
using RSMods.Util;
using System.Collections.Generic;

namespace RSMods.Rocksmith
{
    class WriteSettings
    {
        public static Dictionary<string, Dictionary<string, string>> settings = new Dictionary<string, Dictionary<string, string>>()
        {
            // Section                  name    value
            {"[Audio]", new Dictionary<string, string>
            {
                { ReadSettings.EnableMicrophoneIdentifier, CreateDefaultOnOldINI(ReadSettings.EnableMicrophoneIdentifier, "1") },
                { ReadSettings.ExclusiveModeIdentifier, CreateDefaultOnOldINI(ReadSettings.ExclusiveModeIdentifier, "1") },
                { ReadSettings.LatencyBufferIdentifier, CreateDefaultOnOldINI(ReadSettings.LatencyBufferIdentifier, "4") },
                { ReadSettings.ForceDefaultPlaybackDeviceIdentifier, CreateDefaultOnOldINI(ReadSettings.ForceDefaultPlaybackDeviceIdentifier, "") },
                { ReadSettings.ForceWDMIdentifier, CreateDefaultOnOldINI(ReadSettings.ForceWDMIdentifier, "0") },
                { ReadSettings.ForceDirectXSinkIdentifier, CreateDefaultOnOldINI(ReadSettings.ForceDirectXSinkIdentifier, "0") },
                { ReadSettings.DumpAudioLogIdentifier, CreateDefaultOnOldINI(ReadSettings.DumpAudioLogIdentifier, "0") },
                { ReadSettings.MaxOutputBufferSizeIdentifier, CreateDefaultOnOldINI(ReadSettings.MaxOutputBufferSizeIdentifier, "0") },
                { ReadSettings.RealToneCableOnlyIdentifier, CreateDefaultOnOldINI(ReadSettings.RealToneCableOnlyIdentifier, "0") },
                { ReadSettings.Win32UltraLowLatencyModeIdentifier, CreateDefaultOnOldINI(ReadSettings.Win32UltraLowLatencyModeIdentifier, "1") },
            }},
            {"[Renderer.Win32]", new Dictionary<string, string>
            {
                { ReadSettings.ShowGamepadUIIdentifier, CreateDefaultOnOldINI(ReadSettings.ShowGamepadUIIdentifier, "0") },
                { ReadSettings.ScreenWidthIdentifier, CreateDefaultOnOldINI(ReadSettings.ScreenWidthIdentifier, "0") },
                { ReadSettings.ScreenHeightIdentifier, CreateDefaultOnOldINI(ReadSettings.ScreenHeightIdentifier, "0") },
                { ReadSettings.FullscreenIdentifier, CreateDefaultOnOldINI(ReadSettings.FullscreenIdentifier, "2") },
                { ReadSettings.VisualQualityIdentifier, CreateDefaultOnOldINI(ReadSettings.VisualQualityIdentifier, "1") },
                { ReadSettings.RenderingWidthIdentifier, CreateDefaultOnOldINI(ReadSettings.RenderingWidthIdentifier, "0") },
                { ReadSettings.RenderingHeightIdentifier, CreateDefaultOnOldINI(ReadSettings.RenderingHeightIdentifier, "0") },
                { ReadSettings.EnablePostEffectsIdentifier, CreateDefaultOnOldINI(ReadSettings.EnablePostEffectsIdentifier, "1") },
                { ReadSettings.EnableShadowsIdentifier, CreateDefaultOnOldINI(ReadSettings.EnableShadowsIdentifier, "1") },
                { ReadSettings.EnableHighResScopeIdentifier, CreateDefaultOnOldINI(ReadSettings.EnableHighResScopeIdentifier, "1") },
                { ReadSettings.EnableDepthOfFieldIdentifier, CreateDefaultOnOldINI(ReadSettings.EnableDepthOfFieldIdentifier, "1") },
                { ReadSettings.EnablePerPixelLightingIdentifier, CreateDefaultOnOldINI(ReadSettings.EnablePerPixelLightingIdentifier, "1") },
                { ReadSettings.MsaaSamplesIdentifier, CreateDefaultOnOldINI(ReadSettings.MsaaSamplesIdentifier, "4") },
                { ReadSettings.DisableBrowserIdentifier, CreateDefaultOnOldINI(ReadSettings.DisableBrowserIdentifier, "0") },
            }},
            {"[Net]", new Dictionary<string, string>
            {
                { ReadSettings.UseProxyIdentifier, CreateDefaultOnOldINI(ReadSettings.UseProxyIdentifier, "1") },
            }},
        };

        public static void WriteINI(Dictionary<string, Dictionary<string, string>> DictionaryToWrite)
        {
            using (StreamWriter sw = File.CreateText(Path.Combine(GenUtil.GetRSDirectory(), "Rocksmith.ini")))
            {
                if (DictionaryToWrite == null)
                    return;

                foreach (string section in DictionaryToWrite.Keys)
                {
                    sw.WriteLine(section);
                    foreach (KeyValuePair<string, string> entry in DictionaryToWrite[section])
                    {
                        sw.WriteLine(entry.Key + entry.Value);
                    }
                }
            }
        }

        private static string CreateDefaultOnOldINI(string settingIdentifier, string @default) => ReadSettings.DoesSettingsINIExist() && ReadSettings.ProcessSettings(settingIdentifier) != string.Empty ? ReadSettings.ProcessSettings(settingIdentifier) : @default;

        public static void SaveChanges(string IdentifierToChange, string ChangedSettingValue)
        {
            // Right before launch, we switched from the boolean names of (true / false) to (1 / 0) for users to be able to edit the mods without the GUI (by hand).
            if (ChangedSettingValue == "true")
                ChangedSettingValue = "1";
            else if (ChangedSettingValue == "false")
                ChangedSettingValue = "0";

            foreach (string section in settings.Keys)
            {
                foreach (KeyValuePair<string, string> entry in settings[section])
                {
                    if (entry.Key == IdentifierToChange)
                    {
                        settings[section][IdentifierToChange] = ChangedSettingValue;
                        break; // We found what we need, so let's leave.
                    }
                }
            }

            WriteINI(settings);
        }
    }
}
