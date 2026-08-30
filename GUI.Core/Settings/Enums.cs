namespace RSMods
{
    // Values are lowercased when stored in the INI,
    // so member names must match the expected INI string exactly (case-insensitive).

    public enum LoftMode { Manual, Startup, Song }
    public enum SkylineMode { Song, Startup }
    public enum LyricsMode { Manual, Startup }
    public enum HeadstockMode { Song, Startup }
    public enum EnumerationMode { Off, Automatic, Manual }
    public enum AutoTuneWhen { Manual, Tuner }
    public enum SongTimerWhen { Manual, Automatic }

    public enum OnOffMode { Off, On }

    // Stored as an integer (0/1/2) in the INI, not as a string
    public enum NoteColorMode { Off = 0, RocksmithColors = 1, Custom = 2 }
    public enum CustomStringColorMode { Off = 0, Custom = 2 }
    public enum FullscreenMode { Windowed = 0, NonExclusive = 1, Exclusive = 2 }
    public enum VisualQualityMode { Low = 0, Medium = 1, High = 2, Custom = 3 }
    public enum MsaaMode { Off = 1, X4 = 4 }
    public enum TuningPedalDevice { None = 0, WhammyDT = 1, WhammyBass = 2, WhammyFive = 3, SoftwarePedal = 4, WhammyFour = 5 }
}
