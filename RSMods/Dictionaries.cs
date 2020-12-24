using System.Windows.Forms;
using System.Collections.Generic;


namespace RSMods
{
    public partial class MainForm : Form
    {
        private Dictionary<Control, string> TooltipDictionary = new Dictionary<Control, string>() {};

        private void FillToolTipDictionary()
        {
            // INI Edits
            // Checkboxes
            TooltipDictionary.Add(checkBox_ToggleLoft, "Disables the game background, amps and noise reactive speaker rings.\nBest used with Venue Mode off (setting in game).\nUsed by a lot of Rocksmith Streamers to make it easy to Luma Key out the game background.\nPlayer just sees an all black background when this is enabled.\nOptions for turning the loft off only when in a song, when the game first starts up, or on a key press.");
            TooltipDictionary.Add(checkBox_SongTimer, "Experimental.\nIntent is to show a box with your timestamp position through the song.");
            TooltipDictionary.Add(checkBox_ExtendedRange, "Alters the string and note colors to make it easier to play a 5 string bass or 7 string guitar.");
            TooltipDictionary.Add(checkBox_ForceEnumeration, "Game will automatically start an Enumeration sequence when a new psarc (CDLC) file is detected as having been added to the dlc folder.\nNot necesary to enable if you're already using Rocksniffer to do the same thing.");
            TooltipDictionary.Add(checkBox_RemoveHeadstock, "Stops the Headstock of the guitar being drawn.\n“Headless” guitar mode. Just cleans up some more of the UI.");
            TooltipDictionary.Add(checkBox_RemoveSkyline, "Removes the purple and orange bars from the top of the display in LAS.\nUse in conjunction with No Loft for a cleaner UI.\nOptions for always off, only off when in a song, or only when toggled by key press.");
            TooltipDictionary.Add(checkBox_GreenScreen, "Changes just a section of the game background to all black, amusing for a selective “green screen” stream experience.\nInvalidated by \"No Loft\".");
            TooltipDictionary.Add(checkBox_AutoLoadProfile, "Essentially holds down the ENTER key until the game has reached the main menu.\nLets you auto load the last used profile without needing to interact with the game at all.");
            TooltipDictionary.Add(checkBox_Fretless, "Removes the Fret Wire from the neck, making your instrument appear to be fretless.");
            TooltipDictionary.Add(checkBox_RemoveInlays, "Disables the guitar neck inlay display entirely.\nNote: This only works with the standard dot inlays.");
            TooltipDictionary.Add(checkBox_ControlVolume, "Allows you to control how loud the game is using the in-game mixer without needing to open it.\nAlso includes a hidden \"Master Volume\" control.");
            TooltipDictionary.Add(checkBox_GuitarSpeak, "Use your guitar to control the menus!");
            TooltipDictionary.Add(checkBox_RemoveLyrics, "Disables the display of song lyrics while in Learn-A-Song mode.");
            TooltipDictionary.Add(checkBox_RainbowStrings, "Experimental.\nHow Pro are you? This makes the players guitar strings constantly cycling through colors.");
            TooltipDictionary.Add(checkBox_CustomColors, "Lets you define the string / note colors you want.\nSaves a normal set and a Colorblind mode set.");
            TooltipDictionary.Add(checkBox_RemoveLaneMarkers, "Removes the additional lane marker lines seen in the display.\nWhen used with No Loft, provides a cleaner Luma Key.");
            TooltipDictionary.Add(checkBox_ScreenShotScores, "We will automatically take a steam screenshot whenever you finish a song");
            TooltipDictionary.Add(checkBox_RiffRepeaterSpeedAboveOneHundred, "Allow you to play a song faster than 100% speed in Riff Repeater");
            TooltipDictionary.Add(checkBox_ChangeTheme, "Use this feature to customize the colors used in this GUI.");
            TooltipDictionary.Add(checkBox_useMidiAutoTuning, "If you have a drop tuning pedal with a MIDI port, we will attempt to automatically tune.");
            TooltipDictionary.Add(checkBox_ShowCurrentNote, "Shows the note you are currently playing on screen.");

            // Mods
            TooltipDictionary.Add(groupBox_HowToEnumerate, "Choose to Enumerate on key press,\nor automatically scan for changes every X seconds and start enumeration if a new file has been added.");
            TooltipDictionary.Add(groupBox_ExtendedRangeWhen, "Mod is enabled when the lowest string is tuned to the note defined here.\nSee the Custom Colors - Color Blind mode for the colors that will be used while in ER mode.");
            TooltipDictionary.Add(groupBox_LoftOffWhen, "Turn the loft off via hotkey, as soon as the game starts up or only when in a song.");
            TooltipDictionary.Add(radio_colorBlindERColors, "When ER mode is enabled, these are the colors that the strings will be changed to.");
            TooltipDictionary.Add(groupBox_ToggleSkylineWhen, "Turn the skyline (Purple and Orange DD level bars) as soon as the game starts up, or only when in a song.");
            TooltipDictionary.Add(groupBox_ToggleLyricsOffWhen, "How or when do you want the lyric display disabled, always, or toggled by a hotkey only?");
            TooltipDictionary.Add(radio_LyricsAlwaysOff, "Lyrics display will always be disabled in Learn-A-Song game mode.");
            TooltipDictionary.Add(radio_LyricsOffHotkey, "Lyrics can be toggled on or off by a defined hotkey.");
            TooltipDictionary.Add(checkbox_GuitarSpeakWhileTuning, "For Advanced Users Only!\nUse Guitar Speak in tuning menus.\nThis can potentially stop you from tuning, or playing songs if setup improperly.");
            TooltipDictionary.Add(groupBox_MidiAutoTuneDevice, "Select the MIDI device that goes to your drop tuning pedal.\nWe will send a signal to the pedal to try to automatically tune it.");
            TooltipDictionary.Add(checkBox_WhammyChordsMode, "If you are using the Whammy or Whammy Bass.\nAre you using the pedal in Chords Mode or Classic Mode.\nClassic Mode = UnChecked, Chords Mode = Checked.");
            TooltipDictionary.Add(checkBox_ExtendedRangeDrop, "By default we require a song to be in standard to trigger Extended Range.\nTurn this on if you want drop tunings to also trigger Extended Range.\n(Ex: If you drop at B, but are playing Drop B, this checkbox will trigger Extended Range Mode)");
            TooltipDictionary.Add(groupBox_OnScreenFont, "If RSMods needs to show text in game, what font should we use?");

            // Misc
            TooltipDictionary.Add(groupBox_Songlist, "Custom names for the 6 \"SONG LISTS\" shown in game.");
            TooltipDictionary.Add(groupBox_Keybindings_MODS, "Set keybindings for the toggle on / off by keypress modifications.\nYou need to press ENTER after setting the key for it to be saved.");
            TooltipDictionary.Add(groupBox_Keybindings_AUDIO, "Set keybindings for changing the volume in game.\nPress the keybinding to increase the volume.\nPress control and the keybinding to decrease the volume.\nYou need to press ENTER after setting the key for it to be saved.");
            TooltipDictionary.Add(button_ResetModsToDefault, "Resets all RSMods values to defaults");

            // Set & Forget Mods (Cache.psarc Modifications)
            // Tones
            TooltipDictionary.Add(label_ChangeTonesHeader, "This section lets you change the default menu tone for Lead, Rhythm Or Bass.\nYou need to have the tone you want to set saved in your profile first,\nthen you can load it here and set it as the default tone that will be used when you start up Rocksmith.");
            TooltipDictionary.Add(button_LoadTones, "Step 1.\nClick this to load the tones that are saved in your profile.");
            TooltipDictionary.Add(listBox_ProfileTones, "Step2.\n Highlight a tone name.");
            TooltipDictionary.Add(radio_DefaultRhythmTone, "Set Highlighted Tone As New Default Rhythm Tone.");
            TooltipDictionary.Add(radio_DefaultLeadTone, "Set Highlighted Tone As New Default Lead Tone.");
            TooltipDictionary.Add(radio_DefaultBassTone, "Set Highlighted Tone As New Default Bass Tone.");
            TooltipDictionary.Add(button_AssignNewDefaultTone, "Assign the currently highlighted tone to the chosen path.");

            // Custom Tuning
            TooltipDictionary.Add(listBox_Tunings, "Shows the list of tuning definitions currently in Rocksmith.");
            TooltipDictionary.Add(button_AddTuning, "Adds the tuning as defined above.");
            TooltipDictionary.Add(button_RemoveTuning, "Removes the highlighted tuning.");
            TooltipDictionary.Add(nUpDown_String0, "Set the offset for the low-E string.");
            TooltipDictionary.Add(nUpDown_String1, "Set the offset for the A string.");
            TooltipDictionary.Add(nUpDown_String2, "Set the offset for the D string.");
            TooltipDictionary.Add(nUpDown_String3, "Set the offset for the G string.");
            TooltipDictionary.Add(nUpDown_String4, "Set the offset for the B string.");
            TooltipDictionary.Add(nUpDown_String5, "Set the offset for the high-E string.");
            TooltipDictionary.Add(button_SaveTuningChanges, "Saves the tuning list to Rocksmith.");

            // One Click Mods
            TooltipDictionary.Add(button_AddExitGame, "Replaces UPLAY on the main menu with an EXIT GAME option.");
            TooltipDictionary.Add(button_AddDCInput, "Adds the Direct Connect mode - microphone mode with tone simulations.");
            TooltipDictionary.Add(button_AddCustomTunings, "Adds some preset definitions for the most common Custom Tunings.");
            TooltipDictionary.Add(button_AddFastLoad, "Requires: SSD drive or faster.\nSkips some of the intro sequences.\nThis may cause the game to not launch properly.\nCombined with Auto Load Last Profile and huzzah!");

            // Misc
            TooltipDictionary.Add(button_RemoveTemp, "Removes the temporary files used by RSMods.");
            TooltipDictionary.Add(button_RestoreCacheBackup, "Restores the original cache.psarc file\nUndoes all \"Set-and-forget\" mods.");
            TooltipDictionary.Add(button_CleanUpUnpackedCache, "Removes temporary files and un-packs cache.psarc as it is being used now, again.");

            // Twitch Bot
            TooltipDictionary.Add(button_TwitchReAuthorize, "Click this to get the authorisation key needed to let these mods listen to your twitch alerts.\nIt is possible this button may need to be clicked to re-anable the triggers.");
            TooltipDictionary.Add(button_SolidNoteColorPicker, "Choose a color for this event trigger.");
            TooltipDictionary.Add(button_SolidNoteColorRandom, "This will choose a random color for you. \nThe color does not change per activation, what you see here is how it is set for good.");
            TooltipDictionary.Add(button_AddSelectedReward, "Add the configured event trigger.");
            TooltipDictionary.Add(button_RemoveReward, "Remove the selected event trigger.");
            TooltipDictionary.Add(button_TestTwitchReward, "Manually activate the mod without needing to have recieved a donation.");
            TooltipDictionary.Add(dgv_DefaultRewards, "Lists the possible events you can use to set a trigger.");
            TooltipDictionary.Add(dgv_EnabledRewards, "Lists the events you have configured - how long they are activated for - and their cost.");
            TooltipDictionary.Add(textBox_TwitchLog, "Shows notifications from Twitch - and what got triggered from these tools.");
            TooltipDictionary.Add(label_TwitchAuthorized, "Please take care to make sure none of these entries are shown on your stream.");
            TooltipDictionary.Add(label_TwitchUsername, "Please take care to make sure none of these entries are shown on your stream.");
            TooltipDictionary.Add(label_TwitchChannelID, "Please take care to make sure none of these entries are shown on your stream.");
            TooltipDictionary.Add(label_TwitchAccessTokenVal, "Please make sure this value is never shown live.\nClick to copy this to your clipboard.\nThis value is needed when asking for Twitch support from the RSMods devs.");
            TooltipDictionary.Add(label_TwitchUsernameVal, "Please make sure this value is never shown live.\nClick to copy this to your clipboard.\nThis value is needed when asking for Twitch support from the RSMods devs.");
            TooltipDictionary.Add(label_TwitchChannelIDVal, "Please make sure this value is never shown live.\nClick to copy this to your clipboard.\nThis value is needed when asking for Twitch support from the RSMods devs.");
            TooltipDictionary.Add(checkBox_RevealTwitchAuthToken, "Only reveal this when asked by RSMods developers.\nThis is how we look to see when events happen in your stream.");
        }

        public Dictionary<int, TextBox> stringNumberToColorTextBox = new Dictionary<int, TextBox>(){}; // Can't put variables into it until after we create it.

        private void StringColors_FillStringNumberToColorDictionary()
        {
            stringNumberToColorTextBox.Clear();

            stringNumberToColorTextBox.Add(0, textBox_String0Color);
            stringNumberToColorTextBox.Add(1, textBox_String1Color);
            stringNumberToColorTextBox.Add(2, textBox_String2Color);
            stringNumberToColorTextBox.Add(3, textBox_String3Color);
            stringNumberToColorTextBox.Add(4, textBox_String4Color);
            stringNumberToColorTextBox.Add(5, textBox_String5Color);
        }
    };

    class Dictionaries
    {
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


        public static List<string> SongListIndexToINISetting = new List<string>()
        {
            ReadSettings.Songlist1Identifier,
            ReadSettings.Songlist2Identifier,
            ReadSettings.Songlist3Identifier,
            ReadSettings.Songlist4Identifier,
            ReadSettings.Songlist5Identifier,
            ReadSettings.Songlist6Identifier
        };

        public static List<string> KeybindingsIndexToINISetting = new List<string>()
        {
            ReadSettings.ToggleLoftIdentifier,
            ReadSettings.ShowSongTimerIdentifier,
            ReadSettings.ForceReEnumerationIdentifier,
            ReadSettings.RainbowStringsIdentifier,
            ReadSettings.RemoveLyricsKeyIdentifier,
            ReadSettings.RRSpeedKeyIdentifier
        };

        public static List<string> AudioKeybindingsIndexToINISetting = new List<string>()
        {
            ReadSettings.MasterVolumeKeyIdentifier,
            ReadSettings.SongVolumeKeyIdentifier,
            ReadSettings.Player1VolumeKeyIdentifier,
            ReadSettings.Player2VolumeKeyIdentifier,
            ReadSettings.MicrophoneVolumeKeyIdentifier,
            ReadSettings.VoiceOverVolumeKeyIdentifier,
            ReadSettings.SFXVolumeKeyIdentifier,
            ReadSettings.ChangeSelectedVolumeKeyIdentifier
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
            "Show Song Timer",
            "Force ReEnumeration",
            "Rainbow Strings",
            "Remove Lyrics",
            "RR Speed Change"
        };

        public static List<string> currentAudioKeypressList = new List<string>()
        {
            "Master Volume",
            "Song Volume",
            "Player 1 Volume",
            "Player 2 Volume",
            "Microphone Volume",
            "Voice-Over Volume",
            "SFX Volume",
            "Show Volume On Screen"
        };

        public static List<string> songlists = new List<string>();

        public static List<string> savedKeysForModToggles = new List<string>();

        public static List<string> savedKeysForVolumes = new List<string>();

        public static List<string> refreshKeybindingList()
        {
            savedKeysForModToggles.Clear();
            savedKeysForModToggles.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ToggleLoftIdentifier)));
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

        public static List<string> refreshAudioKeybindingList()
        {
            savedKeysForVolumes.Clear();
            savedKeysForVolumes.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.MasterVolumeKeyIdentifier)));
            savedKeysForVolumes.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.SongVolumeKeyIdentifier)));
            savedKeysForVolumes.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.Player1VolumeKeyIdentifier)));
            savedKeysForVolumes.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.Player2VolumeKeyIdentifier)));
            savedKeysForVolumes.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.MicrophoneVolumeKeyIdentifier)));
            savedKeysForVolumes.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.VoiceOverVolumeKeyIdentifier)));
            savedKeysForVolumes.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.SFXVolumeKeyIdentifier)));
            savedKeysForVolumes.Add(KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ChangeSelectedVolumeKeyIdentifier)));
            return savedKeysForVolumes;
        }

    }
}
