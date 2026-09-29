#nullable enable
using System;
using System.IO;
using System.Linq;
using RSMods.Util;
using Xunit;

namespace RSMods.Core.Tests;

public sealed class WwiseInstallationTests
{
    private static string MakeInstall(TemporaryDirectory dir, string name, bool withCli)
    {
        string folder = dir.File(Path.Combine("Audiokinetic", name));
        string bin = Path.Combine(folder, "Authoring", "Win32", "Release", "bin");
        Directory.CreateDirectory(bin);
        if (withCli)
            File.WriteAllText(Path.Combine(bin, "WwiseCLI.exe"), "");
        return folder;
    }

    [Fact]
    public void NothingInstalled()
    {
        using var dir = new TemporaryDirectory();

        WwiseCheckResult result = WwiseInstallation.Check(null, [dir.File("Audiokinetic")], useCompatible: false);

        Assert.Equal(WwiseStatus.NotInstalled, result.Status);
        Assert.Empty(result.Installs);
    }

    [Fact]
    public void OnlyVersionsWithoutWwiseCliAreIncompatible()
    {
        using var dir = new TemporaryDirectory();
        string newer = MakeInstall(dir, "Wwise_2023.1.19.8928", withCli: false);
        MakeInstall(dir, "Wwise v2013.2.10 build 4884", withCli: false); // leftover folder, no executables

        WwiseCheckResult result = WwiseInstallation.Check(newer, [dir.File("Audiokinetic")], useCompatible: false);

        Assert.Equal(WwiseStatus.Incompatible, result.Status);
        Assert.Equal(2, result.Installs.Count); // the WWISEROOT install isn't listed twice
        Assert.Contains(result.Installs, i => i.Year == 2023);
    }

    [Fact]
    public void CompatibleInstallIsUsedWhenWwiseRootPointsElsewhere()
    {
        using var dir = new TemporaryDirectory();
        string newer = MakeInstall(dir, "Wwise_2023.1.19.8928", withCli: false);
        string compatible = MakeInstall(dir, "Wwise v2013.2.10 build 4884", withCli: true);

        WwiseCheckResult result = WwiseInstallation.Check(newer, [dir.File("Audiokinetic")], useCompatible: false);

        Assert.Equal(WwiseStatus.Ready, result.Status);
        Assert.Equal(compatible, result.Selected?.Folder);
    }

    [Theory]
    [InlineData("Wwise v2013.2.10 build 4884", 2013, true)]
    [InlineData("Wwise 2017.2.9.6726", 2017, true)]
    [InlineData("Wwise 2019.1.0.6947", 2019, false)]
    public void VersionYearDecidesCompatibility(string name, int year, bool compatible)
    {
        using var dir = new TemporaryDirectory();
        string folder = MakeInstall(dir, name, withCli: true);

        WwiseInstall? install = WwiseInstallation.Inspect(folder);

        Assert.NotNull(install);
        Assert.Equal(year, install.Year);
        Assert.Equal(compatible, install.IsCompatible);
    }

    [Fact]
    public void NormalizedPathsUseBackslashesAndOnDiskCasing()
    {
        using var dir = new TemporaryDirectory();
        string folder = dir.File(Path.Combine("Steam", "UserData", "221680"));
        Directory.CreateDirectory(folder);

        string mixed = folder.Replace(@"\Steam\UserData\", "/steam/userdata\\") + "\\";

        Assert.Equal(GenUtil.NormalizePath(folder), GenUtil.NormalizePath(mixed));
        Assert.DoesNotContain('/', GenUtil.NormalizePath(mixed));
        Assert.EndsWith(@"Steam\UserData\221680", GenUtil.NormalizePath(mixed), StringComparison.Ordinal);
    }
}
