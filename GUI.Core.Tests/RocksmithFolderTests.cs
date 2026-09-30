using RSMods.Util;

namespace RSMods.Core.Tests;

/// <summary>A folder only counts as a Rocksmith install when it has both the game and its cache.psarc.</summary>
public sealed class RocksmithFolderTests : IDisposable
{
    private readonly TemporaryDirectory _folder = new();

    public void Dispose() => _folder.Dispose();

    [Fact]
    public void GameAndCachePsarcIsAnInstall()
    {
        File.WriteAllText(_folder.File("Rocksmith2014.exe"), string.Empty);
        File.WriteAllText(_folder.File("cache.psarc"), string.Empty);

        Assert.True(_folder.Path.IsRSFolder());
    }

    [Fact]
    public void CachePsarcWithoutTheGameIsNot()
    {
        File.WriteAllText(_folder.File("cache.psarc"), string.Empty);

        Assert.False(_folder.Path.IsRSFolder());
    }

    [Fact]
    public void GameWithoutCachePsarcIsNot()
    {
        File.WriteAllText(_folder.File("Rocksmith2014.exe"), string.Empty);

        Assert.False(_folder.Path.IsRSFolder());
    }

    [Fact]
    public void MissingFolderIsNot()
    {
        Assert.False(Path.Combine(_folder.Path, "missing").IsRSFolder());
    }
}
