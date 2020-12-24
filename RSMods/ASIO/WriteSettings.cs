using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
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
                { ReadSettings.BufferSizeModeIdentifier, ReadSettings.ProcessSettings(ReadSettings.BufferSizeModeIdentifier, ReadSettings.Sections.Asio)},
                { ReadSettings.CustomBufferSizeIdentifier, ReadSettings.ProcessSettings(ReadSettings.CustomBufferSizeIdentifier, ReadSettings.Sections.Asio)}
            }},
            {"[Asio.Output]", new Dictionary<string, string>
            {
                { ReadSettings.DriverIdentifier, ReadSettings.ProcessSettings(ReadSettings.DriverIdentifier, ReadSettings.Sections.Output)},
                { ReadSettings.BaseChannelIdentifier, ReadSettings.ProcessSettings(ReadSettings.BaseChannelIdentifier, ReadSettings.Sections.Output)},
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
            }}
        };

        public static void WriteINI(Dictionary<string, Dictionary<string, string>> DictionaryToWrite)
        {
            using (StreamWriter sw = File.CreateText(Path.Combine(GenUtil.GetRSDirectory(), "RS_ASIO.ini")))
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

        private void SaveChanges(string IdentifierToChange, string SectionToChange, string ChangedSettingValue)
        {
            // Right before launch, we switched from the boolean names of (true / false) to (on / off) for users to be able to edit the mods without the GUI (by hand).
            if (ChangedSettingValue == "true")
                ChangedSettingValue = "on";
            else if (ChangedSettingValue == "false")
                ChangedSettingValue = "off";

                foreach (KeyValuePair<string, string> entry in newSettings[SectionToChange])
                {
                    if (IdentifierToChange == entry.Key)
                    {
                        newSettings[SectionToChange][IdentifierToChange] = ChangedSettingValue;
                        break; // We found what we need, so let's leave.
                    }
                }

            WriteINI(newSettings);
        }
    }
}
