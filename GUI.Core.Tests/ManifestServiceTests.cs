#nullable enable
using System.Linq;
using RSMods.Core.Settings;
using Xunit;

namespace RSMods.Core.Tests;

public sealed class ManifestServiceTests
{
    [Fact]
    public void ManifestLoadsAllDescriptors()
    {
        var service = new ManifestService();
        Assert.NotNull(service.AllSettings);
        Assert.True(service.AllSettings.Count >= 89, $"Expected >= 89 descriptors, but found {service.AllSettings.Count}");
    }

    [Fact]
    public void ManifestFindsKnownSettings()
    {
        var service = new ManifestService();

        var volumeControl = service.Find("VolumeControlEnabled");
        Assert.NotNull(volumeControl);
        Assert.Equal(SettingType.Bool, volumeControl.Type);
        Assert.Equal("VolumeControl", volumeControl.Ini.Name);
        Assert.Equal("Toggle Switches", volumeControl.Ini.Section);

        var interval = service.Find("VolumeControlInterval");
        Assert.NotNull(interval);
        Assert.Equal(SettingType.Int, interval.Type);
        Assert.Equal(1, interval.Min);
        Assert.Equal(25, interval.Max);
        Assert.NotNull(interval.VisibleWhen);
        Assert.Equal("VolumeControlEnabled", interval.VisibleWhen.Key);
        Assert.Equal("on", interval.VisibleWhen.ExpectedValue);
    }

    [Fact]
    public void ManifestGroupsCategories()
    {
        var service = new ManifestService();
        var categories = service.GetCategories().ToList();

        Assert.NotEmpty(categories);
        Assert.Contains(categories, g => g.Key == "Toggle Switches");
        Assert.Contains(categories, g => g.Key == "Mod Settings");
    }
}
