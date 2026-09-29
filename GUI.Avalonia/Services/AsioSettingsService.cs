using System.Collections.Generic;
using System.IO;
using System.Linq;
using RSMods.ASIO;
using RSMods.Data;

namespace RSMods.Services;

/// <summary>
/// Owns the single <see cref="AsioSettings"/> instance, built lazily from the resolved install folder,
/// and exposes the frontend-neutral ASIO device enumeration for the RS_ASIO screen.
/// </summary>
internal sealed class AsioSettingsService
{
    // RS_ASIO only runs with all three beside the game: Rocksmith loads avrt.dll, which loads RS_ASIO.dll, which reads RS_ASIO.ini.
    private static readonly string[] RequiredFiles = ["avrt.dll", "RS_ASIO.dll", AsioSettings.DefaultFileName];

    private AsioSettings? _settings;

    public AsioSettings Get() => _settings ??= new AsioSettings(
        Path.Combine(Constants.RSFolder, AsioSettings.DefaultFileName));

    /// <summary>The RS_ASIO files missing from the Rocksmith folder; empty when it is fully installed.</summary>
    public IReadOnlyList<string> FindMissingFiles() =>
        RequiredFiles.Where(file => !File.Exists(Path.Combine(Constants.RSFolder, file))).ToList();

    public IReadOnlyList<string> FindDeviceNames()
    {
        var names = new List<string>();
        foreach (var device in Devices.FindDevices())
        {
            if (!string.IsNullOrEmpty(device.deviceName))
                names.Add(device.deviceName);
        }
        return names;
    }
}
