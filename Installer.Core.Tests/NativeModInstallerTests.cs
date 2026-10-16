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
        Assert.Empty(Directory.GetFiles(temp.Path, "*.new")); // the swap's temporary files don't linger
    }

    [Fact]
    public void Install_TargetLockedByRunningGame_LeavesBothFilesAlone()
    {
        using var temp = new TemporaryDirectory();
        string dll = Path.Combine(temp.Path, "xinput1_3.dll");
        string pdb = Path.Combine(temp.Path, "xinput1_3.pdb");
        File.WriteAllText(dll, "old dll");
        File.WriteAllText(pdb, "old pdb");

        using (new FileStream(dll, FileMode.Open, FileAccess.Read, FileShare.Read))
        {
            Assert.Equal(NativeModInstallStatus.GameRunning, NativeModInstaller.Install(temp.Path, Dll, Pdb));
        }

        Assert.Equal("old dll", File.ReadAllText(dll));
        Assert.Equal("old pdb", File.ReadAllText(pdb));
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
    public void Install_PdbLocked_PutsOldDllBack()
    {
        using var temp = new TemporaryDirectory();
        string dll = Path.Combine(temp.Path, "xinput1_3.dll");
        string pdb = Path.Combine(temp.Path, "xinput1_3.pdb");
        File.WriteAllText(dll, "old dll");
        File.WriteAllText(pdb, "old pdb");

        // A debugger holding the PDB: the DLL swap succeeds, replacing the PDB is refused.
        using (new FileStream(pdb, FileMode.Open, FileAccess.Read, FileShare.Read))
        {
            Assert.Throws<UnauthorizedAccessException>(() => NativeModInstaller.Install(temp.Path, Dll, Pdb, out NativeModBackup _));
        }

        Assert.Equal("old dll", File.ReadAllText(dll));
        Assert.Equal("old pdb", File.ReadAllText(pdb));
        Assert.Empty(Directory.GetFiles(temp.Path, "*.rsmods-previous"));
    }

    [Fact]
    public void Install_BackupRestore_PutsPriorFilesBack()
    {
        using var temp = new TemporaryDirectory();
        string dll = Path.Combine(temp.Path, "xinput1_3.dll");
        File.WriteAllText(dll, "old dll"); // no PDB before: restoring removes the new one

        Assert.Equal(NativeModInstallStatus.Success, NativeModInstaller.Install(temp.Path, Dll, Pdb, out NativeModBackup backup));
        Assert.Equal(Dll, File.ReadAllBytes(dll));

        Assert.True(backup.Restore());
        Assert.Equal("old dll", File.ReadAllText(dll));
        Assert.False(File.Exists(Path.Combine(temp.Path, "xinput1_3.pdb")));
        Assert.Empty(Directory.GetFiles(temp.Path, "*.rsmods-previous"));
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
