#nullable enable
using System;
using System.IO;
using System.Linq;
using RSMods.Util;
using Xunit;

namespace RSMods.Core.Tests;

public sealed class WwiseInstallationTests
{
    private static string MakeInstall(TemporaryDirectory dir, string name, bool withConsole)
    {
        string folder = dir.File(Path.Combine("Audiokinetic", name));
        string bin = Path.Combine(folder, "Authoring", "x64", "Release", "bin");
        Directory.CreateDirectory(bin);
        if (withConsole)
            File.WriteAllText(Path.Combine(bin, "WwiseConsole.exe"), "");
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
    public void OnlyUnsupportedVersionsAreIncompatible()
    {
        using var dir = new TemporaryDirectory();
        string newer = MakeInstall(dir, "Wwise_2024.1.0.8669", withConsole: true);
        MakeInstall(dir, "Wwise_2023.1.19.8928", withConsole: false); // leftover folder, no executables

        WwiseCheckResult result = WwiseInstallation.Check(newer, [dir.File("Audiokinetic")], useCompatible: false);

        Assert.Equal(WwiseStatus.Incompatible, result.Status);
        Assert.Equal(2, result.Installs.Count); // the WWISEROOT install isn't listed twice
        Assert.Contains(result.Installs, i => i.Year == 2023 && !i.HasConsole);
    }

    [Fact]
    public void CompatibleInstallIsUsedWhenWwiseRootPointsElsewhere()
    {
        using var dir = new TemporaryDirectory();
        string newer = MakeInstall(dir, "Wwise_2024.1.0.8669", withConsole: true);
        string compatible = MakeInstall(dir, "Wwise_2022.1.18.8567", withConsole: true);

        WwiseCheckResult result = WwiseInstallation.Check(newer, [dir.File("Audiokinetic")], useCompatible: false);

        Assert.Equal(WwiseStatus.Ready, result.Status);
        Assert.Equal(compatible, result.Selected?.Folder);
    }

    [Theory]
    [InlineData("Wwise v2017.2.9 build 6726", 2017, false)]
    [InlineData("Wwise 2019.1.0.6947", 2019, true)]
    [InlineData("Wwise_2023.1.19.8928", 2023, true)]
    [InlineData("Wwise_2024.1.0.8669", 2024, false)]
    public void VersionYearDecidesCompatibility(string name, int year, bool compatible)
    {
        using var dir = new TemporaryDirectory();
        string folder = MakeInstall(dir, name, withConsole: true);

        WwiseInstall? install = WwiseInstallation.Inspect(folder);

        Assert.NotNull(install);
        Assert.Equal(year, install.Year);
        Assert.Equal(compatible, install.IsCompatible);
    }

    [WindowsFact]
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
