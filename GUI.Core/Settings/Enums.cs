namespace RSMods
{
    // Values are lowercased when stored in the INI,
    // so member names must match the expected INI string exactly (case-insensitive).
    public enum OnOffMode { Off, On }

    // Stored as an integer (0/1/2) in the INI, not as a string
    public enum NoteColorMode { Off = 0, RocksmithColors = 1, Custom = 2 }
    // Every mode the DLL reads (Settings.hpp StringColorMode), so a file set to Zag or Test isn't reset to Off.
    public enum CustomStringColorMode { Off = 0, Zag = 1, Custom = 2, Test = 3 }
    public enum FullscreenMode { Windowed = 0, NonExclusive = 1, Exclusive = 2 }
    public enum VisualQualityMode { Low = 0, Medium = 1, High = 2, Custom = 3 }
    public enum MsaaMode { Off = 1, X4 = 4 }
}
