using Rocksmith2014PsarcLib.Psarc.Models.Json;
using RSMods;
using RSMods.SetAndForget;
using RSMods.SetAndForget.Models;
using ArrangementTuning = Rocksmith2014PsarcLib.Psarc.Models.Json.SongArrangement.ArrangementAttributes.ArrangementTuning;

namespace RSMods.Core.Tests;

/// <summary>The Set &amp; Forget song lists: one entry per song, and the Custom Tuning list grouped by tuning.</summary>
public sealed class TuningSongListTests : IDisposable
{
    private static readonly int[] Standard = [0, 0, 0, 0, 0, 0];
    private static readonly int[] Dadgad = [-2, 0, 0, 0, -2, -2];
    private static readonly int[] DropDBass = [-2, 0, 0, 0, 0, 0];
    private static readonly int[] EbStandard = [-1, -1, -1, -1, -1, -1];
    private static readonly int[] EbBass = [-1, -1, -1, -1, 0, 0];

    private readonly TemporaryDirectory _folder = new();
    private readonly TuningService _service = new();

    public TuningSongListTests()
    {
        // Only E Standard is defined, so everything else shows as Custom Tuning.
        string path = _folder.File("tuning.database.json");
        File.WriteAllText(path, """
            {
              "Static": {
                "TuningDefinitions": {
                  "Standard": {
                    "UIName": "$[36970]E Standard",
                    "Strings": { "string0": 0, "string1": 0, "string2": 0, "string3": 0, "string4": 0, "string5": 0 }
                  }
                }
              }
            }
            """);
        _service.Load(path);
    }

    public void Dispose() => _folder.Dispose();

    [Fact]
    public void CustomTuningsAreGroupedByTuningWithOneEntryPerSong()
    {
        SongData[] songs =
        [
            Song("Alpha", "One", Part("Lead", Dadgad), Part("Bass", DropDBass, bass: true), Part("Bass", DropDBass, bass: true, alt: true)),
            Song("Beta", "Two", Part("Rhythm", EbStandard)),
            Song("Gamma", "Three", Part("Lead", Standard), Part("Bass", EbBass, bass: true)),
        ];

        IReadOnlyList<TuningSongGroup> groups = _service.GetCustomTuningGroups(songs);

        Assert.Equal(["Eb Ab Db Gb Bb Eb (-1 -1 -1 -1 -1 -1)", "D A D G A D (-2 0 0 0 -2 -2)"], groups.Select(g => g.Label));

        // Gamma's bass has no guitar part in the tuning, so it joins the guitar songs whose four bass strings match.
        Assert.Equal(["Beta - Two (Rhythm)", "Gamma - Three (Bass)"], groups[0].Songs);

        // Alpha's basses only tune four strings, which match its lead, so the song is one entry, and every part.
        Assert.Equal(["Alpha - One (All)"], groups[1].Songs);
        Assert.Equal(-2, groups[1].Strings.String5);
    }

    [Fact]
    public void BassWithNoMatchingGuitarTuningGetsItsOwnGroup()
    {
        SongData[] songs = [Song("Delta", "Four", Part("Bass", DropDBass, bass: true))];

        TuningSongGroup group = Assert.Single(_service.GetCustomTuningGroups(songs));

        Assert.Equal("Bass: D A D G (-2 0 0 0)", group.Label);
        Assert.True(group.BassOnly);
        Assert.Equal(["Delta - Four (Bass)"], group.Songs);
    }

    [Fact]
    public void SongsWithSelectedTuningAreOneEntryPerSongWithoutVocals()
    {
        SongData[] songs =
        [
            // Vocals don't count as a part, so Lead and Rhythm are all of Alpha's.
            Song("Alpha", "One", Part("Rhythm", Standard), Part("Lead", Standard), Part("Vocals", null)),
            Song("Beta", "Two", Part("Rhythm", Standard), Part("Lead", Standard, alt: true), Part("Lead", Dadgad)),
            Song("Gamma", "Three", Part("Bass", Standard, bass: true)),
            Song("Delta", "Four", Part("Lead", Dadgad)),
        ];

        Assert.Equal(
            ["Alpha - One (All)", "Beta - Two (Alt Lead & Rhythm)", "Gamma - Three (Bass)"],
            _service.GetSongsWithSelectedTuning("Standard", songs));
    }

    private static SongData Song(string artist, string title, params SongArrangement[] parts) =>
        new() { Artist = artist, Title = title, Arrangements = [.. parts] };

    private static SongArrangement Part(string name, int[]? offsets, bool bass = false, bool alt = false)
    {
        // The manifest model only lets its JSON reader set Attributes.
        var part = new SongArrangement();
        typeof(SongArrangement).GetProperty(nameof(SongArrangement.Attributes))!.SetValue(part, new SongArrangement.ArrangementAttributes
        {
            ArrangementName = name,
            ArrangementProperties = new SongArrangement.ArrangementAttributes.Properties
            {
                Represent = alt ? 0 : 1,
                PathLead = name == "Lead" ? 1 : 0,
                PathRhythm = name == "Rhythm" ? 1 : 0,
                PathBass = bass ? 1 : 0,
            },
            Tuning = offsets is null ? null : new ArrangementTuning
            {
                String0 = offsets[0], String1 = offsets[1], String2 = offsets[2],
                String3 = offsets[3], String4 = offsets[4], String5 = offsets[5],
            },
        });
        return part;
    }
}
