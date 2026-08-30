using RSMods.Data;
using System;
using System.IO;

namespace RSMods.SetAndForget
{
    /// <summary>Physical drive media type, mapped from the WMI MSFT_PhysicalDisk MediaType code.</summary>
    public enum DriveMediaType { Unspecified, Hdd, Ssd, Scm }

    /// <summary>
    /// The boundary between fast-load logic and the WMI/OS drive query, so the decision of which prompt to
    /// show and which intro asset to use can be tested without touching real hardware.
    /// </summary>
    public interface IDriveInfoProvider
    {
        (DriveMediaType Type, bool IsNVMe) GetDriveType(char driveLetter);
    }

    /// <summary>
    /// Which confirmation (if any) the UI should show before applying the fast-load mod, based on the
    /// detected drive type.
    /// </summary>
    public enum FastLoadDrivePrompt
    {
        None,            // Fast enough (SSD, non-NVMe) - no question needed.
        ConfirmHddRisk,  // HDD - warn it may crash; proceed only if confirmed.
        ConfirmNvme,     // NVMe detected - confirm before using the fastest option.
        ConfirmUnknown   // Drive type unknown - ask whether it's NVMe.
    }

    /// <summary>
    /// The Fast Load mod: detects the Rocksmith drive type (through an injected boundary), decides which
    /// confirmation the frontend should raise, and injects the matching intro asset into the cache.
    /// </summary>
    public sealed class FastLoadService(CachePsarcService cache, IDriveInfoProvider drives)
    {
        private readonly CachePsarcService _cache = cache ?? throw new ArgumentNullException(nameof(cache));
        private readonly IDriveInfoProvider _drives = drives ?? throw new ArgumentNullException(nameof(drives));

        public FastLoadDrivePrompt GetDrivePrompt()
        {
            char driveLetter = Constants.RSFolder.ToUpper()[0];
            var (driveType, isNVMe) = _drives.GetDriveType(driveLetter);

            if (driveType == DriveMediaType.Hdd) return FastLoadDrivePrompt.ConfirmHddRisk;
            if (driveType == DriveMediaType.Ssd && isNVMe) return FastLoadDrivePrompt.ConfirmNvme;
            if (driveType == DriveMediaType.Unspecified) return FastLoadDrivePrompt.ConfirmUnknown;
            return FastLoadDrivePrompt.None;
        }

        public void Apply(bool useNvmeFastLoad)
        {
            _cache.Modify(cache =>
            {
                string introAsset = useNvmeFastLoad ? Constants.IntroGFX_MaxPath : Constants.IntroGFX_MidPath;
                File.Copy(introAsset, Constants.IntroGFX_CustomPath, true);

                cache.Inject(Constants.IntroGFX_CustomPath, Constants.Cache4_7zPath, Constants.IntroGFX_InternalPath);
            });
        }
    }
}
