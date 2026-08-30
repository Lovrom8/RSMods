using System;
using System.Collections.Generic;
using System.Windows.Forms;
using Microsoft.Win32;
using System.Linq;

namespace RSMods.ASIO
{
    public static class Devices
    {
        private const string AsioX86RegistryPath = "Software\\WOW6432Node\\ASIO";
        private const string AsioX64RegistryPath = "Software\\ASIO";

        public static List<DriverInfo> FindDevices()
        {
            var availableDevices = new List<DriverInfo>();
            var registryPaths = new[] { AsioX86RegistryPath, AsioX64RegistryPath };

            try
            {
                foreach (var path in registryPaths)
                {
                    using RegistryKey registryKey = Registry.LocalMachine.OpenSubKey(path);
                    if (registryKey != null)
                    {
                        var subKeyNames = registryKey.GetSubKeyNames().ToList();
                        var devices = ScanDevices(path, subKeyNames);
                        availableDevices.AddRange(devices);
                    }
                }
            }
            catch (NullReferenceException ex)
            {
                MessageBox.Show($"ASIO Error: {ex.Message}", "ASIO Error");
            }

            // Return the devices, but remove any duplicate entries.
            // Like when there is a x86, and a x64 driver with the same name.
            // RS_ASIO will combine these into one entry, so we only need to store one.
            return availableDevices.Distinct().ToList();
        }

        private static List<DriverInfo> ScanDevices(string rootRegistryDir, List<string> subKeys)
        {
            List<DriverInfo> availableDevices = [];
            foreach (string asioDevice in subKeys)
            {
                DriverInfo deviceInfo = new();
                RegistryKey registry_device = Registry.LocalMachine.OpenSubKey($"{rootRegistryDir}\\{asioDevice}");

                deviceInfo.clsID = (string)registry_device.GetValue("CLSID");
                deviceInfo.deviceDescription = (string)registry_device.GetValue("Description");
                deviceInfo.deviceName = asioDevice;

                registry_device.Close();

                // Verify we have a real device and not just a fake key
                if (deviceInfo.clsID == null || deviceInfo.deviceDescription == null || deviceInfo.deviceName == null)
                    continue;

                availableDevices.Add(deviceInfo);
            }

            return availableDevices;
        }

        public struct DriverInfo
        {
            public string clsID;
            public string deviceName;
            public string deviceDescription;

            public override readonly bool Equals(object obj)
            {
                if (obj == null)
                    return false;

                DriverInfo driverInfo = (DriverInfo)obj;
                return deviceName == driverInfo.deviceName;
            }

            public override readonly int GetHashCode() => deviceName.GetHashCode();
        }
    }
}
