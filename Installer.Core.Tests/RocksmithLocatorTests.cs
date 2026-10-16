using System.IO;
using RS2014_Mod_Installer.Core;
using Xunit;

namespace RS2014_Mod_Installer.Tests;

public sealed class RocksmithLocatorTests
{
    // Builds a folder that looks like a real Rocksmith 2014 install: a non-empty dlc/ and a cache.psarc.
    private static string MakeRocksmithFolder(string root)
    {
        Directory.CreateDirectory(Path.Combine(root, "dlc"));
        File.WriteAllText(Path.Combine(root, "dlc", "song.psarc"), "x");
        File.WriteAllText(Path.Combine(root, "cache.psarc"), "x");
        return root;
    }

    [Fact]
    public void IsRocksmithFolder_ValidLayout_IsTrue()
    {
        using var temp = new TemporaryDirectory();
        MakeRocksmithFolder(temp.Path);
        Assert.True(RocksmithLocator.IsRocksmithFolder(temp.Path));
    }

    [Fact]
    public void IsRocksmithFolder_MissingCachePsarc_IsFalse()
    {
        using var temp = new TemporaryDirectory();
        Directory.CreateDirectory(Path.Combine(temp.Path, "dlc"));
        File.WriteAllText(Path.Combine(temp.Path, "dlc", "song.psarc"), "x");
        Assert.False(RocksmithLocator.IsRocksmithFolder(temp.Path));
    }

    [Fact]
    public void IsRocksmithFolder_EmptyDlc_IsFalse()
    {
        using var temp = new TemporaryDirectory();
        Directory.CreateDirectory(Path.Combine(temp.Path, "dlc"));
        File.WriteAllText(Path.Combine(temp.Path, "cache.psarc"), "x");
        Assert.False(RocksmithLocator.IsRocksmithFolder(temp.Path));
    }

    [Fact]
    public void IsRocksmithFolder_NonexistentFolder_IsFalse()
    {
        Assert.False(RocksmithLocator.IsRocksmithFolder(Path.Combine(Path.GetTempPath(), "definitely-not-here-" + System.Guid.NewGuid().ToString("N"))));
    }

    [Fact]
    public void ParseLibraryFolders_CurrentNestedFormat_ReadsEveryPath()
    {
        string[] vdf =
        [
            "\"libraryfolders\"",
            "{",
            "\t\"0\"",
            "\t{",
            "\t\t\"path\"\t\t\"C:\\\\Program Files (x86)\\\\Steam\"",
            "\t\t\"label\"\t\t\"\"",
            "\t\t\"apps\"",
            "\t\t{",
            "\t\t\t\"228980\"\t\t\"250758\"",
            "\t\t}",
            "\t}",
            "\t\"12\"",
            "\t{",
            "\t\t\"path\"\t\t\"E:\\\\Games\\\\Steam Library\"",
            "\t\t\"apps\"",
            "\t\t{",
            "\t\t\t\"221680\"\t\t\"13445\"",
            "\t\t}",
            "\t}",
            "}"
        ];

        Assert.Equal([@"C:\Program Files (x86)\Steam", @"E:\Games\Steam Library"], RocksmithLocator.ParseLibraryFolders(vdf));
    }

    [Fact]
    public void ParseLibraryFolders_OldOneLineFormat_StillRead()
    {
        string[] vdf =
        [
            "\"LibraryFolders\"",
            "{",
            "\t\"TimeNextStatsReport\"\t\t\"1612345678\"",
            "\t\"ContentStatsID\"\t\t\"-123\"",
            "\t\"1\"\t\t\"D:\\\\SteamLibrary\"",
            "\t\"10\"\t\t\"F:\\\\More\"",
            "}"
        ];

        Assert.Equal([@"D:\SteamLibrary", @"F:\More"], RocksmithLocator.ParseLibraryFolders(vdf));
    }

    [Fact]
    public void IsRocksmithFolder_NullOrEmpty_IsFalse()
    {
        Assert.False(RocksmithLocator.IsRocksmithFolder(null));
        Assert.False(RocksmithLocator.IsRocksmithFolder(string.Empty));
    }
}
