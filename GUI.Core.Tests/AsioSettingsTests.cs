#nullable enable
using System.IO;
using RSMods.ASIO;
using Xunit;

namespace RSMods.Core.Tests;

public sealed class AsioSettingsTests
{
    // RS_ASIO's own default file uses 1/0, and the WinForms configurator always wrote that.
    [Fact]
    public void BooleansAreWrittenAsOneAndZero()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File(AsioSettings.DefaultFileName);
        var settings = new AsioSettings(path);

        using (settings.SuspendSave())
        {
            settings.Config.EnableWasapiInputs = true;
            settings.Config.EnableAsio = false;
            settings.Output.EnableRefCountHack = true;
            settings.Input0.EnableSoftwareMasterVolumeControl = false;
            settings.Input1.EnableSoftwareEndpointVolumeControl = true;
            settings.InputMic.EnableRefCountHack = false;
        }

        string[] lines = File.ReadAllLines(path);
        Assert.Contains("EnableWasapiInputs=1", lines);
        Assert.Contains("EnableAsio=0", lines);
        Assert.Contains("EnableRefCountHack=1", lines);
        Assert.Contains("EnableSoftwareMasterVolumeControl=0", lines);
        Assert.Contains("EnableSoftwareEndpointVolumeControl=1", lines);
        Assert.DoesNotContain(lines, l => l.EndsWith("=on") || l.EndsWith("=off"));
    }

    // RS_ASIO's own default RS_ASIO.ini leaves several values blank; that means "default", not "invalid".
    [Fact]
    public void BlankValuesReadAsDefaultsWithoutWarnings()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File(AsioSettings.DefaultFileName);
        File.WriteAllText(path, "[Asio.Output]\nDriver=\nAltBaseChannel=\nEnableRefCountHack=\n\n[Asio.Input.Mic]\nChannel=\nSoftwareMasterVolumePercent=\n");

        var settings = new AsioSettings(path);
        int warnings = 0;
        settings.ValidationWarning += _ => warnings++;

        Assert.Equal(0, settings.Output.AltBaseChannel);
        Assert.False(settings.Output.EnableRefCountHack);
        Assert.Equal(1, settings.InputMic.Channel);
        Assert.Equal(100, settings.InputMic.SoftwareMasterVolumePercent);
        Assert.Equal(0, warnings);
    }

    [Fact]
    public void OnOffWrittenByEarlierBuildsStillReads()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File(AsioSettings.DefaultFileName);
        File.WriteAllText(path, "[Config]\nEnableWasapiInputs=on\nEnableAsio=off\n");

        var settings = new AsioSettings(path);

        Assert.True(settings.Config.EnableWasapiInputs);
        Assert.False(settings.Config.EnableAsio);
    }

    // A fork's own keys, values RSMods doesn't understand, alternatives kept as comments: none of it is RSMods' to change.
    [Fact]
    public void SavingEverySettingUnchangedLeavesAForksFileAlone()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File(AsioSettings.DefaultFileName);
        string[] fork =
        [
            "; fork header", "[Config]", "EnableWasapiOutputs=1", "EnableWasapiInputs=0", "EnableAsio=1", "ForkZeroConfig=1", "",
            "[Asio]", "; my notes", "BufferSizeMode=auto", "CustomBufferSize=64 ; low latency", "",
            "[Asio.Output]", "Driver=Current", ";Driver=Old", "BaseChannel=0", "ForkOutputKey = spaced", "",
            "[Asio.Input.0]", "Driver=Current", "Channel=0", "",
            "[Asio.Input.1]", "Driver=", "Channel=1", "",
            "[Fork.Custom]", "Latency=3",
        ];
        File.WriteAllLines(path, fork);

        var settings = new AsioSettings(path);
        using (settings.SuspendSave())
        {
            settings.Config.EnableWasapiOutputs = settings.Config.EnableWasapiOutputs;
            settings.Config.EnableWasapiInputs = settings.Config.EnableWasapiInputs;
            settings.Config.EnableAsio = settings.Config.EnableAsio;
            settings.AsioSection.CustomBufferSize = settings.AsioSection.CustomBufferSize;
            settings.Output.Disabled = settings.Output.Disabled;
            settings.Output.Driver = settings.Output.Driver;
            settings.Output.AltBaseChannel = settings.Output.AltBaseChannel;
            settings.Output.EnableRefCountHack = settings.Output.EnableRefCountHack;
            foreach (var input in new[] { settings.Input0, settings.InputMic })
            {
                input.Disabled = input.Disabled;
                input.Channel = input.Channel;
                input.SoftwareMasterVolumePercent = input.SoftwareMasterVolumePercent;
            }
            settings.Input1.Disabled = settings.Input1.Disabled;
            settings.Input1.Channel = settings.Input1.Channel;
            settings.Input1.EnableRefCountHack = settings.Input1.EnableRefCountHack;
        }

        Assert.Equal<string[]>(fork, File.ReadAllLines(path));
    }

    [Fact]
    public void WasapiOutputsUsesRsAsiosKey()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File(AsioSettings.DefaultFileName);
        File.WriteAllLines(path, ["[Config]", "EnableWasapiOutputs=1"]);
        var settings = new AsioSettings(path);

        Assert.Equal(WasapiOutputMode.On, settings.Config.EnableWasapiOutputs);
        settings.Config.EnableWasapiOutputs = WasapiOutputMode.Prompt;

        Assert.Equal<string[]>(["[Config]", "EnableWasapiOutputs=-1"], File.ReadAllLines(path));
    }

    [Fact]
    public void WasapiOutputsWrittenUnderTheOldKeyIsReadAndMoved()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File(AsioSettings.DefaultFileName);
        File.WriteAllLines(path, ["[Config]", "EnableAsio=1", "WasapiOutputs=1"]);
        var settings = new AsioSettings(path);

        Assert.Equal(WasapiOutputMode.On, settings.Config.EnableWasapiOutputs);
        settings.Config.EnableWasapiOutputs = WasapiOutputMode.On;

        Assert.Equal<string[]>(["[Config]", "EnableAsio=1", "EnableWasapiOutputs=1"], File.ReadAllLines(path));
    }

    [Fact]
    public void Input1KeepsAnAlternativeDriverCommentedOut()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File(AsioSettings.DefaultFileName);
        File.WriteAllLines(path, ["[Asio.Input.1]", "Driver=Current", ";Driver=Old", "Channel=1"]);
        var settings = new AsioSettings(path);

        Assert.False(settings.Input1.Disabled);
        Assert.Equal("Current", settings.Input1.Driver);

        settings.Input1.Disabled = true;
        Assert.Equal<string[]>(["[Asio.Input.1]", ";Driver=Current", ";Driver=Old", "Channel=1"], File.ReadAllLines(path));

        settings.Input1.Disabled = false;
        Assert.Equal<string[]>(["[Asio.Input.1]", "Driver=Current", ";Driver=Old", "Channel=1"], File.ReadAllLines(path));
    }
}
