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
        Assert.Equal(100, interval.Max);
        Assert.NotNull(interval.VisibleWhen);
        Assert.Equal("VolumeControlEnabled", interval.VisibleWhen.Key);
        Assert.Equal("on", interval.VisibleWhen.ExpectedValue);

        var altSampleRate = service.Find("AlternativeOutputSampleRate");
        Assert.NotNull(altSampleRate);
        Assert.Equal(SettingType.Enum, altSampleRate.Type);
        Assert.Equal("48000", altSampleRate.Default);
        Assert.NotNull(altSampleRate.Choices);
        Assert.Contains("48000", altSampleRate.Choices);
        Assert.Contains("96000", altSampleRate.Choices);

        var nsp = service.Find("CustomNSPTimeLimit");
        Assert.NotNull(nsp);
        Assert.Equal(2000, nsp.Min);
        Assert.Equal(60000, nsp.Max);
        Assert.Equal(0.001, nsp.Scale);

        var rewind = service.Find("RewindBy");
        Assert.NotNull(rewind);
        Assert.Equal(0, rewind.Min);
        Assert.Equal(90000, rewind.Max);
        Assert.Equal(0.001, rewind.Scale);

        var font = service.Find("OnScreenFont");
        Assert.NotNull(font);
        Assert.Equal("SystemFonts", font.ChoicesSource);
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
