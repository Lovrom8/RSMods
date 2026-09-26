#nullable enable
using RSMods.Core.Settings;
using RSMods.Util;

namespace RSMods.Core.Tests;

public sealed class ManifestKeybindsTests
{
    private const string Manifest = """
        [
          { "key": "ToggleLoftKey", "ini": { "section": "Keybinds", "name": "ToggleLoftKey" }, "type": "Key",
            "default": "T", "label": "Toggle Loft", "category": "Keybinds" },
          { "key": "MasterVolumeKey", "ini": { "section": "Audio Keybindings", "name": "MasterVolumeKey" }, "type": "Key",
            "default": "5", "label": "Master Volume", "category": "Audio Keybindings" },
          { "key": "ToggleLoftEnabled", "ini": { "section": "Toggle Switches", "name": "ToggleLoft" }, "type": "Bool",
            "default": "off", "label": "Toggle Loft", "category": "Toggle Switches" }
        ]
        """;

    [Fact]
    public void SplitsKeyBindsIntoModAndAudioLists()
    {
        using var directory = new TemporaryDirectory();
        var manifest = new ManifestService(Manifest);
        var ini = new IniManager(directory.File("RSMods.ini"));

        Assert.Equal(["Toggle Loft"], ManifestKeybinds.Mod(manifest, ini).Select(k => k.DisplayName));
        Assert.Equal(["Master Volume"], ManifestKeybinds.Audio(manifest, ini).Select(k => k.DisplayName));
    }

    [Fact]
    public void ReadsTheDeclaredDefaultAndWritesToTheDeclaredSection()
    {
        using var directory = new TemporaryDirectory();
        var manifest = new ManifestService(Manifest);
        var ini = new IniManager(directory.File("RSMods.ini"));
        KeybindItem loft = ManifestKeybinds.Mod(manifest, ini).Single();

        Assert.Equal(KeyConversion.VirtualKey("T"), loft.GetKey());

        loft.SetKey("VK_F5");

        Assert.Equal("VK_F5", ini.GetString("[Keybinds]", "ToggleLoftKey"));
        Assert.Equal("VK_F5", loft.GetKey());
    }
}
