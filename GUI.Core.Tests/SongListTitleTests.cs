#nullable enable
using RSMods.Core.Settings;
using RSMods.Data;

namespace RSMods.Core.Tests;

/// <summary>Song list titles live under <c>[SongListTitles]</c> as <c>SongListTitle_{n}</c>, the keys the DLL reads.</summary>
[Collection(ConstantsCollection.Name)]
public sealed class SongListTitleTests : IDisposable
{
    private const string Missing = "<missing>";
    private readonly TemporaryDirectory _rsFolder = new();

    public SongListTitleTests()
    {
        // A folder with a cache.psarc is what GenUtil.GetRSDirectory accepts as a Rocksmith install.
        File.WriteAllText(_rsFolder.File("cache.psarc"), string.Empty);
        File.WriteAllText(_rsFolder.File("RSMods.ini"), "[SongListTitles]\nSongListTitle_1=Metal\n");
        Constants.RSFolder = _rsFolder.Path;

        RsModsSettings.LoadSettingsFromINI(new ManifestService("[]"));
    }

    public void Dispose() => _rsFolder.Dispose();

    [Fact]
    public void ReadsTheStoredTitle()
    {
        Assert.Equal("Metal", RsModsSettings.GetSongListTitle(1));
    }

    [Fact]
    public void UnsetTitlesFallBackToThePlaceholder()
    {
        Assert.Equal("Define Song List 2 Here", RsModsSettings.GetSongListTitle(2));
        Assert.Equal("Define Song List 2 Here", RsModsSettings.Ini.GetString("[SongListTitles]", "SongListTitle_2", Missing));
    }

    [Fact]
    public void SeedingDoesNotWriteTheTitleAsAKey()
    {
        // The arguments were once swapped, writing "Define Song List 2 Here=SongListTitle_2" on every launch.
        Assert.Equal(Missing, RsModsSettings.Ini.GetString("[SongListTitles]", "Define Song List 2 Here", Missing));
    }

    [Fact]
    public void SetWritesUnderTheNumberedKey()
    {
        RsModsSettings.SetSongListTitle(3, "Blues");

        Assert.Equal("Blues", RsModsSettings.Ini.GetString("[SongListTitles]", "SongListTitle_3", Missing));
        Assert.Equal("Blues", RsModsSettings.GetSongListTitle(3));
    }
}
