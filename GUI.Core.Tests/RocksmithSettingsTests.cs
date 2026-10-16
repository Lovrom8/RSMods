#nullable enable
using System.IO;
using RSMods.Rocksmith;
using Xunit;

namespace RSMods.Core.Tests;

public sealed class RocksmithSettingsTests
{
    // The game accepts MSAA 2 and 8 (and keys the screen doesn't know); saving the screen unchanged mustn't reset them.
    [Fact]
    public void GameValuesTheScreenCantShowAreKept()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File(RocksmithSettings.DefaultFileName);
        string[] game = ["[Renderer.Win32]", "MsaaSamples=8", "Fullscreen=2", "[Audio]", "ExclusiveMode=1"];
        File.WriteAllLines(path, game);

        var settings = new RocksmithSettings(path);
        IniValidationWarning? warning = null;
        settings.ValidationWarning += w => warning = w;

        MsaaMode shown = settings.RendererWin32.MsaaSamples;
        Assert.Equal(MsaaMode.X4, shown);
        Assert.True(warning!.ValueKept);

        using (settings.SuspendSave())
        {
            settings.RendererWin32.MsaaSamples = shown;
            settings.Audio.LatencyBuffer = settings.Audio.LatencyBuffer; // Missing: the game's default stays implicit.
            settings.RendererWin32.Fullscreen = FullscreenMode.Windowed;
        }

        Assert.Equal<string[]>(["[Renderer.Win32]", "MsaaSamples=8", "Fullscreen=0", "[Audio]", "ExclusiveMode=1"], File.ReadAllLines(path));
    }

    [Fact]
    public void AMissingFileIsCreatedWhole()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File(RocksmithSettings.DefaultFileName);

        var settings = new RocksmithSettings(path);
        settings.Audio.LatencyBuffer = settings.Audio.LatencyBuffer;
        _ = settings.RendererWin32.MsaaSamples;
        settings.Save();

        string[] lines = File.ReadAllLines(path);
        Assert.Contains("LatencyBuffer=4", lines);
        Assert.Contains("MsaaSamples=4", lines);
    }
}
