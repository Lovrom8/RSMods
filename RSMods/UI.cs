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


#pragma warning disable IDE0017 // ... Warning about how code can be simplified... Yeah I know it isn't perfect.
#pragma warning disable IDE0044 // "This should be readonly" .... No. No it shouldn't.
#pragma warning disable IDE0059 // "You made this variable and didn't use it". It's called future proofing.
#pragma warning disable IDE0071 // "Interpolation can be simplified"
#pragma warning disable CS0168 // Variable Declared But Not Used
#pragma warning disable IDE0018 // Variable Declaration can be inlined, but it can't or it will shoot an error.

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

            InitializeComponent();
            this.Text = $"{this.Text}-{Assembly.GetExecutingAssembly().GetName().Version.ToString()}";

            // Fill Songlist List
            this.Songlist.Items.AddRange(new object[] {
                    ReadSettings.ProcessSettings(ReadSettings.Songlist1Identifier), // Song List 1
                    ReadSettings.ProcessSettings(ReadSettings.Songlist2Identifier), // Song List 2
                    ReadSettings.ProcessSettings(ReadSettings.Songlist3Identifier), // Song List 3
                    ReadSettings.ProcessSettings(ReadSettings.Songlist4Identifier), // Song List 4
                    ReadSettings.ProcessSettings(ReadSettings.Songlist5Identifier), // Song List 5
                    ReadSettings.ProcessSettings(ReadSettings.Songlist6Identifier)  // Song List 6 
                });

            // Fill Modlist List
            this.ModList.Items.AddRange(new object[] {
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
                    this.ToggleLoftCheckbox.Checked = true;
                    this.ToggleLoftWhenStartupRadio.Visible = true;
                    this.ToggleLoftWhenManualRadio.Visible = true;
                    this.ToggleLoftWhenSongRadio.Visible = true;
                    this.ToggleLoftOffWhenBox.Visible = true;

                    if (ReadSettings.ProcessSettings(ReadSettings.ToggleLoftWhenIdentifier) == "startup")
                        this.ToggleLoftWhenStartupRadio.Checked = true;
                    else if (ReadSettings.ProcessSettings(ReadSettings.ToggleLoftWhenIdentifier) == "manual")
                        this.ToggleLoftWhenManualRadio.Checked = true;
                    else if (ReadSettings.ProcessSettings(ReadSettings.ToggleLoftWhenIdentifier) == "song")
                        this.ToggleLoftWhenSongRadio.Checked = true;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.AddVolumeEnabledIdentifier) == "on") // Add Volume Enabled / Disabled
                    this.VolumeControlsCheckbox.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.DecreaseVolumeEnabledIdentifier) == "on") // Decrease Volume Enabled / Disabled
                    this.VolumeControlsCheckbox.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerEnabledIdentifier) == "on") // Show Song Timer Enabled / Disabled
                    this.SongTimerCheckbox.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationEnabledIdentifier) != "off") // Force Enumeration Settings
                {
                    this.ForceEnumerationCheckbox.Checked = true;
                    this.ForceEnumerationAutomaticRadio.Visible = true;
                    this.ForceEnumerationManualRadio.Visible = true;
                    this.HowToEnumerateBox.Visible = true;
                    if (ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationEnabledIdentifier) == "automatic")
                        this.ForceEnumerationAutomaticRadio.Checked = true;
                    else
                        this.ForceEnumerationManualRadio.Checked = true;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.RainbowStringsEnabledIdentifier) == "on") // Rainbow String Enabled / Disabled
                    this.RainbowStringsEnabled.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.ExtendedRangeEnabledIdentifier) == "on") // Extended Range Enabled / Disabled
                {
                    this.ExtendedRangeEnabled.Checked = true;
                    this.ExtendedRangeTuningBox.Visible = true;
                    this.ExtendedRangeTunings.Visible = true;
                }
                if (ReadSettings.ProcessSettings(ReadSettings.CustomStringColorNumberIndetifier) != "0") // Custom String Colors
                {
                    this.CustomColorsCheckbox.Checked = true;
                    this.ChangeStringColorsBox.Visible = true;
                }

                /* Disco Mode: Deprecated, as of now, because you can't toggle it off easily.

                 if (ReadSettings.ProcessSettings(ReadSettings.DiscoModeIdentifier) == "on") // Disco Mode Enabled / Disabled
                    this.DiscoModeCheckbox.Checked = true;
                 */

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveHeadstockIdentifier) == "on") // Remove Headstock Enabled / Disabled
                {
                    this.HeadstockCheckbox.Checked = true;
                    this.ToggleHeadstockWhenBox.Visible = true;

                    if (ReadSettings.ProcessSettings(ReadSettings.RemoveHeadstockWhenIdentifier) == "startup")
                        this.HeadStockAlwaysOffButton.Checked = true;
                    else if (ReadSettings.ProcessSettings(ReadSettings.RemoveHeadstockWhenIdentifier) == "song")
                        this.HeadstockOffInSongOnlyButton.Checked = true;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveSkylineIdentifier) == "on") // Remove Skyline Enabled / Disabled
                {
                    this.RemoveSkylineCheckbox.Checked = true;
                    this.ToggleSkylineBox.Visible = true;

                    if (ReadSettings.ProcessSettings(ReadSettings.ToggleSkylineWhenIdentifier) == "song") // Remove Skyline on Song Load
                        this.ToggleSkylineSongRadio.Checked = true;
                    else if (ReadSettings.ProcessSettings(ReadSettings.ToggleSkylineWhenIdentifier) == "startup") // Remove Skyline on Game Startup 
                        this.ToggleSkylineStartupRadio.Checked = true;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.GreenScreenWallIdentifier) == "on") // Greenscreen Wall Enabled / Disabled
                    this.GreenScreenWallCheckbox.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.ForceProfileEnabledIdentifier) == "on") // Force Load Profile On Game Boot Enabled / Disabled
                    this.AutoLoadProfileCheckbox.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.FretlessModeEnabledIdentifier) == "on") // Fretless Mode Enabled / Disabled
                    this.FretlessModeCheckbox.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveInlaysIdentifier) == "on") // Remove Inlay Markers Enabled / Disabled
                    this.RemoveInlaysCheckbox.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveLaneMarkersIdentifier) == "on") // Remove Line Markers Enabled / Disabled
                    this.RemoveLineMarkersCheckBox.Checked = true;

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsIdentifier) == "on") // Remove Lyrics
                {
                    this.RemoveLyricsCheckbox.Checked = true;
                    this.HowToToggleLyrics.Visible = true;

                    if (ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsWhenIdentifier) == "startup") // Remove Lyrics When ...
                        this.ToggleLyricsRadio.Checked = true;
                    else if (ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsWhenIdentifier) == "manual") // Remove Lyrics When ...
                        this.ToggleLyricsManualRadio.Checked = true;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakIdentifier) == "on")
                {
                    this.GuitarSpeakEnableCheckbox.Checked = true;
                    this.GuitarSpeakBox.Visible = true;
                    this.GuitarSpeakWhileTuningBox.Visible = true;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakTuningIdentifier) == "on")
                    this.GuitarSpeakWhileTuningBox.Checked = true;
            }

            // Initialize Default String Colors
            LoadDefaultStringColors();

            // Mod Settings
            this.EnumerateEveryXMS.Value = Decimal.Parse(ReadSettings.ProcessSettings(ReadSettings.CheckForNewSongIntervalIdentifier)) / 1000; // Loads old settings for enumeration every x ms
            this.ExtendedRangeTunings.SelectedIndex = (Convert.ToInt32(ReadSettings.ProcessSettings(ReadSettings.ExtendedRangeTuningIdentifier)) * -1) - 2;

            // Set And Forget Mods
            LoadSetAndForgetMods();
        }

        private void ModList_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch (this.ModList.SelectedIndex) {
                case 0:
                    this.NewAssignmentTxtBox.Text = KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ToggleLoftIdentifier));
                    break;
                case 1:
                    this.NewAssignmentTxtBox.Text = KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.AddVolumeIdentifier));
                    break;
                case 2:
                    this.NewAssignmentTxtBox.Text = KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.DecreaseVolumeIdentifier));
                    break;
                case 3:
                    this.NewAssignmentTxtBox.Text = KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerIdentifier));
                    break;
                case 4:
                    this.NewAssignmentTxtBox.Text = KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationIdentifier));
                    break;
                case 5:
                    this.NewAssignmentTxtBox.Text = KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RainbowStringsIdentifier));
                    break;
                case 6:
                    this.NewAssignmentTxtBox.Text = KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsKeyIdentifier));
                    break;

                default:
                    this.NewAssignmentTxtBox.Text = String.Empty;
                    break;
            }
        }

        private void CheckKeyPressesDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter) // If enter is pressed
            {
                e.SuppressKeyPress = true; // Turns off the windows beep for pressing an invalid key.
                Save_Songlists_Keybindings(sender, e);
            }

            else if (sender == this.NewAssignmentTxtBox)
            {
                e.SuppressKeyPress = true; // Turns off the windows beep for pressing an invalid key.

                if (KeyConversion.KeyDownDictionary.Contains(e.KeyCode))
                    NewAssignmentTxtBox.Text = e.KeyCode.ToString();
                // Number or Letter was pressed (Will be overrided by text input)
                else if ((e.KeyValue > 47 && e.KeyValue < 60) || (e.KeyValue > 64 && e.KeyValue < 91))
                {
                    if (MessageBox.Show("The key you entered is currently used by Rocksmith and may interfere with being able to use the game properly. Are you sure you want to use this keybinding?", "Keybinding Warning!", MessageBoxButtons.OKCancel, MessageBoxIcon.Warning) == DialogResult.OK)
                        return;
                    else
                        NewAssignmentTxtBox.Text = "";
                }
            }
        }

        private void CheckKeyPressesUp(object sender, KeyEventArgs e)
        {
            if (KeyConversion.KeyUpDictionary.Contains(e.KeyCode))
                NewAssignmentTxtBox.Text = e.KeyCode.ToString();
        }

        private void CheckMouseInput(object sender, MouseEventArgs e)
        {
            if (KeyConversion.MouseButtonDictionary.Contains(e.Button))
                NewAssignmentTxtBox.Text = e.Button.ToString();
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
            this.Hide();
            var newForm = new MainForm();
            newForm.Closed += (s, args) => this.Close();
            newForm.Show();
        }

        public static Dictionary<string, Dictionary<string, string>> priorSettings = new Dictionary<string, Dictionary<string, string>>()
        {
            // [Section]                         mod    default
            {"[SongListTitles]", new Dictionary<string, string> {
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

        public static Dictionary<int, TextBox> stringNumberToColorTextBox = new Dictionary<int, TextBox>() {}; // Can't put variables into it until after we create it.


        private void ChangeStringColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            bool isNormalStrings = DefaultStringColorsRadio.Checked.ToString().ToLower() == "true"; // True = Normal, False = Colorblind
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

            stringNumberToColorTextBox.Add(0, String0Color);
            stringNumberToColorTextBox.Add(1, String1Color);
            stringNumberToColorTextBox.Add(2, String2Color);
            stringNumberToColorTextBox.Add(3, String3Color);
            stringNumberToColorTextBox.Add(4, String4Color);
            stringNumberToColorTextBox.Add(5, String5Color);
        }

        private void LoadDefaultStringColors(bool colorBlind = false)
        {
            if (!colorBlind)
            {
                String0Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String0Color_N_Identifier));
                String1Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String1Color_N_Identifier));
                String2Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String2Color_N_Identifier));
                String3Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String3Color_N_Identifier));
                String4Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String4Color_N_Identifier));
                String5Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String5Color_N_Identifier));
            }
            else
            {
                String0Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String0Color_CB_Identifier));
                String1Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String1Color_CB_Identifier));
                String2Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String2Color_CB_Identifier));
                String3Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String3Color_CB_Identifier));
                String4Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String4Color_CB_Identifier));
                String5Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String5Color_CB_Identifier));
            }
           
        }

        private void DefaultStringColorsRadio_CheckedChanged(object sender, EventArgs e)
        {
            LoadDefaultStringColors();
        }

        private void ColorBlindStringColorsRadio_CheckedChanged(object sender, EventArgs e)
        {
            LoadDefaultStringColors(true);
        }

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
            ListTunings.Items.Clear();
            tuningsCollection = LoadTuningsCollection();

            ListTunings.Items.Add("<New>");
            foreach (var key in tuningsCollection.Keys)
                ListTunings.Items.Add(key);
        }

        private TuningDefinitionInfo GetCurrentTuningInfo()
        {
            var tuningDefinition = new TuningDefinitionInfo();
            var strings = new Dictionary<string, int>();

            for (int strIdx = 0; strIdx < 6; strIdx++)
                strings[$"string{strIdx}"] = (int)((NumericUpDown)Controls["groupSetAndForget"].Controls[$"nupString{strIdx}"]).Value;

            tuningDefinition.Strings = strings;
            tuningDefinition.UIName = String.Format("$[{0}]{1}", NupTuningIndex.Value.ToString(), TxtUIName.Text);

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
            if (ListTunings.SelectedItem == null)
                return;

            string selectedItem = ListTunings.SelectedItem.ToString();

            if (selectedItem == "<New>")
            {
                TxtInternalName.Text = "";
                NupTuningIndex.Value = 0;
                TxtUIName.Text = "";
                return;
            }

            var selectedTuning = tuningsCollection[selectedItem];
            var uiName = SplitTuningUIName(selectedTuning.UIName);

            TxtInternalName.Text = selectedItem;
            NupTuningIndex.Value = Convert.ToInt32(uiName.Item1);
            TxtUIName.Text = uiName.Item2;

            for (int strIdx = 0; strIdx < 6; strIdx++) // If you are lazy and don't want to list each string separately, just do this sexy two-liner
                ((NumericUpDown)Controls["groupSetAndForget"].Controls[$"nupString{strIdx}"]).Value = selectedTuning.Strings[$"string{strIdx}"];
        }

        private void BtnSaveTuningChanges_Click(object sender, EventArgs e)
        {
            if (ListTunings.SelectedItem != null) // If we are saving a change to the currently selected tuning, perform a change in the collection, otherwise directly go to saving
            {
                string selectedItem = ListTunings.SelectedItem.ToString();

                if (selectedItem != "<New>")
                    tuningsCollection[selectedItem] = GetCurrentTuningInfo();
            }

            SaveTuningsJSON();

            MessageBox.Show("Saved current tuning, don't forget to press \"Add Custom Tunings\" button when you are done!", "Success");
        }

        private void BtnRemoveTuning_Click(object sender, EventArgs e)
        {
            if (ListTunings.SelectedItem == null)
                return;

            string selectedItem = ListTunings.SelectedItem.ToString();

            if (selectedItem == "<New>")
                return;

            tuningsCollection.Remove(selectedItem); // I guess we would be better here using BindingSource on Listbox + ObservableCollection instead of Dict to make changes reflect automatically, but... one day
            ListTunings.Items.Remove(selectedItem);
        }

        private void BtnAddTuning_Click(object sender, EventArgs e)
        {
            if (ListTunings.SelectedItem.ToString() != "<New>")
                return;

            var currTuning = GetCurrentTuningInfo();
            string internalName = TxtInternalName.Text;

            if (!tuningsCollection.ContainsKey(internalName)) // Unlikely to happen, but still... prevent users accidentaly trying to add existing stuff
            {
                tuningsCollection.Add(internalName, currTuning);
                ListTunings.Items.Add(internalName);
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
            if (ListProfileTones.SelectedItem == null)
                return;

            ZipUtilities.ExtractSingleFile(Constants.CustomModsFolder, Constants.Cache7_7zPath, Constants.ToneManager_InternalPath);

            string toneManagerFileContent = File.ReadAllText(Constants.ToneManager_CustomPath);
            var tonesJson = JObject.Parse(toneManagerFileContent);
            var toneList = tonesJson["Static"]["ToneManager"]["Tones"];
            var defaultTones = JsonConvert.DeserializeObject<List<Tone2014>>(toneList.ToString());

            var selectedTone = TonesFromAllProfiles[ListProfileTones.SelectedItem.ToString()];
            if (RbTone0.Checked)
                tonesJson["Static"]["ToneManager"]["Tones"][0]["GearList"] = JObject.FromObject(selectedTone.GearList);
            else if (RbTone1.Checked)
                tonesJson["Static"]["ToneManager"]["Tones"][1]["GearList"] = JObject.FromObject(selectedTone.GearList);
            else if (RbTone2.Checked)
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
                    ListProfileTones.Items.Clear();

                    foreach (string profile in profiles)
                        foreach (var tone in Tone2014.Import(profile))
                        {
                            ListProfileTones.Items.Add(tone.Name);
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
            if(!File.Exists(Constants.Cache4_7zPath) || !File.Exists(Constants.Cache7_7zPath))
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
            string currentTab = this.TabController.SelectedTab.Text.ToString();

            if (currentTab == "Song Lists")
            {
                foreach (KeyValuePair<int, string> currentSongList in SongListIndexToINISetting)
                {
                    if (this.NewSongListNameTxtbox.Text.Trim() == "") // The game UI will break with a blank name.
                    {
                        MessageBox.Show("You cannot save a blank song list name as the game will break", "Warning!", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        break;
                    }

                    else if (currentSongList.Key == this.Songlist.SelectedIndex)
                    {
                        SaveChanges(currentSongList.Value, this.NewSongListNameTxtbox.Text);
                        this.Songlist.Items[currentSongList.Key] = this.NewSongListNameTxtbox.Text;
                        break;
                    };
                }
            }
           
            if (currentTab == "Keybindings")
            {
                foreach (KeyValuePair<int, string> currentKeybinding in KeybindingsIndexToINISetting)
                {
                    if (this.NewAssignmentTxtBox.Text == "")
                    {
                        MessageBox.Show("You cannot set a blank keybind", "Warning!", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        break;
                    }

                    else if (currentKeybinding.Key == this.ModList.SelectedIndex)
                    {
                        SaveChanges(currentKeybinding.Value, KeyConversion.VirtualKey(this.NewAssignmentTxtBox.Text));
                        break;
                    }
                }

                this.NewAssignmentTxtBox.Text = String.Empty;
            }

            ShowCurrentKeybindingValues();
        }

        private void ToggleLoftCheckbox_CheckedChanged(object sender, EventArgs e) // Toggle Loft Enabled/ Disabled
        {
            if (this.ToggleLoftCheckbox.Checked)
            {
                SaveChanges(ReadSettings.ToggleLoftEnabledIdentifier, "true");
                this.ToggleLoftCheckbox.Checked = true;
                this.ToggleLoftWhenStartupRadio.Visible = true;
                this.ToggleLoftWhenManualRadio.Visible = true;
                this.ToggleLoftWhenSongRadio.Visible = true;
                this.ToggleLoftOffWhenBox.Visible = true;
            }
            else
            {
                SaveChanges(ReadSettings.ToggleLoftEnabledIdentifier, "false");
                this.ToggleLoftCheckbox.Checked = false;
                this.ToggleLoftWhenStartupRadio.Visible = false;
                this.ToggleLoftWhenManualRadio.Visible = false;
                this.ToggleLoftWhenSongRadio.Visible = false;
                this.ToggleLoftOffWhenBox.Visible = false;
            }
        }

        private void AddVolumeCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (this.AddVolumeCheckbox.Checked)
                SaveChanges(ReadSettings.AddVolumeEnabledIdentifier, "true");
            else
                SaveChanges(ReadSettings.AddVolumeEnabledIdentifier, "false");
        }

        private void DecreaseVolumeCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (this.DecreaseVolumeCheckbox.Checked)
                SaveChanges(ReadSettings.DecreaseVolumeEnabledIdentifier, "true");
            else
                SaveChanges(ReadSettings.DecreaseVolumeEnabledIdentifier, "false");
        }

        private void SongTimerCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (this.SongTimerCheckbox.Checked)
                SaveChanges(ReadSettings.ShowSongTimerEnabledIdentifier, "true");
            else
                SaveChanges(ReadSettings.ShowSongTimerEnabledIdentifier, "false");
        }

        private void ForceEnumerationCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (ForceEnumerationCheckbox.Checked)
            {
                SaveChanges(ReadSettings.ForceReEnumerationEnabledIdentifier, "manual");
                this.ForceEnumerationCheckbox.Checked = true;
                this.ForceEnumerationAutomaticRadio.Visible = true;
                this.ForceEnumerationManualRadio.Visible = true;
                this.HowToEnumerateBox.Visible = true;
            }
            else
            {
                SaveChanges(ReadSettings.ForceReEnumerationEnabledIdentifier, "false");
                this.ForceEnumerationCheckbox.Checked = false;
                this.ForceEnumerationAutomaticRadio.Visible = false;
                this.ForceEnumerationManualRadio.Visible = false;
                this.HowToEnumerateBox.Visible = false;
            }
        }

        private void EnumerateEveryXMS_ValueChanged(object sender, EventArgs e)
        {
            SaveChanges(ReadSettings.CheckForNewSongIntervalIdentifier, (this.EnumerateEveryXMS.Value * 1000).ToString());
        }

        private void ForceEnumerationAutomaticRadio_CheckedChanged(object sender, EventArgs e)
        {
            this.CheckEveryXmsText.Visible = true;
            this.EnumerateEveryXMS.Visible = true;
            SaveChanges(ReadSettings.ForceReEnumerationEnabledIdentifier, "automatic");
        }

        private void ForceEnumerationManualRadio_CheckedChanged(object sender, EventArgs e)
        {
            this.CheckEveryXmsText.Visible = false;
            this.EnumerateEveryXMS.Visible = false;
            SaveChanges(ReadSettings.ForceReEnumerationEnabledIdentifier, "manual");
        }

        private void RainbowStringsEnabled_CheckedChanged(object sender, EventArgs e)
        {
            if (RainbowStringsEnabled.Checked)
                SaveChanges(ReadSettings.RainbowStringsEnabledIdentifier, "true");
            else
                SaveChanges(ReadSettings.RainbowStringsEnabledIdentifier, "false");
        }

        private void ExtendedRangeEnabled_CheckedChanged(object sender, EventArgs e)
        {
            if (ExtendedRangeEnabled.Checked)
            {
                this.ExtendedRangeTuningBox.Visible = true;
                this.ExtendedRangeTunings.Visible = true;
                SaveChanges(ReadSettings.ExtendedRangeEnabledIdentifier, "true");
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "2");
            }
            else
            {
                this.ExtendedRangeTuningBox.Visible = false;
                this.ExtendedRangeTunings.Visible = false;
                SaveChanges(ReadSettings.ExtendedRangeEnabledIdentifier, "false");
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "0");
            }
        }

        private void CustomColorsCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (CustomColorsCheckbox.Checked)
            {
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "2");
                this.CustomColorsCheckbox.Checked = true;
                this.ChangeStringColorsBox.Visible = true;
            }
            else
            {
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "0");
                this.CustomColorsCheckbox.Checked = false;
                this.ChangeStringColorsBox.Visible = false;
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
            if (HeadstockCheckbox.Checked)
            {
                SaveChanges(ReadSettings.RemoveHeadstockIdentifier, "true");
                this.ToggleHeadstockWhenBox.Visible = true;
            }

            else
            {
                SaveChanges(ReadSettings.RemoveHeadstockIdentifier, "false");
                this.ToggleHeadstockWhenBox.Visible = false;
            }

        }

        private void RemoveSkylineCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (RemoveSkylineCheckbox.Checked)
            {
                SaveChanges(ReadSettings.RemoveSkylineIdentifier, "true");
                this.ToggleSkylineBox.Visible = true;
            }
            else
            {
                SaveChanges(ReadSettings.RemoveSkylineIdentifier, "false");
                this.ToggleSkylineBox.Visible = false;
            }
        }

        private void GreenScreenWallCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (GreenScreenWallCheckbox.Checked)
                SaveChanges(ReadSettings.GreenScreenWallIdentifier, "true");
            else
                SaveChanges(ReadSettings.GreenScreenWallIdentifier, "false");
        }

        private void AutoLoadProfileCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (AutoLoadProfileCheckbox.Checked)
                SaveChanges(ReadSettings.ForceProfileEnabledIdentifier, "true");
            else
                SaveChanges(ReadSettings.ForceProfileEnabledIdentifier, "false");
        }

        private void FretlessModeCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (FretlessModeCheckbox.Checked)
                SaveChanges(ReadSettings.FretlessModeEnabledIdentifier, "true");
            else
                SaveChanges(ReadSettings.FretlessModeEnabledIdentifier, "false");
        }

        private void RemoveInlaysCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (RemoveInlaysCheckbox.Checked)
                SaveChanges(ReadSettings.RemoveInlaysIdentifier, "true");
            else
                SaveChanges(ReadSettings.RemoveInlaysIdentifier, "false");
        }

        private void ToggleLoftWhenManualRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (ToggleLoftWhenManualRadio.Checked)
                SaveChanges(ReadSettings.ToggleLoftWhenIdentifier, "manual");
        }

        private void ToggleLoftWhenSongRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (ToggleLoftWhenSongRadio.Checked)
                SaveChanges(ReadSettings.ToggleLoftWhenIdentifier, "song");
        }

        private void ToggleLoftWhenStartupRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (ToggleLoftWhenStartupRadio.Checked)
                SaveChanges(ReadSettings.ToggleLoftWhenIdentifier, "startup");
        }

        private void RemoveLineMarkersCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (RemoveLineMarkersCheckBox.Checked)
                SaveChanges(ReadSettings.RemoveLaneMarkersIdentifier, "true");
            else
                SaveChanges(ReadSettings.RemoveLaneMarkersIdentifier, "false");
        }

        private void ToggleSkylineSongRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (ToggleSkylineSongRadio.Checked)
                SaveChanges(ReadSettings.ToggleSkylineWhenIdentifier, "song");
        }

        private void ToggleSkylineStartupRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (ToggleSkylineStartupRadio.Checked)
                SaveChanges(ReadSettings.ToggleSkylineWhenIdentifier, "startup");
        }

        private void ExtendedRangeTunings_SelectedIndexChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.ExtendedRangeTuningIdentifier, Convert.ToString((this.ExtendedRangeTunings.SelectedIndex * -1) - 2));

        private void DeleteKeyBind_Click(object sender, EventArgs e)
        {
            this.NewAssignmentTxtBox.Text = "";

            foreach (KeyValuePair<int, string> currentMod in KeybindingsIndexToINISetting)
            {
                if (currentMod.Key == this.ModList.SelectedIndex)
                {
                    SaveChanges(currentMod.Value, "");
                    break;
                }
            }
            ShowCurrentKeybindingValues();
        }

        private void ShowCurrentKeybindingValues()
        {
            this.ToggleLoftKey.Text = "Toggle Loft: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ToggleLoftIdentifier));
            this.AddVolumeKey.Text = "Add Volume: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.AddVolumeIdentifier));
            this.DecreaseVolumeKey.Text = "Decrease Volume: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.DecreaseVolumeIdentifier));
            this.SongTimerKey.Text = "Show Song Timer: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerIdentifier));
            this.ReEnumerationKey.Text = "Force ReEnumeration: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationIdentifier));
            this.RainbowStringsAssignment.Text = "Rainbow Strings: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RainbowStringsIdentifier));
            this.RemoveLyricsKeyLabel.Text = "Remove Lyrics: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsKeyIdentifier));
        }

        private void RemoveLyricsCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (RemoveLyricsCheckbox.Checked)
            {
                SaveChanges(ReadSettings.RemoveLyricsIdentifier, "on");
                this.ToggleLyricsRadio.Visible = true;
                this.ToggleLyricsManualRadio.Visible = true;
                this.HowToToggleLyrics.Visible = true;
            }

            else
            {
                SaveChanges(ReadSettings.RemoveLyricsIdentifier, "off");
                this.ToggleLyricsRadio.Visible = false;
                this.ToggleLyricsManualRadio.Visible = false;
                this.HowToToggleLyrics.Visible = false;
            }
        }


        public static Dictionary<Control, string> TooltipDictionary = new Dictionary<Control, string>() {}; // I really wish I could make this full of stuff, but `this.` and `MainForm.` don't work, so I need to call a different function `FillToolTipDictionary()` do it for me. :(


        private void FillToolTipDictionary()
        {
            HasToolTipDictionaryBeenCreatedYet = true;

            // INI Edits
            // Checkboxes
            TooltipDictionary.Add(ToggleLoftCheckbox, "Disables the game background, amps and noise reactive speaker rings.\nBest used with Venue Mode off (setting in game).\nUsed by a lot of Rocksmith Streamers to make it easy to Luma Key out the game background.\nPlayer just sees an all black background when this is enabled.\nOptions for turning the loft off only when in a song, when the game first starts up, or on a key press.");
            //TooltipDictionary.Add(AddVolumeCheckbox, "Experimental"); // Changed to "VolumeControlsCheckbox"
            //TooltipDictionary.Add(DecreaseVolumeCheckbox, "Experimental"); // Changed to "VolumeControlsCheckbox"
            TooltipDictionary.Add(SongTimerCheckbox, "Experimental.\nIntent is to show a box with your timestamp position through the song.");
            TooltipDictionary.Add(ExtendedRangeEnabled, "Alters the string and note colors to make it easier to play a 5 string bass or 7 string guitar.");
            TooltipDictionary.Add(ForceEnumerationCheckbox, "Game will automatically start an Enumeration sequence when a new psarc (CDLC) file is detected as having been added to the dlc folder.\nNot necesary to enable if you're already using Rocksniffer to do the same thing.");
            TooltipDictionary.Add(HeadstockCheckbox, "Stops the Headstock of the guitar being drawn.\n“Headless” guitar mode. Just cleans up some more of the UI.");
            TooltipDictionary.Add(RemoveSkylineCheckbox, "Removes the purple and orange bars from the top of the display in LAS.\nUse in conjunction with No Loft for a cleaner UI.\nOptions for always off, only off when in a song, or only when toggled by key press.");
            TooltipDictionary.Add(GreenScreenWallCheckbox, "Changes just a section of the game background to all black, amusing for a selective “green screen” stream experience.\nInvalidated by \"No Loft\".");
            TooltipDictionary.Add(AutoLoadProfileCheckbox, "Essentially holds down the ENTER key until the game has reached the main menu.\nLets you auto load the last used profile without needing to interact with the game at all.");
            TooltipDictionary.Add(FretlessModeCheckbox, "Removes the Fret Wire from the neck, making your instrument appear to be fretless.");
            TooltipDictionary.Add(RemoveInlaysCheckbox, "Disables the guitar neck inlay display entirely.\nNote: This only works with the standard dot inlays.");
            TooltipDictionary.Add(VolumeControlsCheckbox, "Allows you to control how loud the song is using the in-game mixer without needing to open it.");
            TooltipDictionary.Add(GuitarSpeakEnableCheckbox, "Use your guitar to control the menus!");
            TooltipDictionary.Add(RemoveLyricsCheckbox, "Disables the display of song lyrics while in Learn-A-Song mode.");
            TooltipDictionary.Add(RainbowStringsEnabled, "Experimental.\nHow Pro are you? This makes the players guitar strings constantly cycling through colors.");
            TooltipDictionary.Add(CustomColorsCheckbox, "Lets you define the string / note colors you want.\nSaves a normal set and a Colorblind mode set.");
            TooltipDictionary.Add(RemoveLineMarkersCheckBox, "Removes the additional lane marker lines seen in the display.\nWhen used with No Loft, provides a cleaner Luma Key.");

            // Mods
            TooltipDictionary.Add(HowToEnumerateBox, "Choose to Enumerate on key press,\nor automatically scan for changes every X seconds and start enumeration if a new file has been added.");
            TooltipDictionary.Add(ExtendedRangeTuningBox, "Mod is enabled when the lowest string is tuned to the note defined here.\nSee the Custom Colors - Color Blind mode for the colors that will be used while in ER mode.");
            TooltipDictionary.Add(ToggleLoftOffWhenBox, "Turn the loft off via hotkey, as soon as the game starts up or only when in a song.");
            TooltipDictionary.Add(ColorBlindStringColorsRadio, "When ER mode is enabled, these are the colors that the strings will be changed to.");
            TooltipDictionary.Add(ToggleSkylineBox, "Turn the skyline (Purple and Orange DD level bars) as soon as the game starts up, or only when in a song.");
            TooltipDictionary.Add(HowToToggleLyrics, "How or when do you want the lyric display disabled, always, or toggled by a hotkey only?");
            TooltipDictionary.Add(ToggleLyricsRadio, "Lyrics display will always be disabled in Learn-A-Song game mode.");
            TooltipDictionary.Add(ToggleLyricsManualRadio, "Lyrics can be toggled on or off by a defined hotkey.");
            TooltipDictionary.Add(GuitarSpeakWhileTuningBox, "For Advanced Users Only!\nUse Guitar Speak in tuning menus.\nThis can potentially stop you from tuning, or playing songs if setup improperly.");

            // Misc
            TooltipDictionary.Add(SongListBox, "Custom names for the 6 \"SONG LISTS\" shown in game.");
            TooltipDictionary.Add(KeybindingsBox, "Set key binds for the toggle on / off by keypress modifications.\nYou need to press ENTER after setting teh key for it to be saved.");
            TooltipDictionary.Add(ResetToDefaultButton, "Resets all RSMods values to defaults");

            // Set & Forget Mods (Cache.psarc Modifications)
            // Tones
            TooltipDictionary.Add(ChangeTonesHeader, "This section lets you change the default menu tone for Lead, Rhythm Or Bass.\nYou need to have the tone you want to set saved in your profile first,\nthen you can load it here and set it as the default tone that will be used when you start up Rocksmith.");
            TooltipDictionary.Add(BtnLoadTonesFromProfiles, "Step 1.\nClick this to load the tones that are saved in your profile.");
            TooltipDictionary.Add(ListProfileTones, "Step2.\n Highlight a tone name.");
            TooltipDictionary.Add(RbTone0, "Set Highlighted Tone As New Default Rhythm Tone.");
            TooltipDictionary.Add(RbTone1, "Set Highlighted Tone As New Default Lead Tone.");
            TooltipDictionary.Add(RbTone2, "Set Highlighted Tone As New Default Bass Tone.");
            TooltipDictionary.Add(BtnSetDefaultTones, "Assign the currently highlighted tone to the chosen path.");

            // Custom Tuning
            TooltipDictionary.Add(ListTunings, "Shows the list of tuning definitions currently in Rocksmith.");
            TooltipDictionary.Add(BtnAddTuning, "Adds the tuning as defined above.");
            TooltipDictionary.Add(BtnRemoveTuning, "Removes the highlighted tuning.");
            TooltipDictionary.Add(NupString0, "Set the offset for the low-E string.");
            TooltipDictionary.Add(NupString1, "Set the offset for the A string.");
            TooltipDictionary.Add(NupString2, "Set the offset for the D string.");
            TooltipDictionary.Add(NupString3, "Set the offset for the G string.");
            TooltipDictionary.Add(NupString4, "Set the offset for the B string.");
            TooltipDictionary.Add(NupString5, "Set the offset for the high-E string.");
            TooltipDictionary.Add(BtnSaveTuningChanges, "Saves the tuning list to Rocksmith.");

            // One Click Mods
            TooltipDictionary.Add(BtnAddCustomMenu, "Adds the Direct Connect mode - microphone mode with tone simulations.\nAlso replaces UPLAY on the main menu with an EXIT GAME option.");
            TooltipDictionary.Add(BtnAddCustomTunings, "Adds some preset definitions for the most common Custom Tunings.");
            TooltipDictionary.Add(BtnAddFastLoadMod, "SSD drive or faster or may cause the game to not launch properly, skips some of the intro sequences.\nCombined with Auto Load Last Profile and huzzah!");
            
            // Misc
            TooltipDictionary.Add(BtnRemoveTempFolders, "Removes the temporary files used by RSMods.");
            TooltipDictionary.Add(BtnRestoreDefaults, "Restores the original cache.psarc file\nUndoes all \"Set-and-forget\" mods.");
            TooltipDictionary.Add(BtnUnpackCacheAgain, "Removes temporary files and un-packs cache.psarc as it is being used now, again.");
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
            {
                FillToolTipDictionary();
            }

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
            if (this.Songlist.SelectedIndex >= 0)
            {
                this.NewSongListNameTxtbox.Text = this.Songlist.SelectedItem.ToString();
            }
        }

        private void ToggleLyricsRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (ToggleLyricsRadio.Checked)
                SaveChanges(ReadSettings.RemoveLyricsWhenIdentifier, "startup");
        }

        private void ToggleLyricsManualRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (ToggleLyricsManualRadio.Checked)
                SaveChanges(ReadSettings.RemoveLyricsWhenIdentifier, "manual");
        }

        private void GuitarSpeakSaveButton_Click(object sender, EventArgs e)
        {
            if (GuitarSpeakNote.SelectedIndex >= 0 && GuitarSpeakOctave.SelectedIndex >= 0 && GuitarSpeakKeypress.SelectedIndex >= 0)
            {
                int inputNote = GuitarSpeakNote.SelectedIndex + 36; // We skip the first 3 octaves to give an accurate representation of the notes being played
                int inputOctave = GuitarSpeakOctave.SelectedIndex - 3; // -1 for the offset, and -2 for octave offset in DLL.
                int outputNoteOctave = inputNote + (inputOctave * 12);
                MessageBox.Show(GuitarSpeakNote.SelectedItem.ToString() + GuitarSpeakOctave.SelectedItem.ToString() + " was saved to " + GuitarSpeakKeypress.SelectedItem.ToString(), "Note Saved!", MessageBoxButtons.OK, MessageBoxIcon.Information);

                foreach (KeyValuePair<string, string> entry in GuitarSpeakKeyPressDictionary)
                {
                    if (GuitarSpeakKeypress.SelectedItem.ToString() == entry.Key)
                    {
                        SaveChanges(entry.Value, outputNoteOctave.ToString());
                        GuitarSpeakPresetsBox.ClearSelected();

                        foreach (string guitarSpeakItem in GuitarSpeakPresetsBox.Items)
                        {
                            if (guitarSpeakItem.Contains(GuitarSpeakKeypress.SelectedItem.ToString()))
                            {
                                GuitarSpeakPresetsBox.Items.Remove(guitarSpeakItem);
                                break;
                            }
                        }
                        GuitarSpeakPresetsBox.Items.Add(GuitarSpeakKeypress.SelectedItem.ToString() + ": " + GuitarSpeakNoteOctaveMath(outputNoteOctave.ToString()));
                        RefreshGuitarSpeakPresets();
                    }
                }

                GuitarSpeakNote.ClearSelected();
                GuitarSpeakOctave.ClearSelected();
                GuitarSpeakKeypress.ClearSelected();
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
            if (GuitarSpeakEnableCheckbox.Checked)
            {
                this.GuitarSpeakEnableCheckbox.Checked = true;
                this.GuitarSpeakBox.Visible = true;
                this.GuitarSpeakWhileTuningBox.Visible = true;
                SaveChanges(ReadSettings.GuitarSpeakIdentifier, "on");
            }
            else
            {
                this.GuitarSpeakEnableCheckbox.Checked = false;
                this.GuitarSpeakBox.Visible = false;
                this.GuitarSpeakWhileTuningBox.Visible = false;
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
                    CustomTuningLowEStringLetter.Text = IntToNote(Convert.ToInt32(NupString0.Value) + offset);
                    break;
                case 1:
                    offset = 33; // Offset from (24 - 1) + 10
                    CustomTuningAStringLetter.Text = IntToNote(Convert.ToInt32(NupString1.Value) + offset);
                    break;
                case 2:
                    offset = 26; // Offset from (24 - 1) + 3
                    CustomTuningDStringLetter.Text = IntToNote(Convert.ToInt32(NupString2.Value) + offset);
                    break;
                case 3:
                    offset = 31;// Offset from (24 - 1) + 8
                    CustomTuningGStringLetter.Text = IntToNote(Convert.ToInt32(NupString3.Value) + offset);
                    break;
                case 4:
                    offset = 35; // Offset from (24 - 1) + 12
                    CustomTuningBStringLetter.Text = IntToNote(Convert.ToInt32(NupString4.Value) + offset);
                    break;
                case 5:
                    offset = 28; // Offset from (24 - 1) + 5
                    CustomTuningHighEStringLetter.Text = IntToNote(Convert.ToInt32(NupString5.Value) + offset);
                    break;
                default: // Yeah we don't know wtf happened here
                    MessageBox.Show("Invalid String Number! Please report this to the GUI devs!");
                    break;
            }
        }

        private void VolumeControlsCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (VolumeControlsCheckbox.Checked)
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
            if (GuitarSpeakWhileTuningBox.Checked)
            {
                SaveChanges(ReadSettings.GuitarSpeakTuningIdentifier, "on");
            }
            else
            {
                SaveChanges(ReadSettings.GuitarSpeakTuningIdentifier, "off");
            }
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
            if (HeadStockAlwaysOffButton.Checked)
                SaveChanges(ReadSettings.RemoveHeadstockWhenIdentifier, "startup");
        }

        private void HeadstockOffInSongOnlyButton_CheckedChanged(object sender, EventArgs e)
        {
            if (HeadstockOffInSongOnlyButton.Checked)
                SaveChanges(ReadSettings.RemoveHeadstockWhenIdentifier, "song");
        }

        private void RefreshGuitarSpeakPresets()
        {
            this.GuitarSpeakPresetsBox.Items.Clear();

            this.GuitarSpeakPresetsBox.Items.AddRange(new object[]
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
    }
}