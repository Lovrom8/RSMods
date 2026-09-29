#nullable enable
using System;
using System.Linq;
using RSMods.Core.Settings;
using Xunit;

namespace RSMods.Core.Tests;

public sealed class ModCatalogTests
{
    private static readonly SettingsCoordinator Coordinator = new(new ManifestService());

    private static ModEntryViewModel Mod(string rootKey) => Coordinator.Mods.Single(m => m.Key == rootKey);

    [Fact]
    public void SubSettingsAndKeyBindsLandUnderTheirMod()
    {
        ModEntryViewModel volume = Mod("VolumeControlEnabled");
        Assert.NotNull(volume.Toggle);
        Assert.Contains(volume.Fields, f => f.Key == "VolumeControlInterval");
        Assert.Contains(volume.Keybinds, k => k.Key == "MasterVolumeKey");

        ModEntryViewModel looping = Mod("AllowLooping");
        Assert.Equal(["LoopStartKey", "LoopEndKey"], looping.Keybinds.Select(k => k.Key));
        Assert.Equal(["RewindKey"], Mod("AllowRewind").Keybinds.Select(k => k.Key));
        Assert.Equal(["RRSpeedKey"], Mod("RRSpeedAboveOneHundred").Keybinds.Select(k => k.Key));
    }

    [Fact]
    public void FontSettingsAreOneEntry()
    {
        ModEntryViewModel text = Mod("OnScreenFont");

        Assert.Equal("On-Screen Text", text.Title);
        Assert.Null(text.Toggle);
        Assert.Equal(["OnScreenFont", "OnScreenFontSize"], text.Fields.Select(f => f.Key));
    }

    [Fact]
    public void EveryKeyBindBelongsToExactlyOneMod()
    {
        var keybinds = new ManifestService().AllSettings.Where(d => d.Type == SettingType.Key).Select(d => d.Key);
        var listed = Coordinator.Mods.SelectMany(m => m.Keybinds).Select(k => k.Key).ToList();

        Assert.Equal(keybinds.OrderBy(k => k), listed.OrderBy(k => k));
    }

    [Fact]
    public void NoFieldIsListedTwice()
    {
        var listed = Coordinator.Mods
            .SelectMany(m => m.Toggle is null ? m.Fields : m.Fields.Prepend(m.Toggle))
            .Select(f => f.Key)
            .ToList();

        Assert.Equal(listed.Count, listed.Distinct(StringComparer.OrdinalIgnoreCase).Count());
    }

    [Fact]
    public void SettingsOwnedByOtherScreensAreNotListed()
    {
        var listed = Coordinator.Mods.SelectMany(m => m.Fields.Append(m.Root)).Select(f => f.Key).ToHashSet();

        Assert.DoesNotContain("TwitchSettings", listed);
        Assert.DoesNotContain("MidiCustomEditor", listed);
        Assert.DoesNotContain("GuitarSpeakCustomEditor", listed);
        Assert.DoesNotContain("SeparateNoteColors", listed);
    }

    [Fact]
    public void ModsAreSortedByTitle()
    {
        var titles = Coordinator.Mods.Select(m => m.Title).ToList();

        Assert.Equal(titles.OrderBy(t => t, StringComparer.CurrentCultureIgnoreCase), titles);
    }
}
