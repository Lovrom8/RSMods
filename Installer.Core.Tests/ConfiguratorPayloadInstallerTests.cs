using System.IO;
using System.Linq;
using RS2014_Mod_Installer.Payload;
using Xunit;

namespace RS2014_Mod_Installer.Tests;

public sealed class ConfiguratorPayloadInstallerTests
{
    private static string Exe(string rsMods) => Path.Combine(rsMods, "RSMods.exe");

    [Fact]
    public void FreshInstall_DeploysPayload_WritesManifest_AndCleansStaging()
    {
        using var temp = new TemporaryDirectory();
        string rsMods = temp.Combine("RSMods");
        var payload = PayloadBuilder.Default();

        ConfiguratorPayloadInstaller.Install(rsMods, payload.BuildZip(), payload.BuildManifest());

        Assert.True(File.Exists(Exe(rsMods)));
        Assert.True(File.Exists(Path.Combine(rsMods, "ddc", "ddc.exe")));
        Assert.True(File.Exists(Path.Combine(rsMods, "tools", "ww2ogg.exe")));
        Assert.True(Directory.Exists(Path.Combine(rsMods, "CustomMods")));

        string manifestPath = Path.Combine(rsMods, ConfiguratorPayloadInstaller.InstalledManifestName);
        Assert.True(File.Exists(manifestPath));
        PayloadManifest installed = PayloadManifest.Parse(File.ReadAllText(manifestPath));
        Assert.Equal("RSMods.exe", installed.Executable);

        // The unique sibling staging directory is removed on success.
        Assert.Empty(Directory.GetDirectories(temp.Path, "RSMods._install_*"));
    }

    [Fact]
    public void Upgrade_PreservesUserOwnedFilesAndCustomMods()
    {
        using var temp = new TemporaryDirectory();
        string rsMods = temp.Combine("RSMods");
        Directory.CreateDirectory(rsMods);
        File.WriteAllText(Path.Combine(rsMods, "GUI_Settings.ini"), "RSPath=foo\n; keep this comment");
        File.WriteAllText(Path.Combine(rsMods, "TwitchAuth.dat"), "secret-token");
        File.WriteAllText(Path.Combine(rsMods, "TwitchEnabledEffects.xml"), "<effects/>");
        Directory.CreateDirectory(Path.Combine(rsMods, "CustomMods"));
        File.WriteAllText(Path.Combine(rsMods, "CustomMods", "mymod.txt"), "mine");
        File.WriteAllText(Exe(rsMods), "old exe");

        var payload = PayloadBuilder.Default();
        ConfiguratorPayloadInstaller.Install(rsMods, payload.BuildZip(), payload.BuildManifest());

        Assert.Equal("RSPath=foo\n; keep this comment", File.ReadAllText(Path.Combine(rsMods, "GUI_Settings.ini")));
        Assert.Equal("secret-token", File.ReadAllText(Path.Combine(rsMods, "TwitchAuth.dat")));
        Assert.Equal("<effects/>", File.ReadAllText(Path.Combine(rsMods, "TwitchEnabledEffects.xml")));
        Assert.Equal("mine", File.ReadAllText(Path.Combine(rsMods, "CustomMods", "mymod.txt")));
        Assert.Equal("fake configurator executable v-new", File.ReadAllText(Exe(rsMods)));
    }

    [Fact]
    public void Upgrade_RemovesStalePriorManifestAndLegacyFiles()
    {
        using var temp = new TemporaryDirectory();
        string rsMods = temp.Combine("RSMods");

        // A prior install that shipped a file the new payload drops.
        var old = new PayloadBuilder()
            .AddText("RSMods.exe", "old exe")
            .AddText("OldOnly.dll", "stale");
        ConfiguratorPayloadInstaller.Install(rsMods, old.BuildZip(), old.BuildManifest());
        Assert.True(File.Exists(Path.Combine(rsMods, "OldOnly.dll")));

        // Legacy WinForms/Twitch leftovers the one-time obsolete list should clean up.
        File.WriteAllText(Path.Combine(rsMods, "Ookii.Dialogs.dll"), "x");
        File.WriteAllText(Path.Combine(rsMods, "TwitchLib.Api.dll"), "x");
        File.WriteAllText(Path.Combine(rsMods, "RSMods.pdb"), "x");
        // A user file that must survive regardless.
        File.WriteAllText(Path.Combine(rsMods, "GUI_Settings.ini"), "keep");

        var next = PayloadBuilder.Default();
        ConfiguratorPayloadInstaller.Install(rsMods, next.BuildZip(), next.BuildManifest());

        Assert.False(File.Exists(Path.Combine(rsMods, "OldOnly.dll")));
        Assert.False(File.Exists(Path.Combine(rsMods, "Ookii.Dialogs.dll")));
        Assert.False(File.Exists(Path.Combine(rsMods, "TwitchLib.Api.dll")));
        Assert.False(File.Exists(Path.Combine(rsMods, "RSMods.pdb")));
        Assert.True(File.Exists(Exe(rsMods)));
        Assert.Equal("keep", File.ReadAllText(Path.Combine(rsMods, "GUI_Settings.ini")));
    }

    [Fact]
    public void Upgrade_UnsafePathInPreviousManifest_CannotDeleteOutsideInstallFolder()
    {
        using var temp = new TemporaryDirectory();
        string rsMods = temp.Combine("RSMods");
        Directory.CreateDirectory(rsMods);

        string sibling = temp.Combine("keep.txt");
        File.WriteAllText(sibling, "keep");

        PayloadManifest previous = PayloadBuilder.Default().BuildManifest();
        previous.Files.Add(new PayloadFile { Path = "../keep.txt" });
        File.WriteAllText(
            Path.Combine(rsMods, ConfiguratorPayloadInstaller.InstalledManifestName),
            previous.ToJson());

        var next = PayloadBuilder.Default();
        ConfiguratorPayloadInstaller.Install(rsMods, next.BuildZip(), next.BuildManifest());

        Assert.Equal("keep", File.ReadAllText(sibling));
    }

    [Fact]
    public void Upgrade_MalformedPreviousManifest_IsIgnored()
    {
        using var temp = new TemporaryDirectory();
        string rsMods = temp.Combine("RSMods");
        Directory.CreateDirectory(rsMods);
        File.WriteAllText(
            Path.Combine(rsMods, ConfiguratorPayloadInstaller.InstalledManifestName),
            "not valid json");

        var payload = PayloadBuilder.Default();
        ConfiguratorPayloadInstaller.Install(rsMods, payload.BuildZip(), payload.BuildManifest());

        Assert.True(File.Exists(Exe(rsMods)));
        PayloadManifest installed = PayloadManifest.Parse(File.ReadAllText(
            Path.Combine(rsMods, ConfiguratorPayloadInstaller.InstalledManifestName)));
        Assert.Equal("RSMods.exe", installed.Executable);
    }

    [Fact]
    public void Upgrade_PreviousManifestCannotDeletePreservedFile()
    {
        using var temp = new TemporaryDirectory();
        string rsMods = temp.Combine("RSMods");
        Directory.CreateDirectory(rsMods);

        string settingsPath = Path.Combine(rsMods, "GUI_Settings.ini");
        File.WriteAllText(settingsPath, "keep");

        PayloadManifest previous = PayloadBuilder.Default().BuildManifest();
        previous.Files.Add(new PayloadFile { Path = "GUI_Settings.ini" });
        File.WriteAllText(
            Path.Combine(rsMods, ConfiguratorPayloadInstaller.InstalledManifestName),
            previous.ToJson());

        var next = PayloadBuilder.Default();
        ConfiguratorPayloadInstaller.Install(rsMods, next.BuildZip(), next.BuildManifest());

        Assert.Equal("keep", File.ReadAllText(settingsPath));
    }

    [Fact]
    public void CorruptPayload_HashMismatch_ThrowsAndWritesNothing()
    {
        using var temp = new TemporaryDirectory();
        string rsMods = temp.Combine("RSMods");
        var payload = PayloadBuilder.Default();
        PayloadManifest manifest = payload.BuildManifest();
        manifest.Files[0].Sha256 = new string('0', 64); // corrupt one recorded hash

        var ex = Assert.Throws<PayloadInstallException>(() =>
            ConfiguratorPayloadInstaller.Install(rsMods, payload.BuildZip(), manifest));

        Assert.Equal(PayloadInstallFailure.CorruptPayload, ex.Reason);
        Assert.False(File.Exists(Exe(rsMods)));
        Assert.Empty(Directory.GetDirectories(temp.Path, "RSMods._install_*"));
    }

    [Fact]
    public void ZipSlipEntry_IsRejected_AndNothingEscapes()
    {
        using var temp = new TemporaryDirectory();
        string rsMods = temp.Combine("RSMods");
        MemoryStream slip = PayloadBuilder.BuildZipSlipZip("../evil.txt");
        PayloadManifest manifest = PayloadBuilder.Default().BuildManifest();

        var ex = Assert.Throws<PayloadInstallException>(() =>
            ConfiguratorPayloadInstaller.Install(rsMods, slip, manifest));

        Assert.Equal(PayloadInstallFailure.UnsafeArchivePath, ex.Reason);
        Assert.False(File.Exists(temp.Combine("evil.txt"))); // never written to the sibling/parent
    }

    [Fact]
    public void FailedUpgrade_LeavesPriorInstallUsable()
    {
        using var temp = new TemporaryDirectory();
        string rsMods = temp.Combine("RSMods");
        var first = PayloadBuilder.Default();
        ConfiguratorPayloadInstaller.Install(rsMods, first.BuildZip(), first.BuildManifest());
        string before = File.ReadAllText(Exe(rsMods));

        var next = new PayloadBuilder()
            .AddText("RSMods.exe", "should not land")
            .AddText("extra.dll", "x");
        PayloadManifest bad = next.BuildManifest();
        bad.Files.First(f => f.Path == "extra.dll").Sha256 = new string('0', 64);

        var ex = Assert.Throws<PayloadInstallException>(() =>
            ConfiguratorPayloadInstaller.Install(rsMods, next.BuildZip(), bad));

        Assert.Equal(PayloadInstallFailure.CorruptPayload, ex.Reason);
        Assert.Equal(before, File.ReadAllText(Exe(rsMods)));           // prior exe intact
        Assert.False(File.Exists(Path.Combine(rsMods, "extra.dll")));   // nothing from the bad payload landed
    }

    [Fact]
    public void LockedExecutable_ThrowsConfiguratorRunning()
    {
        using var temp = new TemporaryDirectory();
        string rsMods = temp.Combine("RSMods");
        var first = PayloadBuilder.Default();
        ConfiguratorPayloadInstaller.Install(rsMods, first.BuildZip(), first.BuildManifest());

        PayloadInstallException ex;
        using (new FileStream(Exe(rsMods), FileMode.Open, FileAccess.Read, FileShare.None))
        {
            var next = new PayloadBuilder().AddText("RSMods.exe", "v-upgrade");
            ex = Assert.Throws<PayloadInstallException>(() =>
                ConfiguratorPayloadInstaller.Install(rsMods, next.BuildZip(), next.BuildManifest()));
        }

        Assert.Equal(PayloadInstallFailure.ConfiguratorRunning, ex.Reason);
        Assert.Equal("fake configurator executable v-new", File.ReadAllText(Exe(rsMods))); // upgrade did not land
    }

    [Fact]
    public void Shortcut_TargetsInstalledRSModsExe()
    {
        using var temp = new TemporaryDirectory();
        string url = temp.Combine("RSMods.url");
        string target = Path.Combine(temp.Path, "RSMods", "RSMods.exe");

        DesktopShortcut.WriteUrlShortcut(url, target);

        string content = File.ReadAllText(url);
        Assert.Contains("URL=file:///" + target, content);
        Assert.Contains("IconFile=" + target.Replace('\\', '/'), content);
        Assert.EndsWith("RSMods.exe", target);
    }
}
