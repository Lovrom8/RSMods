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

    [Fact]
    public void WasapiOutputsUsesRsAsiosKeyName()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File(AsioSettings.DefaultFileName);
        File.WriteAllText(path, "[Config]\nEnableWasapiOutputs=1\n");

        var settings = new AsioSettings(path);
        Assert.Equal(WasapiOutputMode.On, settings.Config.WasapiOutputs);

        settings.Config.WasapiOutputs = WasapiOutputMode.Prompt;
        string[] lines = File.ReadAllLines(path);
        Assert.Contains("EnableWasapiOutputs=-1", lines);
        Assert.DoesNotContain(lines, l => l.StartsWith("WasapiOutputs="));
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
}
