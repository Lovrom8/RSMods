using System.Collections.Generic;
using System.IO;
using RSMods.Util;

namespace RSMods.ASIO
{
    public class WriteSettings
    {
        public static Dictionary<string, Dictionary<string, string>> newSettings = new Dictionary<string, Dictionary<string, string>>()
        {
            // Section                   name   value
            {"[Config]", new Dictionary<string, string>
            {
                { ReadSettings.EnableWasapiOutputsIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableWasapiOutputsIdentifier, ReadSettings.Sections.Config)},
                { ReadSettings.EnableWasapiInputsIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableWasapiInputsIdentifier, ReadSettings.Sections.Config)},
                { ReadSettings.EnableAsioIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableAsioIdentifier, ReadSettings.Sections.Config) },
            }},
            {"[Asio]", new Dictionary<string, string>
            {
                {"; available buffer size modes:", "" },
                {";    driver - respect buffer size setting set in the driver", ""},
                {";    host   - use a buffer size as close as possible as that requested by the host application", ""},
                {";    custom - use the buffer size specified in CustomBufferSize field", ""},
                { ReadSettings.BufferSizeModeIdentifier, ReadSettings.ProcessSettings(ReadSettings.BufferSizeModeIdentifier, ReadSettings.Sections.Asio)},
                { ReadSettings.CustomBufferSizeIdentifier, ReadSettings.ProcessSettings(ReadSettings.CustomBufferSizeIdentifier, ReadSettings.Sections.Asio)}
            }},
            {"[Asio.Output]", new Dictionary<string, string>
            {
                { ReadSettings.DriverIdentifier, ReadSettings.ProcessSettings(ReadSettings.DriverIdentifier, ReadSettings.Sections.Output)},
                { ReadSettings.BaseChannelIdentifier, ReadSettings.ProcessSettings(ReadSettings.BaseChannelIdentifier, ReadSettings.Sections.Output)},
                { ReadSettings.AltBaseChannelIdentifier, ReadSettings.ProcessSettings(ReadSettings.AltBaseChannelIdentifier, ReadSettings.Sections.Output)},
                { ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ReadSettings.Sections.Output)},
                { ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ReadSettings.Sections.Output)},
                { ReadSettings.SoftwareMasterVolumePercentIdentifier, ReadSettings.ProcessSettings(ReadSettings.SoftwareMasterVolumePercentIdentifier, ReadSettings.Sections.Output)},
            }},
            {"[Asio.Input.0]", new Dictionary<string, string>
            {
                { ReadSettings.DriverIdentifier, ReadSettings.ProcessSettings(ReadSettings.DriverIdentifier, ReadSettings.Sections.Input0)},
                { ReadSettings.ChannelIdentifier, ReadSettings.ProcessSettings(ReadSettings.ChannelIdentifier, ReadSettings.Sections.Input0)},
                { ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ReadSettings.Sections.Input0)},
                { ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ReadSettings.Sections.Input0)},
                { ReadSettings.SoftwareMasterVolumePercentIdentifier, ReadSettings.ProcessSettings(ReadSettings.SoftwareMasterVolumePercentIdentifier, ReadSettings.Sections.Input0)},
            }},
            {"[Asio.Input.1]", new Dictionary<string, string>
            {
                { ReadSettings.DriverIdentifier, ReadSettings.ProcessSettings(ReadSettings.DriverIdentifier, ReadSettings.Sections.Input1)},
                { ReadSettings.ChannelIdentifier, ReadSettings.ProcessSettings(ReadSettings.ChannelIdentifier, ReadSettings.Sections.Input1)},
                { ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ReadSettings.Sections.Input1)},
                { ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ReadSettings.Sections.Input1)},
                { ReadSettings.SoftwareMasterVolumePercentIdentifier, ReadSettings.ProcessSettings(ReadSettings.SoftwareMasterVolumePercentIdentifier, ReadSettings.Sections.Input1)},
            }},
            {"[Asio.Input.Mic]", new Dictionary<string, string>
            {
                { ReadSettings.DriverIdentifier, ReadSettings.ProcessSettings(ReadSettings.DriverIdentifier, ReadSettings.Sections.InputMic)},
                { ReadSettings.ChannelIdentifier, ReadSettings.ProcessSettings(ReadSettings.ChannelIdentifier, ReadSettings.Sections.InputMic)},
                { ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ReadSettings.Sections.InputMic)},
                { ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ReadSettings.ProcessSettings(ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ReadSettings.Sections.InputMic)},
                { ReadSettings.SoftwareMasterVolumePercentIdentifier, ReadSettings.ProcessSettings(ReadSettings.SoftwareMasterVolumePercentIdentifier, ReadSettings.Sections.InputMic)},
            }},
        };

        private static void WriteINI(Dictionary<string, Dictionary<string, string>> DictionaryToWrite, bool disableOutput = false, bool disableInput0 = false, bool disableInput1 = false, bool disableInputMic = false)
        {
            using (StreamWriter sw = File.CreateText(Path.Combine(GenUtil.GetRSDirectory(), "RS_ASIO.ini")))
            {
                foreach (string section in DictionaryToWrite.Keys)
                {
                    sw.WriteLine(section);
                    foreach (KeyValuePair<string, string> entry in DictionaryToWrite[section])
                    {
                        if (((section == ReadSettings.SectionToName(ReadSettings.Sections.Output) && disableOutput) || (section == ReadSettings.SectionToName(ReadSettings.Sections.Input0) && disableInput0) || (section == ReadSettings.SectionToName(ReadSettings.Sections.InputMic) && disableInputMic)) && entry.Key == ReadSettings.DriverIdentifier)
                            sw.WriteLine(entry.Key);
                        else if ((section == ReadSettings.SectionToName(ReadSettings.Sections.Input1) && disableInput1) && entry.Key == ReadSettings.DriverIdentifier)
                            sw.WriteLine(';' + entry.Key + entry.Value);
                        else
                            sw.WriteLine(entry.Key + entry.Value);
                    }
                    sw.WriteLine();
                }
            }
        }

        public static void SaveChanges(string IdentifierToChange, ReadSettings.Sections iniSection, string ChangedSettingValue, bool disableOutput = false, bool disableInput0 = false, bool disableInput1 = false, bool disableInputMic = false)
        {
            // Right before launch, we switched from the boolean names of (true / false) to (on / off) for users to be able to edit the mods without the GUI (by hand).
            if (ChangedSettingValue == "true")
                ChangedSettingValue = "on";
            else if (ChangedSettingValue == "false")
                ChangedSettingValue = "off";

            foreach (string section in newSettings.Keys)
            {
                foreach (KeyValuePair<string, string> entry in newSettings[section])
                {
                    if (IdentifierToChange == entry.Key && ReadSettings.SectionToName(iniSection) == section)
                    {
                        newSettings[section][IdentifierToChange] = ChangedSettingValue;
                        break; // We found what we need, so let's leave.
                    }
                }
            }

            WriteINI(newSettings, disableOutput, disableInput0, disableInput1, disableInputMic);
        }
    }
}
