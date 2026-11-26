using RSMods.Data;
using RSMods.SetAndForget;

namespace RSMods.Core.Tests;

/// <summary>
/// The psarc is faked: "static.psarc" is a text file, and replacing an entry writes "entry=content" into it, so each
/// test can see what was replaced and which archive state the backup holds.
/// </summary>
[Collection(ConstantsCollection.Name)]
public sealed class StaticPsarcServiceTests : IDisposable
{
    private readonly TemporaryDirectory _rsFolder = new();
    private readonly TemporaryDirectory _sources = new();

    public StaticPsarcServiceTests()
    {
        Constants.RSFolder = _rsFolder.Path;
        File.WriteAllText(Constants.StaticPsarcPath, "stock");
    }

    public void Dispose()
    {
        _rsFolder.Dispose();
        _sources.Dispose();
    }

    private static StaticPsarcService CreateService() => new(new TextFileArchive());

    private string Source(string content)
    {
        string path = Path.Combine(_sources.Path, Guid.NewGuid() + ".gfx");
        File.WriteAllText(path, content);
        return path;
    }

    private sealed class TextFileArchive : ICacheArchive
    {
        public bool Contains(string archivePath, string entryName) => true;

        public void Extract(string archivePath, string entryName, string targetFile) =>
            throw new NotSupportedException();

        public void Replace(string archivePath, IReadOnlyDictionary<string, string> filesByEntry)
        {
            foreach (var (entry, file) in filesByEntry)
                File.WriteAllText(archivePath, entry + "=" + File.ReadAllText(file));
        }
    }

    [Fact]
    public void TheEntryIsNamedWithForwardSlashes()
    {
        CreateService().Replace(Source("patched"), Path.Combine("gfxassets", "views", "sa_songoptions.gfx"));

        Assert.Equal("gfxassets/views/sa_songoptions.gfx=patched", File.ReadAllText(Constants.StaticPsarcPath));
    }

    [Fact]
    public void TheFirstReplacementBacksUpTheUntouchedArchive()
    {
        var service = CreateService();

        service.Replace(Source("first"), "a.gfx");
        service.Replace(Source("second"), "a.gfx");

        Assert.Equal("stock", File.ReadAllText(Constants.StaticBackupPath));
    }

    [Fact]
    public void RestoreDefaultsPutsTheBackupBack()
    {
        var service = CreateService();
        service.Replace(Source("patched"), "a.gfx");

        Assert.True(service.RestoreDefaults());
        Assert.Equal("stock", File.ReadAllText(Constants.StaticPsarcPath));
    }

    [Fact]
    public void RestoreDefaultsWithoutAnEditLeavesTheArchiveAlone()
    {
        Assert.False(CreateService().RestoreDefaults());
        Assert.Equal("stock", File.ReadAllText(Constants.StaticPsarcPath));
    }

    [Fact]
    public void AnotherToolsStaticBakIsNeverTouched()
    {
        string foreign = Path.Combine(_rsFolder.Path, "static.bak");
        File.WriteAllText(foreign, "someone else's backup");

        CreateService().Replace(Source("patched"), "a.gfx");

        Assert.Equal("someone else's backup", File.ReadAllText(foreign));
    }
}
