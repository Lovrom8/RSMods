using System.IO;
using System.Text;
using RS2014_Mod_Installer.Core;
using RS2014_Mod_Installer.Payload;
using Xunit;

namespace RS2014_Mod_Installer.Tests;

public sealed class RsModsInstallationTests
{
    private static readonly byte[] Dll = Encoding.UTF8.GetBytes("new dll");
    private static readonly byte[] Pdb = Encoding.UTF8.GetBytes("new pdb");
    private static readonly ExistingNativeMod CustomBuild = new(ExistingNativeModKind.CustomRsMods, "DropPedal");

    // A Rocksmith folder with a custom RSMods build: its DLL, a prior configurator, and its loose manifest.
    private static string MakeCustomInstall(TemporaryDirectory temp)
    {
        string rocksmith = temp.Combine("Rocksmith2014");
        string rsMods = ConfiguratorPaths.RsModsFolder(rocksmith);
        var prior = PayloadBuilder.Default();
        ConfiguratorPayloadInstaller.Install(rsMods, prior.BuildZip(), prior.BuildManifest("1.0.0"));

        File.WriteAllText(Path.Combine(rocksmith, "xinput1_3.dll"), "custom dll");
        File.WriteAllText(Path.Combine(rocksmith, "xinput1_3.pdb"), "custom pdb");
        File.WriteAllText(Path.Combine(rsMods, "mods.manifest.json"), "custom manifest");
        return rocksmith;
    }

    [Fact]
    public void Install_ReplacesNativeMod_UpdatesConfigurator_SetsCustomManifestAside()
    {
        using var temp = new TemporaryDirectory();
        string rocksmith = MakeCustomInstall(temp);
        string rsMods = ConfiguratorPaths.RsModsFolder(rocksmith);
        var next = PayloadBuilder.Default().AddText("RSMods.exe", "v2");

        RsModsInstallation.Install(rocksmith, CustomBuild, replaceNativeMod: true, Dll, Pdb, next.BuildZip(), next.BuildManifest("2.0.0"));

        Assert.Equal(Dll, File.ReadAllBytes(Path.Combine(rocksmith, "xinput1_3.dll")));
        Assert.Equal(Pdb, File.ReadAllBytes(Path.Combine(rocksmith, "xinput1_3.pdb")));
        Assert.Equal("v2", File.ReadAllText(Path.Combine(rsMods, "RSMods.exe")));
        Assert.False(File.Exists(Path.Combine(rsMods, "mods.manifest.json")));
        Assert.Equal("custom manifest", File.ReadAllText(Path.Combine(rsMods, "mods.manifest.json.bak")));
        Assert.Empty(Directory.GetFiles(rocksmith, "*.rsmods-previous")); // native backups dropped once it all went in
    }

    [Fact]
    public void PayloadFailureAfterNativeStep_RestoresOldDllAndCustomManifest()
    {
        using var temp = new TemporaryDirectory();
        string rocksmith = MakeCustomInstall(temp);
        string rsMods = ConfiguratorPaths.RsModsFolder(rocksmith);
        var next = PayloadBuilder.Default().AddText("tools/ww2ogg.exe", "v2");

        // Held open by another program, so the configurator deploy fails after the native mod went in.
        PayloadInstallException ex;
        using (new FileStream(Path.Combine(rsMods, "tools", "ww2ogg.exe"), FileMode.Open, FileAccess.Read, FileShare.Read))
        {
            ex = Assert.Throws<PayloadInstallException>(() =>
                RsModsInstallation.Install(rocksmith, CustomBuild, replaceNativeMod: true, Dll, Pdb, next.BuildZip(), next.BuildManifest("2.0.0")));
        }

        Assert.Equal(PayloadInstallFailure.IOError, ex.Reason);
        Assert.Equal("custom dll", File.ReadAllText(Path.Combine(rocksmith, "xinput1_3.dll")));
        Assert.Equal("custom pdb", File.ReadAllText(Path.Combine(rocksmith, "xinput1_3.pdb")));
        Assert.Equal("custom manifest", File.ReadAllText(Path.Combine(rsMods, "mods.manifest.json")));
        Assert.False(File.Exists(Path.Combine(rsMods, "mods.manifest.json.bak")));
        Assert.Empty(Directory.GetFiles(rocksmith, "*.rsmods-previous"));
    }

    [Fact]
    public void CorruptPayload_LeavesNativeModUntouched()
    {
        using var temp = new TemporaryDirectory();
        string rocksmith = MakeCustomInstall(temp);
        var next = PayloadBuilder.Default();
        PayloadManifest manifest = next.BuildManifest("2.0.0");
        manifest.Files[0].Sha256 = new string('0', 64);

        var ex = Assert.Throws<PayloadInstallException>(() =>
            RsModsInstallation.Install(rocksmith, CustomBuild, replaceNativeMod: true, Dll, Pdb, next.BuildZip(), manifest));

        Assert.Equal(PayloadInstallFailure.CorruptPayload, ex.Reason);
        Assert.Equal("custom dll", File.ReadAllText(Path.Combine(rocksmith, "xinput1_3.dll")));
        Assert.Equal("custom manifest", File.ReadAllText(Path.Combine(ConfiguratorPaths.RsModsFolder(rocksmith), "mods.manifest.json")));
    }

    [Fact]
    public void KeepingTheNativeMod_OnlyUpdatesConfigurator()
    {
        using var temp = new TemporaryDirectory();
        string rocksmith = MakeCustomInstall(temp);
        string rsMods = ConfiguratorPaths.RsModsFolder(rocksmith);
        var next = PayloadBuilder.Default().AddText("RSMods.exe", "v2");

        RsModsInstallation.Install(rocksmith, CustomBuild, replaceNativeMod: false, null, null, next.BuildZip(), next.BuildManifest("2.0.0"));

        Assert.Equal("custom dll", File.ReadAllText(Path.Combine(rocksmith, "xinput1_3.dll")));
        Assert.Equal("custom manifest", File.ReadAllText(Path.Combine(rsMods, "mods.manifest.json")));
        Assert.Equal("v2", File.ReadAllText(Path.Combine(rsMods, "RSMods.exe")));
    }
}
