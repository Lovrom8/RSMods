#nullable enable
using RSMods.Core.Settings;

namespace RSMods.Core.Tests;

public sealed class ManifestDefaultSeedingTests
{
    [Theory]
    [InlineData(null, "off")]
    [InlineData("on", "on")]
    [InlineData("off", "off")]
    public void ProfileMigrationSurvivesGuiSave(string? explicitValue, string expected)
    {
        using var directory = new TemporaryDirectory();
        string path = directory.File("RSMods.ini");
        System.IO.File.WriteAllText(path, "[Toggle Switches]\nForceProfileLoad=off\n[Mod Settings]\n" +
            (explicitValue == null ? "" : $"ProfileAutoLoad={explicitValue}\n"));
        var ini = new IniManager(path);
        ini.Load();
        RsModsSettings.SeedManifestDefaults(ini, new ManifestService());
        ini.Save();

        var reopened = new IniManager(path);
        reopened.Load();
        Assert.Equal(expected, reopened.GetString("[Mod Settings]", "ProfileAutoLoad"));

        // Integration tests use the framework executable to read GUI settings through the DLL.
        string? reader = System.Environment.GetEnvironmentVariable("RSMODS_TEST_SETTINGS_READER");
        if (!string.IsNullOrEmpty(reader))
        {
            var start = new System.Diagnostics.ProcessStartInfo(reader) { UseShellExecute = false };
            start.ArgumentList.Add(path);
            start.ArgumentList.Add(expected);
            using var process = System.Diagnostics.Process.Start(start)!;
            Assert.True(process.WaitForExit(30000));
            Assert.Equal(0, process.ExitCode);
        }
    }

    private const string Missing = "<missing>";

    private const string Manifest = """
        [
          { "key": "ToggleLoftEnabled", "ini": { "section": "Toggle Switches", "name": "ToggleLoft" }, "type": "Bool",
            "default": "off", "label": "Toggle Loft", "category": "Toggle Switches" },
          { "key": "ToggleLoftWhen", "ini": { "section": "Toggle Switches", "name": "ToggleLoftWhen" }, "type": "Enum",
            "default": "manual", "label": "Loft When", "category": "Toggle Switches" },
          { "key": "RewindBy", "ini": { "section": "Mod Settings", "name": "RewindBy" }, "type": "Int",
            "default": "5000", "label": "Rewind By", "category": "Mod Settings" },
          { "key": "ToggleLoftKey", "ini": { "section": "Keybinds", "name": "ToggleLoftKey" }, "type": "Key",
            "default": "T", "label": "Toggle Loft", "category": "Keybinds" },
          { "key": "MidiCustomEditor", "ini": { "section": "Mod Settings", "name": "MidiCustomEditor" }, "type": "String",
            "default": "", "label": "MIDI", "category": "Mod Settings", "editor": "Midi" }
        ]
        """;

    [Fact]
    public void SeedsMissingValuesWithTheDeclaredDefaultInTheDeclaredSection()
    {
        using var directory = new TemporaryDirectory();
        var ini = new IniManager(directory.File("RSMods.ini"));

        RsModsSettings.SeedManifestDefaults(ini, new ManifestService(Manifest));

        Assert.Equal("off", ini.GetString("[Toggle Switches]", "ToggleLoft", Missing));
        Assert.Equal("manual", ini.GetString("[Toggle Switches]", "ToggleLoftWhen", Missing));
        Assert.Equal("5000", ini.GetString("[Mod Settings]", "RewindBy", Missing));
    }

    [Fact]
    public void KeepsValuesAlreadyInTheIni()
    {
        using var directory = new TemporaryDirectory();
        var ini = new IniManager(directory.File("RSMods.ini"));
        ini.SetString("[Toggle Switches]", "ToggleLoft", "on");

        RsModsSettings.SeedManifestDefaults(ini, new ManifestService(Manifest));

        Assert.Equal("on", ini.GetString("[Toggle Switches]", "ToggleLoft", Missing));
    }

    [Fact]
    public void LeavesKeyBindsAndCustomEditorsUnseeded()
    {
        using var directory = new TemporaryDirectory();
        var ini = new IniManager(directory.File("RSMods.ini"));

        RsModsSettings.SeedManifestDefaults(ini, new ManifestService(Manifest));

        Assert.Equal(Missing, ini.GetString("[Keybinds]", "ToggleLoftKey", Missing));
        Assert.Equal(Missing, ini.GetString("[Mod Settings]", "MidiCustomEditor", Missing));
    }
}
