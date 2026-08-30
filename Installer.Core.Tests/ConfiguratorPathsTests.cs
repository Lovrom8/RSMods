using System.IO;
using RS2014_Mod_Installer.Core;
using Xunit;

namespace RS2014_Mod_Installer.Tests;

public sealed class ConfiguratorPathsTests
{
    [Fact]
    public void RsModsFolder_IsUnderRocksmith()
    {
        string rs = Path.Combine("C:", "Games", "Rocksmith2014");
        Assert.Equal(Path.Combine(rs, "RSMods"), ConfiguratorPaths.RsModsFolder(rs));
    }

    [Fact]
    public void ExecutablePath_IsRsModsExe_UnderRsModsFolder()
    {
        string rs = Path.Combine("C:", "Games", "Rocksmith2014");
        string exe = ConfiguratorPaths.ExecutablePath(rs);

        Assert.Equal(Path.Combine(rs, "RSMods", "RSMods.exe"), exe);
        Assert.EndsWith("RSMods.exe", exe);
    }
}
