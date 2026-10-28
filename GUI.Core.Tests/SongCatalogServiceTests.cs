using RSMods.Util;

namespace RSMods.Core.Tests;

public sealed class SongCatalogServiceTests
{
    private static string Manifest(string arrangement, string properties, string tuning, string extra = "") => $$"""
        {
          "Entries": {
            "ABC123": {
              "Attributes": {
                "ArrangementName": "{{arrangement}}",
                "ArrangementProperties": {{properties}},
                "ArtistName": "Alpha",
                "SongName": "One",
                "SongKey": "AlphaOne",
                "Shipping": true,
                "Tuning": {{tuning}}{{extra}}
              }
            }
          },
          "ModelName": "RSEnumerable_Song",
          "IterationVersion": 2,
          "InsertRoot": "Static.Songs.Entries"
        }
        """;

    private static SongCatalogService.ArchiveContents Read(TemporaryDirectory dir, params (string Name, string Text)[] files)
    {
        string source = dir.File("source");
        foreach ((string name, string text) in files)
        {
            string path = Path.Combine(source, name);
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            File.WriteAllText(path, text);
        }

        string archive = dir.File("alpha_p.psarc");
        PsarcPacker.Pack(source, archive);
        return SongCatalogService.ReadArchive(archive, CancellationToken.None);
    }

    [Fact]
    public void ArrangementManifestsAreReadFromTheArchive()
    {
        using var dir = new TemporaryDirectory();

        SongCatalogService.ArchiveContents contents = Read(dir,
            ("manifests/songs_dlc_alphaone/alphaone_lead.json",
                Manifest("Lead", """{ "Represent": 0, "BonusArr": 0, "PathLead": 1, "PathRhythm": 0, "PathBass": 0 }""",
                    """{ "string0": -2, "string1": 0, "string2": 0, "string3": 0, "string4": -2, "string5": -2 }""")),
            ("manifests/songs_dlc_alphaone/alphaone_vocals.json", Manifest("Vocals", "null", "null")),
            ("manifests/songs_dlc_alphaone/songs_dlc_alphaone.hsan", "{}"),
            ("toolkit.version", "Toolkit version: 2.9.2.1\r\nPackage Author: Someone\r\nPackage Version: 1\r\n"));

        Assert.False(contents.IsOfficialDlc);
        Assert.Equal(2, contents.Arrangements.Count);
        ArrangementManifest lead = contents.Arrangements.Single(a => a.Arrangement.Name == "Lead");
        Assert.Equal(("AlphaOne", "Alpha", "One", true), (lead.SongKey, lead.Artist, lead.Title, lead.Shipping));
        Assert.Equal(new ArrangementTuning(-2, 0, 0, 0, -2, -2), lead.Arrangement.Tuning);
        Assert.True(lead.Arrangement.IsAlternate);
        Assert.True(lead.Arrangement.PathLead);
        Assert.Null(contents.Arrangements.Single(a => a.Arrangement.Name == "Vocals").Arrangement.Tuning);
    }

    [Fact]
    public void AnArchiveWithoutToolkitInfoIsOfficial()
    {
        using var dir = new TemporaryDirectory();

        SongCatalogService.ArchiveContents contents = Read(dir,
            ("manifests/songs_dlc_alphaone/alphaone_bass.json",
                Manifest("Bass", """{ "Represent": 1, "PathBass": 1 }""", "{}",
                    """, "SKU": "RS1", "DLCRS1Key": [ { "Win32": "248750" } ]""")));

        Assert.True(contents.IsOfficialDlc);
        ArrangementManifest bass = Assert.Single(contents.Arrangements);
        Assert.Equal(("RS1", 248750), (bass.Sku, bass.Rs1AppId));
        Assert.False(bass.Arrangement.IsAlternate);
        Assert.Equal(new ArrangementTuning(0, 0, 0, 0, 0, 0), bass.Arrangement.Tuning);
    }

    [Theory]
    [InlineData(null, "Ubisoft")]
    [InlineData("Toolkit version: 2.9.2.1\nPackage Version: 1\n", "Ubisoft")]
    [InlineData("Toolkit version: 2.9.2.1\r\nPackage Author: Someone Else \r\n", "Someone Else")]
    [InlineData("package author: Ubisoft\n", "Ubisoft")]
    public void PackageAuthorDefaultsToUbisoft(string? toolkitInfo, string author) =>
        Assert.Equal(author, SongCatalogService.PackageAuthor(toolkitInfo));
}
