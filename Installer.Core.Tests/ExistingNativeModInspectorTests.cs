using System;
using System.IO;
using RS2014_Mod_Installer.Core;
using Xunit;

namespace RS2014_Mod_Installer.Tests;

public sealed class ExistingNativeModInspectorTests
{
    [Fact]
    public void Inspect_NoDll_ReturnsNone()
    {
        using var temp = new TemporaryDirectory();

        Assert.Equal(ExistingNativeModKind.None, ExistingNativeModInspector.Inspect(temp.Path).Kind);
    }

    [Fact]
    public void Inspect_DllWithoutVersionInfo_CountsAsRsMods()
    {
        using var temp = new TemporaryDirectory();
        File.WriteAllText(temp.Combine("xinput1_3.dll"), "not a PE file");

        ExistingNativeMod existing = ExistingNativeModInspector.Inspect(temp.Path);

        Assert.Equal(ExistingNativeModKind.RsMods, existing.Kind);
        Assert.False(existing.NeedsConfirmation);
    }

    [Fact]
    public void Inspect_AnotherProgramsDll_NeedsConfirmation()
    {
        using var temp = new TemporaryDirectory();
        // Any real DLL with someone else's product name will do.
        File.Copy(Path.Combine(Environment.SystemDirectory, "kernel32.dll"), temp.Combine("xinput1_3.dll"));

        ExistingNativeMod existing = ExistingNativeModInspector.Inspect(temp.Path);

        Assert.Equal(ExistingNativeModKind.OtherProgram, existing.Kind);
        Assert.True(existing.NeedsConfirmation);
        Assert.NotEmpty(existing.Description);
    }

    [Fact]
    public void Classify_OfficialBuild_IsRsMods()
    {
        ExistingNativeMod existing = ExistingNativeModInspector.Classify("RSMods", "1.2.8.4-ffd44ffb", null);

        Assert.Equal(ExistingNativeModKind.RsMods, existing.Kind);
        Assert.False(existing.NeedsConfirmation);
    }

    [Fact]
    public void Classify_BuildWithExternalMods_NamesThem()
    {
        ExistingNativeMod existing = ExistingNativeModInspector.Classify("RSMods", "1.2.8.4", "External mods: DropPedal, AudioBridge");

        Assert.Equal(ExistingNativeModKind.CustomRsMods, existing.Kind);
        Assert.Equal("DropPedal, AudioBridge", existing.Description);
        Assert.True(existing.NeedsConfirmation);
    }

    [Fact]
    public void Classify_OtherProduct_DescribesIt()
    {
        ExistingNativeMod existing = ExistingNativeModInspector.Classify("x360ce", "4.17.15.0", null);

        Assert.Equal(ExistingNativeModKind.OtherProgram, existing.Kind);
        Assert.Equal("x360ce 4.17.15.0", existing.Description);
    }

    [Fact]
    public void SetAsideCustomManifest_RenamesAndReplacesOldBackup()
    {
        using var temp = new TemporaryDirectory();
        File.WriteAllText(temp.Combine("mods.manifest.json"), "custom");
        File.WriteAllText(temp.Combine("mods.manifest.json.bak"), "older");

        ExistingNativeModInspector.SetAsideCustomManifest(temp.Path);

        Assert.False(File.Exists(temp.Combine("mods.manifest.json")));
        Assert.Equal("custom", File.ReadAllText(temp.Combine("mods.manifest.json.bak")));

        ExistingNativeModInspector.SetAsideCustomManifest(temp.Path); // nothing to move: no throw
    }
}
