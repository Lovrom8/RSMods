using System;
using System.Linq;
using System.Management;

namespace RSMods.SetAndForget
{
    /// <summary>
    /// Resolves a drive's physical media type via WMI (Windows 8+). Best-effort: any failure resolves to
    /// <see cref="DriveMediaType.Unspecified"/>, leaving the frontend to ask the user.
    /// </summary>
    public sealed class WmiDriveInfoProvider : IDriveInfoProvider
    {
        public (DriveMediaType Type, bool IsNVMe) GetDriveType(char driveLetter)
        {
            if (!DoesOSSupportReadingDriveTypes()) // This may not work on Win7, MSDN says its for >= Win8
                return (DriveMediaType.Unspecified, false);

            try
            {
                ManagementScope scope = new(@"\\.\root\microsoft\windows\storage");
                scope.Connect();

                uint? diskNumber = GetDiskNumber(scope, driveLetter);

                if (diskNumber == null)
                    return (DriveMediaType.Unspecified, false);

                return GetDriveSpecifications(scope, diskNumber.Value);
            }
            catch (ManagementException)
            {
                // Best effort - not much we can do in this case and it's not really important that we inform the user
                return (DriveMediaType.Unspecified, false);
            }
        }

        private static bool DoesOSSupportReadingDriveTypes()
        {
            return Environment.OSVersion.Version >= new Version(6, 2); // OS Chart here: https://stackoverflow.com/a/2819962
        }

        private static uint? GetDiskNumber(ManagementScope scope, char driveLetter)
        {
            string query = $"SELECT DiskNumber FROM MSFT_Partition WHERE DriveLetter = '{driveLetter}'";

            using var searcher = new ManagementObjectSearcher(scope, new ObjectQuery(query));
            using var results = searcher.Get();

            var partition = results.Cast<ManagementObject>().FirstOrDefault();

            if (partition == null)
                return null;

            return Convert.ToUInt32(partition["DiskNumber"]);
        }

        private static (DriveMediaType Type, bool IsNVMe) GetDriveSpecifications(ManagementScope scope, uint diskNumber)
        {
            string query = $"SELECT MediaType, BusType FROM MSFT_PhysicalDisk WHERE DeviceId = '{diskNumber}'";

            using var searcher = new ManagementObjectSearcher(scope, new ObjectQuery(query));
            using var results = searcher.Get();

            var disk = results.Cast<ManagementObject>().FirstOrDefault();

            if (disk == null)
                return (DriveMediaType.Unspecified, false);

            DriveMediaType type = Convert.ToInt16(disk["MediaType"]) switch
            {
                3 => DriveMediaType.Hdd,
                4 => DriveMediaType.Ssd,
                5 => DriveMediaType.Scm,
                _ => DriveMediaType.Unspecified,
            };

            bool isNVMe = Convert.ToInt16(disk["BusType"]) == 17;

            return (type, isNVMe);
        }
    }
}
