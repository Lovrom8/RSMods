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

#pragma warning disable IDE0017 // ... Warning about how code can be simplified... Yeah I know it isn't perfect.
#pragma warning disable IDE0044 // "This should be readonly" .... No. No it shouldn't.
#pragma warning disable IDE0059 // "You made this variable and didn't use it". It's called future proofing.
#pragma warning disable IDE0071 // "Interpolation can be simplified"
#pragma warning disable CS0168 // Variable Declared But Not Used
#pragma warning disable IDE0018 // Variable Declaration can be inlined, but it can't or it will shoot an error.
#pragma warning disable IDE1006 // Naming Rule Violation... We can name our functions whatever we want.... Plus it was an auto-generated one made by YOU so stop complaining.

namespace RSMods
{
    public partial class MainForm : Form
    {
        // Global Vars
        bool HasToolTipDictionaryBeenCreatedYet = false;
        bool CreatedToolTipYet = false;

        public MainForm()
        {
            string RSFolder = GenUtil.GetRSDirectory();
            if (RSFolder == String.Empty)
            {
                MessageBox.Show("We cannot detect where you have Rocksmith located. Please try reinstalling your game on Steam.", "Error: RSLocation Not Found", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Environment.Exit(1);
                return;
            }
            else
                Constants.RSFolder = RSFolder;

            // Check if the GUI settings, and DLL settings already exist
            WriteSettings.IsVoid(GenUtil.GetRSDirectory());
            if (!File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini")))
                WriteSettings.WriteINI(WriteSettings.Settings); // Creates Settings File

            if (!File.Exists(Constants.SettingsPath))
                File.WriteAllText(Constants.SettingsPath, "RSPath = " + Constants.RSFolder);

            TwitchSettings.LoadSettings();

            InitializeComponent();
            Text = $"{Text}-{Assembly.GetExecutingAssembly().GetName().Version.ToString()}";

            // Fill Songlist List
            listBox_Songlist.Items.AddRange(new object[] {
                    ReadSettings.ProcessSettings(ReadSettings.Songlist1Identifier), // Song List 1
                    ReadSettings.ProcessSettings(ReadSettings.Songlist2Identifier), // Song List 2
                    ReadSettings.ProcessSettings(ReadSettings.Songlist3Identifier), // Song List 3
                    ReadSettings.ProcessSettings(ReadSettings.Songlist4Identifier), // Song List 4
                    ReadSettings.ProcessSettings(ReadSettings.Songlist5Identifier), // Song List 5
                    ReadSettings.ProcessSettings(ReadSettings.Songlist6Identifier)  // Song List 6 
                });

            // Fill Modlist List
            listBox_Modlist.Items.AddRange(new object[] {
                "Toggle Loft",
                "Add Volume",
                "Decrease Volume",
                "Show Song Timer",
                "Force ReEnumeration",
                "Rainbow Strings",
                "Remove Lyrics"});

            // Load Keybinding Values
            ShowCurrentKeybindingValues();

            // Guitar Speak Preset Values
            RefreshGuitarSpeakPresets();

            // Load Checkbox Values From RSMods.ini
            {
                if (ReadSettings.ProcessSettings(ReadSettings.ToggleLoftEnabledIdentifier) == "on") // Toggle Loft Enabled / Disabled
                {
                    checkBox_ToggleLoft.Checked = true;
                    radio_LoftAlwaysOff.Visible = true;
                    radio_LoftOffHotkey.Visible = true;
                    radio_LoftOffInSong.Visible = true;
                    groupBox_LoftOffWhen.Visible = true;

                    if (ReadSettings.ProcessSettings(ReadSettings.ToggleLoftWhenIdentifier) == "startup")
                        radio_LoftAlwaysOff.Checked = true;
                    else if (ReadSettings.ProcessSettings(ReadSettings.ToggleLoftWhenIdentifier) == "manual")
                        radio_LoftOffHotkey.Checked = true;
                    else if (ReadSettings.ProcessSettings(ReadSettings.ToggleLoftWhenIdentifier) == "song")
                        radio_LoftOffInSong.Checked = true;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.AddVolumeEnabledIdentifier) == "on") // Add Volume Enabled / Disabled
                    checkBox_ControlVolume.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.DecreaseVolumeEnabledIdentifier) == "on") // Decrease Volume Enabled / Disabled
                    checkBox_ControlVolume.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerEnabledIdentifier) == "on") // Show Song Timer Enabled / Disabled
                    checkBox_SongTimer.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationEnabledIdentifier) != "off") // Force Enumeration Settings
                {
                    checkBox_ForceEnumeration.Checked = true;
                    radio_ForceEnumerationAutomatic.Visible = true;
                    radio_ForceEnumerationManual.Visible = true;
                    groupBox_HowToEnumerate.Visible = true;
                    if (ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationEnabledIdentifier) == "automatic")
                        radio_ForceEnumerationAutomatic.Checked = true;
                    else
                        radio_ForceEnumerationManual.Checked = true;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.RainbowStringsEnabledIdentifier) == "on") // Rainbow String Enabled / Disabled
                    checkBox_RainbowStrings.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.ExtendedRangeEnabledIdentifier) == "on") // Extended Range Enabled / Disabled
                {
                    checkBox_ExtendedRange.Checked = true;
                    groupBox_ExtendedRangeWhen.Visible = true;
                    listBox_ExtendedRangeTunings.Visible = true;
                }
                if (ReadSettings.ProcessSettings(ReadSettings.CustomStringColorNumberIndetifier) != "0") // Custom String Colors
                {
                    checkBox_CustomColors.Checked = true;
                    groupBox_StringColors.Visible = true;
                }

                /* Disco Mode: Deprecated, as of now, because you can't toggle it off easily.

                 if (ReadSettings.ProcessSettings(ReadSettings.DiscoModeIdentifier) == "on") // Disco Mode Enabled / Disabled
                    DiscoModeCheckbox.Checked = true;
                 */

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveHeadstockIdentifier) == "on") // Remove Headstock Enabled / Disabled
                {
                    checkBox_RemoveHeadstock.Checked = true;
                    groupBox_ToggleHeadstockOffWhen.Visible = true;

                    if (ReadSettings.ProcessSettings(ReadSettings.RemoveHeadstockWhenIdentifier) == "startup")
                        radio_HeadstockAlwaysOff.Checked = true;
                    else if (ReadSettings.ProcessSettings(ReadSettings.RemoveHeadstockWhenIdentifier) == "song")
                        radio_HeadstockOffInSong.Checked = true;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveSkylineIdentifier) == "on") // Remove Skyline Enabled / Disabled
                {
                    checkBox_RemoveSkyline.Checked = true;
                    groupBox_ToggleSkylineWhen.Visible = true;

                    if (ReadSettings.ProcessSettings(ReadSettings.ToggleSkylineWhenIdentifier) == "song") // Remove Skyline on Song Load
                        radio_SkylineOffInSong.Checked = true;
                    else if (ReadSettings.ProcessSettings(ReadSettings.ToggleSkylineWhenIdentifier) == "startup") // Remove Skyline on Game Startup 
                        radio_SkylineAlwaysOff.Checked = true;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.GreenScreenWallIdentifier) == "on") // Greenscreen Wall Enabled / Disabled
                    checkBox_GreenScreen.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.ForceProfileEnabledIdentifier) == "on") // Force Load Profile On Game Boot Enabled / Disabled
                    checkBox_AutoLoadProfile.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.FretlessModeEnabledIdentifier) == "on") // Fretless Mode Enabled / Disabled
                    checkBox_Fretless.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveInlaysIdentifier) == "on") // Remove Inlay Markers Enabled / Disabled
                    checkBox_RemoveInlays.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveLaneMarkersIdentifier) == "on") // Remove Line Markers Enabled / Disabled
                    checkBox_RemoveLineMarkers.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsIdentifier) == "on") // Remove Lyrics
                {
                    checkBox_RemoveLyrics.Checked = true;
                    groupBox_ToggleLyricsOffWhen.Visible = true;

                    if (ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsWhenIdentifier) == "startup") // Remove Lyrics When ...
                        radio_LyricsAlwaysOff.Checked = true;
                    else if (ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsWhenIdentifier) == "manual") // Remove Lyrics When ...
                        radio_LyricsOffHotkey.Checked = true;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakIdentifier) == "on")
                {
                    checkBox_GuitarSpeak.Checked = true;
                    groupBox_GuitarSpeak.Visible = true;
                    checkbox_GuitarSpeakWhileTuning.Visible = true;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakTuningIdentifier) == "on")
                    checkbox_GuitarSpeakWhileTuning.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.CustomGUIThemeIdentifier) == "on")
                    checkBox_ChangeTheme.Checked = true;
            }

            // Initialize Default String Colors
            LoadDefaultStringColors();

            // Load Colors Saved as Theme Colors.
            LoadCustomThemeColors();

            // Mod Settings
            nUpDown_ForceEnumerationXMS.Value = Decimal.Parse(ReadSettings.ProcessSettings(ReadSettings.CheckForNewSongIntervalIdentifier)) / 1000; // Loads old settings for enumeration every x ms
            listBox_ExtendedRangeTunings.SelectedIndex = (Convert.ToInt32(ReadSettings.ProcessSettings(ReadSettings.ExtendedRangeTuningIdentifier)) * -1) - 2;

            // Set And Forget Mods
            LoadSetAndForgetMods();
        }

        private void ModList_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch (listBox_Modlist.SelectedIndex)
            {
                case 0:
                    textBox_NewKeyAssignment.Text = KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ToggleLoftIdentifier));
                    break;
                case 1:
                    textBox_NewKeyAssignment.Text = KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.AddVolumeIdentifier));
                    break;
                case 2:
                    textBox_NewKeyAssignment.Text = KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.DecreaseVolumeIdentifier));
                    break;
                case 3:
                    textBox_NewKeyAssignment.Text = KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerIdentifier));
                    break;
                case 4:
                    textBox_NewKeyAssignment.Text = KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationIdentifier));
                    break;
                case 5:
                    textBox_NewKeyAssignment.Text = KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RainbowStringsIdentifier));
                    break;
                case 6:
                    textBox_NewKeyAssignment.Text = KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsKeyIdentifier));
                    break;

                default:
                    textBox_NewKeyAssignment.Text = String.Empty;
                    break;
            }
        }

        public static Dictionary<Control, bool> themeDictionaryBackColor = new Dictionary<Control, bool>() { }; // Can't be filled by default, so we run a function to fill it.
        public static Dictionary<Control, bool> themeDictionaryForeColor = new Dictionary<Control, bool>() { }; // Can't be filled by default, so we run a function to fill it.

        private void FillThemeDictionary()
        {
            // We don't want duplicate values, so let's clear the dictionaries just to make sure.
            themeDictionaryBackColor.Clear();
            themeDictionaryForeColor.Clear();

            // Lets start with the Back Colors

            // Tabs
            themeDictionaryBackColor.Add(tab_Songlists, true);
            themeDictionaryBackColor.Add(tab_Keybindings, true);
            themeDictionaryBackColor.Add(tab_ModToggles, true);
            themeDictionaryBackColor.Add(tab_ModSettings, true);
            themeDictionaryBackColor.Add(tab_SetAndForget, true);
            themeDictionaryBackColor.Add(tab_Twitch, true);
            themeDictionaryBackColor.Add(tab_GUISettings, true);

            // Group Boxes
            themeDictionaryBackColor.Add(groupBox_SetAndForget, true);

            // List Boxes
            themeDictionaryBackColor.Add(listBox_Songlist, true);
            themeDictionaryBackColor.Add(listBox_Modlist, true);
            themeDictionaryBackColor.Add(listBox_Tunings, true);
            themeDictionaryBackColor.Add(listBox_ProfileTones, true);
            themeDictionaryBackColor.Add(listBox_ExtendedRangeTunings, true);
            themeDictionaryBackColor.Add(listBox_GuitarSpeakKeypress, true);
            themeDictionaryBackColor.Add(listBox_GuitarSpeakOctave, true);
            themeDictionaryBackColor.Add(listBox_GuitarSpeakNote, true);
            themeDictionaryBackColor.Add(listBox_GuitarSpeakPresets, true);

            // Now let's fill the ForeColors.

            // Group Boxes
            themeDictionaryForeColor.Add(groupBox_ExtendedRangeWhen, false);
            themeDictionaryForeColor.Add(groupBox_GuitarSpeak, false);
            themeDictionaryForeColor.Add(groupBox_Keybindings, false);
            themeDictionaryForeColor.Add(groupBox_Songlist, false);
            themeDictionaryForeColor.Add(groupBox_ToggleHeadstockOffWhen, false);
            themeDictionaryForeColor.Add(groupBox_HowToEnumerate, false);
            themeDictionaryForeColor.Add(groupBox_LoftOffWhen, false);
            themeDictionaryForeColor.Add(groupBox_ToggleSkylineWhen, false);
            themeDictionaryForeColor.Add(groupBox_StringColors, false);
            themeDictionaryForeColor.Add(groupBox_ToggleLyricsOffWhen, false);
            themeDictionaryForeColor.Add(groupBox_SetAndForget, false);

            // List Boxes
            themeDictionaryForeColor.Add(listBox_Songlist, false);
            themeDictionaryForeColor.Add(listBox_Modlist, false);
            themeDictionaryForeColor.Add(listBox_Tunings, false);
            themeDictionaryForeColor.Add(listBox_ProfileTones, false);
            themeDictionaryForeColor.Add(listBox_ExtendedRangeTunings, false);
            themeDictionaryForeColor.Add(listBox_GuitarSpeakKeypress, false);
            themeDictionaryForeColor.Add(listBox_GuitarSpeakOctave, false);
            themeDictionaryForeColor.Add(listBox_GuitarSpeakNote, false);
            themeDictionaryForeColor.Add(listBox_GuitarSpeakPresets, false);
        }

        private void ChangeTheme(Color backgroundColor, Color textColor)
        {
            FillThemeDictionary(); // We need to fill the dictionary since it doesn't like filling itself with form elements.

            BackColor = backgroundColor; // MainForm BackColor
            ForeColor = textColor; // MainForm ForeColor

            foreach (KeyValuePair<Control, bool> backColorToChange in themeDictionaryBackColor)
                backColorToChange.Key.BackColor = backgroundColor;
            foreach (KeyValuePair<Control, bool> foreColorToChange in themeDictionaryForeColor)
                foreColorToChange.Key.ForeColor = textColor;
        }

        private void CheckKeyPressesDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter) // If enter is pressed
            {
                e.SuppressKeyPress = true; // Turns off the windows beep for pressing an invalid key.
                Save_Songlists_Keybindings(sender, e);
            }

            else if (sender == textBox_NewKeyAssignment)
            {
                e.SuppressKeyPress = true; // Turns off the windows beep for pressing an invalid key.

                if (KeyConversion.KeyDownDictionary.Contains(e.KeyCode))
                    textBox_NewKeyAssignment.Text = e.KeyCode.ToString();
                // Number or Letter was pressed (Will be overrided by text input)
                else if ((e.KeyValue > 47 && e.KeyValue < 60) || (e.KeyValue > 64 && e.KeyValue < 91))
                {
                    if (MessageBox.Show("The key you entered is currently used by Rocksmith and may interfere with being able to use the game properly. Are you sure you want to use this keybinding?", "Keybinding Warning!", MessageBoxButtons.OKCancel, MessageBoxIcon.Warning) == DialogResult.OK)
                        return;
                    else
                        textBox_NewKeyAssignment.Text = "";
                }
            }
        }

        private void CheckKeyPressesUp(object sender, KeyEventArgs e)
        {
            if (KeyConversion.KeyUpDictionary.Contains(e.KeyCode))
                textBox_NewKeyAssignment.Text = e.KeyCode.ToString();
        }

        private void CheckMouseInput(object sender, MouseEventArgs e)
        {
            if (KeyConversion.MouseButtonDictionary.Contains(e.Button))
                textBox_NewKeyAssignment.Text = e.Button.ToString();
        }

        private void ResetToDefaultSettings(object sender, EventArgs e)
        {
            if (MessageBox.Show("Are you sure you want to reset your mod settings to their defaults?", "WARNING: RESET TO DEFAULT?", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
            {
                File.Delete(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini"));
                RefreshForm();
                WriteSettings.WriteINI(WriteSettings.Settings); // Refresh Form will regenerate all the settings, so we need to overwrite them.
            }
            else
                MessageBox.Show("All your settings have been saved, and nothing was reset");
        }

        private void RefreshForm()
        {
            Hide();
            var newForm = new MainForm();
            newForm.Closed += (s, args) => Close();
            newForm.Show();
        }

        public static Dictionary<string, Dictionary<string, string>> priorSettings = new Dictionary<string, Dictionary<string, string>>()
        {
            // [Section]                         mod    default
            {"[SongListTitles]", new Dictionary<string, string> {
                { String.Empty, ReadSettings.ProcessSettings(ReadSettings.Songlist1Identifier)}, // For some reason this dictionary craps the bed on the first line, so here's a "fix" until we find a better solution.
                { ReadSettings.Songlist1Identifier, ReadSettings.ProcessSettings(ReadSettings.Songlist1Identifier) }, // Songlist 1
                { ReadSettings.Songlist2Identifier, ReadSettings.ProcessSettings(ReadSettings.Songlist2Identifier) }, // Songlist 2
                { ReadSettings.Songlist3Identifier, ReadSettings.ProcessSettings(ReadSettings.Songlist3Identifier) }, // Songlist 3
                { ReadSettings.Songlist4Identifier, ReadSettings.ProcessSettings(ReadSettings.Songlist4Identifier) }, // Songlist 4
                { ReadSettings.Songlist5Identifier, ReadSettings.ProcessSettings(ReadSettings.Songlist5Identifier) }, // Songlist 5
                { ReadSettings.Songlist6Identifier, ReadSettings.ProcessSettings(ReadSettings.Songlist6Identifier) }, // Songlist 6
            }},
            {"[Keybinds]", new Dictionary<string, string> {
                { ReadSettings.ToggleLoftIdentifier, ReadSettings.ProcessSettings(ReadSettings.ToggleLoftIdentifier) }, // Toggle Loft
                { ReadSettings.AddVolumeIdentifier, ReadSettings.ProcessSettings(ReadSettings.AddVolumeIdentifier) }, // Add Volume
                { ReadSettings.DecreaseVolumeIdentifier, ReadSettings.ProcessSettings(ReadSettings.DecreaseVolumeIdentifier) }, // Decrease Volume
                { ReadSettings.ShowSongTimerIdentifier, ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerIdentifier) }, // Show Song Timer
                { ReadSettings.ForceReEnumerationIdentifier, ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationIdentifier) }, // Force ReEnumeration
                { ReadSettings.RainbowStringsIdentifier, ReadSettings.ProcessSettings(ReadSettings.RainbowStringsIdentifier) }, // Rainbow Strings
                { ReadSettings.RemoveLyricsKeyIdentifier, ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsKeyIdentifier) }, // Remove Lyrics Key
            }},
            {"[Toggle Switches]", new Dictionary<string, string>
            {
                { ReadSettings.ToggleLoftEnabledIdentifier, ReadSettings.ProcessSettings(ReadSettings.ToggleLoftEnabledIdentifier) }, // Toggle Loft Enabled / Disabled
                { ReadSettings.AddVolumeEnabledIdentifier, ReadSettings.ProcessSettings(ReadSettings.AddVolumeEnabledIdentifier) }, // Add Volume Enabled / Disabled
                { ReadSettings.DecreaseVolumeEnabledIdentifier, ReadSettings.ProcessSettings(ReadSettings.DecreaseVolumeEnabledIdentifier) }, // Decrease Volume Enabled/ Disabled
                { ReadSettings.ShowSongTimerEnabledIdentifier, ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerEnabledIdentifier) }, // Show Song Timer Enabled / Disabled
                { ReadSettings.ForceReEnumerationEnabledIdentifier, ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationEnabledIdentifier) }, // Force ReEnumeration Manual / Automatic / Disabled
                { ReadSettings.RainbowStringsEnabledIdentifier, ReadSettings.ProcessSettings(ReadSettings.RainbowStringsEnabledIdentifier) }, // Rainbow String Enabled / Disabled
                { ReadSettings.ExtendedRangeEnabledIdentifier, ReadSettings.ProcessSettings(ReadSettings.ExtendedRangeEnabledIdentifier) }, // Extended Range Enabled / Disabled
                { ReadSettings.CustomStringColorNumberIndetifier, ReadSettings.ProcessSettings(ReadSettings.CustomStringColorNumberIndetifier) }, // Custom String Colors (0 - Default, 1 - ZZ, 2 - Custom Colors)
                //{ ReadSettings.DiscoModeIdentifier, ReadSettings.ProcessSettings(ReadSettings.DiscoModeIdentifier) }, // Disco Mode Enabled / Disabled
                { ReadSettings.RemoveHeadstockIdentifier, ReadSettings.ProcessSettings(ReadSettings.RemoveHeadstockIdentifier) }, // Remove Headstock Enabled / Disabled
                { ReadSettings.RemoveSkylineIdentifier, ReadSettings.ProcessSettings(ReadSettings.RemoveSkylineIdentifier) }, // Remove Skyline Enabled / Disabled
                { ReadSettings.GreenScreenWallIdentifier, ReadSettings.ProcessSettings(ReadSettings.GreenScreenWallIdentifier)}, // Greenscreen Back Wall Enabled / Disabled
                { ReadSettings.ForceProfileEnabledIdentifier, ReadSettings.ProcessSettings(ReadSettings.ForceProfileEnabledIdentifier) }, // Force Load Profile On Game Boot Enabled / Disabled
                { ReadSettings.FretlessModeEnabledIdentifier, ReadSettings.ProcessSettings(ReadSettings.FretlessModeEnabledIdentifier) }, // Fretless Mode Enabled / Disabled
                { ReadSettings.RemoveInlaysIdentifier, ReadSettings.ProcessSettings(ReadSettings.RemoveInlaysIdentifier) }, // Remove Inlay Markers Enabled / Disabled
                { ReadSettings.ToggleLoftWhenIdentifier, ReadSettings.ProcessSettings(ReadSettings.ToggleLoftWhenIdentifier) }, // Define how or when the loft is disabled - game startup, on key command, or in song only
                { ReadSettings.RemoveLaneMarkersIdentifier, ReadSettings.ProcessSettings(ReadSettings.RemoveLaneMarkersIdentifier) }, // Remove Lane Markers Enabled / Disabled
                { ReadSettings.ToggleSkylineWhenIdentifier, ReadSettings.ProcessSettings(ReadSettings.ToggleSkylineWhenIdentifier) }, // Define how or when the skyline is disabled - game startup, or in song only
                { ReadSettings.RemoveLyricsIdentifier, ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsIdentifier) }, // Remove Song Lyrics Enabled / Disabled
                { ReadSettings.RemoveLyricsWhenIdentifier, ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsWhenIdentifier) }, // Remove Song Lyrics When Manual / Automatic
                { ReadSettings.GuitarSpeakIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakIdentifier) }, // Guitar Speak Enabled / Disabled
                { ReadSettings.RemoveHeadstockWhenIdentifier, ReadSettings.ProcessSettings(ReadSettings.RemoveHeadstockWhenIdentifier) }, // Remove Headstock When Startup / Song
            }},
            {"[String Colors]", new Dictionary<string, string>
            {
                { ReadSettings.String0Color_N_Identifier, ReadSettings.ProcessSettings(ReadSettings.String0Color_N_Identifier) }, // Default Low E String Color (HEX) | Red
                { ReadSettings.String1Color_N_Identifier, ReadSettings.ProcessSettings(ReadSettings.String1Color_N_Identifier) }, // Default A String Color (HEX) | Yellow
                { ReadSettings.String2Color_N_Identifier, ReadSettings.ProcessSettings(ReadSettings.String2Color_N_Identifier) }, // Default D String Color (HEX) | Blue
                { ReadSettings.String3Color_N_Identifier, ReadSettings.ProcessSettings(ReadSettings.String3Color_N_Identifier) }, // Default G String Color (HEX) | Orange
                { ReadSettings.String4Color_N_Identifier, ReadSettings.ProcessSettings(ReadSettings.String4Color_N_Identifier) }, // Default B String Color (HEX) | Green
                { ReadSettings.String5Color_N_Identifier, ReadSettings.ProcessSettings(ReadSettings.String5Color_N_Identifier) }, // Default High E String Color (HEX) | Purple

                { ReadSettings.String0Color_CB_Identifier, ReadSettings.ProcessSettings(ReadSettings.String0Color_CB_Identifier) }, // Colorblind Low E String Color (HEX) | Red
                { ReadSettings.String1Color_CB_Identifier, ReadSettings.ProcessSettings(ReadSettings.String1Color_CB_Identifier) }, // Colorblind A String Color (HEX) | Yellow
                { ReadSettings.String2Color_CB_Identifier, ReadSettings.ProcessSettings(ReadSettings.String2Color_CB_Identifier) }, // Colorblind D String Color (HEX) | Blue
                { ReadSettings.String3Color_CB_Identifier, ReadSettings.ProcessSettings(ReadSettings.String3Color_CB_Identifier) }, // Colorblind G String Color (HEX) | Orange
                { ReadSettings.String4Color_CB_Identifier, ReadSettings.ProcessSettings(ReadSettings.String4Color_CB_Identifier) }, // Colorblind B String Color (HEX) | Green
                { ReadSettings.String5Color_CB_Identifier, ReadSettings.ProcessSettings(ReadSettings.String5Color_CB_Identifier) }, // Colorblind High E String Color (HEX) | Purple
            }},
            {"[Mod Settings]", new Dictionary<string, string>
            {
                { ReadSettings.ExtendedRangeTuningIdentifier, ReadSettings.ProcessSettings(ReadSettings.ExtendedRangeTuningIdentifier) }, // Enable Extended Range Mode When Low E Is X Below E
                { ReadSettings.CheckForNewSongIntervalIdentifier, ReadSettings.ProcessSettings(ReadSettings.CheckForNewSongIntervalIdentifier) }, // Enumerate new CDLC / ODLC every X ms
            }},
            {"[Guitar Speak]", new Dictionary<string, string>
            {
                { ReadSettings.GuitarSpeakDeleteIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakDeleteIdentifier) },
                { ReadSettings.GuitarSpeakSpaceIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakSpaceIdentifier) },
                { ReadSettings.GuitarSpeakEnterIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakEnterIdentifier) },
                { ReadSettings.GuitarSpeakTabIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakTabIdentifier) },
                { ReadSettings.GuitarSpeakPGUPIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakPGUPIdentifier) },
                { ReadSettings.GuitarSpeakPGDNIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakPGDNIdentifier) },
                { ReadSettings.GuitarSpeakUPIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakUPIdentifier) },
                { ReadSettings.GuitarSpeakDNIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakDNIdentifier) },
                { ReadSettings.GuitarSpeakESCIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakESCIdentifier) },
                { ReadSettings.GuitarSpeakCloseIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakCloseIdentifier) },
                { ReadSettings.GuitarSpeakOBracketIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakOBracketIdentifier) },
                { ReadSettings.GuitarSpeakCBracketIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakCBracketIdentifier) },
                { ReadSettings.GuitarSpeakTildeaIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakTildeaIdentifier) },
                { ReadSettings.GuitarSpeakForSlashIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakForSlashIdentifier) },
                { ReadSettings.GuitarSpeakTuningIdentifier, ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakTuningIdentifier) },
            }},
            {"[GUI Settings]", new Dictionary<string, string>
            {
                { ReadSettings.CustomGUIThemeIdentifier, ReadSettings.ProcessSettings(ReadSettings.CustomGUIThemeIdentifier) },
                { ReadSettings.CustomGUIBackgroundColorIdentifier, ReadSettings.ProcessSettings(ReadSettings.CustomGUIBackgroundColorIdentifier) },
                { ReadSettings.CustomGUITextColorIdentifier, ReadSettings.ProcessSettings(ReadSettings.CustomGUITextColorIdentifier) },
            }}
        };

        private void SaveChanges(string IdentifierToChange, string ChangedSettingValue)
        {
            // Right before launch, we switched from the boolean names of (true / false) to (on / off) for users to be able to edit the mods without the GUI (by hand).
            if (ChangedSettingValue == "true")
                ChangedSettingValue = "on";
            else if (ChangedSettingValue == "false")
                ChangedSettingValue = "off";

            foreach (string section in priorSettings.Keys)
            {
                foreach (KeyValuePair<string, string> entry in priorSettings[section])
                {
                    if (IdentifierToChange == entry.Key)
                    {
                        priorSettings[section][IdentifierToChange] = ChangedSettingValue;
                        break; // We found what we need, so let's leave.
                    }
                }
            }
            WriteSettings.WriteINI(priorSettings);
        }

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

        public static Dictionary<int, TextBox> stringNumberToColorTextBox = new Dictionary<int, TextBox>() { }; // Can't put variables into it until after we create it.

        private void ChangeStringColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            bool isNormalStrings = radio_DefaultStringColors.Checked; // True = Normal, False = Colorblind
            string stringColorButtonIdentifier = String.Empty;
            int stringNumber = 0;
            FillStringNumberToColorDictionary();

            foreach (KeyValuePair<string, string> stringColorButton in stringColorButtonsToSettingIdentifiers[isNormalStrings])
            {
                if (sender.ToString().Contains(stringColorButton.Key.ToString()))
                {
                    stringColorButtonIdentifier = stringColorButton.Value.ToString();
                    break; // We have the one value we need, so we can leave.
                }
                stringNumber++;
            }

            colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(stringColorButtonIdentifier));

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "2"); // Tell the game to use custom colors
                SaveChanges(stringColorButtonIdentifier, (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6"));
                stringNumberToColorTextBox[stringNumber].BackColor = colorDialog.Color;
            }
        }

        private void FillStringNumberToColorDictionary()
        {
            stringNumberToColorTextBox.Clear();

            stringNumberToColorTextBox.Add(0, textBox_String0Color);
            stringNumberToColorTextBox.Add(1, textBox_String1Color);
            stringNumberToColorTextBox.Add(2, textBox_String2Color);
            stringNumberToColorTextBox.Add(3, textBox_String3Color);
            stringNumberToColorTextBox.Add(4, textBox_String4Color);
            stringNumberToColorTextBox.Add(5, textBox_String5Color);
        }

        private void LoadDefaultStringColors(bool colorBlind = false)
        {
            if (!colorBlind)
            {
                textBox_String0Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String0Color_N_Identifier));
                textBox_String1Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String1Color_N_Identifier));
                textBox_String2Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String2Color_N_Identifier));
                textBox_String3Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String3Color_N_Identifier));
                textBox_String4Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String4Color_N_Identifier));
                textBox_String5Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String5Color_N_Identifier));
            }
            else
            {
                textBox_String0Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String0Color_CB_Identifier));
                textBox_String1Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String1Color_CB_Identifier));
                textBox_String2Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String2Color_CB_Identifier));
                textBox_String3Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String3Color_CB_Identifier));
                textBox_String4Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String4Color_CB_Identifier));
                textBox_String5Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String5Color_CB_Identifier));
            }

        }

        private void LoadCustomThemeColors()
        {
            if (ReadSettings.ProcessSettings(ReadSettings.CustomGUITextColorIdentifier) != String.Empty && ReadSettings.ProcessSettings(ReadSettings.CustomGUIBackgroundColorIdentifier) != String.Empty)
            {
                textBox_ChangeBackgroundColor.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.CustomGUIBackgroundColorIdentifier));
                textBox_ChangeTextColor.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.CustomGUITextColorIdentifier));
            }
            else
            {
                textBox_ChangeBackgroundColor.BackColor = WriteSettings.defaultBackgroundColor;
                textBox_ChangeTextColor.BackColor = WriteSettings.defaultTextColor;
            }
        }

        private void DefaultStringColorsRadio_CheckedChanged(object sender, EventArgs e) => LoadDefaultStringColors();

        private void ColorBlindStringColorsRadio_CheckedChanged(object sender, EventArgs e) => LoadDefaultStringColors(true);

        private static TuningDefinitionList tuningsCollection;

        private TuningDefinitionList LoadTuningsCollection()
        {
            string tuningsFileContent = File.ReadAllText(Constants.TuningJSON_CustomPath);
            var tuningsJson = JObject.Parse(tuningsFileContent);
            var tuningsList = tuningsJson["Static"]["TuningDefinitions"];

            return JsonConvert.DeserializeObject<TuningDefinitionList>(tuningsList.ToString());
        }

        private void SaveTuningsJSON()
        {
            string tuningsFileContent = File.ReadAllText(Constants.TuningJSON_CustomPath);
            var tuningsJson = JObject.Parse(tuningsFileContent);
            tuningsJson["Static"]["TuningDefinitions"] = JObject.FromObject(tuningsCollection);

            try
            {
                File.WriteAllText(Constants.TuningJSON_CustomPath, tuningsJson.ToString());
            }
            catch (IOException ioex)
            {
                MessageBox.Show("Error: " + ioex.ToString(), "Error");
            }
        }

        private Tuple<string, string> SplitTuningUIName(string uiName)
        {
            string index, name = uiName;

            Regex rxIndexExists = new Regex(@"\[.*?\]", RegexOptions.Compiled | RegexOptions.IgnoreCase); // If it already has an index enclosed by []
            Regex rxGetIndex = new Regex(@"\[(\d+)\]", RegexOptions.Compiled | RegexOptions.IgnoreCase); // Extract the digits that lay between []
            Regex rxGrabAfterBracket = new Regex(@"\](.*)", RegexOptions.Compiled | RegexOptions.IgnoreCase); // Extract everything post ]
            if (rxIndexExists.IsMatch(uiName))
            {
                index = rxGetIndex.Matches(uiName)[0].Groups[1].Value;
                name = rxGrabAfterBracket.Matches(uiName)[0].Groups[1].Value;
            }
            else
                index = "0";

            return new Tuple<string, string>(index, name);
        }

        private void AddLocalizationForTuningEntries()
        {
            try
            {
                string currentUIName, csvContents = File.ReadAllText(Constants.LocalizationCSV_CustomPath);
                int newIndex = 37500;

                using (StreamWriter sw = new StreamWriter(Constants.LocalizationCSV_CustomPath, true))
                {
                    foreach (var tuningDefinition in tuningsCollection)
                    {
                        currentUIName = tuningDefinition.Value.UIName;
                        var tuning = SplitTuningUIName(currentUIName);
                        string index = tuning.Item1;
                        string onlyName = tuning.Item2;

                        if (index == "0") // I.e. if it does not contain an index, give it one
                        {
                            while (csvContents.Contains(newIndex.ToString())) // Efficient ? Nope, but does the job
                                newIndex++;

                            tuningDefinition.Value.UIName = String.Format("$[{0}]{1}", newIndex, onlyName); // Append its index in front
                            index = newIndex.ToString();
                        }

                        if (!csvContents.Contains(index)) // If the CSV already contains that index, don't add it to it
                        {
                            sw.Write(index);
                            for (int i = 0; i < 7; i++)
                            {
                                sw.Write(',');
                                sw.Write(tuning.Item2);
                            }

                            sw.Write(sw.NewLine);
                        }
                    }
                }
            }
            catch (IOException ioex)
            {
                MessageBox.Show("Error: " + ioex.Message.ToString(), "Error");
            }

            SaveTuningsJSON();
        }

        private void FillUI()
        {
            listBox_Tunings.Items.Clear();
            tuningsCollection = LoadTuningsCollection();

            listBox_Tunings.Items.Add("<New>");
            foreach (var key in tuningsCollection.Keys)
                listBox_Tunings.Items.Add(key);
        }

        private TuningDefinitionInfo GetCurrentTuningInfo()
        {
            var tuningDefinition = new TuningDefinitionInfo();
            var strings = new Dictionary<string, int>();

            for (int strIdx = 0; strIdx < 6; strIdx++)
                strings[$"string{strIdx}"] = (int)((NumericUpDown)Controls["groupSetAndForget"].Controls[$"nupString{strIdx}"]).Value;

            tuningDefinition.Strings = strings;
            tuningDefinition.UIName = String.Format("$[{0}]{1}", nUpDown_UIIndex.Value.ToString(), textBox_UIName.Text);

            return tuningDefinition;
        }

        private void UnpackCachePsarc()
        {
            if (!Directory.Exists(Constants.WorkFolder))
                Directory.CreateDirectory(Constants.WorkFolder);

            if (!File.Exists(Constants.CacheBackupPath))
                File.Copy(Constants.CachePsarcPath, Constants.CacheBackupPath);

            Packer.Unpack(Constants.CachePsarcPath, Constants.WorkFolder);
        }

        private void RepackCachePsarc()
        {
            try
            {
                if (!Directory.Exists(Constants.CachePcPath))
                {
                    MessageBox.Show("Unpacked cache gone...");
                    return;
                }

                if (!File.Exists(Path.Combine(Constants.CachePcPath, "sltsv1_aggregategraph.nt")))
                    GenUtil.ExtractEmbeddedResource(Constants.CachePcPath, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "sltsv1_aggregategraph.nt" }); //NOTE: when adding resources, change Build Action to Embeded Resource  

                Packer.Pack(Constants.CachePcPath, Constants.CachePsarcPath);
                MessageBox.Show("cache.psarc repackaged successfully", "Success");
            }
            catch (IOException ex)
            {
                MessageBox.Show("Unable to repack cache.psarc" + Environment.NewLine + "Error: " + ex.Message.ToString(), "Repacking error", MessageBoxButtons.OK);
            }
        }

        private void BtnRestoreDefaults_Click(object sender, EventArgs e)
        {
            if (MessageBox.Show(@"Do you wish to restore your cache.psarc to it's original state?", "Restore cache.psarc?", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.No)
                return;

            try
            {
                if (File.Exists(Constants.CacheBackupPath))
                {
                    File.Copy(Constants.CacheBackupPath, Constants.CachePsarcPath, true);
                    MessageBox.Show("Cache backup was restored!", "Backup restored", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }

                else
                    MessageBox.Show("No cache backup found!", "Error");

                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "tuning.database.json" });
                //TODO: extract the rest

                FillUI();
            }
            catch (IOException ioex)
            {
                MessageBox.Show("Problems restoring backup: " + ioex.Message, "Error");
            }
        }

        private void BtnUnpackCacheAgain_Click(object sender, EventArgs e)
        {
            if (!Directory.Exists(Constants.CachePcPath))
                return;

            ZipUtilities.DeleteDirectory(Constants.CachePcPath, true);

            UnpackCachePsarc();
        }

        private void BtnAddCustomTunings_Click(object sender, EventArgs e)
        {
            if (!Directory.Exists(Constants.CachePcPath) || GenUtil.IsDirectoryEmpty(Constants.CachePcPath)) // Don't replace existing unpacked cache, in case the user wants to add more mods together
                UnpackCachePsarc();

            AddLocalizationForTuningEntries();

            ZipUtilities.InjectFile(Constants.TuningJSON_CustomPath, Constants.Cache7_7zPath, Constants.TuningsJSON_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);
            ZipUtilities.InjectFile(Constants.LocalizationCSV_CustomPath, Constants.Cache4_7zPath, Constants.LocalizationCSV_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            RepackCachePsarc();
        }

        private Tuple<string, bool> GetDriveType(char driveLetter) // This may not work on Win7, MSDN says its for >= Win8
        {
            try
            {
                uint driveNumber = 0;

                ManagementScope scope = new ManagementScope(@"\\.\root\microsoft\windows\storage");
                using (ManagementObjectSearcher searcher = new ManagementObjectSearcher("SELECT * FROM MSFT_Partition")) // Grab drive ID for this partition
                {
                    scope.Connect();
                    searcher.Scope = scope;

                    foreach (ManagementObject queryObj in searcher.Get())
                    {
                        char letter = (char)queryObj["DriveLetter"];

                        if (letter == driveLetter)
                        {
                            driveNumber = (uint)queryObj["DiskNumber"];
                            break;
                        }
                    }
                }

                using (ManagementObjectSearcher searcher = new ManagementObjectSearcher("SELECT * FROM MSFT_PhysicalDisk"))
                {
                    string type = "";
                    bool isNVMe = false;
                    scope.Connect();
                    searcher.Scope = scope;

                    foreach (ManagementObject queryObj in searcher.Get())
                    {
                        string devID = queryObj["DeviceId"].ToString();

                        if (devID != driveNumber.ToString()) // For whatever reason, DeviceID seems to be equivalent to driveNumber, but unlike driveNumber, it's a string
                            continue;

                        switch (Convert.ToInt16(queryObj["MediaType"]))
                        {
                            case 1:
                                type = "Unspecified";
                                break;

                            case 3:
                                type = "HDD";
                                break;

                            case 4:
                                type = "SSD";
                                break;

                            case 5:
                                type = "SCM";
                                break;

                            default:
                                type = "Unspecified";
                                break;
                        }

                        if (Convert.ToInt16(queryObj["BusType"]) == 17)
                            isNVMe = true;

                        return new Tuple<string, bool>(type, isNVMe);
                    }
                }
            }
            catch (ManagementException ex) //Not much we can do in this case and it's not really important that we inform the user
            {
            }

            return new Tuple<string, bool>("Unspecified", false);
        }

        private void AddFastLoadMod(bool NVMe)
        {
            if (NVMe)
                File.Copy(Constants.IntroGFX_MaxPath, Constants.IntroGFX_CustomPath, true);
            else
                File.Copy(Constants.IntroGFX_MidPath, Constants.IntroGFX_CustomPath, true);
        }

        private void BtnAddFastLoadMod_Click(object sender, EventArgs e)
        {
            if (!Directory.Exists(Constants.CachePcPath) || GenUtil.IsDirectoryEmpty(Constants.CachePcPath))
                UnpackCachePsarc();

            try
            {
                char driveLetter = Constants.RSFolder.ToUpper()[0];
                var driveType = GetDriveType(driveLetter);

                if (driveType.Item1 == "HDD")
                {
                    if (MessageBox.Show(@"It appears as though Rocksmith installed on a hard disk drive. HDDs are normally too slow to support fast load mod and will likely result in a crash. \n Do you wish to proceed?", "Drive too slow for fast load", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.No)
                        return;

                    AddFastLoadMod(false);
                }
                else if (driveType.Item1 == "SSD")
                {
                    if (driveType.Item2) // If is NVMe
                    {
                        if (MessageBox.Show("Can you confirm Rocksmith is installed on a NVMe drive?\n If you are unsure, press \"No\", because Rocksmith is likely to crash if you pick the fastest option!", "Is RS on a NVMe drive?", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                            AddFastLoadMod(true);
                        else
                            AddFastLoadMod(false);
                    }
                    else
                        AddFastLoadMod(false);
                }
                else
                {
                    if (MessageBox.Show(@"We were unable to detect the drive type on which Rocksmith is installed. \n Is it on a NVMe drive? (if it's not, fastest loading option is likely to crash your game!)", "Fast drive?", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                        AddFastLoadMod(true);
                    else
                        AddFastLoadMod(false);
                }
            }
            catch (IOException ioex)
            {
                MessageBox.Show($"Unable to copy required files. Error: {ioex.Message.ToString()}");
            }

            ZipUtilities.InjectFile(Constants.IntroGFX_CustomPath, Constants.Cache4_7zPath, Constants.IntroGFX_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            RepackCachePsarc();
        }

        private void LoadDefaultFiles()
        {
            if (!File.Exists(Path.Combine(Constants.TuningJSON_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "tuning.database.json" });

            if (!File.Exists(Path.Combine(Constants.IntroGFX_MidPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "introsequence_mid.gfx" });

            if (!File.Exists(Path.Combine(Constants.IntroGFX_MaxPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "introsequence_max.gfx" });

            if (!File.Exists(Path.Combine(Constants.LocalizationCSV_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "maingame.csv" });

            if (!File.Exists(Path.Combine(Constants.ExtendedMenuJson_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "ui_menu_pillar_mission.database.json" });

            if (!File.Exists(Path.Combine(Constants.MainMenuJson_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "ui_menu_pillar_main.database.json" });
        }

        private void LoadSetAndForgetMods()
        {
            Constants.RSFolder = GenUtil.GetRSDirectory(); //TODO: utilize stuff from GenUtil to make it even more fullproof
            LoadDefaultFiles();
            FillUI();
        }

        private void ListTunings_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (listBox_Tunings.SelectedItem == null)
                return;

            string selectedItem = listBox_Tunings.SelectedItem.ToString();

            if (selectedItem == "<New>")
            {
                textBox_InternalTuningName.Text = "";
                nUpDown_UIIndex.Value = 0;
                textBox_UIName.Text = "";
                return;
            }

            var selectedTuning = tuningsCollection[selectedItem];
            var uiName = SplitTuningUIName(selectedTuning.UIName);

            textBox_InternalTuningName.Text = selectedItem;
            nUpDown_UIIndex.Value = Convert.ToInt32(uiName.Item1);
            textBox_UIName.Text = uiName.Item2;

            for (int strIdx = 0; strIdx < 6; strIdx++) // If you are lazy and don't want to list each string separately, just do this sexy two-liner
                ((NumericUpDown)Controls["groupSetAndForget"].Controls[$"nupString{strIdx}"]).Value = selectedTuning.Strings[$"string{strIdx}"];
        }

        private void BtnSaveTuningChanges_Click(object sender, EventArgs e)
        {
            if (listBox_Tunings.SelectedItem != null) // If we are saving a change to the currently selected tuning, perform a change in the collection, otherwise directly go to saving
            {
                string selectedItem = listBox_Tunings.SelectedItem.ToString();

                if (selectedItem != "<New>")
                    tuningsCollection[selectedItem] = GetCurrentTuningInfo();
            }

            SaveTuningsJSON();

            MessageBox.Show("Saved current tuning, don't forget to press \"Add Custom Tunings\" button when you are done!", "Success");
        }

        private void BtnRemoveTuning_Click(object sender, EventArgs e)
        {
            if (listBox_Tunings.SelectedItem == null)
                return;

            string selectedItem = listBox_Tunings.SelectedItem.ToString();

            if (selectedItem == "<New>")
                return;

            tuningsCollection.Remove(selectedItem); // I guess we would be better here using BindingSource on Listbox + ObservableCollection instead of Dict to make changes reflect automatically, but... one day
            listBox_Tunings.Items.Remove(selectedItem);
        }

        private void BtnAddTuning_Click(object sender, EventArgs e)
        {
            if (listBox_Tunings.SelectedItem.ToString() != "<New>")
                return;

            var currTuning = GetCurrentTuningInfo();
            string internalName = textBox_InternalTuningName.Text;

            if (!tuningsCollection.ContainsKey(internalName)) // Unlikely to happen, but still... prevent users accidentaly trying to add existing stuff
            {
                tuningsCollection.Add(internalName, currTuning);
                listBox_Tunings.Items.Add(internalName);
            }
        }

        private void BtnAddCustomMenu_Click(object sender, EventArgs e)
        {
            if (!Directory.Exists(Constants.CachePcPath) || GenUtil.IsDirectoryEmpty(Constants.CachePcPath))
                UnpackCachePsarc();

            ZipUtilities.InjectFile(Constants.ExtendedMenuJson_CustomPath, Constants.Cache7_7zPath, Constants.ExtendedMenuJson_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);
            ZipUtilities.InjectFile(Constants.MainMenuJson_CustomPath, Constants.Cache7_7zPath, Constants.MainMenuJson_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            RepackCachePsarc();
        }

        private void BtnRemoveTempFolders_Click(object sender, EventArgs e)
        {
            ZipUtilities.DeleteDirectory(Constants.WorkFolder);
            ZipUtilities.DeleteDirectory(Constants.CustomModsFolder);
        }

        private static Dictionary<string, Tone2014> TonesFromAllProfiles = new Dictionary<string, Tone2014>();

        private void BtnSetDefaultTones_Click(object sender, EventArgs e)
        {
            if (listBox_ProfileTones.SelectedItem == null)
                return;

            ZipUtilities.ExtractSingleFile(Constants.CustomModsFolder, Constants.Cache7_7zPath, Constants.ToneManager_InternalPath);

            string toneManagerFileContent = File.ReadAllText(Constants.ToneManager_CustomPath);
            var tonesJson = JObject.Parse(toneManagerFileContent);
            var toneList = tonesJson["Static"]["ToneManager"]["Tones"];
            var defaultTones = JsonConvert.DeserializeObject<List<Tone2014>>(toneList.ToString());

            var selectedTone = TonesFromAllProfiles[listBox_ProfileTones.SelectedItem.ToString()];
            if (radio_DefaultRhythmTone.Checked)
                tonesJson["Static"]["ToneManager"]["Tones"][0]["GearList"] = JObject.FromObject(selectedTone.GearList);
            else if (radio_DefaultLeadTone.Checked)
                tonesJson["Static"]["ToneManager"]["Tones"][1]["GearList"] = JObject.FromObject(selectedTone.GearList);
            else if (radio_DefaultBassTone.Checked)
                tonesJson["Static"]["ToneManager"]["Tones"][2]["GearList"] = JObject.FromObject(selectedTone.GearList);

            try
            {
                File.WriteAllText(Constants.ToneManager_CustomPath, tonesJson.ToString());
            }
            catch (IOException ioex)
            {
                MessageBox.Show($"Error:{ioex.Message}");
                return;
            }

            ZipUtilities.InjectFile(Constants.ToneManager_CustomPath, Constants.Cache7_7zPath, Constants.ToneManager_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            RepackCachePsarc();

            MessageBox.Show("Successfully changed default tones!", "Success");
        }

        private void BtnLoadTonesFromProfiles_Click(object sender, EventArgs e)
        {
            string steamUserdataPath = Path.Combine(GenUtil.GetSteamDirectory(), "userdata");
            try
            {
                var subdirs = new DirectoryInfo(steamUserdataPath).GetDirectories(@"221680", SearchOption.AllDirectories).ToArray();
                var userprofileFolder = subdirs.FirstOrDefault(dir => !dir.FullName.Contains("760")); //760 is the ID for Steam's screenshot thingy

                if (Directory.Exists(userprofileFolder.FullName))
                {
                    var profiles = Directory.EnumerateFiles(userprofileFolder.FullName, "*_PRFLDB", SearchOption.AllDirectories).ToList();

                    TonesFromAllProfiles.Clear();
                    listBox_ProfileTones.Items.Clear();

                    foreach (string profile in profiles)
                        foreach (var tone in Tone2014.Import(profile))
                        {
                            listBox_ProfileTones.Items.Add(tone.Name);
                            TonesFromAllProfiles.Add(tone.Name, tone);
                        }
                }
                else
                    MessageBox.Show("Could not find profile folder!", "Error");
            }
            catch (IOException ioex)
            {
                MessageBox.Show($"Could not find Steam profiles folder: {ioex.Message}", "Error");
            }
        }

        private void BtnImportExistingSettings_Click(object sender, EventArgs e)
        {
            if (!File.Exists(Constants.Cache4_7zPath) || !File.Exists(Constants.Cache7_7zPath))
                UnpackCachePsarc();

            ZipUtilities.ExtractSingleFile(Constants.CustomModsFolder, Constants.Cache7_7zPath, Constants.TuningsJSON_InternalPath);
            ZipUtilities.ExtractSingleFile(Constants.CustomModsFolder, Constants.Cache4_7zPath, Constants.LocalizationCSV_InternalPath);

            FillUI();
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
            {3, ReadSettings.ShowSongTimerIdentifier},
            {4, ReadSettings.ForceReEnumerationIdentifier},
            {5, ReadSettings.RainbowStringsIdentifier},
            {6, ReadSettings.RemoveLyricsKeyIdentifier}
        };

        private void Save_Songlists_Keybindings(object sender, EventArgs e) // Save Songlists and Keybindings when pressing Enter
        {
            string currentTab = TabController.SelectedTab.Text.ToString();

            if (currentTab == "Song Lists")
            {
                foreach (KeyValuePair<int, string> currentSongList in SongListIndexToINISetting)
                {
                    if (textBox_NewSonglistName.Text.Trim() == "") // The game UI will break with a blank name.
                    {
                        MessageBox.Show("You cannot save a blank song list name as the game will break", "Warning!", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        break;
                    }

                    else if (currentSongList.Key == listBox_Songlist.SelectedIndex)
                    {
                        SaveChanges(currentSongList.Value, textBox_NewSonglistName.Text);
                        listBox_Songlist.Items[currentSongList.Key] = textBox_NewSonglistName.Text;
                        break;
                    };
                }
            }

            if (currentTab == "Keybindings")
            {
                foreach (KeyValuePair<int, string> currentKeybinding in KeybindingsIndexToINISetting)
                {
                    if (textBox_NewKeyAssignment.Text == "")
                    {
                        MessageBox.Show("You cannot set a blank keybind", "Warning!", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        break;
                    }

                    else if (currentKeybinding.Key == listBox_Modlist.SelectedIndex)
                    {
                        SaveChanges(currentKeybinding.Value, KeyConversion.VirtualKey(textBox_NewKeyAssignment.Text));
                        break;
                    }
                }

                textBox_NewKeyAssignment.Text = String.Empty;
            }
            ShowCurrentKeybindingValues();
        }

        private void ToggleLoftCheckbox_CheckedChanged(object sender, EventArgs e) // Toggle Loft Enabled/ Disabled
        {
            if (checkBox_ToggleLoft.Checked)
            {
                SaveChanges(ReadSettings.ToggleLoftEnabledIdentifier, "true");
                checkBox_ToggleLoft.Checked = true;
                radio_LoftAlwaysOff.Visible = true;
                radio_LoftOffHotkey.Visible = true;
                radio_LoftOffInSong.Visible = true;
                groupBox_LoftOffWhen.Visible = true;
            }
            else
            {
                SaveChanges(ReadSettings.ToggleLoftEnabledIdentifier, "false");
                checkBox_ToggleLoft.Checked = false;
                radio_LoftAlwaysOff.Visible = false;
                radio_LoftOffHotkey.Visible = false;
                radio_LoftOffInSong.Visible = false;
                groupBox_LoftOffWhen.Visible = false;
            }
        }

        private void AddVolumeCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (AddVolumeCheckbox.Checked)
                SaveChanges(ReadSettings.AddVolumeEnabledIdentifier, "true");
            else
                SaveChanges(ReadSettings.AddVolumeEnabledIdentifier, "false");
        }

        private void DecreaseVolumeCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (DecreaseVolumeCheckbox.Checked)
                SaveChanges(ReadSettings.DecreaseVolumeEnabledIdentifier, "true");
            else
                SaveChanges(ReadSettings.DecreaseVolumeEnabledIdentifier, "false");
        }

        private void SongTimerCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_SongTimer.Checked)
                SaveChanges(ReadSettings.ShowSongTimerEnabledIdentifier, "true");
            else
                SaveChanges(ReadSettings.ShowSongTimerEnabledIdentifier, "false");
        }

        private void ForceEnumerationCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_ForceEnumeration.Checked)
            {
                SaveChanges(ReadSettings.ForceReEnumerationEnabledIdentifier, "manual");
                checkBox_ForceEnumeration.Checked = true;
                radio_ForceEnumerationAutomatic.Visible = true;
                radio_ForceEnumerationManual.Visible = true;
                groupBox_HowToEnumerate.Visible = true;
            }
            else
            {
                SaveChanges(ReadSettings.ForceReEnumerationEnabledIdentifier, "false");
                checkBox_ForceEnumeration.Checked = false;
                radio_ForceEnumerationAutomatic.Visible = false;
                radio_ForceEnumerationManual.Visible = false;
                groupBox_HowToEnumerate.Visible = false;
            }
        }

        private void EnumerateEveryXMS_ValueChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.CheckForNewSongIntervalIdentifier, (nUpDown_ForceEnumerationXMS.Value * 1000).ToString());

        private void ForceEnumerationAutomaticRadio_CheckedChanged(object sender, EventArgs e)
        {
            label_ForceEnumerationXMS.Visible = true;
            nUpDown_ForceEnumerationXMS.Visible = true;
            SaveChanges(ReadSettings.ForceReEnumerationEnabledIdentifier, "automatic");
        }

        private void ForceEnumerationManualRadio_CheckedChanged(object sender, EventArgs e)
        {
            label_ForceEnumerationXMS.Visible = false;
            nUpDown_ForceEnumerationXMS.Visible = false;
            SaveChanges(ReadSettings.ForceReEnumerationEnabledIdentifier, "manual");
        }

        private void RainbowStringsEnabled_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_RainbowStrings.Checked)
                SaveChanges(ReadSettings.RainbowStringsEnabledIdentifier, "true");
            else
                SaveChanges(ReadSettings.RainbowStringsEnabledIdentifier, "false");
        }

        private void ExtendedRangeEnabled_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_ExtendedRange.Checked)
            {
                groupBox_ExtendedRangeWhen.Visible = true;
                listBox_ExtendedRangeTunings.Visible = true;
                SaveChanges(ReadSettings.ExtendedRangeEnabledIdentifier, "true");
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "2");
            }
            else
            {
                groupBox_ExtendedRangeWhen.Visible = false;
                listBox_ExtendedRangeTunings.Visible = false;
                SaveChanges(ReadSettings.ExtendedRangeEnabledIdentifier, "false");
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "0");
            }
        }

        private void CustomColorsCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_CustomColors.Checked)
            {
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "2");
                checkBox_CustomColors.Checked = true;
                groupBox_StringColors.Visible = true;
            }
            else
            {
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "0");
                checkBox_CustomColors.Checked = false;
                groupBox_StringColors.Visible = false;
            }
        }
        /*
        private void DiscoModeCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (DiscoModeCheckbox.Checked)
                SaveChanges(ReadSettings.DiscoModeIdentifier, "true");
            else
                SaveChanges(ReadSettings.DiscoModeIdentifier, "false");
        }
        */

        private void HeadstockCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_RemoveHeadstock.Checked)
            {
                SaveChanges(ReadSettings.RemoveHeadstockIdentifier, "true");
                groupBox_ToggleHeadstockOffWhen.Visible = true;
            }

            else
            {
                SaveChanges(ReadSettings.RemoveHeadstockIdentifier, "false");
                groupBox_ToggleHeadstockOffWhen.Visible = false;
            }
        }

        private void RemoveSkylineCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_RemoveSkyline.Checked)
            {
                SaveChanges(ReadSettings.RemoveSkylineIdentifier, "true");
                groupBox_ToggleSkylineWhen.Visible = true;
            }
            else
            {
                SaveChanges(ReadSettings.RemoveSkylineIdentifier, "false");
                groupBox_ToggleSkylineWhen.Visible = false;
            }
        }

        private void GreenScreenWallCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_GreenScreen.Checked)
                SaveChanges(ReadSettings.GreenScreenWallIdentifier, "true");
            else
                SaveChanges(ReadSettings.GreenScreenWallIdentifier, "false");
        }

        private void AutoLoadProfileCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_AutoLoadProfile.Checked)
                SaveChanges(ReadSettings.ForceProfileEnabledIdentifier, "true");
            else
                SaveChanges(ReadSettings.ForceProfileEnabledIdentifier, "false");
        }

        private void FretlessModeCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_Fretless.Checked)
                SaveChanges(ReadSettings.FretlessModeEnabledIdentifier, "true");
            else
                SaveChanges(ReadSettings.FretlessModeEnabledIdentifier, "false");
        }

        private void RemoveInlaysCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_RemoveInlays.Checked)
                SaveChanges(ReadSettings.RemoveInlaysIdentifier, "true");
            else
                SaveChanges(ReadSettings.RemoveInlaysIdentifier, "false");
        }

        private void ToggleLoftWhenManualRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (radio_LoftOffHotkey.Checked)
                SaveChanges(ReadSettings.ToggleLoftWhenIdentifier, "manual");
        }

        private void ToggleLoftWhenSongRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (radio_LoftOffInSong.Checked)
                SaveChanges(ReadSettings.ToggleLoftWhenIdentifier, "song");
        }

        private void ToggleLoftWhenStartupRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (radio_LoftAlwaysOff.Checked)
                SaveChanges(ReadSettings.ToggleLoftWhenIdentifier, "startup");
        }

        private void RemoveLineMarkersCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_RemoveLineMarkers.Checked)
                SaveChanges(ReadSettings.RemoveLaneMarkersIdentifier, "true");
            else
                SaveChanges(ReadSettings.RemoveLaneMarkersIdentifier, "false");
        }

        private void ToggleSkylineSongRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (radio_SkylineOffInSong.Checked)
                SaveChanges(ReadSettings.ToggleSkylineWhenIdentifier, "song");
        }

        private void ToggleSkylineStartupRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (radio_SkylineAlwaysOff.Checked)
                SaveChanges(ReadSettings.ToggleSkylineWhenIdentifier, "startup");
        }

        private void ExtendedRangeTunings_SelectedIndexChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.ExtendedRangeTuningIdentifier, Convert.ToString((listBox_ExtendedRangeTunings.SelectedIndex * -1) - 2));

        private void DeleteKeyBind_Click(object sender, EventArgs e)
        {
            textBox_NewKeyAssignment.Text = "";

            foreach (KeyValuePair<int, string> currentMod in KeybindingsIndexToINISetting)
            {
                if (currentMod.Key == listBox_Modlist.SelectedIndex)
                {
                    SaveChanges(currentMod.Value, "");
                    break;
                }
            }
            ShowCurrentKeybindingValues();
        }

        private void ShowCurrentKeybindingValues()
        {
            label_ToggleLoftKey.Text = "Toggle Loft: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ToggleLoftIdentifier));
            label_AddVolumeKey.Text = "Add Volume: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.AddVolumeIdentifier));
            label_DecreaseVolumeKey.Text = "Decrease Volume: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.DecreaseVolumeIdentifier));
            label_SongTimerKey.Text = "Show Song Timer: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerIdentifier));
            label_ReEnumerationKey.Text = "Force ReEnumeration: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationIdentifier));
            label_RainbowStringsKey.Text = "Rainbow Strings: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RainbowStringsIdentifier));
            label_RemoveLyricsKey.Text = "Remove Lyrics: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsKeyIdentifier));
        }

        private void RemoveLyricsCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_RemoveLyrics.Checked)
            {
                SaveChanges(ReadSettings.RemoveLyricsIdentifier, "on");
                radio_LyricsAlwaysOff.Visible = true;
                radio_LyricsOffHotkey.Visible = true;
                groupBox_ToggleLyricsOffWhen.Visible = true;
            }

            else
            {
                SaveChanges(ReadSettings.RemoveLyricsIdentifier, "off");
                radio_LyricsAlwaysOff.Visible = false;
                radio_LyricsOffHotkey.Visible = false;
                groupBox_ToggleLyricsOffWhen.Visible = false;
            }
        }

        public static Dictionary<Control, string> TooltipDictionary = new Dictionary<Control, string>() { }; // I really wish I could make this full of stuff, but `this.` and `MainForm.` don't work, so I need to call a different function `FillToolTipDictionary()` do it for me. :(

        private void FillToolTipDictionary()
        {
            HasToolTipDictionaryBeenCreatedYet = true;

            // INI Edits
            // Checkboxes
            TooltipDictionary.Add(checkBox_ToggleLoft, "Disables the game background, amps and noise reactive speaker rings.\nBest used with Venue Mode off (setting in game).\nUsed by a lot of Rocksmith Streamers to make it easy to Luma Key out the game background.\nPlayer just sees an all black background when this is enabled.\nOptions for turning the loft off only when in a song, when the game first starts up, or on a key press.");
            //TooltipDictionary.Add(AddVolumeCheckbox, "Experimental"); // Changed to "VolumeControlsCheckbox"
            //TooltipDictionary.Add(DecreaseVolumeCheckbox, "Experimental"); // Changed to "VolumeControlsCheckbox"
            TooltipDictionary.Add(checkBox_SongTimer, "Experimental.\nIntent is to show a box with your timestamp position through the song.");
            TooltipDictionary.Add(checkBox_ExtendedRange, "Alters the string and note colors to make it easier to play a 5 string bass or 7 string guitar.");
            TooltipDictionary.Add(checkBox_ForceEnumeration, "Game will automatically start an Enumeration sequence when a new psarc (CDLC) file is detected as having been added to the dlc folder.\nNot necesary to enable if you're already using Rocksniffer to do the same thing.");
            TooltipDictionary.Add(checkBox_RemoveHeadstock, "Stops the Headstock of the guitar being drawn.\n“Headless” guitar mode. Just cleans up some more of the UI.");
            TooltipDictionary.Add(checkBox_RemoveSkyline, "Removes the purple and orange bars from the top of the display in LAS.\nUse in conjunction with No Loft for a cleaner UI.\nOptions for always off, only off when in a song, or only when toggled by key press.");
            TooltipDictionary.Add(checkBox_GreenScreen, "Changes just a section of the game background to all black, amusing for a selective “green screen” stream experience.\nInvalidated by \"No Loft\".");
            TooltipDictionary.Add(checkBox_AutoLoadProfile, "Essentially holds down the ENTER key until the game has reached the main menu.\nLets you auto load the last used profile without needing to interact with the game at all.");
            TooltipDictionary.Add(checkBox_Fretless, "Removes the Fret Wire from the neck, making your instrument appear to be fretless.");
            TooltipDictionary.Add(checkBox_RemoveInlays, "Disables the guitar neck inlay display entirely.\nNote: This only works with the standard dot inlays.");
            TooltipDictionary.Add(checkBox_ControlVolume, "Allows you to control how loud the song is using the in-game mixer without needing to open it.");
            TooltipDictionary.Add(checkBox_GuitarSpeak, "Use your guitar to control the menus!");
            TooltipDictionary.Add(checkBox_RemoveLyrics, "Disables the display of song lyrics while in Learn-A-Song mode.");
            TooltipDictionary.Add(checkBox_RainbowStrings, "Experimental.\nHow Pro are you? This makes the players guitar strings constantly cycling through colors.");
            TooltipDictionary.Add(checkBox_CustomColors, "Lets you define the string / note colors you want.\nSaves a normal set and a Colorblind mode set.");
            TooltipDictionary.Add(checkBox_RemoveLineMarkers, "Removes the additional lane marker lines seen in the display.\nWhen used with No Loft, provides a cleaner Luma Key.");
            TooltipDictionary.Add(checkBox_ChangeTheme, "Use this feature to customize the colors used in this GUI.");

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

            // Misc
            TooltipDictionary.Add(groupBox_Songlist, "Custom names for the 6 \"SONG LISTS\" shown in game.");
            TooltipDictionary.Add(groupBox_Keybindings, "Set key binds for the toggle on / off by keypress modifications.\nYou need to press ENTER after setting teh key for it to be saved.");
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
            TooltipDictionary.Add(button_AddDCExitGame, "Adds the Direct Connect mode - microphone mode with tone simulations.\nAlso replaces UPLAY on the main menu with an EXIT GAME option.");
            TooltipDictionary.Add(button_AddCustomTunings, "Adds some preset definitions for the most common Custom Tunings.");
            TooltipDictionary.Add(button_AddFastLoad, "SSD drive or faster or may cause the game to not launch properly, skips some of the intro sequences.\nCombined with Auto Load Last Profile and huzzah!");

            // Misc
            TooltipDictionary.Add(button_RemoveTemp, "Removes the temporary files used by RSMods.");
            TooltipDictionary.Add(button_RestoreCacheBackup, "Restores the original cache.psarc file\nUndoes all \"Set-and-forget\" mods.");
            TooltipDictionary.Add(button_CleanUpUnpackedCache, "Removes temporary files and un-packs cache.psarc as it is being used now, again.");
        }

        private void HideToolTips(object sender, EventArgs e)
        {
            if (MainForm.ActiveForm != null) // This fixes a glitch where if you are hovering over a Control that calls the tooltip, and alt-tab, the program will crash since ActiveFrame turns to null... If the user is highlighting something, and the window becomes null, we need to refrain from trying to hide the tooltip that "does not exist".
            {
                ToolTip.Hide(MainForm.ActiveForm);
                ToolTip.Active = false;
            };
        }

        public ToolTip currentTooltip = new ToolTip(); // Fixes toolTip duplication glitch.

        private void RunToolTips(object sender, EventArgs e)
        {
            if (CreatedToolTipYet) // Do we already have a filled tooltip? If so, clear it.
            {
                currentTooltip.Dispose();
                currentTooltip = new ToolTip();
            }

            currentTooltip.Active = true;

            if (!HasToolTipDictionaryBeenCreatedYet) // Have we filled the tooltip dictionary? If so, skip this, if not, fill the dictionary.
                FillToolTipDictionary();

            foreach (Control ControlHoveredOver in TooltipDictionary.Keys)
            {
                if (ControlHoveredOver == sender)
                {
                    string toolTipString; // Do not inline this as it will break the statement || Needs to be null as it will be written over
                    TooltipDictionary.TryGetValue(ControlHoveredOver, out toolTipString);
                    currentTooltip.Show(toolTipString, ControlHoveredOver, 5000000); // Don't change the duration number, even if it's higher. It works as it is, and changing it to even Int32.MaxValue causes it to go back to the 5-second max.
                    CreatedToolTipYet = true;
                    break; // We found what we needed, now GTFO of here.
                }
            }
        }

        private void Songlist_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (listBox_Songlist.SelectedIndex >= 0)
                textBox_NewSonglistName.Text = listBox_Songlist.SelectedItem.ToString();
        }

        private void ToggleLyricsRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (radio_LyricsAlwaysOff.Checked)
                SaveChanges(ReadSettings.RemoveLyricsWhenIdentifier, "startup");
        }

        private void ToggleLyricsManualRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (radio_LyricsOffHotkey.Checked)
                SaveChanges(ReadSettings.RemoveLyricsWhenIdentifier, "manual");
        }

        private void GuitarSpeakSaveButton_Click(object sender, EventArgs e)
        {
            if (listBox_GuitarSpeakNote.SelectedIndex >= 0 && listBox_GuitarSpeakOctave.SelectedIndex >= 0 && listBox_GuitarSpeakKeypress.SelectedIndex >= 0)
            {
                int inputNote = listBox_GuitarSpeakNote.SelectedIndex + 36; // We skip the first 3 octaves to give an accurate representation of the notes being played
                int inputOctave = listBox_GuitarSpeakOctave.SelectedIndex - 3; // -1 for the offset, and -2 for octave offset in DLL.
                int outputNoteOctave = inputNote + (inputOctave * 12);
                MessageBox.Show(listBox_GuitarSpeakNote.SelectedItem.ToString() + listBox_GuitarSpeakOctave.SelectedItem.ToString() + " was saved to " + listBox_GuitarSpeakKeypress.SelectedItem.ToString(), "Note Saved!", MessageBoxButtons.OK, MessageBoxIcon.Information);

                foreach (KeyValuePair<string, string> entry in GuitarSpeakKeyPressDictionary)
                {
                    if (listBox_GuitarSpeakKeypress.SelectedItem.ToString() == entry.Key)
                    {
                        SaveChanges(entry.Value, outputNoteOctave.ToString());
                        listBox_GuitarSpeakPresets.ClearSelected();

                        foreach (string guitarSpeakItem in listBox_GuitarSpeakPresets.Items)
                        {
                            if (guitarSpeakItem.Contains(listBox_GuitarSpeakKeypress.SelectedItem.ToString()))
                            {
                                listBox_GuitarSpeakPresets.Items.Remove(guitarSpeakItem);
                                break;
                            }
                        }
                        listBox_GuitarSpeakPresets.Items.Add(listBox_GuitarSpeakKeypress.SelectedItem.ToString() + ": " + GuitarSpeakNoteOctaveMath(outputNoteOctave.ToString()));
                        RefreshGuitarSpeakPresets();
                    }
                }

                listBox_GuitarSpeakNote.ClearSelected();
                listBox_GuitarSpeakOctave.ClearSelected();
                listBox_GuitarSpeakKeypress.ClearSelected();
            }
            else
                MessageBox.Show("One of the Guitar Speak boxes not selected", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        private Dictionary<string, string> GuitarSpeakKeyPressDictionary = new Dictionary<string, string>()
        {
            {"Delete", ReadSettings.GuitarSpeakDeleteIdentifier },
            {"Space", ReadSettings.GuitarSpeakSpaceIdentifier },
            {"Enter", ReadSettings.GuitarSpeakEnterIdentifier },
            {"Tab", ReadSettings.GuitarSpeakTabIdentifier },
            {"Page Up", ReadSettings.GuitarSpeakPGUPIdentifier },
            {"Page Down", ReadSettings.GuitarSpeakPGDNIdentifier },
            {"Up Arrow", ReadSettings.GuitarSpeakUPIdentifier },
            {"Down Arrow", ReadSettings.GuitarSpeakDNIdentifier },
            {"Escape", ReadSettings.GuitarSpeakESCIdentifier },
            {"Open Bracket", ReadSettings.GuitarSpeakOBracketIdentifier},
            {"Close Bracket", ReadSettings.GuitarSpeakCBracketIdentifier},
            {"Tilde / Tilda", ReadSettings.GuitarSpeakTildeaIdentifier},
            {"Forward Slash", ReadSettings.GuitarSpeakForSlashIdentifier},
            {"Close Guitar Speak", ReadSettings.GuitarSpeakCloseIdentifier }
        };

        private void GuitarSpeakEnableCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_GuitarSpeak.Checked)
            {
                checkBox_GuitarSpeak.Checked = true;
                groupBox_GuitarSpeak.Visible = true;
                checkbox_GuitarSpeakWhileTuning.Visible = true;
                SaveChanges(ReadSettings.GuitarSpeakIdentifier, "on");
            }
            else
            {
                checkBox_GuitarSpeak.Checked = false;
                groupBox_GuitarSpeak.Visible = false;
                checkbox_GuitarSpeakWhileTuning.Visible = false;
                SaveChanges(ReadSettings.GuitarSpeakIdentifier, "off");
            }
        }

        private void TuningOffsets(object sender, EventArgs e)
        {
            int stringNumber = Int32.Parse(((NumericUpDown)sender).Name.Substring(9, 1)); // Returns the current sender's name.
            int offset = 0;
            switch (stringNumber)
            {
                case 0:
                    offset = 28; // Offset from (24 - 1) + 5
                    label_CustomTuningLowEStringLetter.Text = IntToNote(Convert.ToInt32(nUpDown_String0.Value) + offset);
                    break;
                case 1:
                    offset = 33; // Offset from (24 - 1) + 10
                    label_CustomTuningAStringLetter.Text = IntToNote(Convert.ToInt32(nUpDown_String1.Value) + offset);
                    break;
                case 2:
                    offset = 26; // Offset from (24 - 1) + 3
                    label_CustomTuningDStringLetter.Text = IntToNote(Convert.ToInt32(nUpDown_String2.Value) + offset);
                    break;
                case 3:
                    offset = 31;// Offset from (24 - 1) + 8
                    label_CustomTuningGStringLetter.Text = IntToNote(Convert.ToInt32(nUpDown_String3.Value) + offset);
                    break;
                case 4:
                    offset = 35; // Offset from (24 - 1) + 12
                    label_CustomTuningBStringLetter.Text = IntToNote(Convert.ToInt32(nUpDown_String4.Value) + offset);
                    break;
                case 5:
                    offset = 28; // Offset from (24 - 1) + 5
                    label_CustomTuningHighEStringLetter.Text = IntToNote(Convert.ToInt32(nUpDown_String5.Value) + offset);
                    break;
                default: // Yeah we don't know wtf happened here
                    MessageBox.Show("Invalid String Number! Please report this to the GUI devs!");
                    break;
            }
        }

        private void VolumeControlsCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_ControlVolume.Checked)
            {
                SaveChanges(ReadSettings.AddVolumeEnabledIdentifier, "on");
                SaveChanges(ReadSettings.DecreaseVolumeEnabledIdentifier, "on");
            }
            else
            {
                SaveChanges(ReadSettings.AddVolumeEnabledIdentifier, "off");
                SaveChanges(ReadSettings.DecreaseVolumeEnabledIdentifier, "off");
            }
        }

        private void GuitarSpeakWhileTuningBox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkbox_GuitarSpeakWhileTuning.Checked)
                SaveChanges(ReadSettings.GuitarSpeakTuningIdentifier, "on");
            else
                SaveChanges(ReadSettings.GuitarSpeakTuningIdentifier, "off");
        }

        private string GuitarSpeakNoteOctaveMath(string inputString)
        {
            if (inputString == "")
                return "";

            int inputInt = Int32.Parse(inputString);

            int octave = (inputInt / 12) - 1; // We support the -1st octave, so we need to minus 1 from our octave.

            return IntToNote(inputInt) + octave.ToString();
        }

        private string IntToNote(int intToConvert) => noteArray[intToConvert % 12];

        private string[] noteArray = new string[12] { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };

        private void GuitarSpeakHelpButton_Click(object sender, EventArgs e) => System.Diagnostics.Process.Start("https://pastebin.com/raw/PZ0FQTn0");
        private void HeadStockAlwaysOffButton_CheckedChanged(object sender, EventArgs e)
        {
            if (radio_HeadstockAlwaysOff.Checked)
                SaveChanges(ReadSettings.RemoveHeadstockWhenIdentifier, "startup");
        }

        private void HeadstockOffInSongOnlyButton_CheckedChanged(object sender, EventArgs e)
        {
            if (radio_HeadstockOffInSong.Checked)
                SaveChanges(ReadSettings.RemoveHeadstockWhenIdentifier, "song");
        }

        private void RefreshGuitarSpeakPresets()
        {
            listBox_GuitarSpeakPresets.Items.Clear();

            listBox_GuitarSpeakPresets.Items.AddRange(new object[]
            {
                "Delete: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakDeleteIdentifier)),
                "Space: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakSpaceIdentifier)),
                "Enter: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakEnterIdentifier)),
                "Tab: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakTabIdentifier)),
                "Page Up: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakPGUPIdentifier)),
                "Page Down: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakPGDNIdentifier)),
                "Up Arrow: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakUPIdentifier)),
                "Down Arrow: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakDNIdentifier)),
                "Escape: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakESCIdentifier)),
                "Open Bracket: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakOBracketIdentifier)),
                "Close Bracket: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakCBracketIdentifier)),
                "Tilde / Tilda: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakTildeaIdentifier)),
                "Forward Slash: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakForSlashIdentifier)),
                "Close Guitar Speak: " + GuitarSpeakNoteOctaveMath(ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakCloseIdentifier))
            });
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            if (ReadSettings.ProcessSettings(ReadSettings.CustomGUIThemeIdentifier) == "on" && ReadSettings.ProcessSettings(ReadSettings.CustomGUIBackgroundColorIdentifier) != String.Empty && ReadSettings.ProcessSettings(ReadSettings.CustomGUITextColorIdentifier) != String.Empty)
                ChangeTheme(ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.CustomGUIBackgroundColorIdentifier)), ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.CustomGUITextColorIdentifier)));
            else
                ChangeTheme(WriteSettings.defaultBackgroundColor, WriteSettings.defaultTextColor);
        }

        private void DarkModeCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_ChangeTheme.Checked) // We turn ChangeTheme on.
            {
                SaveChanges(ReadSettings.CustomGUIThemeIdentifier, "on");
                groupBox_ChangeTheme.Visible = true;
                
            }
            else // We turn ChangeTheme off.
            {
                SaveChanges(ReadSettings.CustomGUIThemeIdentifier, "off");
                groupBox_ChangeTheme.Visible = false;
                ChangeTheme(WriteSettings.defaultBackgroundColor, WriteSettings.defaultTextColor);
            }
        }

        private void button_TwitchReAuthorize_Click(object sender, EventArgs e)
        {
            ImplicitAuth auth = new ImplicitAuth();
            auth.MakeAuthRequest();
        }

        private void TabController_SelectedIndexChanged(object sender, EventArgs e)
        {
            if(TabController.SelectedTab.Name == "tab_Twitch") // I would prefer not to call
            {
                if (TwitchSettings.Authorized)
                    label_AuthorizedAs.Text = $"{TwitchSettings.Username} with channel ID: {TwitchSettings.ChannelID} and access token: {TwitchSettings.AccessToken}";
            }
        }

        private void button_ChangeBackgroundColor_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            colorDialog.Color = WriteSettings.defaultBackgroundColor;

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                SaveChanges(ReadSettings.CustomGUIBackgroundColorIdentifier, (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6"));
                textBox_ChangeBackgroundColor.BackColor = colorDialog.Color;
            }
        }

        private void button_ChangeTextColor_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            colorDialog.Color = WriteSettings.defaultTextColor;

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                SaveChanges(ReadSettings.CustomGUITextColorIdentifier, (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6"));
                textBox_ChangeTextColor.BackColor = colorDialog.Color;
            }
        }

        private void button_SaveThemeColors_Click(object sender, EventArgs e) => ChangeTheme(textBox_ChangeBackgroundColor.BackColor, textBox_ChangeTextColor.BackColor);
    }
}