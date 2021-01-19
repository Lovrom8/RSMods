using System.IO;
using RSMods.Util;
using System.Collections.Generic;

namespace RSMods.Rocksmith
{
    class WriteSettings
    {
        public static Dictionary<string, Dictionary<string, string>> newSettings = new Dictionary<string, Dictionary<string, string>>()
        {
            // Section                  name    value
            {"[Audio]", new Dictionary<string, string>
            {
                { ReadSettings.EnableMicrophoneIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableMicrophoneIdentifier) },
                { ReadSettings.ExclusiveModeIdentifier, ReadSettings.ProcessSettings(ReadSettings.ExclusiveModeIdentifier) },
                { ReadSettings.LatencyBufferIdentifier, ReadSettings.ProcessSettings(ReadSettings.LatencyBufferIdentifier) },
                { ReadSettings.ForceDefaultPlaybackDeviceIdentifier, ReadSettings.ProcessSettings(ReadSettings.ForceDefaultPlaybackDeviceIdentifier) },
                { ReadSettings.ForceWDMIdentifier, ReadSettings.ProcessSettings(ReadSettings.ForceWDMIdentifier) },
                { ReadSettings.ForceDirectXSinkIdentifier, ReadSettings.ProcessSettings(ReadSettings.ForceDirectXSinkIdentifier) },
                { ReadSettings.DumpAudioLogIdentifier, ReadSettings.ProcessSettings(ReadSettings.DumpAudioLogIdentifier) },
                { ReadSettings.MaxOutputBufferSizeIdentifier, ReadSettings.ProcessSettings(ReadSettings.MaxOutputBufferSizeIdentifier) },
                { ReadSettings.RealToneCableOnlyIdentifier, ReadSettings.ProcessSettings(ReadSettings.RealToneCableOnlyIdentifier) },
                { ReadSettings.Win32UltraLowLatencyModeIdentifier, ReadSettings.ProcessSettings(ReadSettings.Win32UltraLowLatencyModeIdentifier) },
            }},
            {"[Renderer.Win32]", new Dictionary<string, string>
            {
                { ReadSettings.ShowGamepadUIIdentifier, ReadSettings.ProcessSettings(ReadSettings.ShowGamepadUIIdentifier) },
                { ReadSettings.ScreenWidthIdentifier, ReadSettings.ProcessSettings(ReadSettings.ScreenWidthIdentifier) },
                { ReadSettings.ScreenHeightIdentifier, ReadSettings.ProcessSettings(ReadSettings.ScreenHeightIdentifier) },
                { ReadSettings.FullscreenIdentifier, ReadSettings.ProcessSettings(ReadSettings.FullscreenIdentifier) },
                { ReadSettings.VisualQualityIdentifier, ReadSettings.ProcessSettings(ReadSettings.VisualQualityIdentifier) },
                { ReadSettings.RenderingWidthIdentifier, ReadSettings.ProcessSettings(ReadSettings.RenderingWidthIdentifier) },
                { ReadSettings.RenderingHeightIdentifier, ReadSettings.ProcessSettings(ReadSettings.RenderingHeightIdentifier) },
                { ReadSettings.EnablePostEffectsIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnablePostEffectsIdentifier) },
                { ReadSettings.EnableShadowsIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableShadowsIdentifier) },
                { ReadSettings.EnableHighResScopeIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableHighResScopeIdentifier) },
                { ReadSettings.EnableDepthOfFieldIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableDepthOfFieldIdentifier) },
                { ReadSettings.EnablePerPixelLightingIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnablePerPixelLightingIdentifier) },
                { ReadSettings.MsaaSamplesIdentifier, ReadSettings.ProcessSettings(ReadSettings.MsaaSamplesIdentifier) },
                { ReadSettings.DisableBrowserIdentifier, ReadSettings.ProcessSettings(ReadSettings.DisableBrowserIdentifier) },
            }},
            {"[Net]", new Dictionary<string, string>
            {
                { ReadSettings.UseProxyIdentifier, ReadSettings.ProcessSettings(ReadSettings.UseProxyIdentifier) },
            }},
        };

        public static void WriteINI(Dictionary<string, Dictionary<string, string>> DictionaryToWrite)
        {
            using (StreamWriter sw = File.CreateText(Path.Combine(GenUtil.GetRSDirectory(), "Rocksmith.ini")))
            {
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

        public static void SaveChanges(string IdentifierToChange, string ChangedSettingValue)
        {
            // Right before launch, we switched from the boolean names of (true / false) to (1 / 0) for users to be able to edit the mods without the GUI (by hand).
            if (ChangedSettingValue == "true")
                ChangedSettingValue = "1";
            else if (ChangedSettingValue == "false")
                ChangedSettingValue = "0";

            foreach (string section in newSettings.Keys)
            {
                foreach (KeyValuePair<string, string> entry in newSettings[section])
                {
                    if (entry.Key == IdentifierToChange)
                    {
                        newSettings[section][IdentifierToChange] = ChangedSettingValue;
                        break; // We found what we need, so let's leave.
                    }
                }
            }

            WriteINI(newSettings);
        }
    }
}
