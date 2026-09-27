using RSMods.Data;
using RSMods.SetAndForget;

namespace RSMods.Core.Tests;

/// <summary>
/// The psarc itself is faked: "cache.psarc" is a text file, unpacking copies its text into the unpacked
/// folder and packing copies it back, so each test can see which archive state a modification started from.
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

    private CachePsarcService CreateService() => new(
        unpack: (archive, destination) =>
        {
            Directory.CreateDirectory(Constants.CachePcPath);
            File.WriteAllText(UnpackedContent, File.ReadAllText(archive));
        },
        pack: (source, archive) =>
        {
            ++_packs;
            File.WriteAllText(archive, File.ReadAllText(Path.Combine(source, "content.txt")));
        });

    private static void Append(string mod) => File.AppendAllText(UnpackedContent, "+" + mod);

    [Fact]
    public void AStaleUnpackFromAnEarlierSessionIsNotReused()
    {
        // Left behind after Fast Load was applied; the user has since restored the stock cache.psarc.
        Directory.CreateDirectory(Constants.CachePcPath);
        File.WriteAllText(UnpackedContent, "stock+fastload");

        CreateService().Modify(_ => Append("directconnect"));

        Assert.Equal("stock+directconnect", File.ReadAllText(Constants.CachePsarcPath));
    }

    [Fact]
    public void AppliedModsStillStack()
    {
        var service = CreateService();

        service.Modify(_ => Append("fastload"));
        service.Modify(_ => Append("directconnect"));

        Assert.Equal("stock+fastload+directconnect", File.ReadAllText(Constants.CachePsarcPath));
    }

    [Fact]
    public void TheUnpackedCopyIsDeletedAfterwards()
    {
        CreateService().Modify(_ => Append("fastload"));

        Assert.False(Directory.Exists(Constants.WorkFolder));
    }

    [Fact]
    public void AFailedModificationIsNotRepackedAndStillCleansUp()
    {
        Assert.Throws<IOException>(() => CreateService().Modify(_ =>
        {
            Append("half-applied");
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

        service.Modify(_ => Append("fastload"));
        service.Modify(_ => Append("directconnect"));

        Assert.Equal("stock", File.ReadAllText(Constants.CacheBackupPath));
    }
}

/// <summary>Serializes tests that set the process-wide <see cref="Constants.RSFolder"/>.</summary>
[CollectionDefinition(Name)]
public sealed class ConstantsCollection
{
    public const string Name = "Constants.RSFolder";
}
