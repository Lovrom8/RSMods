using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Windows.Forms;
using RSMods.Util;

namespace RSMods.ASIO
{
    public class ReadSettings
    {
        private static string EnableWasapiOutputs, EnableWasapiInputs, EnableAsio,
        BufferSizeMode, CustomBufferSize,
        OUTPUT_Driver, OUTPUT_BaseChannel, OUTPUT_EnableSoftwareEndpointVolumeControl, OUTPUT_EnableSoftwareMasterVolumeControl, OUTPUT_SoftwareMasterVolumePercent,
        INPUT0_Driver, INPUT0_Channel, INPUT0_EnableSoftwareEndpointVolumeControl, INPUT0_EnableSoftwareMasterVolumeControl, INPUT0_SoftwareMasterVolumePercent,
        INPUT1_Driver, INPUT1_Channel, INPUT1_EnableSoftwareEndpointVolumeControl, INPUT1_EnableSoftwareMasterVolumeControl, INPUT1_SoftwareMasterVolumePercent;

        public static string EnableWasapiOutputsIdentifier = "EnableWasapiOutputs=",
        EnableWasapiInputsIdentifier = "EnableWasapiInputs=",
        EnableAsioIdentifier = "EnableAsio=",

        BufferSizeModeIdentifier = "BufferSizeMode=",
        CustomBufferSizeIdentifier = "CustomBufferSize=",

        DriverIdentifier = "Driver=",
        BaseChannelIdentifier = "BaseChannel=",
        ChannelIdentifier = "Channel=",
        EnableSoftwareEndpointVolumeControlIdentifier = "EnableSoftwareEndpointVolumeControl=",
        EnableSoftwareMasterVolumeControlIdentifier = "EnableSoftwareMasterVolumeControl=",
        SoftwareMasterVolumePercentIdentifier = "SoftwareMasterVolumePercent=";

        public enum Sections
        {
            Output = 0,
            Input0 = 1,
            Input1 = 2,
            Config = 3,
            Asio = 4
        };

        private static string lastKnownSection = "";

        public static string SectionToName(Sections section)
        {
            switch(section)
            {
                case Sections.Output:
                    return "[Asio.Output]";
                case Sections.Input0:
                    return "[Asio.Input.0]";
                case Sections.Input1:
                    return "[Asio.Input.1]";
                case Sections.Config:
                    return "[Config]";
                case Sections.Asio:
                    return "[Asio]";
            }
            return "";
        }

        private static string FillSettingVariable(string settingIdentifier, Sections section, string currentLine, out string setting)
        {
            setting = "";

            if (currentLine.Contains(settingIdentifier) )
                setting = currentLine.Substring(settingIdentifier.Length, (currentLine.Length - settingIdentifier.Length));
            return setting;
        }

        private static bool IdentifierIsFound(string currentLine, string settingToFind, string identifierToGrab) => currentLine.Contains(settingToFind) && settingToFind == identifierToGrab;

        public static bool VerifySettingsExist()
        {
            if (!File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "RS_ASIO.ini")))
                return false;
            return true;
        }

        public static string ProcessSettings(string identifierToGrab, Sections sectionToGrab)
        {
            VerifySettingsExist();
            foreach (string currentLine in File.ReadLines(Path.Combine(GenUtil.GetRSDirectory(), "RS_ASIO.ini")))
            {

                if (currentLine.Length > 0)
                {
                    if (currentLine[0] == '[') // Section
                        lastKnownSection = currentLine;
                    else if (currentLine[0] == ';') // Commented out
                        continue;
                }
                    

                #region Config
                    // Config
                if (sectionToGrab == Sections.Config && lastKnownSection == SectionToName(Sections.Config))
                {
                    if (IdentifierIsFound(currentLine, EnableWasapiOutputsIdentifier, identifierToGrab))
                        return FillSettingVariable(EnableWasapiOutputsIdentifier, sectionToGrab, currentLine, out EnableWasapiOutputs);
                    if (IdentifierIsFound(currentLine, EnableWasapiInputsIdentifier, identifierToGrab))
                        return FillSettingVariable(EnableWasapiInputsIdentifier, sectionToGrab, currentLine, out EnableWasapiInputs);
                    if (IdentifierIsFound(currentLine, EnableAsioIdentifier, identifierToGrab))
                        return FillSettingVariable(EnableAsioIdentifier, sectionToGrab, currentLine, out EnableAsio);
                }
                #endregion
                #region Asio
                // Asio

                if (sectionToGrab == Sections.Asio && lastKnownSection == SectionToName(Sections.Asio))
                {
                    if (IdentifierIsFound(currentLine, BufferSizeModeIdentifier, identifierToGrab))
                        return FillSettingVariable(BufferSizeModeIdentifier, sectionToGrab, currentLine, out BufferSizeMode);
                    if (IdentifierIsFound(currentLine, CustomBufferSizeIdentifier, identifierToGrab))
                        return FillSettingVariable(CustomBufferSizeIdentifier, sectionToGrab, currentLine, out CustomBufferSize);
                }
                #endregion
                #region Output
                // Output

                if (sectionToGrab == Sections.Output && lastKnownSection == SectionToName(Sections.Output))
                {
                    if (IdentifierIsFound(currentLine, DriverIdentifier, identifierToGrab))
                        return FillSettingVariable(DriverIdentifier, sectionToGrab, currentLine, out OUTPUT_Driver);
                    if (IdentifierIsFound(currentLine, BaseChannelIdentifier, identifierToGrab))
                        return FillSettingVariable(BaseChannelIdentifier, sectionToGrab, currentLine, out OUTPUT_BaseChannel);
                    if (IdentifierIsFound(currentLine, EnableSoftwareEndpointVolumeControlIdentifier, identifierToGrab))
                        return FillSettingVariable(EnableSoftwareEndpointVolumeControlIdentifier, sectionToGrab, currentLine, out OUTPUT_EnableSoftwareEndpointVolumeControl);
                    if (IdentifierIsFound(currentLine, EnableSoftwareMasterVolumeControlIdentifier, identifierToGrab))
                        return FillSettingVariable(EnableSoftwareMasterVolumeControlIdentifier, sectionToGrab, currentLine, out OUTPUT_EnableSoftwareMasterVolumeControl);
                    if (IdentifierIsFound(currentLine, SoftwareMasterVolumePercentIdentifier, identifierToGrab))
                        return FillSettingVariable(SoftwareMasterVolumePercentIdentifier, sectionToGrab, currentLine, out OUTPUT_SoftwareMasterVolumePercent);
                }
                #endregion
                #region Input0
                // Input0

                else if (sectionToGrab == Sections.Input0 && lastKnownSection == SectionToName(Sections.Input0))
                {
                    if (IdentifierIsFound(currentLine, DriverIdentifier, identifierToGrab))
                        return FillSettingVariable(DriverIdentifier, sectionToGrab, currentLine, out INPUT0_Driver);
                    if (IdentifierIsFound(currentLine, ChannelIdentifier, identifierToGrab))
                        return FillSettingVariable(ChannelIdentifier, sectionToGrab, currentLine, out INPUT0_Channel);
                    if (IdentifierIsFound(currentLine, EnableSoftwareEndpointVolumeControlIdentifier, identifierToGrab))
                        return FillSettingVariable(EnableSoftwareEndpointVolumeControlIdentifier, sectionToGrab, currentLine, out INPUT0_EnableSoftwareEndpointVolumeControl);
                    if (IdentifierIsFound(currentLine, EnableSoftwareMasterVolumeControlIdentifier, identifierToGrab))
                        return FillSettingVariable(EnableSoftwareMasterVolumeControlIdentifier, sectionToGrab, currentLine, out INPUT0_EnableSoftwareMasterVolumeControl);
                    if (IdentifierIsFound(currentLine, SoftwareMasterVolumePercentIdentifier, identifierToGrab))
                        return FillSettingVariable(SoftwareMasterVolumePercentIdentifier, sectionToGrab, currentLine, out INPUT0_SoftwareMasterVolumePercent);
                }
                #endregion
                #region Input1
                // Input1

                else if (sectionToGrab == Sections.Input1 && lastKnownSection == SectionToName(Sections.Input1))
                {
                    if (IdentifierIsFound(currentLine, DriverIdentifier, identifierToGrab))
                        return FillSettingVariable(DriverIdentifier, sectionToGrab, currentLine, out INPUT1_Driver);
                    if (IdentifierIsFound(currentLine, ChannelIdentifier, identifierToGrab))
                        return FillSettingVariable(ChannelIdentifier, sectionToGrab, currentLine, out INPUT1_Channel);
                    if (IdentifierIsFound(currentLine, EnableSoftwareEndpointVolumeControlIdentifier, identifierToGrab))
                        return FillSettingVariable(EnableSoftwareEndpointVolumeControlIdentifier, sectionToGrab, currentLine, out INPUT1_EnableSoftwareEndpointVolumeControl);
                    if (IdentifierIsFound(currentLine, EnableSoftwareMasterVolumeControlIdentifier, identifierToGrab))
                        return FillSettingVariable(EnableSoftwareMasterVolumeControlIdentifier, sectionToGrab, currentLine, out INPUT1_EnableSoftwareMasterVolumeControl);
                    if (IdentifierIsFound(currentLine, SoftwareMasterVolumePercentIdentifier, identifierToGrab))
                        return FillSettingVariable(SoftwareMasterVolumePercentIdentifier, sectionToGrab, currentLine, out INPUT1_SoftwareMasterVolumePercent);
                }
                #endregion
            }
            return "";
                
        }
    }
}
