using RSMods.Util;

namespace RSMods.Core.Tests;

public sealed class PsarcPackerTests
{
    [Fact]
    public void PackedFolderUnpacksToTheSameFilesInTheToolkitLayout()
    {
        using var dir = new TemporaryDirectory();
        string source = dir.File("source");
        Directory.CreateDirectory(Path.Combine(source, "audio", "windows"));
        File.WriteAllText(Path.Combine(source, "manifest.json"), "{\"Entries\":{}}");
        File.WriteAllBytes(Path.Combine(source, "cache7.7z"), Enumerable.Range(0, 200_000).Select(i => (byte)i).ToArray());
        File.WriteAllBytes(Path.Combine(source, "audio", "windows", "1.wem"), [1, 2, 3]);
        string archive = dir.File("cache.psarc");
        File.WriteAllText(archive, "previous archive");
        var reported = new List<int>();

        PsarcPacker.Pack(source, archive, new SynchronousProgress(reported.Add));
        PsarcPacker.Unpack(archive, dir.File("unpacked"));

        string unpacked = PsarcPacker.UnpackedFolder(archive, dir.File("unpacked"));
        Assert.Equal(dir.File(Path.Combine("unpacked", "cache_psarc_RS2014_Pc")), unpacked);
        foreach (string file in Directory.GetFiles(source, "*", SearchOption.AllDirectories))
            Assert.Equal(File.ReadAllBytes(file), File.ReadAllBytes(Path.Combine(unpacked, Path.GetRelativePath(source, file))));
        Assert.Equal(3, Directory.GetFiles(unpacked, "*", SearchOption.AllDirectories).Length);
        Assert.False(File.Exists(archive + ".tmp"));
        Assert.Equal(100, reported.Last());
    }

    // Progress<T> posts to the thread pool; the test needs every report before it asserts.
    private sealed class SynchronousProgress(Action<int> report) : IProgress<int>
    {
        public void Report(int value) => report(value);
    }
}
