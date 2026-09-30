using System.Globalization;

namespace RSMods.Core.Tests;

public sealed class BeatMapFileTests : IDisposable
{
    private const string Archive = @"C:\Rocksmith2014\dlc\2minutes_p.psarc";

    private readonly TemporaryDirectory _folder = new();

    public void Dispose() => _folder.Dispose();

    [Fact]
    public void EachBeatIsWrittenAsSecondsAndAMeasureFlag()
    {
        string path = _folder.File("2minutes.beats");

        BeatMapFile.Write(path, new SongBeats(Archive, [new Beat(10f, true), new Beat(10.333f, false)]));

        Assert.Equal(["10 1", "10.333 0"], File.ReadLines(path).Where(line => !line.StartsWith('#')));
    }

    [Fact]
    public void BeatsAreWrittenTheSameWayInEveryCulture()
    {
        string path = _folder.File("2minutes.beats");
        CultureInfo original = CultureInfo.CurrentCulture;
        CultureInfo.CurrentCulture = new CultureInfo("es-MX");
        try
        {
            BeatMapFile.Write(path, new SongBeats(Archive, [new Beat(10.5f, true)]));
        }
        finally
        {
            CultureInfo.CurrentCulture = original;
        }

        Assert.Contains("10.5 1", File.ReadLines(path));
    }

    [Fact]
    public void TheSourceArchiveIsReadBack()
    {
        string path = _folder.File("2minutes.beats");

        BeatMapFile.Write(path, new SongBeats(Archive, [new Beat(10f, true)]));

        Assert.Equal(Archive, BeatMapFile.ReadSourceArchive(path));
    }

    [Fact]
    public void AMissingFileHasNoSourceArchive()
    {
        Assert.Null(BeatMapFile.ReadSourceArchive(_folder.File("missing.beats")));
    }

    [Fact]
    public void WritingReplacesThePreviousBeats()
    {
        string path = _folder.File("2minutes.beats");
        BeatMapFile.Write(path, new SongBeats(Archive, [new Beat(10f, true), new Beat(11f, false)]));

        BeatMapFile.Write(path, new SongBeats(Archive, [new Beat(12f, true)]));

        Assert.Equal(["12 1"], File.ReadLines(path).Where(line => !line.StartsWith('#')));
    }
}
