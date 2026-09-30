namespace RSMods.Core.Tests;

public sealed class BeatMapExtractorTests
{
    [Fact]
    public void AnArrangementOfTheSongMatches()
    {
        Assert.True(BeatMapExtractor.IsArrangementOf("songs/bin/generic/2minutes_lead.sng", "2minutes"));
    }

    [Fact]
    public void TheSongKeyMatchesRegardlessOfCase()
    {
        Assert.True(BeatMapExtractor.IsArrangementOf("songs/bin/generic/2minutes_bass.sng", "2Minutes"));
    }

    [Fact]
    public void ASongWhoseKeyStartsWithTheSameTextDoesNotMatch()
    {
        Assert.False(BeatMapExtractor.IsArrangementOf("songs/bin/generic/2minutes_live_lead.sng", "2minutes"));
    }

    [Fact]
    public void FilesOtherThanSngsDoNotMatch()
    {
        Assert.False(BeatMapExtractor.IsArrangementOf("manifests/songs_dlc_2minutes/2minutes_lead.json", "2minutes"));
    }

    [Fact]
    public void TheLongestArrangementProvidesTheBeats()
    {
        Beat[] lead = [new(10f, true), new(10.5f, false)];
        Beat[] rhythm = [new(10f, true), new(10.5f, false), new(11f, true)];

        Assert.Same(rhythm, BeatMapExtractor.Longest([lead, rhythm]));
    }

    [Fact]
    public void ASongWithoutArrangementsHasNoBeats()
    {
        Assert.Empty(BeatMapExtractor.Longest([]));
    }
}
