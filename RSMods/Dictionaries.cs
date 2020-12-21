using System.Windows.Forms;
using System.Collections.Generic;


namespace RSMods
{
    class Dictionaries
    {
        public static Dictionary<int, TextBox> stringNumberToColorTextBox = new Dictionary<int, TextBox>() { }; // Can't put variables into it until after we create it.
        public static Dictionary<Control, string> TooltipDictionary = new Dictionary<Control, string>() { }; // I really wish I could make this full of stuff, but `this.` and `MainForm.` don't work, so I need to call a different function `FillToolTipDictionary()` do it for me. :(

        public static Dictionary<string, string> GuitarSpeakKeyPressDictionary = new Dictionary<string, string>()
        {
            {"Delete", ReadSettings.GuitarSpeakDeleteIdentifier},
            {"Space", ReadSettings.GuitarSpeakSpaceIdentifier},
            {"Enter", ReadSettings.GuitarSpeakEnterIdentifier},
            {"Tab", ReadSettings.GuitarSpeakTabIdentifier},
            {"Page Up", ReadSettings.GuitarSpeakPGUPIdentifier},
            {"Page Down", ReadSettings.GuitarSpeakPGDNIdentifier},
            {"Up Arrow", ReadSettings.GuitarSpeakUPIdentifier},
            {"Down Arrow", ReadSettings.GuitarSpeakDNIdentifier},
            {"Escape", ReadSettings.GuitarSpeakESCIdentifier},
            {"Open Bracket", ReadSettings.GuitarSpeakOBracketIdentifier},
            {"Close Bracket", ReadSettings.GuitarSpeakCBracketIdentifier},
            {"Tilde / Tilda", ReadSettings.GuitarSpeakTildeaIdentifier},
            {"Forward Slash", ReadSettings.GuitarSpeakForSlashIdentifier},
            {"Close Guitar Speak", ReadSettings.GuitarSpeakCloseIdentifier}
        };

        public static Dictionary<string, string> GuitarSpeakPresetDictionary = new Dictionary<string, string>();

        public static Dictionary<string, string> RefreshGuitarSpeakPresets()
        {
            GuitarSpeakPresetDictionary.Clear();
            GuitarSpeakPresetDictionary.Add("Delete: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakDeleteIdentifier)));
            GuitarSpeakPresetDictionary.Add("Space: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakSpaceIdentifier)));
            GuitarSpeakPresetDictionary.Add("Enter: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakEnterIdentifier)));
            GuitarSpeakPresetDictionary.Add("Tab: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakTabIdentifier)));
            GuitarSpeakPresetDictionary.Add("Page Up: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakPGUPIdentifier)));
            GuitarSpeakPresetDictionary.Add("Page Down: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakPGDNIdentifier)));
            GuitarSpeakPresetDictionary.Add("Up Arrow: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakUPIdentifier)));
            GuitarSpeakPresetDictionary.Add("Down Arrow: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakDNIdentifier)));
            GuitarSpeakPresetDictionary.Add("Escape: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakESCIdentifier)));
            GuitarSpeakPresetDictionary.Add("Open Bracket: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakOBracketIdentifier)));
            GuitarSpeakPresetDictionary.Add("Close Bracket: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakCBracketIdentifier)));
            GuitarSpeakPresetDictionary.Add("Tilde / Tilda: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakTildeaIdentifier)));
            GuitarSpeakPresetDictionary.Add("Forward Slash: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakForSlashIdentifier)));
            GuitarSpeakPresetDictionary.Add("Close Guitar Speak: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakCloseIdentifier)));
            return GuitarSpeakPresetDictionary;
        }

        public static Dictionary<int, string> SongListIndexToINISetting = new Dictionary<int, string>()
        {
            {0, ReadSettings.Songlist1Identifier},
            {1, ReadSettings.Songlist2Identifier},
            {2, ReadSettings.Songlist3Identifier},
            {3, ReadSettings.Songlist4Identifier},
            {4, ReadSettings.Songlist5Identifier},
            {5, ReadSettings.Songlist6Identifier}
        };

        public static Dictionary<int, string> KeybindingsIndexToINISetting = new Dictionary<int, string>()
        {
            {0, ReadSettings.ToggleLoftIdentifier},
            {1, ReadSettings.AddVolumeIdentifier},
            {2, ReadSettings.DecreaseVolumeIdentifier},
            {3, ReadSettings.ChangeSelectedVolumeKeyIdentifier},
            {4, ReadSettings.ShowSongTimerIdentifier},
            {5, ReadSettings.ForceReEnumerationIdentifier},
            {6, ReadSettings.RainbowStringsIdentifier},
            {7, ReadSettings.RemoveLyricsKeyIdentifier},
            {8, ReadSettings.RRSpeedKeyIdentifier}
        };

        public static Dictionary<bool, Dictionary<string, string>> stringColorButtonsToSettingIdentifiers = new Dictionary<bool, Dictionary<string, string>>()
        {
            { true, new Dictionary<string, string> { // Normal Colors
            
                {"E String", ReadSettings.String0Color_N_Identifier},
                {"A String", ReadSettings.String1Color_N_Identifier},
                {"D String", ReadSettings.String2Color_N_Identifier},
                {"G String", ReadSettings.String3Color_N_Identifier},
                {"B String", ReadSettings.String4Color_N_Identifier},
                {"e String", ReadSettings.String5Color_N_Identifier}
            }},

            { false,  new Dictionary<string, string> { // Colorblind Colors
            
                {"E String", ReadSettings.String0Color_CB_Identifier},
                {"A String", ReadSettings.String1Color_CB_Identifier},
                {"D String", ReadSettings.String2Color_CB_Identifier},
                {"G String", ReadSettings.String3Color_CB_Identifier},
                {"B String", ReadSettings.String4Color_CB_Identifier},
                {"e String", ReadSettings.String5Color_CB_Identifier}
            }}
        };

        public static List<string> currentModKeypressList = new List<string>()
        {
            "Toggle Loft",
            "Add Volume",
            "Decrease Volume",
            "Change Selected Volume",
            "Show Song Timer",
            "Force ReEnumeration",
            "Rainbow Strings",
            "Remove Lyrics",
            "RR Speed Change"
        };

        public static List<string> songlists = new List<string>();

        public static List<string> savedKeysForModToggles = new List<string>();

        public static List<string> refreshKeybindingList()
        {
            savedKeysForModToggles.Clear();
            savedKeysForModToggles.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ToggleLoftIdentifier)));
            savedKeysForModToggles.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.AddVolumeIdentifier)));
            savedKeysForModToggles.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.DecreaseVolumeIdentifier)));
            savedKeysForModToggles.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ChangeSelectedVolumeKeyIdentifier)));
            savedKeysForModToggles.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerIdentifier)));
            savedKeysForModToggles.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationIdentifier)));
            savedKeysForModToggles.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RainbowStringsIdentifier)));
            savedKeysForModToggles.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsKeyIdentifier)));
            savedKeysForModToggles.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RRSpeedKeyIdentifier)));
            return savedKeysForModToggles;
        }

        public static List<string> refreshSonglists()
        {
            songlists.Clear();
            songlists.Add(ReadSettings.ProcessSettings(ReadSettings.Songlist1Identifier));
            songlists.Add(ReadSettings.ProcessSettings(ReadSettings.Songlist2Identifier));
            songlists.Add(ReadSettings.ProcessSettings(ReadSettings.Songlist3Identifier));
            songlists.Add(ReadSettings.ProcessSettings(ReadSettings.Songlist4Identifier));
            songlists.Add(ReadSettings.ProcessSettings(ReadSettings.Songlist5Identifier));
            songlists.Add(ReadSettings.ProcessSettings(ReadSettings.Songlist6Identifier));
            return songlists;
        }

    }
}
