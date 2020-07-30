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
using RocksmithToolkitLib.DLCPackage.Manifest.Tone;

namespace RSMods
{
    public partial class MainForm : Form
    {
        public MainForm()
        {
            WriteSettings.IsVoid(GenUtil.GetRSDirectory());
            if (!File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini")))
                WriteSettings.WriteINI(WriteSettings.Settings); // Creates Settings File
            InitializeComponent();


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
                "Rainbow Strings"});

            // Mod Key Values
            ResetModKeyValues();


            // Load Checkbox Values
            {
                if (ReadSettings.ProcessSettings(ReadSettings.ToggleLoftEnabledIdentifier) == "on") // Toggle Loft Enabled / Disabled
                {
                    this.ToggleLoftCheckbox.Checked = true;
                    this.ToggleLoftWhenStartupRadio.Visible = true;
                    this.ToggleLoftWhenManualRadio.Visible = true;
                    this.ToggleLoftWhenSongRadio.Visible = true;
                    this.ToggleLoftOffWhenBox.Visible = true;

                    if (ReadSettings.ProcessSettings(ReadSettings.ToggleLoftWhenIdentifier) == "startup")
                    {
                        this.ToggleLoftWhenStartupRadio.Checked = true;
                    }
                    if (ReadSettings.ProcessSettings(ReadSettings.ToggleLoftWhenIdentifier) == "manual")
                    {
                        this.ToggleLoftWhenManualRadio.Checked = true;
                    }
                    if (ReadSettings.ProcessSettings(ReadSettings.ToggleLoftWhenIdentifier) == "song")
                    {
                        this.ToggleLoftWhenSongRadio.Checked = true;
                    }
                }
                else
                {
                    this.ToggleLoftCheckbox.Checked = false;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.AddVolumeEnabledIdentifier) == "on") // Add Volume Enabled / Disabled
                {
                    this.AddVolumeCheckbox.Checked = true;
                }
                else
                {
                    this.AddVolumeCheckbox.Checked = false;
                }
                if (ReadSettings.ProcessSettings(ReadSettings.DecreaseVolumeEnabledIdentifier) == "on") // Decrease Volume Enabled / Disabled
                {
                    this.DecreaseVolumeCheckbox.Checked = true;
                }
                else
                {
                    this.DecreaseVolumeCheckbox.Checked = false;
                }
                if (ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerEnabledIdentifier) == "on") // Show Song Timer Enabled / Disabled
                {
                    this.SongTimerCheckbox.Checked = true;
                }
                else
                {
                    this.SongTimerCheckbox.Checked = false;
                }
                if (ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationEnabledIdentifier) != "false") // Force Enumeration Settings
                {
                    this.ForceEnumerationCheckbox.Checked = true;
                    this.ForceEnumerationAutomaticRadio.Visible = true;
                    this.ForceEnumerationManualRadio.Visible = true;
                    this.HowToEnumerateBox.Visible = true;
                    if (ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationEnabledIdentifier) == "automatic")
                    {
                        this.ForceEnumerationAutomaticRadio.Checked = true;
                    }
                    else
                    {
                        this.ForceEnumerationManualRadio.Checked = true;
                    }

                }
                else
                {
                    this.ForceEnumerationCheckbox.Checked = false;
                    this.ForceEnumerationAutomaticRadio.Visible = false;
                    this.ForceEnumerationManualRadio.Visible = false;
                    this.HowToEnumerateBox.Visible = false;
                }
                if (ReadSettings.ProcessSettings(ReadSettings.RainbowStringsEnabledIdentifier) == "on") // Rainbow String Enabled / Disabled
                {
                    this.RainbowStringsEnabled.Checked = true;
                }
                else
                {
                    this.RainbowStringsEnabled.Checked = false;
                }
                if (ReadSettings.ProcessSettings(ReadSettings.ExtendedRangeEnabledIdentifier) == "on") // Extended Range Enabled / Disabled
                {
                    this.ExtendedRangeEnabled.Checked = true;
                    this.ExtendedRangeTuningBox.Visible = true;
                    this.ExtendedRangeTunings.Visible = true;
                }
                else
                {
                    this.ExtendedRangeEnabled.Checked = false;
                }
                if (ReadSettings.ProcessSettings(ReadSettings.CustomStringColorNumberIndetifier) != "0") // Custom String Colors
                {
                    this.customColorsCheckbox.Checked = true;
                    this.ChangeStringColorsBox.Visible = true;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.DiscoModeIdentifier) == "on") // Disco Mode Enabled / Disabled
                {
                    this.DiscoModeCheckbox.Checked = true;
                }
                else
                {
                    this.DiscoModeCheckbox.Checked = false;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveHeadstockIdentifier) == "on") // Remove Headstock Enabled / Disabled
                {
                    this.HeadstockCheckbox.Checked = true;
                }
                else
                {
                    this.HeadstockCheckbox.Checked = false;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveSkylineIdentifier) == "on") // Remove Skyline Enabled / Disabled
                {
                    this.RemoveSkylineCheckbox.Checked = true;
                    this.ToggleSkylineBox.Visible = true;
                }
                else
                {
                    this.RemoveSkylineCheckbox.Checked = false;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.GreenScreenWallIdentifier) == "on") // Greenscreen Wall Enabled / Disabled
                {
                    this.GreenScreenWallCheckbox.Checked = true;
                }
                else
                {
                    this.GreenScreenWallCheckbox.Checked = false;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.ForceProfileEnabledIdentifier) == "on") // Force Load Profile On Game Boot Enabled / Disabled
                {
                    this.AutoLoadProfileCheckbox.Checked = true;
                }
                else
                {
                    this.AutoLoadProfileCheckbox.Checked = false;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.FretlessModeEnabledIdentifier) == "on") // Fretless Mode Enabled / Disabled
                {
                    this.FretlessModeCheckbox.Checked = true;
                }
                else
                {
                    this.FretlessModeCheckbox.Checked = false;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveInlaysIdentifier) == "on") // Remove Inlay Markers Enabled / Disabled
                {
                    this.RemoveInlaysCheckbox.Checked = true;
                }
                else
                {
                    this.RemoveInlaysCheckbox.Checked = false;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveLaneMarkersIdentifier) == "on") // Remove Line Markers Enabled / Disabled
                {
                    this.RemoveLineMarkersCheckBox.Checked = true;
                }
                else
                {
                    this.RemoveLineMarkersCheckBox.Checked = false;
                }
                if (ReadSettings.ProcessSettings(ReadSettings.ToggleSkylineWhenIdentifier) == "song") // Remove Skyline on Song Load
                {
                    this.ToggleSkylineSongRadio.Checked = true;
                }
                else if (ReadSettings.ProcessSettings(ReadSettings.ToggleSkylineWhenIdentifier) == "startup") // Remove Skyline on Game Startup 
                {
                    this.ToggleSkylineStartupRadio.Checked = true;
                }

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsIdentifier) == "on") // Remove Lyrics
                {
                    this.RemoveLyricsCheckbox.Checked = true;
                }
                else
                {
                    this.RemoveLyricsCheckbox.Checked = false;
                }
            }

            // Initialize Default String Colors
            {
                String0Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String0Color_N_Identifier));
                String1Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String1Color_N_Identifier));
                String2Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String2Color_N_Identifier));
                String3Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String3Color_N_Identifier));
                String4Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String4Color_N_Identifier));
                String5Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String5Color_N_Identifier));
            }

            // Mod Settings
            {

                this.EnumerateEveryXMS.Value = Decimal.Parse(ReadSettings.ProcessSettings(ReadSettings.CheckForNewSongIntervalIdentifier)) / 1000; // Loads old settings for enumeration every x ms
                {
                    int selectedIndexERTuning = (Convert.ToInt32(ReadSettings.ProcessSettings(ReadSettings.ExtendedRangeTuningIdentifier)) * -1) - 2;
                    this.ExtendedRangeTunings.SelectedIndex = selectedIndexERTuning;
                }

            }

            LoadSetAndForgetMods();
        }

        private void ModList_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.ModList.GetSelected(0))
            {
                this.NewAssignmentTxtBox.Text = ReadSettings.ProcessSettings(ReadSettings.ToggleLoftIdentifier);
            }
            if (this.ModList.GetSelected(1))
            {
                this.NewAssignmentTxtBox.Text = ReadSettings.ProcessSettings(ReadSettings.AddVolumeIdentifier);
            }
            if (this.ModList.GetSelected(2))
            {
                this.NewAssignmentTxtBox.Text = ReadSettings.ProcessSettings(ReadSettings.DecreaseVolumeIdentifier);
            }
            if (this.ModList.GetSelected(3))
            {
                this.NewAssignmentTxtBox.Text = ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerIdentifier);
            }
            if (this.ModList.GetSelected(4))
            {
                this.NewAssignmentTxtBox.Text = ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationIdentifier);
            }
            if (this.ModList.GetSelected(5))
            {
                this.NewAssignmentTxtBox.Text = ReadSettings.ProcessSettings(ReadSettings.RainbowStringsIdentifier);
            }
        }

        private void CheckKeyPressesDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter) // If enter is pressed
            {
                Save_Songlists_Keybindings(sender, e);
                e.SuppressKeyPress = true;
                //RefreshForm();
            }

            else if (sender == this.NewAssignmentTxtBox)
            {
                e.SuppressKeyPress = true;

                if (KeyConversion.KeyDownDictionary.Contains(e.KeyCode))
                {
                    NewAssignmentTxtBox.Text = e.KeyCode.ToString();
                }
                // Number or Letter was pressed (Will be overrided by text input)
                else if ((e.KeyValue > 47 && e.KeyValue < 60) || (e.KeyValue > 64 && e.KeyValue < 91))
                {
                    return;
                }


                //else if (e.KeyValue != null) // If key is unknown, give me the int to look at this document for reference: https://docs.microsoft.com/en-us/dotnet/api/system.windows.forms.keys?view=netcore-3.1
                //{
                //    NewAssignmentTxtBox.Text = e.KeyValue.ToString();
                //}
            }
        }

        private void CheckKeyPressesUp(object sender, KeyEventArgs e)
        {
            if (KeyConversion.KeyUpDictionary.Contains(e.KeyCode))
            {
                NewAssignmentTxtBox.Text = e.KeyCode.ToString();
            }

        }

        private void CheckMouseInput(object sender, MouseEventArgs e)
        {
            if (KeyConversion.MouseButtonDictionary.Contains(e.Button))
            {
                NewAssignmentTxtBox.Text = e.Button.ToString();
            }
        }

        private void ResetToDefaultSettings(object sender, EventArgs e)
        {
            File.Delete(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini"));
            RefreshForm();
        }

        private void RefreshForm()
        {
            this.Hide();
            var newForm = new MainForm();
            newForm.Closed += (s, args) => this.Close();
            newForm.Show();
        }


        private void ImportPriorSettings(Dictionary<string, Dictionary<string, string>> DictionaryToWrite)
        {
            using (StreamWriter sw = File.CreateText(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini")))
            {
                foreach (string section in DictionaryToWrite.Keys)
                {
                    sw.WriteLine(section);
                    foreach (KeyValuePair<string, string> entry in DictionaryToWrite[section])
                    {
                        sw.WriteLine(entry.Key + entry.Value);
                    }
                }
            }
        }



        public static Dictionary<string, Dictionary<string, string>> priorSettings = new Dictionary<string, Dictionary<string, string>>()
        {
            // Section                           mod   default
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
                { ReadSettings.DiscoModeIdentifier, ReadSettings.ProcessSettings(ReadSettings.DiscoModeIdentifier) }, // Disco Mode Enabled / Disabled
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
        };



        private void SaveChanges(string IdentifierToChange, string ChangedSettingValue)
        {
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
                            break;
                        }
                    }
                }
            WriteSettings.WriteINI(priorSettings);
        }
      
        private void ChangeString0ColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String0Color_N_Identifier));
            }
            else
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String0Color_CB_Identifier));
            }

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                String0Color.BackColor = colorDialog.Color;
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
                {
                    SaveChanges(ReadSettings.String0Color_N_Identifier, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
                else
                {
                    SaveChanges(ReadSettings.String0Color_CB_Identifier, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
            }
        }

        private void String1ColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String1Color_N_Identifier));
            }
            else
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String1Color_CB_Identifier));
            }

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                String1Color.BackColor = colorDialog.Color;
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
                {
                    SaveChanges(ReadSettings.String1Color_N_Identifier, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
                else
                {
                    SaveChanges(ReadSettings.String1Color_CB_Identifier, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
            }
        }

        private void String2ColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String2Color_N_Identifier));
            }
            else
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String2Color_CB_Identifier));
            }

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                String2Color.BackColor = colorDialog.Color;
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
                {
                    SaveChanges(ReadSettings.String2Color_N_Identifier, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
                else
                {
                    SaveChanges(ReadSettings.String2Color_CB_Identifier, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
            }
        }

        private void String3ColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String3Color_N_Identifier));
            }
            else
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String3Color_CB_Identifier));
            }

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                String3Color.BackColor = colorDialog.Color;
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
                {
                    SaveChanges(ReadSettings.String3Color_N_Identifier, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
                else
                {
                    SaveChanges(ReadSettings.String3Color_CB_Identifier, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
            }
        }

        private void String4ColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String4Color_N_Identifier));
            }
            else
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String4Color_CB_Identifier));
            }

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                String4Color.BackColor = colorDialog.Color;
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
                {
                    SaveChanges(ReadSettings.String4Color_N_Identifier, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
                else
                {
                    SaveChanges(ReadSettings.String4Color_CB_Identifier, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
            }
        }

        private void String5ColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String5Color_N_Identifier));
            }
            else
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String5Color_CB_Identifier));
            }

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                String5Color.BackColor = colorDialog.Color;
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
                {
                    SaveChanges(ReadSettings.String5Color_N_Identifier, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
                else
                {
                    SaveChanges(ReadSettings.String5Color_CB_Identifier, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
            }
        }

        private void DefaultStringColorsRadio_CheckedChanged(object sender, EventArgs e)
        {
            String0Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String0Color_N_Identifier));
            String1Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String1Color_N_Identifier));
            String2Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String2Color_N_Identifier));
            String3Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String3Color_N_Identifier));
            String4Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String4Color_N_Identifier));
            String5Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String5Color_N_Identifier));
        }

        private void ColorBlindStringColorsRadio_CheckedChanged(object sender, EventArgs e)
        {
            String0Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String0Color_CB_Identifier));
            String1Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String1Color_CB_Identifier));
            String2Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String2Color_CB_Identifier));
            String3Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String3Color_CB_Identifier));
            String4Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String4Color_CB_Identifier));
            String5Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.String5Color_CB_Identifier));
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
                MessageBox.Show("Error: " + ioex.ToString());
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
            int currentTuningIndex = 69000; // Cos we can ;)
            try
            {
                string currentUIName, newUIName, csvContents = File.ReadAllText(Constants.LocalizationCSV_CustomPath);

                using (StreamWriter sw = new StreamWriter(Constants.LocalizationCSV_CustomPath, true))
                {
                    foreach (var tuningDefinition in tuningsCollection)
                    {
                        currentUIName = tuningDefinition.Value.UIName;
                        var tuning = SplitTuningUIName(currentUIName);
                        string index = tuning.Item1;

                        if (index == "0") // I.e. if it does not contain an index, give it one
                            tuningDefinition.Value.UIName = String.Format("$[{0}]{1}", currentTuningIndex++, currentUIName); // Append its index in front

                        if (!csvContents.Contains(index)) // If the CSV already contains that index, don't add it to it
                        {
                            MessageBox.Show(tuningDefinition.Value.UIName); //TODO: check which index is actually added
                            /* sw.Write(currentTuningIndex);
                             for (int i = 0; i < 7; i++) //TODO: maybe add some actual translations for standard/drop tunings
                             {
                                 sw.Write(',');
                                 sw.Write(currentUIName);
                             }

                             sw.Write(sw.NewLine);*/
                        }
                    }
                }
            }
            catch (IOException ioex)
            {
                MessageBox.Show("Error: " + ioex.Message.ToString());
            }

            SaveTuningsJSON();
        }

        private void FillUI()
        {
            listTunings.Items.Clear();
            tuningsCollection = LoadTuningsCollection();

            listTunings.Items.Add("<New>");
            foreach (var key in tuningsCollection.Keys)
                listTunings.Items.Add(key);
        }

        private TuningDefinitionInfo GetCurrentTuningInfo()
        {
            var tuningDefinition = new TuningDefinitionInfo();
            var strings = new Dictionary<string, int>();

            for (int strIdx = 0; strIdx < 6; strIdx++)
                strings[$"string{strIdx}"] = (int)((NumericUpDown)Controls["groupSetAndForget"].Controls[$"nupString{strIdx}"]).Value;

            tuningDefinition.Strings = strings;
            tuningDefinition.UIName = String.Format("$[{0}]{1}", nupTuningIndex.Value.ToString(), txtUIName.Text);

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

        private void btnRestoreDefaults_Click(object sender, EventArgs e)
        {
            try
            {
                if (File.Exists(Constants.CacheBackupPath))
                    File.Copy(Constants.CacheBackupPath, Constants.CachePsarcPath, true);
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

        private void btnUnpackCacheAgain_Click(object sender, EventArgs e)
        {
            if (!Directory.Exists(Constants.CachePcPath))
                return;

            ZipUtilities.DeleteDirectory(Constants.CachePcPath, true);

            UnpackCachePsarc();
        }

        private void btnAddCustomTunings_Click(object sender, EventArgs e)
        {
            if (!Directory.Exists(Constants.CachePcPath)) // Don't replace existing unpacked cache, in case the user wants to add more mods together
                UnpackCachePsarc();

            AddLocalizationForTuningEntries();

            ZipUtilities.InjectFile(Constants.TuningJSON_CustomPath, Constants.Cache7_7zPath, Constants.TuningsJSON_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);
            ZipUtilities.InjectFile(Constants.LocalizationCSV_CustomPath, Constants.Cache4_7zPath, Constants.LocalizationCSV_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            RepackCachePsarc();
        }

        private void btnAddFastLoadMod_Click(object sender, EventArgs e)
        {
            if (!Directory.Exists(Constants.CachePcPath))
                UnpackCachePsarc();

            ZipUtilities.InjectFile(Constants.IntroGFX_CustomPath, Constants.Cache4_7zPath, Constants.IntroGFX_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            RepackCachePsarc();
        }

        private void LoadDefaultFiles()
        {
            if (!File.Exists(Path.Combine(Constants.TuningJSON_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "tuning.database.json" });

            if (!File.Exists(Path.Combine(Constants.IntroGFX_CustomPath)))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, Assembly.GetExecutingAssembly(), "RSMods.Resources", new string[] { "introsequence.gfx" });

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

        private void listTunings_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (listTunings.SelectedItem == null)
                return;

            string selectedItem = listTunings.SelectedItem.ToString();

            if (selectedItem == "<New>")
            {
                txtInternalName.Text = "";
                nupTuningIndex.Value = 0;
                txtUIName.Text = "";
                return;
            }

            var selectedTuning = tuningsCollection[selectedItem];
            var uiName = SplitTuningUIName(selectedTuning.UIName);

            txtInternalName.Text = selectedItem;
            nupTuningIndex.Value = Convert.ToInt32(uiName.Item1);
            txtUIName.Text = uiName.Item2;

            for (int strIdx = 0; strIdx < 6; strIdx++) // If you are lazy and don't want to list each string separately, just do this sexy two-liner
                ((NumericUpDown)Controls["groupSetAndForget"].Controls[$"nupString{strIdx}"]).Value = selectedTuning.Strings[$"string{strIdx}"];
        }

        private void btnSaveTuningChanges_Click(object sender, EventArgs e)
        {
            if (listTunings.SelectedItem != null) // If we are saving a change to the currently selected tuning, perform a change in the collection, otherwise directly go to saving
            {
                string selectedItem = listTunings.SelectedItem.ToString();

                if (selectedItem != "<New>")
                    tuningsCollection[selectedItem] = GetCurrentTuningInfo();
            }

            SaveTuningsJSON();
        }

        private void btnRemoveTuning_Click(object sender, EventArgs e)
        {
            if (listTunings.SelectedItem == null)
                return;

            string selectedItem = listTunings.SelectedItem.ToString();

            if (selectedItem == "<New>")
                return;

            tuningsCollection.Remove(selectedItem); // I guess we would be better here using BindingSource on Listbox + ObservableCollection instead of Dict to make changes reflect automatically, but... one day
            listTunings.Items.Remove(selectedItem);
        }

        private void btnAddTuning_Click(object sender, EventArgs e)
        {
            if (listTunings.SelectedItem.ToString() != "<New>")
                return;

            var currTuning = GetCurrentTuningInfo();
            string internalName = txtInternalName.Text;

            if (!tuningsCollection.ContainsKey(internalName)) // Unlikely to happen, but still... prevent users accidentaly trying to add existing stuff
            {
                tuningsCollection.Add(internalName, currTuning);
                listTunings.Items.Add(internalName);
            }
        }

        private void btnAddCustomMenu_Click(object sender, EventArgs e)
        {
            if (!Directory.Exists(Constants.CachePcPath))
                UnpackCachePsarc();

            ZipUtilities.InjectFile(Constants.ExtendedMenuJson_CustomPath, Constants.Cache7_7zPath, Constants.ExtendedMenuJson_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);
            ZipUtilities.InjectFile(Constants.MainMenuJson_CustomPath, Constants.Cache7_7zPath, Constants.MainMenuJson_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            RepackCachePsarc();
        }



        private void Save_Songlists_Keybindings(object sender, EventArgs e) // Save Songlists and Keybindings when pressing Enter
        {
            // Songlists
            {
                if (this.Songlist.GetSelected(0) & (this.NewSongListNameTxtbox.Text != ReadSettings.ProcessSettings(ReadSettings.Songlist1Identifier)) & (this.NewSongListNameTxtbox.Text != "")) // Songlist 1
                {
                    SaveChanges(ReadSettings.Songlist1Identifier, this.NewSongListNameTxtbox.Text);
                    this.Songlist.Items[0] = this.NewSongListNameTxtbox.Text;
                }
                if (this.Songlist.GetSelected(1) & (this.NewSongListNameTxtbox.Text != ReadSettings.ProcessSettings(ReadSettings.Songlist2Identifier)) & (this.NewSongListNameTxtbox.Text != "")) // Songlist 2
                {
                    SaveChanges(ReadSettings.Songlist2Identifier, this.NewSongListNameTxtbox.Text);
                    this.Songlist.Items[1] = this.NewSongListNameTxtbox.Text;
                }
                if (this.Songlist.GetSelected(2) & (this.NewSongListNameTxtbox.Text != ReadSettings.ProcessSettings(ReadSettings.Songlist3Identifier)) & (this.NewSongListNameTxtbox.Text != "")) // Songlist 3
                {
                    SaveChanges(ReadSettings.Songlist3Identifier, this.NewSongListNameTxtbox.Text);
                    this.Songlist.Items[2] = this.NewSongListNameTxtbox.Text;
                }
                if (this.Songlist.GetSelected(3) & (this.NewSongListNameTxtbox.Text != ReadSettings.ProcessSettings(ReadSettings.Songlist4Identifier)) & (this.NewSongListNameTxtbox.Text != "")) // Songlist 4
                {
                    SaveChanges(ReadSettings.Songlist4Identifier, this.NewSongListNameTxtbox.Text);
                    this.Songlist.Items[3] = this.NewSongListNameTxtbox.Text;
                }
                if (this.Songlist.GetSelected(4) & (this.NewSongListNameTxtbox.Text != ReadSettings.ProcessSettings(ReadSettings.Songlist5Identifier)) & (this.NewSongListNameTxtbox.Text != "")) // Songlist 5
                {
                    SaveChanges(ReadSettings.Songlist5Identifier, this.NewSongListNameTxtbox.Text);
                    this.Songlist.Items[4] = this.NewSongListNameTxtbox.Text;
                }
                if (this.Songlist.GetSelected(5) & (this.NewSongListNameTxtbox.Text != ReadSettings.ProcessSettings(ReadSettings.Songlist6Identifier)) & (this.NewSongListNameTxtbox.Text != "")) // Songlist 6
                {
                    SaveChanges(ReadSettings.Songlist6Identifier, this.NewSongListNameTxtbox.Text);
                    this.Songlist.Items[5] = this.NewSongListNameTxtbox.Text;
                }
            }

            // Mods on KeyPress
            {
                if (this.ModList.GetSelected(0) & (this.NewAssignmentTxtBox.Text != ReadSettings.ProcessSettings(ReadSettings.ToggleLoftIdentifier)) & (this.NewAssignmentTxtBox.Text != "")) // Toggle Loft Key
                {
                    SaveChanges(ReadSettings.ToggleLoftIdentifier, KeyConversion.VirtualKey(this.NewAssignmentTxtBox.Text));
                }
                if (this.ModList.GetSelected(1) & (this.NewAssignmentTxtBox.Text != ReadSettings.ProcessSettings(ReadSettings.AddVolumeIdentifier)) & (this.NewAssignmentTxtBox.Text != "")) // Add Volume Key
                {
                    SaveChanges(ReadSettings.AddVolumeIdentifier, KeyConversion.VirtualKey(this.NewAssignmentTxtBox.Text));
                }
                if (this.ModList.GetSelected(2) & (this.NewAssignmentTxtBox.Text != ReadSettings.ProcessSettings(ReadSettings.DecreaseVolumeIdentifier)) & (this.NewAssignmentTxtBox.Text != "")) // Decrease Volume Key
                {
                    SaveChanges(ReadSettings.DecreaseVolumeIdentifier, KeyConversion.VirtualKey(this.NewAssignmentTxtBox.Text));
                }
                if (this.ModList.GetSelected(3) & (this.NewAssignmentTxtBox.Text != ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerIdentifier)) & (this.NewAssignmentTxtBox.Text != "")) // Show Song Timer Key
                {
                    SaveChanges(ReadSettings.ShowSongTimerIdentifier, KeyConversion.VirtualKey(this.NewAssignmentTxtBox.Text));
                }
                if (this.ModList.GetSelected(4) & (this.NewAssignmentTxtBox.Text != ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationIdentifier)) & (this.NewAssignmentTxtBox.Text != "")) // Force ReEnumeration Key
                {
                    SaveChanges(ReadSettings.ForceReEnumerationIdentifier, KeyConversion.VirtualKey(this.NewAssignmentTxtBox.Text));
                }
                if (this.ModList.GetSelected(5) & (this.NewAssignmentTxtBox.Text != ReadSettings.ProcessSettings(ReadSettings.RainbowStringsIdentifier)) & (this.NewAssignmentTxtBox.Text != "")) // Rainbow Strings Key
                {
                    SaveChanges(ReadSettings.RainbowStringsIdentifier, KeyConversion.VirtualKey(this.NewAssignmentTxtBox.Text));
                }
                this.NewAssignmentTxtBox.Text = "";
            }
            ResetModKeyValues();
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
            }
            else
            {
                this.ExtendedRangeTuningBox.Visible = false;
                this.ExtendedRangeTunings.Visible = false;
                SaveChanges(ReadSettings.ExtendedRangeEnabledIdentifier, "false");
            }
        }

        private void customColorsCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (customColorsCheckbox.Checked)
            {
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "2");
                this.customColorsCheckbox.Checked = true;
                this.ChangeStringColorsBox.Visible = true;
            }
            else
            {
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "0");
                this.customColorsCheckbox.Checked = false;
                this.ChangeStringColorsBox.Visible = false;
            }
        }

        private void DiscoModeCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (DiscoModeCheckbox.Checked)
                SaveChanges(ReadSettings.DiscoModeIdentifier, "true");
            else
                SaveChanges(ReadSettings.DiscoModeIdentifier, "false");
        }

        private void HeadstockCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (HeadstockCheckbox.Checked)
                SaveChanges(ReadSettings.RemoveHeadstockIdentifier, "true");
            else
                SaveChanges(ReadSettings.RemoveHeadstockIdentifier, "false");
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

        private void ExtendedRangeTunings_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.ExtendedRangeTunings.GetSelected(0))
            {
                SaveChanges(ReadSettings.ExtendedRangeTuningIdentifier, "-2");
            }
            else if (this.ExtendedRangeTunings.GetSelected(1))
            {
                SaveChanges(ReadSettings.ExtendedRangeTuningIdentifier, "-3");
            }
            else if (this.ExtendedRangeTunings.GetSelected(2))
            {
                SaveChanges(ReadSettings.ExtendedRangeTuningIdentifier, "-4");
            }
            else if (this.ExtendedRangeTunings.GetSelected(3))
            {
                SaveChanges(ReadSettings.ExtendedRangeTuningIdentifier, "-5");
            }
            else if (this.ExtendedRangeTunings.GetSelected(4))
            {
                SaveChanges(ReadSettings.ExtendedRangeTuningIdentifier, "-6");
            }
            else if (this.ExtendedRangeTunings.GetSelected(5))
            {
                SaveChanges(ReadSettings.ExtendedRangeTuningIdentifier, "-7");
            }
            else if (this.ExtendedRangeTunings.GetSelected(6))
            {
                SaveChanges(ReadSettings.ExtendedRangeTuningIdentifier, "-8");
            }
            else if (this.ExtendedRangeTunings.GetSelected(7))
            {
                SaveChanges(ReadSettings.ExtendedRangeTuningIdentifier, "-9");
            }
            else if (this.ExtendedRangeTunings.GetSelected(8))
            {
                SaveChanges(ReadSettings.ExtendedRangeTuningIdentifier, "-10");
            }
            else if (this.ExtendedRangeTunings.GetSelected(9))
            {
                SaveChanges(ReadSettings.ExtendedRangeTuningIdentifier, "-11");
            }
            else if (this.ExtendedRangeTunings.GetSelected(10))
            {
                SaveChanges(ReadSettings.ExtendedRangeTuningIdentifier, "-12");
            }
        }

        private void DeleteKeyBind_Click(object sender, EventArgs e)
        {
            this.NewAssignmentTxtBox.Text = "";
            if (this.ModList.GetSelected(0)) // Toggle Loft Key
                SaveChanges(ReadSettings.ToggleLoftIdentifier, "");
            else if (this.ModList.GetSelected(1)) // Add Volume Key
                SaveChanges(ReadSettings.AddVolumeIdentifier, "");
            else if (this.ModList.GetSelected(2)) // Decrease Volume Key
                SaveChanges(ReadSettings.DecreaseVolumeIdentifier, "");
            else if (this.ModList.GetSelected(3)) // Show Song Timer Key
                SaveChanges(ReadSettings.ShowSongTimerIdentifier, "");
            else if (this.ModList.GetSelected(4)) // Force ReEnumerate Key
                SaveChanges(ReadSettings.ForceReEnumerationIdentifier, "");
            else if (this.ModList.GetSelected(5)) // Rainbow Strings Key
                SaveChanges(ReadSettings.RainbowStringsIdentifier, "");

            ResetModKeyValues();
        }

        private void ResetModKeyValues()
        {
            this.ToggleLoftKey.Text = "Toggle Loft: " + ReadSettings.ProcessSettings(ReadSettings.ToggleLoftIdentifier);
            this.AddVolumeKey.Text = "Add Volume: " + ReadSettings.ProcessSettings(ReadSettings.AddVolumeIdentifier);
            this.DecreaseVolumeKey.Text = "Decrease Volume: " + ReadSettings.ProcessSettings(ReadSettings.DecreaseVolumeIdentifier);
            this.SongTimerKey.Text = "Show Song Timer: " + ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerIdentifier);
            this.ReEnumerationKey.Text = "Force ReEnumeration: " + ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationIdentifier);
            this.RainbowStringsAssignment.Text = "Rainbow Strings: " + ReadSettings.ProcessSettings(ReadSettings.RainbowStringsIdentifier);
        }

        private void RemoveLyricsCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (RemoveLyricsCheckbox.Checked)
                SaveChanges(ReadSettings.RemoveLyricsIdentifier, "on");
            else
                SaveChanges(ReadSettings.RemoveLyricsIdentifier, "off");
        }

       

        private void MainForm_Load(object sender, System.EventArgs e)
        {
            StartToolTips();
        }
        private void StartToolTips()  // Documentation: https://docs.microsoft.com/en-us/dotnet/api/system.windows.forms.tooltip.settooltip?view=netcore-3.1
        {
            // Create the ToolTip and associate with the Form container.
            ToolTip toolTip1 = new ToolTip();

            // Set up the delays for the ToolTip.
            toolTip1.AutoPopDelay = 5000;
            toolTip1.InitialDelay = 1000;
            toolTip1.ReshowDelay = 500;
            // Force the ToolTip text to be displayed whether or not the form is active.
            toolTip1.ShowAlways = true;

            // Set up the ToolTip text for the Button and Checkbox.
            toolTip1.SetToolTip(this.ToggleLoftCheckbox, "Disables the game background, amps and noise reactive speaker rings. Best used with Venue Mode off (setting in game). Used by a lot of Rocksmith Streamers to make it easy to Luma Key out the game background. Player just sees an all black background when this is enabled. Options for turning the loft off only when in a song, when the game first starts up, or on a key press.");
            toolTip1.SetToolTip(this.AddVolumeCheckbox, "Experimental");
            toolTip1.SetToolTip(this.RainbowStringsEnabled, "How Pro are you? This makes the players guitar strings constantly cycling through colors.");
            toolTip1.SetToolTip(this.HeadstockCheckbox, "Stops the Headstock of the guitar being drawn. “Headless” guitar mode. Just cleans up some more of the UI.");
            toolTip1.SetToolTip(this.GreenScreenWallCheckbox, "Changes just a section of the game background to all black, amusing for a selective “green screen” stream experience. Invalidated by “No Loft”.");
            toolTip1.SetToolTip(this.ForceEnumerationCheckbox, "Game will automatically start an Enumeration sequence when a new psarc (CDLC) file is detected as having been added to the dlc folder. Not necesary to enable if you're already using Rocksniffer to do the same thing.");
            toolTip1.SetToolTip(this.FretlessModeCheckbox, "Removes the Fret Wire from the neck, making your instrument appear to be Fretless.");
            toolTip1.SetToolTip(this.SongTimerCheckbox, "Experimental - Intent is to show a box with your timestamp position through the song.");
            toolTip1.SetToolTip(this.DecreaseVolumeCheckbox, "Experimental");
            toolTip1.SetToolTip(this.ExtendedRangeEnabled, "Alters the string and note colors to make it easier to play a 5 string bass or 7 string guitar.");
            toolTip1.SetToolTip(this.customColorsCheckbox, "Lets you define the string / note colors you want. Saves a normal set and a Colorblind mode set.");
            toolTip1.SetToolTip(this.RemoveSkylineCheckbox, "Removes the purple and orange bars from the top of the display in LAS. Use in conjunction with No Loft for a cleaner UI. Options for always off,only off when in a song, or only when toggled by key press.");
            toolTip1.SetToolTip(this.DiscoModeCheckbox, "Experimental.");
            toolTip1.SetToolTip(this.AutoLoadProfileCheckbox, "Essentially holds down the ENTER key until the game has reached the main menu. Lets you auto load the last used profile without needing to interact with the game at all.");
            toolTip1.SetToolTip(this.RemoveInlaysCheckbox, "Disables the guitar neck inlay display entirely.");
            toolTip1.SetToolTip(this.RemoveLineMarkersCheckBox, "Removes the additional lane marker lines seen in the display. When used with No Loft, provides a cleaner Luma Key.");
            toolTip1.SetToolTip(this.RemoveLyricsCheckbox, "Disables the display of song lyrics while in Learn-A-Song mode.");
            toolTip1.SetToolTip(this.SongListBox, "Custom names for the 6 “SONG LISTS” shown in game.");
            toolTip1.SetToolTip(this.KeybindingsBox, "Set key binds for the toggle on / off by keypress modifications. You need to press ENTER after setting teh key for it to be saved");
            toolTip1.SetToolTip(this.btnAddCustomTunings, "Adds named definitions for some common Custom Tunings.");
            toolTip1.SetToolTip(this.btnAddFastLoadMod, "SSD drive or faster or may cause the game to not launch properly, skips some of the intro sequences. Combined with Auto Load Last Profile and huzzah!");
            toolTip1.SetToolTip(this.btnAddCustomMenu, "Adds the Direct Connect mode - microphone mode with tone simulations. Also replaces UPLAY on the main menu with an EXIT GAME option.");
            toolTip1.SetToolTip(this.listProfileTones, "This section lets you change the default menu tone for Lead, Rhythm Or Bass. You need to have the tone you want to set saved in your profile first, then you can load it here and set it as the default tone that will be used when you start up Rocksmith.");
            toolTip1.SetToolTip(this.ResetToDefaultButton, "Resets all RSMods values to defaults."); // button beside tone selection
            toolTip1.SetToolTip(this.btnRestoreDefaults, "Restores the original cache.psarc file - undoes all 'set and forget' mods."); //reset button in "set and forget" section "restore cache backup"
            toolTip1.SetToolTip(this.btnLoadTonesFromProfiles, "Step 1, click this to load the tones that are saved in your profile.");
            toolTip1.SetToolTip(this.listProfileTones, "Step2, highlight a tone name.");
            toolTip1.SetToolTip(this.rbTone0, "Choose Lead, Rhythm or Bass to assign the highlighted tone to.");
            toolTip1.SetToolTip(this.rbTone1, "Choose Lead, Rhythm or Bass to assign the highlighted tone to.");
            toolTip1.SetToolTip(this.rbTone2, "Choose Lead, Rhythm or Bass to assign the highlighted tone to.");
            toolTip1.SetToolTip(this.btnSetDefaultTones, "Assign the currently highlighted tone to the chosen path.");
        }

        private void Songlist_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.Songlist.SelectedIndex > -1)
            {
                this.NewSongListNameTxtbox.Text = this.Songlist.SelectedItem.ToString();
            }
        }

        private void btnRemoveTempFolders_Click(object sender, EventArgs e)
        {
            ZipUtilities.DeleteDirectory(Constants.WorkFolder);
            ZipUtilities.DeleteDirectory(Constants.CustomModsFolder);
        }

        private static Dictionary<string, Tone2014> TonesFromAllProfiles = new Dictionary<string, Tone2014>();

        /* private class IniEntry
         {
             public static string Section { get; set; }
             public static string 
         }*/

        /*private TuningDefinitionList LoadTuningsCollection()
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
                MessageBox.Show("Error: " + ioex.ToString());
            }
        }*/

        private void btnSetDefaultTones_Click(object sender, EventArgs e)
        {
            if (listProfileTones.SelectedItem == null)
                return;

            ZipUtilities.ExtractSingleFile(Constants.CustomModsFolder, Constants.Cache7_7zPath, Constants.ToneManager_InternalPath);

            string toneManagerFileContent = File.ReadAllText(Constants.ToneManager_CustomPath);
            var tonesJson = JObject.Parse(toneManagerFileContent);
            var toneList = tonesJson["Static"]["ToneManager"]["Tones"];
            var defaultTones = JsonConvert.DeserializeObject<List<Tone2014>>(toneList.ToString());

            var selectedTone = TonesFromAllProfiles[listProfileTones.SelectedItem.ToString()];
            if (rbTone0.Checked)
                tonesJson["Static"]["ToneManager"]["Tones"][0]["GearList"] = JObject.FromObject(selectedTone.GearList);
            else if(rbTone1.Checked)
                tonesJson["Static"]["ToneManager"]["Tones"][1]["GearList"] = JObject.FromObject(selectedTone.GearList);
            else if(rbTone2.Checked)
                tonesJson["Static"]["ToneManager"]["Tones"][2]["GearList"] = JObject.FromObject(selectedTone.GearList);

            try
            {
                File.WriteAllText(Constants.ToneManager_CustomPath, tonesJson.ToString());
            }
            catch (IOException ioex)
            {
                MessageBox.Show("Error: " + ioex.ToString());
                return;
            }

            ZipUtilities.InjectFile(Constants.ToneManager_CustomPath, Constants.Cache7_7zPath, Constants.ToneManager_InternalPath, OutArchiveFormat.SevenZip, CompressionMode.Append);

            MessageBox.Show("Successfully changed default tones!", "Success");
        }

        private void btnLoadTonesFromProfiles_Click(object sender, EventArgs e)
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
                    listProfileTones.Items.Clear();

                    foreach (string profile in profiles)
                        foreach (var tone in Tone2014.Import(profile))
                        {
                            listProfileTones.Items.Add(tone.Name);
                            TonesFromAllProfiles.Add(tone.Name, tone);
                        }
                }
                else
                    MessageBox.Show("Could not find profile folder!", "Error");
            }
            catch (IOException ioex)
            {
                MessageBox.Show("Could not find Steam profiles folder: " + ioex.Message.ToString(), "Error");
            }
        }
    }
}