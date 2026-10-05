#nullable enable
using System.IO;
using RSMods.Core.Settings;
using Xunit;

namespace RSMods.Core.Tests;

public sealed class GameSettingEditTests
{
    private static GameSettingEdit Edit(string iniPath, string value = "on") =>
        new(iniPath, "ToggleLoftEnabled", "Toggle Switches", "ToggleLoft", value);

    [Fact]
    public void ParsesTheGamesMessage()
    {
        var edit = GameSettingEdit.Parse("set\nC:\\RS\\RSMods.ini\nToggleLoftEnabled\nToggle Switches\nToggleLoft\non");

        Assert.Equal(new GameSettingEdit("C:\\RS\\RSMods.ini", "ToggleLoftEnabled", "Toggle Switches", "ToggleLoft", "on"), edit);
        Assert.Equal("", GameSettingEdit.Parse("set\np\nk\ns\nn\n")?.Value);
        Assert.Null(GameSettingEdit.Parse("update all"));
        Assert.Null(GameSettingEdit.Parse("set\np\nk\ns\nn"));
    }

    [Fact]
    public void AppliedEditIsSavedWithoutAskingTheGameToReload()
    {
        using var dir = new TemporaryDirectory();
        var ini = new IniManager(dir.File("RSMods.ini"));
        var coordinator = new SettingsCoordinator(new ManifestService());
        coordinator.Load(ini);
        ini.Save();

        Assert.True(Edit(dir.File("RSMods.ini")).ApplyTo(ini, coordinator));

        Assert.True(coordinator.Find<BoolSettingFieldViewModel>("ToggleLoftEnabled")!.Value);
        Assert.False(coordinator.IsDirty);
        Assert.False(ini.Save()); // The game already has it, so no "update all"
        Assert.Contains("ToggleLoft=on", File.ReadAllText(dir.File("RSMods.ini")));
    }

    [Fact]
    public void UnsavedGuiEditIsLeftForTheGuiToSave()
    {
        using var dir = new TemporaryDirectory();
        var ini = new IniManager(dir.File("RSMods.ini"));
        var coordinator = new SettingsCoordinator(new ManifestService());
        coordinator.Load(ini);
        var field = coordinator.Find<BoolSettingFieldViewModel>("ToggleLoftEnabled")!;
        field.Value = true;

        Assert.True(Edit(dir.File("RSMods.ini"), "off").ApplyTo(ini, coordinator));

        Assert.True(field.Value);
        Assert.True(field.IsDirty);
    }

    [Fact]
    public void EditForAnotherGameFolderIsDeclined()
    {
        using var dir = new TemporaryDirectory();
        var ini = new IniManager(dir.File("RSMods.ini"));
        var coordinator = new SettingsCoordinator(new ManifestService());
        coordinator.Load(ini);

        Assert.False(Edit(dir.File(Path.Combine("Other", "RSMods.ini"))).ApplyTo(ini, coordinator));
        Assert.Equal("off", ini.GetString("[Toggle Switches]", "ToggleLoft", "off"));
    }

    [Fact]
    public void PathComparisonIgnoresCase()
    {
        using var dir = new TemporaryDirectory();
        var ini = new IniManager(dir.File("RSMods.ini"));
        var coordinator = new SettingsCoordinator(new ManifestService());

        Assert.True(Edit(dir.File("RSMods.ini").ToUpperInvariant()).ApplyTo(ini, coordinator));
    }
}
