using RSMods.Data;
using RSMods.SetAndForget;
using RSMods.Util;

namespace RSMods.Core.Tests;

/// <summary>
/// The psarc is faked unless a test says otherwise: "cache.psarc" is a text file holding its one entry, content.txt.
/// Extracting copies its text out and replacing copies it back, so each test can see which archive state a
/// modification started from.
/// </summary>
[Collection(ConstantsCollection.Name)]
public sealed class CachePsarcServiceTests : IDisposable
{
    private readonly TemporaryDirectory _rsFolder = new();
    private int _packs;

    public CachePsarcServiceTests()
    {
        Constants.RSFolder = _rsFolder.Path;
        File.WriteAllText(Constants.CachePsarcPath, "stock");
    }

    public void Dispose()
    {
        if (Directory.Exists(Constants.WorkFolder))
            Directory.Delete(Constants.WorkFolder, recursive: true);
        _rsFolder.Dispose();
    }

    private static string UnpackedContent => Path.Combine(Constants.CachePcPath, "content.txt");

    private CachePsarcService CreateService() => new(new TextFileArchive(this));

    private static void Append(CacheModification cache, string mod) =>
        File.AppendAllText(cache.Open(UnpackedContent, forWriting: true), "+" + mod);

    private sealed class TextFileArchive(CachePsarcServiceTests test) : ICacheArchive
    {
        public bool Contains(string archivePath, string entryName) => true;

        public void Extract(string archivePath, string entryName, string targetFile)
        {
            Assert.Equal("content.txt", entryName);
            File.WriteAllText(targetFile, File.ReadAllText(archivePath));
        }

        public void Replace(string archivePath, IReadOnlyDictionary<string, string> filesByEntry)
        {
            ++test._packs;
            File.WriteAllText(archivePath, File.ReadAllText(filesByEntry["content.txt"]));
        }
    }

    [Fact]
    public void AStaleUnpackFromAnEarlierSessionIsNotReused()
    {
        // Left behind after Fast Load was applied; the user has since restored the stock cache.psarc.
        Directory.CreateDirectory(Constants.CachePcPath);
        File.WriteAllText(UnpackedContent, "stock+fastload");

        CreateService().Modify(cache => Append(cache, "directconnect"));

        Assert.Equal("stock+directconnect", File.ReadAllText(Constants.CachePsarcPath));
    }

    [Fact]
    public void AppliedModsStillStack()
    {
        var service = CreateService();

        service.Modify(cache => Append(cache, "fastload"));
        service.Modify(cache => Append(cache, "directconnect"));

        Assert.Equal("stock+fastload+directconnect", File.ReadAllText(Constants.CachePsarcPath));
    }

    [Fact]
    public void TheUnpackedCopyIsDeletedAfterwards()
    {
        CreateService().Modify(cache => Append(cache, "fastload"));

        Assert.False(Directory.Exists(Constants.WorkFolder));
    }

    [Fact]
    public void AFailedModificationIsNotRepackedAndStillCleansUp()
    {
        Assert.Throws<IOException>(() => CreateService().Modify(cache =>
        {
            Append(cache, "half-applied");
            throw new IOException("injection failed");
        }));

        Assert.Equal(0, _packs);
        Assert.Equal("stock", File.ReadAllText(Constants.CachePsarcPath));
        Assert.False(Directory.Exists(Constants.WorkFolder));
    }

    [Fact]
    public void TheFirstModificationBacksUpTheUntouchedArchive()
    {
        var service = CreateService();

        service.Modify(cache => Append(cache, "fastload"));
        service.Modify(cache => Append(cache, "directconnect"));

        Assert.Equal("stock", File.ReadAllText(Constants.CacheBackupPath));
    }

    [Fact]
    public void AModificationThatChangesNothingLeavesTheArchiveAlone()
    {
        CreateService().Modify(cache => cache.Open(UnpackedContent, forWriting: false));

        Assert.Equal(0, _packs);
    }

    [Fact]
    public void OnlyTheChangedEntryIsRewrittenInTheRealArchive()
    {
        string source = _rsFolder.File("source");
        Directory.CreateDirectory(source);
        File.WriteAllText(Path.Combine(source, "cache4.7z"), "four");
        File.WriteAllText(Path.Combine(source, "cache7.7z"), "seven");
        File.WriteAllText(Path.Combine(source, "sltsv1_aggregategraph.nt"), "graph");
        PsarcPacker.Pack(source, Constants.CachePsarcPath);

        new CachePsarcService().Modify(cache => File.AppendAllText(cache.Open(Constants.Cache7_7zPath, forWriting: true), "+mod"));

        PsarcPacker.Unpack(Constants.CachePsarcPath, _rsFolder.File("result"));
        string result = PsarcPacker.UnpackedFolder(Constants.CachePsarcPath, _rsFolder.File("result"));
        Assert.Equal("seven+mod", File.ReadAllText(Path.Combine(result, "cache7.7z")));
        Assert.Equal("four", File.ReadAllText(Path.Combine(result, "cache4.7z")));
        Assert.Equal("graph", File.ReadAllText(Path.Combine(result, "sltsv1_aggregategraph.nt")));
        Assert.False(File.Exists(Constants.CachePsarcPath + ".tmp"));
        Assert.False(Directory.Exists(Constants.WorkFolder));
    }
}

/// <summary>Serializes tests that set the process-wide <see cref="Constants.RSFolder"/>.</summary>
[CollectionDefinition(Name)]
public sealed class ConstantsCollection
{
    public const string Name = "Constants.RSFolder";
}
