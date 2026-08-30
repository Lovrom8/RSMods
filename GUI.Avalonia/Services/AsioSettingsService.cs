using System.Collections.Generic;
using System.IO;
using RSMods.ASIO;
using RSMods.Data;

namespace RSMods.Services;

/// <summary>
/// Owns the single <see cref="AsioSettings"/> instance, built lazily from the resolved install folder,
/// and exposes the frontend-neutral ASIO device enumeration for the RS_ASIO screen.
/// </summary>
internal sealed class AsioSettingsService
{
    private AsioSettings? _settings;

    public AsioSettings Get() => _settings ??= new AsioSettings(
        Path.Combine(Constants.RSFolder, AsioSettings.DefaultFileName));

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
