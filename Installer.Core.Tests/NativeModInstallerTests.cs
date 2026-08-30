using System.IO;
using System.Text;
using RS2014_Mod_Installer.Core;
using Xunit;

namespace RS2014_Mod_Installer.Tests;

public sealed class NativeModInstallerTests
{
    private static readonly byte[] Dll = Encoding.UTF8.GetBytes("fake xinput1_3.dll");
    private static readonly byte[] Pdb = Encoding.UTF8.GetBytes("fake xinput1_3.pdb");

    [Fact]
    public void Install_WritesNativeMod_ReturnsSuccess()
    {
        using var temp = new TemporaryDirectory();

        NativeModInstallStatus status = NativeModInstaller.Install(temp.Path, Dll, Pdb);

        Assert.Equal(NativeModInstallStatus.Success, status);
        Assert.Equal(Dll, File.ReadAllBytes(Path.Combine(temp.Path, "xinput1_3.dll")));
        Assert.Equal(Pdb, File.ReadAllBytes(Path.Combine(temp.Path, "xinput1_3.pdb")));
    }

    [Fact]
    public void Install_RemovesLargeLegacyProxy_KeepsSmallOne()
    {
        using var temp = new TemporaryDirectory();
        string legacy = Path.Combine(temp.Path, "D3DX9_42.dll");
        File.WriteAllBytes(legacy, new byte[300_000]); // large leftover injector

        NativeModInstaller.Install(temp.Path, Dll, Pdb);
        Assert.False(File.Exists(legacy));

        // A genuine small D3DX9_42.dll must be left alone.
        File.WriteAllBytes(legacy, new byte[1024]);
        NativeModInstaller.Install(temp.Path, Dll, Pdb);
        Assert.True(File.Exists(legacy));
    }

    [Fact]
    public void Install_TargetLockedByRunningGame_ReturnsGameRunning()
    {
        using var temp = new TemporaryDirectory();
        string target = Path.Combine(temp.Path, "xinput1_3.dll");
        File.WriteAllText(target, "existing");

        using (new FileStream(target, FileMode.Open, FileAccess.Read, FileShare.None))
        {
            Assert.Equal(NativeModInstallStatus.GameRunning, NativeModInstaller.Install(temp.Path, Dll, Pdb));
        }
    }
}
