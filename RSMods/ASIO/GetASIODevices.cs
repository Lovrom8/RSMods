using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Win32;

namespace RSMods.ASIO
{
    public class Devices
    {
        public static List<DriverInfo> FindDevices()
        {
            List<DriverInfo> availableDevices = new List<DriverInfo>();

            RegistryKey registry_ASIO = Registry.LocalMachine.OpenSubKey("Software\\ASIO");

            if (registry_ASIO == null)
                return availableDevices;

            string[] subKeyNames = registry_ASIO.GetSubKeyNames();

            foreach (string asioDevice in subKeyNames)
            {
                // Setup variables
                DriverInfo deviceInfo = new DriverInfo();
                RegistryKey registry_device = Registry.LocalMachine.OpenSubKey($"Software\\ASIO\\{asioDevice}");

                // Set device information from Software\ASIO
                deviceInfo.clsID = (string)registry_device.GetValue("CLSID");
                deviceInfo.deviceDescription = (string)registry_device.GetValue("Description");
                deviceInfo.deviceName = asioDevice;

                registry_device.Close();

                // Verify we have a real device and not just a fake key
                if (deviceInfo.clsID == null || deviceInfo.deviceDescription == null || deviceInfo.deviceName == null)
                    continue;

                // Put device into list
                availableDevices.Add(deviceInfo);
            }

            registry_ASIO.Close();
            return availableDevices;
        }

        public struct DriverInfo
        {
            public string clsID;
            public string deviceName;
            public string deviceDescription;
        }
    }
}
