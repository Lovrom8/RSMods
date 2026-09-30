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
    public void ModsAreGroupedByCategoryThenSortedByTitle()
    {
        var categories = Coordinator.Mods.Select(m => m.Category).Distinct().ToList();
        Assert.Equal(ModCatalog.Categories.Where(categories.Contains), categories);

        foreach (var group in Coordinator.Mods.GroupBy(m => m.Category))
        {
            var titles = group.Select(m => m.Title).ToList();
            Assert.Equal(titles.OrderBy(t => t, StringComparer.CurrentCultureIgnoreCase), titles);
        }
    }

    [Fact]
    public void EveryCurrentModHasItsOwnCategory()
    {
        // "Other" catches mods added to the DLL later; today's mods should each have a real heading.
        Assert.DoesNotContain(Coordinator.Mods, m => m.Category == ModCatalog.Categories[^1]);
    }

    [Fact]
    public void GatedSettingsGreyOutWithTheirModAndTheHintKeepsItsPlace()
    {
        var coordinator = new SettingsCoordinator(new ManifestService());
        ModEntryViewModel looping = coordinator.Mods.Single(m => m.Key == "AllowLooping");
        SettingFieldViewModel leadUp = looping.Fields.Single(f => f.Key == "LoopingLeadUp");

        looping.Toggle!.Value = false;
        Assert.False(looping.IsOn);
        Assert.False(leadUp.IsEnabled);
        Assert.True(looping.HasEnableHint);
        Assert.NotEmpty(looping.EnableHint);

        looping.Toggle.Value = true;
        Assert.True(looping.IsOn);
        Assert.True(leadUp.IsEnabled);
        Assert.True(looping.HasEnableHint);
        Assert.Empty(looping.EnableHint);
    }
}
