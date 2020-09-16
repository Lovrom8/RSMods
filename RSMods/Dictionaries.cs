using System;
using System.Windows.Forms;
using System.IO;
using System.Linq;
using System.Drawing;
using RSMods.Data;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json;
using System.Text.RegularExpressions;
using RSMods.Util;
using System.Reflection;
using SevenZip;
using RocksmithToolkitLib.DLCPackage;
using System.Collections.Generic;
using RocksmithToolkitLib.DLCPackage.Manifest2014.Tone;
using System.Management;
using RocksmithToolkitLib.Extensions;
using RSMods.Twitch;
using System.Threading;


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

        public static Dictionary<string, string> GuitarSpeakPresetDictionary = new Dictionary<string, string>()
        {
            {"Delete: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakDeleteIdentifier))},
            {"Space: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakSpaceIdentifier))},
            {"Enter: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakEnterIdentifier))},
            {"Tab: ",  GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakTabIdentifier))},
            {"Page Up: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakPGUPIdentifier))},
            {"Page Down: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakPGDNIdentifier))},
            {"Up Arrow: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakUPIdentifier))},
            {"Down Arrow: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakDNIdentifier))},
            {"Escape: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakESCIdentifier))},
            {"Open Bracket: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakOBracketIdentifier))},
            {"Close Bracket: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakCBracketIdentifier))},
            {"Tilde / Tilda: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakTildeaIdentifier))},
            {"Forward Slash: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakForSlashIdentifier))},
            {"Close Guitar Speak: ", GuitarSpeak.GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakCloseIdentifier))}
        };

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
            {3, ReadSettings.ShowSongTimerIdentifier},
            {4, ReadSettings.ForceReEnumerationIdentifier},
            {5, ReadSettings.RainbowStringsIdentifier},
            {6, ReadSettings.RemoveLyricsKeyIdentifier}
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
            "Show Song Timer",
            "Force ReEnumeration",
            "Rainbow Strings",
            "Remove Lyrics"
        };

        public static List<string> songlists = new List<string>()
        {
            ReadSettings.ProcessSettings(ReadSettings.Songlist1Identifier), // Song List 1
            ReadSettings.ProcessSettings(ReadSettings.Songlist2Identifier), // Song List 2
            ReadSettings.ProcessSettings(ReadSettings.Songlist3Identifier), // Song List 3
            ReadSettings.ProcessSettings(ReadSettings.Songlist4Identifier), // Song List 4
            ReadSettings.ProcessSettings(ReadSettings.Songlist5Identifier), // Song List 5
            ReadSettings.ProcessSettings(ReadSettings.Songlist6Identifier)  // Song List 6 
        };

        public static List<string> savedKeysForModToggles = new List<string>()
        {
            KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ToggleLoftIdentifier)),
            KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.AddVolumeIdentifier)),
            KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.DecreaseVolumeIdentifier)),
            KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerIdentifier)),
            KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationIdentifier)),
            KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RainbowStringsIdentifier)),
            KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsKeyIdentifier))
        };
        
    }
}
