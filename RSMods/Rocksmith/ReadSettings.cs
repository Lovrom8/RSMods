using System.IO;
using RSMods.Util;

#pragma warning disable IDE0052

namespace RSMods.Rocksmith
{
    class ReadSettings
    {
        private static string
            EnableMicrophone, ExclusiveMode, LatencyBuffer, ForceDefaultPlaybackDevice, ForceWDM, ForceDirectXSink, DumpAudioLog, MaxOutputBufferSize, RealToneCableOnly, Win32UltraLowLatencyMode,
            ShowGamepadUI, ScreenWidth, ScreenHeight, Fullscreen, VisualQuality, RenderingWidth, RenderingHeight, EnablePostEffects, EnableShadows, EnableHighResScope, EnableDepthOfField, EnablePerPixelLighting, MsaaSamples, DisableBrowser,
            UseProxy;

        public static string
            EnableMicrophoneIdentifier = "EnableMicrophone=",
            ExclusiveModeIdentifier = "ExclusiveMode=",
            LatencyBufferIdentifier = "LatencyBuffer=",
            ForceDefaultPlaybackDeviceIdentifier = "ForceDefaultPlaybackDevice=",
            ForceWDMIdentifier = "ForceWDM=",
            ForceDirectXSinkIdentifier = "ForceDirectXSink=",
            DumpAudioLogIdentifier = "DumpAudioLog=",
            MaxOutputBufferSizeIdentifier = "MaxOutputBufferSize=",
            RealToneCableOnlyIdentifier = "RealToneCableOnly=",
            Win32UltraLowLatencyModeIdentifier = "Win32UltraLowLatencyMode=",

            ShowGamepadUIIdentifier = "ShowGamepadUI=",
            ScreenWidthIdentifier = "ScreenWidth=",
            ScreenHeightIdentifier = "ScreenHeight=",
            FullscreenIdentifier = "Fullscreen=",
            VisualQualityIdentifier = "VisualQuality=",
            RenderingWidthIdentifier = "RenderingWidth=",
            RenderingHeightIdentifier = "RenderingHeight=",
            EnablePostEffectsIdentifier = "EnablePostEffects=",
            EnableShadowsIdentifier = "EnableShadows=",
            EnableHighResScopeIdentifier = "EnableHighResScope=",
            EnableDepthOfFieldIdentifier = "EnableDepthOfField=",
            EnablePerPixelLightingIdentifier = "EnablePerPixelLighting=",
            MsaaSamplesIdentifier = "MsaaSamples=",
            DisableBrowserIdentifier = "DisableBrowser=",

            UseProxyIdentifier = "UseProxy=";

        private static string FillSettingVariable(string settingIdentifier, string currentLine, out string setting)
        {
            setting = "";
            if (currentLine.Contains(settingIdentifier))
                setting = currentLine.Substring(settingIdentifier.Length, (currentLine.Length - settingIdentifier.Length));
            return setting;
        }

        private static bool IdentifierIsFound(string currentLine, string settingToFind, string identifierToGrab) => currentLine.Contains(settingToFind) && settingToFind == identifierToGrab;

        public static bool VerifySettingsINI()
        {
            if (!File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "Rocksmith.ini")))
                return false;
            return true;
        }


        public static string ProcessSettings(string identifierToGrab)
        {
            VerifySettingsINI();
            foreach (string currentLine in File.ReadLines(Path.Combine(GenUtil.GetRSDirectory(), "Rocksmith.ini")))
            {
                #region Audio
                if (IdentifierIsFound(currentLine, EnableMicrophoneIdentifier, identifierToGrab))
                    return FillSettingVariable(EnableMicrophoneIdentifier, currentLine, out EnableMicrophone);
                if (IdentifierIsFound(currentLine, ExclusiveModeIdentifier, identifierToGrab))
                    return FillSettingVariable(ExclusiveModeIdentifier, currentLine, out ExclusiveMode);
                if (IdentifierIsFound(currentLine, LatencyBufferIdentifier, identifierToGrab))
                    return FillSettingVariable(LatencyBufferIdentifier, currentLine, out LatencyBuffer);
                if (IdentifierIsFound(currentLine, ForceDefaultPlaybackDeviceIdentifier, identifierToGrab))
                    return FillSettingVariable(ForceDefaultPlaybackDeviceIdentifier, currentLine, out ForceDefaultPlaybackDevice);
                if (IdentifierIsFound(currentLine, ForceWDMIdentifier, identifierToGrab))
                    return FillSettingVariable(ForceWDMIdentifier, currentLine, out ForceWDM);
                if (IdentifierIsFound(currentLine, ForceDirectXSinkIdentifier, identifierToGrab))
                    return FillSettingVariable(ForceDirectXSinkIdentifier, currentLine, out ForceDirectXSink);
                if (IdentifierIsFound(currentLine, DumpAudioLogIdentifier, identifierToGrab))
                    return FillSettingVariable(DumpAudioLogIdentifier, currentLine, out DumpAudioLog);
                if (IdentifierIsFound(currentLine, MaxOutputBufferSizeIdentifier, identifierToGrab))
                    return FillSettingVariable(MaxOutputBufferSizeIdentifier, currentLine, out MaxOutputBufferSize);
                if (IdentifierIsFound(currentLine, RealToneCableOnlyIdentifier, identifierToGrab))
                    return FillSettingVariable(RealToneCableOnlyIdentifier, currentLine, out RealToneCableOnly);
                if (IdentifierIsFound(currentLine, Win32UltraLowLatencyModeIdentifier, identifierToGrab))
                    return FillSettingVariable(Win32UltraLowLatencyModeIdentifier, currentLine, out Win32UltraLowLatencyMode);
                #endregion
                #region Renderer.Win32
                if (IdentifierIsFound(currentLine, ShowGamepadUIIdentifier, identifierToGrab))
                    return FillSettingVariable(ShowGamepadUIIdentifier, currentLine, out ShowGamepadUI);
                if (IdentifierIsFound(currentLine, ScreenWidthIdentifier, identifierToGrab))
                    return FillSettingVariable(ScreenWidthIdentifier, currentLine, out ScreenWidth);
                if (IdentifierIsFound(currentLine, ScreenHeightIdentifier, identifierToGrab))
                    return FillSettingVariable(ScreenHeightIdentifier, currentLine, out ScreenHeight);
                if (IdentifierIsFound(currentLine, FullscreenIdentifier, identifierToGrab))
                    return FillSettingVariable(FullscreenIdentifier, currentLine, out Fullscreen);
                if (IdentifierIsFound(currentLine, VisualQualityIdentifier, identifierToGrab))
                    return FillSettingVariable(VisualQualityIdentifier, currentLine, out VisualQuality);
                if (IdentifierIsFound(currentLine, RenderingWidthIdentifier, identifierToGrab))
                    return FillSettingVariable(RenderingWidthIdentifier, currentLine, out RenderingWidth);
                if (IdentifierIsFound(currentLine, RenderingHeightIdentifier, identifierToGrab))
                    return FillSettingVariable(RenderingHeightIdentifier, currentLine, out RenderingHeight);
                if (IdentifierIsFound(currentLine, EnablePostEffectsIdentifier, identifierToGrab))
                    return FillSettingVariable(EnablePostEffectsIdentifier, currentLine, out EnablePostEffects);
                if (IdentifierIsFound(currentLine, EnableShadowsIdentifier, identifierToGrab))
                    return FillSettingVariable(EnableShadowsIdentifier, currentLine, out EnableShadows);
                if (IdentifierIsFound(currentLine, EnableHighResScopeIdentifier, identifierToGrab))
                    return FillSettingVariable(EnableHighResScopeIdentifier, currentLine, out EnableHighResScope);
                if (IdentifierIsFound(currentLine, EnableDepthOfFieldIdentifier, identifierToGrab))
                    return FillSettingVariable(EnableDepthOfFieldIdentifier, currentLine, out EnableDepthOfField);
                if (IdentifierIsFound(currentLine, EnablePerPixelLightingIdentifier, identifierToGrab))
                    return FillSettingVariable(EnablePerPixelLightingIdentifier, currentLine, out EnablePerPixelLighting);
                if (IdentifierIsFound(currentLine, MsaaSamplesIdentifier, identifierToGrab))
                    return FillSettingVariable(MsaaSamplesIdentifier, currentLine, out MsaaSamples);
                if (IdentifierIsFound(currentLine, DisableBrowserIdentifier, identifierToGrab))
                    return FillSettingVariable(DisableBrowserIdentifier, currentLine, out DisableBrowser);
                #endregion
                #region Net
                if (IdentifierIsFound(currentLine, UseProxyIdentifier, identifierToGrab))
                    return FillSettingVariable(UseProxyIdentifier, currentLine, out UseProxy);
                #endregion
            }
            return "";
        }
    }
}
