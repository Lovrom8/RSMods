using System;
using System.Collections.Generic;
using System.Windows.Forms;
using Microsoft.Win32;
using System.Linq;

namespace RSMods.ASIO
{
    public class Devices
    {
        private const string AsioX86RegistryPath = "Software\\WOW6432Node\\ASIO";
        private const string AsioX64RegistryPath = "Software\\ASIO";

        public static List<DriverInfo> FindDevices()
        {
            List<DriverInfo> availableDevices = new List<DriverInfo>();

            try
            {
                // x86 drivers
                RegistryKey registry_ASIO_x86 = Registry.LocalMachine.OpenSubKey(AsioX86RegistryPath);

                if (registry_ASIO_x86 != null)
                {
                    List<DriverInfo> AsioDevices_x86 = ScanDevices(AsioX86RegistryPath, registry_ASIO_x86.GetSubKeyNames().ToList());
                    AsioDevices_x86.ForEach(device => availableDevices.Add(device));
                    registry_ASIO_x86.Close();
                }

                // x64 drivers
                RegistryKey registry_ASIO_x64 = Registry.LocalMachine.OpenSubKey(AsioX64RegistryPath);

                if (registry_ASIO_x64 != null)
                {
                    List<DriverInfo> AsioDevices_x64 = ScanDevices(AsioX64RegistryPath, registry_ASIO_x64.GetSubKeyNames().ToList());
                    AsioDevices_x64.ForEach(device => availableDevices.Add(device));
                    registry_ASIO_x64.Close();
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
            List<DriverInfo> availableDevices = new List<DriverInfo>();
            foreach (string asioDevice in subKeys)
            {
                // Setup variables
                DriverInfo deviceInfo = new DriverInfo();
                RegistryKey registry_device = Registry.LocalMachine.OpenSubKey($"{rootRegistryDir}\\{asioDevice}");

                // Set device information from rootRegistryDir
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

            return availableDevices;
        }

        public struct DriverInfo
        {
            public string clsID;
            public string deviceName;
            public string deviceDescription;

            public override bool Equals(object obj)
            {
                if (obj == null)
                    return false;

                DriverInfo driverInfo = (DriverInfo)obj;
                if (deviceName == driverInfo.deviceName)
                    return true;

                return false;
            }

            public override int GetHashCode()
            {
                return deviceName.GetHashCode();
            }
        }
    }
}
