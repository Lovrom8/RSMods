#nullable enable
using RSMods.Core.Settings;

namespace RSMods.Core.Tests;

/// <summary>
/// A build ships its own manifest next to the GUI (DLL/Framework/README.md, "Other hosts"), which can lack the Windows
/// mods and have settings the embedded one doesn't. The real manifest's tests can't cover that.
/// </summary>
public sealed class HostManifestTests
{
    private const string Manifest = """
        [
          { "key": "SharedEnabled", "ini": { "section": "Toggle Switches", "name": "SharedEnabled" }, "type": "Bool",
            "default": "off", "label": "Shared", "category": "Toggle Switches", "mod": "SharedMod",
            "entry": "SharedEnabled", "heading": "Practice", "entryTitle": "Shared" },
          { "key": "SharedKey", "ini": { "section": "Keybinds", "name": "SharedKey" }, "type": "Key",
            "default": "T", "label": "Shared Key", "category": "Keybinds", "mod": "SharedMod", "entry": "SharedEnabled" },
          { "key": "HostOnlyEnabled", "ini": { "section": "Toggle Switches", "name": "HostOnlyEnabled" }, "type": "Bool",
            "default": "off", "label": "Host Only", "category": "Toggle Switches", "mod": "HostOnlyMod",
            "entry": "HostOnlyEnabled", "heading": "Audio", "entryTitle": "Host Only" },
          { "key": "OrphanInterval", "ini": { "section": "Mod Settings", "name": "OrphanInterval" }, "type": "Int",
            "default": "5", "label": "Orphan", "category": "Mod Settings", "mod": "HostOnlyMod", "entry": "OrphanInterval",
            "entryTitle": "Orphan", "visibleWhen": { "key": "VolumeControlEnabled", "equals": "on" } }
        ]
        """;

    [Fact]
    public void ABuildsOwnManifestIsUsedOverTheEmbeddedOne()
    {
        using var directory = new TemporaryDirectory();
        File.WriteAllText(directory.File("mods.manifest.json"), Manifest);

        Assert.Equal(4, ManifestService.LoadDescriptors(directory.Path, embeddedFitsThisBuild: false).Count);
        Assert.Equal(4, ManifestService.LoadDescriptors(directory.Path, embeddedFitsThisBuild: true).Count);
    }

    [Theory]
    [InlineData(null)]
    [InlineData("[]")]
    [InlineData("not json")]
    public void WithoutAUsableManifestOfItsOwnTheBuildStops(string? content)
    {
        using var directory = new TemporaryDirectory();
        if (content is not null)
            File.WriteAllText(directory.File("mods.manifest.json"), content);

        var error = Assert.Throws<InvalidOperationException>(
            () => ManifestService.LoadDescriptors(directory.Path, embeddedFitsThisBuild: false));

        Assert.Contains(directory.File("mods.manifest.json"), error.Message);
    }

    [Theory]
    [InlineData(null)]
    [InlineData("[]")]
    [InlineData("not json")]
    public void OnWindowsTheEmbeddedManifestStillCoversForIt(string? content)
    {
        using var directory = new TemporaryDirectory();
        if (content is not null)
            File.WriteAllText(directory.File("mods.manifest.json"), content);

        Assert.NotEmpty(ManifestService.LoadDescriptors(directory.Path, embeddedFitsThisBuild: true));
    }

    [Fact]
    public void ShowsWhatTheManifestHasAndNothingElse()
    {
        var coordinator = new SettingsCoordinator(new ManifestService(Manifest));

        Assert.Equal(["HostOnlyEnabled", "OrphanInterval", "SharedEnabled"], coordinator.Mods.Select(m => m.Key).Order());
        Assert.Null(coordinator.Find("VolumeControlEnabled"));
        Assert.Contains(coordinator.Mods.Single(m => m.Key == "SharedEnabled").Keybinds, k => k.Key == "SharedKey");
    }

    [Fact]
    public void AParentThisBuildLacksDisablesNothing()
    {
        var coordinator = new SettingsCoordinator(new ManifestService(Manifest));

        Assert.True(coordinator.Find("OrphanInterval")!.IsEnabled);
    }

    [Fact]
    public void KeepsTheOtherPlatformsSettingsInTheIni()
    {
        using var directory = new TemporaryDirectory();
        string path = directory.File("RSMods.ini");
        File.WriteAllText(path, "[Toggle Switches]\nVolumeControl=on\nSharedEnabled=off\n");

        var ini = new IniManager(path);
        ini.Load();
        var coordinator = new SettingsCoordinator(new ManifestService(Manifest));
        coordinator.Load(ini);
        coordinator.Find<BoolSettingFieldViewModel>("SharedEnabled")!.Value = true;
        coordinator.Save(ini);
        ini.Save();

        var saved = new IniManager(path);
        saved.Load();
        Assert.Equal("on", saved.GetString("[Toggle Switches]", "SharedEnabled"));
        Assert.Equal("on", saved.GetString("[Toggle Switches]", "VolumeControl"));
    }
}
