using System.Collections.Generic;
using System.IO;

namespace RSMods_WPF.Asio
{
    class LoadAsio
    {
        /// <summary>
        /// Load Asio settings into a list, title LoadAsio.Settings;
        /// </summary>
        public LoadAsio()
        {
            AsioSettings.Clear();

            // Config
            _ = new AsioSetting("Config", "EnableWasapiOutputs", "Enable this if you have headphones that don't go through your audio interface.", "0");
            _ = new AsioSetting("Config", "EnableWasapiInputs", "Enable this if you want to play with a USB cable AND your audio interface in multiplayer", "0");
            _ = new AsioSetting("Config", "EnableAsio", "This is the main reason people use RS_ASIO.\nEnable this if you have an audio interface to potentially lower latency.", "1");

            // Asio
            _ = new AsioSetting("Asio", "BufferSizeMode", "Use this to box to change how much latency there is.\nThe lower you go, you have a higher chance of getting crackling audio.", "driver");
            _ = new AsioSetting("Asio", "CustomBufferSize", "Set the buffer size if the BufferSizeMode is set to custom.", "");

            // Asio.Output
            _ = new AsioSetting("Asio.Output", "Driver", "A list of all connected ASIO devices.\nClick to save the selected device as your Output device (headphones)", "");
            _ = new AsioSetting("Asio.Output", "BaseChannel", "This is what channel we want to send the audio to.\nA good way to find this value is to see how many outputs are before and subtract 1.\nEx: My headphones is plugged into the 2nd output, so my base channel is 1.", "0");
            _ = new AsioSetting("Asio.Output", "AltBaseChannel", "Requires RS_ASIO v0.5.6 or greater to work.\nThis is what channel we want to send mirrored audio to.\nA good way to find this value is to see how many outputs are before and subtract 1.\nEx: My headphones is plugged into the 2nd output, so my base channel is 1.", "");
            _ = new AsioSetting("Asio.Output", "EnableSoftwareEndpointVolumeControl", "The EndpointVolume API enables specialized clients to control\nand monitor the volume levels of audio endpoint devices.", "1");
            _ = new AsioSetting("Asio.Output", "EnableSoftwareMasterVolumeControl", "The MasterVolume Control allows the user to set how loud they want their ASIO device.", "1");
            _ = new AsioSetting("Asio.Output", "SoftwareMasterVolumePercent", "The volume the user wants to have their ASIO device at.", "100");

            // Asio.Input.0
            _ = new AsioSetting("Asio.Input.0", "Driver", "A list of all connected ASIO devices.\nClick to save the selected device as your Input0 device (guitar / bass | Player 1)", "");
            _ = new AsioSetting("Asio.Input.0", "Channel", "This is what channel we look for audio on.\nA good way to find this value is to see how many inputs are before and subtract 1.\nEx: My cable is plugged into the 2nd input, so my channel is 1.", "0");
            _ = new AsioSetting("Asio.Input.0", "EnableSoftwareEndpointVolumeControl", "The EndpointVolume API enables specialized clients to control\nand monitor the volume levels of audio endpoint devices.", "1");
            _ = new AsioSetting("Asio.Input.0", "EnableSoftwareMasterVolumeControl", "The MasterVolume Control allows the user to set how loud they want their ASIO device.", "1");
            _ = new AsioSetting("Asio.Input.0", "SoftwareMasterVolumePercent", "The volume the user wants to have their ASIO device at.", "100");

            // Asio.Input.1
            _ = new AsioSetting("Asio.Input.1", "Driver", "A list of all connected ASIO devices.\nClick to save the selected device as your Input1 device (guitar / bass | Player 2)", "");
            _ = new AsioSetting("Asio.Input.1", "Channel", "This is what channel we look for audio on.\nA good way to find this value is to see how many inputs are before and subtract 1.\nEx: My cable is plugged into the 2nd input, so my channel is 1.", "0");
            _ = new AsioSetting("Asio.Input.1", "EnableSoftwareEndpointVolumeControl", "The EndpointVolume API enables specialized clients to control\nand monitor the volume levels of audio endpoint devices.", "1");
            _ = new AsioSetting("Asio.Input.1", "EnableSoftwareMasterVolumeControl", "The MasterVolume Control allows the user to set how loud they want their ASIO device.", "1");
            _ = new AsioSetting("Asio.Input.1", "SoftwareMasterVolumePercent", "The volume the user wants to have their ASIO device at.", "100");

            // Asio.Input.Mic
            _ = new AsioSetting("Asio.Input.Mic", "Driver", "A list of all connected ASIO devices.\nClick to save the selected device as your InputMic device (Singing)", "");
            _ = new AsioSetting("Asio.Input.Mic", "Channel", "This is what channel we look for audio on.\nA good way to find this value is to see how many inputs are before and subtract 1.\nEx: My cable is plugged into the 2nd input, so my channel is 1.", "");
            _ = new AsioSetting("Asio.Input.Mic", "EnableSoftwareEndpointVolumeControl", "The EndpointVolume API enables specialized clients to control\nand monitor the volume levels of audio endpoint devices.", "1");
            _ = new AsioSetting("Asio.Input.Mic", "EnableSoftwareMasterVolumeControl", "The MasterVolume Control allows the user to set how loud they want their ASIO device.", "1");
            _ = new AsioSetting("Asio.Input.Mic", "SoftwareMasterVolumePercent", "The volume the user wants to have their ASIO device at.", "100");

            WriteSettingsFile();
        }

        /// <summary>
        /// <para>A dictionary of what the Settings File says the mod should be.</para>
        /// <para>Use this for caching the Settings File so we aren't wasting time re-reading the same file over, and over again.</para>
        /// </summary>
        public static Dictionary<string, Dictionary<string, object>> SettingsFile_Cache = new();

        /// <summary>
        /// A list of ASIO devices in use by the user, and if they are disabled.
        /// </summary>
        public static List<string> DisabledDevices = new();

        /// <summary>
        /// A list of all the Mods.
        /// </summary>
        public static List<AsioSetting> AsioSettings = new();

        public static void WriteSettingsFile(AsioSetting changedAsioSetting = null)
        {
            using StreamWriter sw = File.CreateText(Settings.AsioSettingsFile);
            Dictionary<string, List<AsioSetting>> splitSettingsIntoSections = new();

            foreach (AsioSetting asioSetting in AsioSettings)
            {
                if (splitSettingsIntoSections.ContainsKey(asioSetting.Section))
                    splitSettingsIntoSections[asioSetting.Section].Add(asioSetting);
                else
                    splitSettingsIntoSections.Add(asioSetting.Section, new() { asioSetting });
            }

            if (changedAsioSetting != null)
                splitSettingsIntoSections[changedAsioSetting.Section][splitSettingsIntoSections[changedAsioSetting.Section].FindIndex(asioSetting => asioSetting.SettingName == asioSetting.SettingName)] = changedAsioSetting;


            bool wroteAsioHelp = false;
            foreach (string section in splitSettingsIntoSections.Keys)
            {
                sw.WriteLine("[" + section + "]");

                foreach (AsioSetting asioSetting in splitSettingsIntoSections[section])
                {
                    if (section == "Asio" && !wroteAsioHelp)
                    {
                        sw.WriteLine("; available buffer size modes:");
                        sw.WriteLine(";    driver - respect buffer size setting set in the driver");
                        sw.WriteLine(";    host   - use a buffer size as close as possible as that requested by the host application");
                        sw.WriteLine(";    custom - use the buffer size specified in CustomBufferSize field");
                        wroteAsioHelp = true;
                    }

                    if (asioSetting.SettingName == "Driver" && DisabledDevices.Contains(asioSetting.Section)) // User wants to have the input or output disabled.
                    {
                        sw.WriteLine(";" + asioSetting.SettingName + "=" + asioSetting.Value);
                        continue;
                    }

                    if (asioSetting.Value == null)
                        sw.WriteLine(asioSetting.SettingName + "=" + asioSetting.DefaultValue);
                    else
                        sw.WriteLine(asioSetting.SettingName + "=" + asioSetting.Value);
                }
                sw.WriteLine(""); // Write blank line between sections.
            }
        }
    }
}
