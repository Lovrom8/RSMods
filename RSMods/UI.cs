using System;
using System.Windows.Forms;
using System.Runtime;
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

namespace RSMods
{
    public partial class MainForm : Form
    {
        public MainForm()
        {
            WriteSettings.WhereIsRocksmith(); // Creates GUI Settings File
            if (!File.Exists(@WriteSettings.dumpLocation))
            {
                WriteSettings.NoSettingsDetected(); // Creates Settings File
            }
            InitializeComponent();

            // Fill Songlist List
            this.Songlist.Items.AddRange(new object[] {
                    ImportPriorSettings()[0], // Song List 1
                    ImportPriorSettings()[1], // Song List 2
                    ImportPriorSettings()[2], // Song List 3
                    ImportPriorSettings()[3], // Song List 4
                    ImportPriorSettings()[4], // Song List 5
                    ImportPriorSettings()[5]  // Song List 6 
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
            {
                this.ToggleLoftKey.Text = "Toggle Loft: " + ImportPriorSettings()[6];
                this.AddVolumeKey.Text = "Add Volume: " + ImportPriorSettings()[7];
                this.DecreaseVolumeKey.Text = "Decrease Volume: " + ImportPriorSettings()[8];
                this.SongTimerKey.Text = "Show Song Timer: " + ImportPriorSettings()[9];
                this.ReEnumerationKey.Text = "Force ReEnumeration: " + ImportPriorSettings()[10];
                this.RainbowStringsAssignment.Text = "Rainbow Strings: " + ImportPriorSettings()[11];
            }


            // Load Checkbox Values
            {
                if (ImportPriorSettings()[12].ToString() == "on") // Toggle Loft Enabled / Disabled
                {
                    this.ToggleLoftCheckbox.Checked = true;
                    this.ToggleLoftWhenStartupRadio.Visible = true;
                    this.ToggleLoftWhenManualRadio.Visible = true;
                    this.ToggleLoftWhenSongRadio.Visible = true;
                    this.ToggleLoftOffWhenBox.Visible = true;

                    if (ImportPriorSettings()[27].ToString() == "startup")
                    {
                        this.ToggleLoftWhenStartupRadio.Checked = true;
                    }
                    if (ImportPriorSettings()[27].ToString() == "manual")
                    {
                        this.ToggleLoftWhenManualRadio.Checked = true;
                    }
                    if (ImportPriorSettings()[27].ToString() == "song")
                    {
                        this.ToggleLoftWhenSongRadio.Checked = true;
                    }
                }
                else
                {
                    this.ToggleLoftCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[13].ToString() == "on") // Add Volume Enabled / Disabled
                {
                    this.AddVolumeCheckbox.Checked = true;
                }
                else
                {
                    this.AddVolumeCheckbox.Checked = false;
                }
                if (ImportPriorSettings()[14].ToString() == "on") // Decrease Volume Enabled / Disabled
                {
                    this.DecreaseVolumeCheckbox.Checked = true;
                }
                else
                {
                    this.DecreaseVolumeCheckbox.Checked = false;
                }
                if (ImportPriorSettings()[15].ToString() == "on") // Show Song Timer Enabled / Disabled
                {
                    this.SongTimerCheckbox.Checked = true;
                }
                else
                {
                    this.SongTimerCheckbox.Checked = false;
                }
                if (ImportPriorSettings()[16].ToString() != "false") // Force Enumeration Settings
                {
                    this.ForceEnumerationCheckbox.Checked = true;
                    this.ForceEnumerationAutomaticRadio.Visible = true;
                    this.ForceEnumerationManualRadio.Visible = true;
                    this.HowToEnumerateBox.Visible = true;
                    if (ImportPriorSettings()[16].ToString() == "automatic")
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
                if (ImportPriorSettings()[17].ToString() == "on") // Rainbow String Enabled / Disabled
                {
                    this.RainbowStringsEnabled.Checked = true;
                }
                else
                {
                    this.RainbowStringsEnabled.Checked = false;
                }
                if (ImportPriorSettings()[18].ToString() == "on") // Extended Range Enabled / Disabled
                {
                    this.ExtendedRangeEnabled.Checked = true;
                    this.ExtendedRangeTuningBox.Visible = true;
                    this.ExtendedRangeTunings.Visible = true;
                }
                else
                {
                    this.ExtendedRangeEnabled.Checked = false;
                }
                if (ImportPriorSettings()[19].ToString() != "0") // Custom String Colors
                {
                    this.customColorsCheckbox.Checked = true;
                    this.ChangeStringColorsBox.Visible = true;
                }

                if (ImportPriorSettings()[20].ToString() == "on") // Disco Mode Enabled / Disabled
                {
                    this.DiscoModeCheckbox.Checked = true;
                }
                else
                {
                    this.DiscoModeCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[21].ToString() == "on") // Remove Headstock Enabled / Disabled
                {
                    this.HeadstockCheckbox.Checked = true;
                }
                else
                {
                    this.HeadstockCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[22].ToString() == "on") // Remove Skyline Enabled / Disabled
                {
                    this.RemoveSkylineCheckbox.Checked = true;
                    this.ToggleSkylineBox.Visible = true;
                }
                else
                {
                    this.RemoveSkylineCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[23].ToString() == "on") // Greenscreen Wall Enabled / Disabled
                {
                    this.GreenScreenWallCheckbox.Checked = true;
                }
                else
                {
                    this.GreenScreenWallCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[24].ToString() == "on") // Force Load Profile On Game Boot Enabled / Disabled
                {
                    this.AutoLoadProfileCheckbox.Checked = true;
                }
                else
                {
                    this.AutoLoadProfileCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[25].ToString() == "on") // Fretless Mode Enabled / Disabled
                {
                    this.FretlessModeCheckbox.Checked = true;
                }
                else
                {
                    this.FretlessModeCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[26].ToString() == "on") // Remove Inlay Markers Enabled / Disabled
                {
                    this.RemoveInlaysCheckbox.Checked = true;
                }
                else
                {
                    this.RemoveInlaysCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[28].ToString() == "on") // Remove Line Markers Enabled / Disabled
                {
                    this.RemoveLineMarkersCheckBox.Checked = true;
                }
                else
                {
                    this.RemoveLineMarkersCheckBox.Checked = false;
                }
                if (ImportPriorSettings()[29].ToString() == "song") // Remove Skyline on Song Load
                {
                    this.ToggleSkylineSongRadio.Checked = true;
                }
                else if (ImportPriorSettings()[29].ToString() == "startup") // Remove Skyline on Game Startup 
                {
                    this.ToggleSkylineStartupRadio.Checked = true;
                }
            }

            // Initialize Default String Colors
            {
                String0Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[30].ToString());
                String1Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[31].ToString());
                String2Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[32].ToString());
                String3Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[33].ToString());
                String4Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[34].ToString());
                String5Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[35].ToString());
            }

            // Mod Settings
            {

                this.EnumerateEveryXMS.Value = Decimal.Parse(ImportPriorSettings()[43].ToString()) / 1000; // Loads old settings for enumeration every x ms
                {
                    if (ImportPriorSettings()[42].ToString() == "-2")
                    {
                        this.ExtendedRangeTunings.SelectedIndex = 0;
                    }
                    if (ImportPriorSettings()[42].ToString() == "-3")
                    {
                        this.ExtendedRangeTunings.SelectedIndex = 1;
                    }
                    if (ImportPriorSettings()[42].ToString() == "-4")
                    {
                        this.ExtendedRangeTunings.SelectedIndex = 2;
                    }
                    if (ImportPriorSettings()[42].ToString() == "-5")
                    {
                        this.ExtendedRangeTunings.SelectedIndex = 3;
                    }
                    if (ImportPriorSettings()[42].ToString() == "-6")
                    {
                        this.ExtendedRangeTunings.SelectedIndex = 4;
                    }
                    if (ImportPriorSettings()[42].ToString() == "-7")
                    {
                        this.ExtendedRangeTunings.SelectedIndex = 5;
                    }
                    if (ImportPriorSettings()[42].ToString() == "-8")
                    {
                        this.ExtendedRangeTunings.SelectedIndex = 6;
                    }
                    if (ImportPriorSettings()[42].ToString() == "-9")
                    {
                        this.ExtendedRangeTunings.SelectedIndex = 7;
                    }
                    if (ImportPriorSettings()[42].ToString() == "-10")
                    {
                        this.ExtendedRangeTunings.SelectedIndex = 8;
                    }
                    if (ImportPriorSettings()[42].ToString() == "-11")
                    {
                        this.ExtendedRangeTunings.SelectedIndex = 9;
                    }
                    if (ImportPriorSettings()[42].ToString() == "-12")
                    {
                        this.ExtendedRangeTunings.SelectedIndex = 10;
                    }

                }

            }

            LoadSetAndForgetMods();
        }

        private void ModList_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.ModList.GetSelected(0))
            {
                this.NewAssignmentTxtBox.Text = ReadSettings.ProcessSettings(6);
            }
            if (this.ModList.GetSelected(1))
            {
                this.NewAssignmentTxtBox.Text = ReadSettings.ProcessSettings(7);
            }
            if (this.ModList.GetSelected(2))
            {
                this.NewAssignmentTxtBox.Text = ReadSettings.ProcessSettings(8);
            }
            if (this.ModList.GetSelected(3))
            {
                this.NewAssignmentTxtBox.Text = ReadSettings.ProcessSettings(9);
            }
            if (this.ModList.GetSelected(4))
            {
                this.NewAssignmentTxtBox.Text = ReadSettings.ProcessSettings(10);
            }
            if (this.ModList.GetSelected(5))
            {
                this.NewAssignmentTxtBox.Text = ReadSettings.ProcessSettings(16);
            }
        }

        private void Songlist_SelectedIndexChanged(object sender, EventArgs e)
        {
            this.NewSongListNameTxtbox.Text = Songlist.SelectedItem.ToString();
        }

        private void CheckEnter(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter) // If enter is pressed
            {
                Save_Songlists_Keybindings(sender, e);
                e.SuppressKeyPress = true;
            }
        }

        private void ResetToDefaultSettings(object sender, EventArgs e)
        {
            File.Delete(@WriteSettings.dumpLocation);
            RefreshForm();
        }

       

        private void RefreshForm()
        {
            this.Hide();
            var newForm = new MainForm();
            newForm.Closed += (s, args) => this.Close();
            newForm.Show();
        }

        private void SaveChanges(int ElementToChange, string ChangedSettingValue)
        {
            if (File.ReadLines(@WriteSettings.dumpLocation).Count() == WriteSettings.StringArray.Length)
            {
                string[] StringArray = File.ReadAllLines(WriteSettings.dumpLocation);

                if (ChangedSettingValue == "true")
                {
                    ChangedSettingValue = "on";
                }
                else if (ChangedSettingValue == "false")
                {
                    ChangedSettingValue = "off";
                }
                // Song Lists
                {
                    if (ElementToChange == 1) // Songlist 1
                    {
                        StringArray[1] = ReadSettings.Songlist1Identifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 2) // Songlist 2
                    {
                        StringArray[2] = ReadSettings.Songlist2Identifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 3) // Songlist 3
                    {
                        StringArray[3] = ReadSettings.Songlist3Identifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 4) // Songlist 4
                    {
                        StringArray[4] = ReadSettings.Songlist4Identifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 5) // Songlist 5
                    {
                        StringArray[5] = ReadSettings.Songlist5Identifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 6) // Songlist 6
                    {
                        StringArray[6] = ReadSettings.Songlist6Identifier + ChangedSettingValue;
                    }
                }
                // Toggle On Key Press
                {
                    if (ElementToChange == 7) // Toggle Loft Key
                    {
                        StringArray[8] = ReadSettings.ToggleLoftIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 8) // Add Volume Key
                    {
                        StringArray[9] = ReadSettings.AddVolumeIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 9) // Decrease Volume Key
                    {
                        StringArray[10] = ReadSettings.DecreaseVolumeIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 10) // Show Timer Key
                    {
                        StringArray[11] = ReadSettings.ShowSongTimerIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 11) // Force ReEnumeration Key
                    {
                        StringArray[12] = ReadSettings.ForceReEnumerationIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 12) // Rainbow Strings Key
                    {
                        StringArray[13] = ReadSettings.RainbowStringsIdentifier + ChangedSettingValue;
                    }
                }
                // Toggle Effects
                {
                    if (ElementToChange == 13) // Toggle Loft Enabled / Disabled
                    {
                        StringArray[15] = ReadSettings.ToggleLoftEnabledIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 14) // Add Volume Enabled / Disabled
                    {
                        StringArray[16] = ReadSettings.AddVolumeEnabledIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 15) // Decrease Volume Enabled / Disabled
                    {
                        StringArray[17] = ReadSettings.DecreaseVolumeEnabledIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 16) // Show Song Timer Enabled / Disabled
                    {
                        StringArray[18] = ReadSettings.ShowSongTimerEnabledIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 17) // Force ReEnumeration Manual / Automatic / Disabled
                    {
                        StringArray[19] = ReadSettings.ForceReEnumerationEnabledIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 18) // Rainbow String Enabled / Disabled
                    {
                        StringArray[20] = ReadSettings.RainbowStringsEnabledIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 19) // Extended Range Enabled / Disabled
                    {
                        StringArray[21] = ReadSettings.ExtendedRangeEnabledIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 20) // Custom String Enabled / Disabled
                    {
                        StringArray[22] = ReadSettings.CustomStringColorNumberIndetifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 21) // Disco Mode Enabled / Disabled
                    {
                        StringArray[23] = ReadSettings.DiscoModeIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 22) // Remove Headstock Enabled / Disabled
                    {
                        StringArray[24] = ReadSettings.RemoveHeadstockIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 23) // Remove Skyline Enabled / Disabled
                    {
                        StringArray[25] = ReadSettings.RemoveSkylineIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 24) // Greenscreen Wall Enabled / Disabled
                    {
                        StringArray[26] = ReadSettings.GreenScreenWallIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 25) // Force Load Profile On Game Boot Enabled / Disabled
                    {
                        StringArray[27] = ReadSettings.ForceProfileEnabledIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 26) // Fretless Mode Enabled / Disabled
                    {
                        StringArray[28] = ReadSettings.FretlessModeEnabledIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 27) // Remove Inlay Markers Enabled / Disabled
                    {
                        StringArray[29] = ReadSettings.RemoveInlaysIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 28) // Toggle Loft When Startup / Manual / OnSong
                    {
                        StringArray[30] = ReadSettings.ToggleLoftWhenIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 29) // Remove Lane Markers Enabled / Disabled
                    {
                        StringArray[31] = ReadSettings.RemoveLaneMarkersIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 30) // Toggle Skyline When Startup / OnSong
                    {
                        StringArray[32] = ReadSettings.ToggleSkylineWhenIdentifier + ChangedSettingValue;
                    }
                }
                // String Colors (Normal {N} & Colorblind {CB})
                {
                    // Normal String Colors
                    {
                        if (ElementToChange == 31)
                        {
                            StringArray[34] = ReadSettings.String0Color_N_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 32)
                        {
                            StringArray[35] = ReadSettings.String1Color_N_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 33)
                        {
                            StringArray[36] = ReadSettings.String2Color_N_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 34)
                        {
                            StringArray[37] = ReadSettings.String3Color_N_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 35)
                        {
                            StringArray[38] = ReadSettings.String4Color_N_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 36)
                        {
                            StringArray[39] = ReadSettings.String5Color_N_Identifier + ChangedSettingValue;
                        }
                    }
                    // Colorblind String Colors
                    {
                        if (ElementToChange == 37)
                        {
                            StringArray[40] = ReadSettings.String0Color_CB_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 38)
                        {
                            StringArray[41] = ReadSettings.String1Color_CB_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 39)
                        {
                            StringArray[42] = ReadSettings.String2Color_CB_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 40)
                        {
                            StringArray[43] = ReadSettings.String3Color_CB_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 41)
                        {
                            StringArray[44] = ReadSettings.String4Color_CB_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 42)
                        {
                            StringArray[45] = ReadSettings.String5Color_CB_Identifier + ChangedSettingValue;
                        }
                    }

                }
                // Mod Settings
                {
                    if (ElementToChange == 43) // Extended Range Tuning Name
                    {
                        StringArray[47] = ReadSettings.ExtendedRangeTuningIdentifier + ChangedSettingValue;
                    }
                    if (ElementToChange == 44)
                    {
                        StringArray[48] = ReadSettings.CheckForNewSongIntervalIdentifier + ChangedSettingValue;
                    }

                }

                WriteSettings.ModifyINI(StringArray);
            }
            else
            {
                string tempDumpFile = "dump.ini";
                File.WriteAllText(@tempDumpFile, File.ReadAllText(WriteSettings.dumpLocation));

                WriteSettings.NoSettingsDetected();

                MessageBox.Show("Your INI is out of date. We created a new INI file in the folder of this application.\n" +
                                "Please copy your old settings, now in " + tempDumpFile + " into your " + WriteSettings.dumpLocation + " settings.\n" +
                                "We are extremely sorry for this inconvience", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Environment.Exit(1);
                return;
            }
        }
        public object[] ImportPriorSettings() // To check new settings against old settings
        {
            object[] priorSettings = new object[45];
            priorSettings[0] = ReadSettings.ProcessSettings(0); // Song List 1
            priorSettings[1] = ReadSettings.ProcessSettings(1); // Song List 2
            priorSettings[2] = ReadSettings.ProcessSettings(2); // Song List 3
            priorSettings[3] = ReadSettings.ProcessSettings(3); // Song List 4
            priorSettings[4] = ReadSettings.ProcessSettings(4); // Song List 5
            priorSettings[5] = ReadSettings.ProcessSettings(5); // Song List 6

            priorSettings[6] = ReadSettings.ProcessSettings(6); // Toggle Loft Key
            priorSettings[7] = ReadSettings.ProcessSettings(7); // Add Volume Key
            priorSettings[8] = ReadSettings.ProcessSettings(8); // Decrease Volume Key
            priorSettings[9] = ReadSettings.ProcessSettings(9); // Show Song Timer Key
            priorSettings[10] = ReadSettings.ProcessSettings(10); // Force Enumeration Key
            priorSettings[11] = ReadSettings.ProcessSettings(16); // Rainbow Strings Key

            priorSettings[12] = ReadSettings.ProcessSettings(11); // Toggle Loft Enabled / Disabled
            priorSettings[13] = ReadSettings.ProcessSettings(12); // Add Volume Enabled / Disabled
            priorSettings[14] = ReadSettings.ProcessSettings(13); // Decrease Volume Enabled / Disabled
            priorSettings[15] = ReadSettings.ProcessSettings(14); // Show Song Timer Enabled / Disabled
            priorSettings[16] = ReadSettings.ProcessSettings(15); // Force Enumeration Manual / Automatic / Disabled
            priorSettings[17] = ReadSettings.ProcessSettings(17); // Rainbow Strings Enabled / Disabled
            priorSettings[18] = ReadSettings.ProcessSettings(18); // Extended Range Enabled / Disabled
            priorSettings[19] = ReadSettings.ProcessSettings(43); // Custom String Settings (0 - Default, 1 - ZZ, 2 - Custom)
            priorSettings[20] = ReadSettings.ProcessSettings(20); // Disco Mode Enabled / Disabled
            priorSettings[21] = ReadSettings.ProcessSettings(21); // Remove Headstock Enabled / Disabled
            priorSettings[22] = ReadSettings.ProcessSettings(22); // Remove Skyline Enabled / Disabled
            priorSettings[23] = ReadSettings.ProcessSettings(23); // Greenscreen Wall Enabled / Disabled
            priorSettings[24] = ReadSettings.ProcessSettings(24); // Force Load Profile On Game Boot Enabled / Disabled
            priorSettings[25] = ReadSettings.ProcessSettings(25); // Fretless Mode Enabled / Disabled
            priorSettings[26] = ReadSettings.ProcessSettings(26); // Remove Inlay Markers Enabled / Disabled
            priorSettings[27] = ReadSettings.ProcessSettings(27); // Toggle Loft When Startup / Manual / Song
            priorSettings[28] = ReadSettings.ProcessSettings(28); // Remove Lane Markers Enabled / Disabled
            priorSettings[29] = ReadSettings.ProcessSettings(29); // Toggle Skyline When Startup / Song

            priorSettings[30] = ReadSettings.ProcessSettings(30); // Default Low E String Color
            priorSettings[31] = ReadSettings.ProcessSettings(31); // Default A String Color
            priorSettings[32] = ReadSettings.ProcessSettings(32); // Default D String Color
            priorSettings[33] = ReadSettings.ProcessSettings(33); // Default G String Color
            priorSettings[34] = ReadSettings.ProcessSettings(34); // Default B String Color
            priorSettings[35] = ReadSettings.ProcessSettings(35); // Default High E String Color
            priorSettings[36] = ReadSettings.ProcessSettings(36); // Colorblind Low E String Color
            priorSettings[37] = ReadSettings.ProcessSettings(37); // Colorblind A String Color
            priorSettings[38] = ReadSettings.ProcessSettings(38); // Colorblind D String Color
            priorSettings[39] = ReadSettings.ProcessSettings(39); // Colorblind G String Color
            priorSettings[40] = ReadSettings.ProcessSettings(40); // Colorblind B String Color
            priorSettings[41] = ReadSettings.ProcessSettings(41); // Colorblind High E String Color

            priorSettings[42] = ReadSettings.ProcessSettings(19); // Extended Range At X Tuning
            priorSettings[43] = ReadSettings.ProcessSettings(42); // Check For Songs Every X ms
            return priorSettings;
        }

        private void ChangeString0ColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "on")
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ImportPriorSettings()[30].ToString());
            }
            else
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ImportPriorSettings()[36].ToString());
            }

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                String0Color.BackColor = colorDialog.Color;
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "on")
                {
                    SaveChanges(31, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
                else
                {
                    SaveChanges(37, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
            }
        }

        private void String1ColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "on")
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ImportPriorSettings()[31].ToString());
            }
            else
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ImportPriorSettings()[37].ToString());
            }

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                String1Color.BackColor = colorDialog.Color;
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "on")
                {
                    SaveChanges(32, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
                else
                {
                    SaveChanges(38, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
            }
        }

        private void String2ColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "on")
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ImportPriorSettings()[32].ToString());
            }
            else
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ImportPriorSettings()[38].ToString());
            }

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                String2Color.BackColor = colorDialog.Color;
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "on")
                {
                    SaveChanges(33, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
                else
                {
                    SaveChanges(39, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
            }
        }

        private void String3ColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "on")
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ImportPriorSettings()[33].ToString());
            }
            else
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ImportPriorSettings()[39].ToString());
            }

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                String3Color.BackColor = colorDialog.Color;
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "on")
                {
                    SaveChanges(34, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
                else
                {
                    SaveChanges(40, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
            }
        }

        private void String4ColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "on")
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ImportPriorSettings()[34].ToString());
            }
            else
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ImportPriorSettings()[40].ToString());
            }

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                String4Color.BackColor = colorDialog.Color;
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "on")
                {
                    SaveChanges(35, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
                else
                {
                    SaveChanges(41, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
            }
        }

        private void String5ColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "on")
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ImportPriorSettings()[35].ToString());
            }
            else
            {
                colorDialog.Color = ColorTranslator.FromHtml("#" + ImportPriorSettings()[41].ToString());
            }

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                String5Color.BackColor = colorDialog.Color;
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "on")
                {
                    SaveChanges(36, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
                else
                {
                    SaveChanges(42, (colorDialog.Color.ToArgb() & 0x00FFFFFF).ToString("X6"));
                }
            }
        }

        private void DefaultStringColorsRadio_CheckedChanged(object sender, EventArgs e)
        {
            String0Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[30].ToString());
            String1Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[31].ToString());
            String2Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[32].ToString());
            String3Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[33].ToString());
            String4Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[34].ToString());
            String5Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[35].ToString());
        }

        private void ColorBlindStringColorsRadio_CheckedChanged(object sender, EventArgs e)
        {
            String0Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[36].ToString());
            String1Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[37].ToString());
            String2Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[38].ToString());
            String3Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[39].ToString());
            String4Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[40].ToString());
            String5Color.BackColor = ColorTranslator.FromHtml("#" + ImportPriorSettings()[41].ToString());
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
            Constants.RSFolder = ReadSettings.SavedRocksmithLocation(); //TODO: utilize stuff from GenUtil to make it even more fullproof
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



        private void Save_Songlists_Keybindings(object sender, EventArgs e) // Save button
        {
            // Songlists
            {
                if (this.Songlist.GetSelected(0) & (this.NewSongListNameTxtbox.Text != ReadSettings.ProcessSettings(0)) & (this.NewSongListNameTxtbox.Text != "")) // Songlist 1
                {
                    SaveChanges(1, this.NewSongListNameTxtbox.Text);
                }
                if (this.Songlist.GetSelected(1) & (this.NewSongListNameTxtbox.Text != ReadSettings.ProcessSettings(1)) & (this.NewSongListNameTxtbox.Text != "")) // Songlist 2
                {
                    SaveChanges(2, this.NewSongListNameTxtbox.Text);
                }
                if (this.Songlist.GetSelected(2) & (this.NewSongListNameTxtbox.Text != ReadSettings.ProcessSettings(2)) & (this.NewSongListNameTxtbox.Text != "")) // Songlist 3
                {
                    SaveChanges(3, this.NewSongListNameTxtbox.Text);
                }
                if (this.Songlist.GetSelected(3) & (this.NewSongListNameTxtbox.Text != ReadSettings.ProcessSettings(3)) & (this.NewSongListNameTxtbox.Text != "")) // Songlist 4
                {
                    SaveChanges(4, this.NewSongListNameTxtbox.Text);
                }
                if (this.Songlist.GetSelected(4) & (this.NewSongListNameTxtbox.Text != ReadSettings.ProcessSettings(4)) & (this.NewSongListNameTxtbox.Text != "")) // Songlist 5
                {
                    SaveChanges(5, this.NewSongListNameTxtbox.Text);
                }
                if (this.Songlist.GetSelected(5) & (this.NewSongListNameTxtbox.Text != ReadSettings.ProcessSettings(5)) & (this.NewSongListNameTxtbox.Text != "")) // Songlist 6
                {
                    SaveChanges(6, this.NewSongListNameTxtbox.Text);
                }
            }

            // Mods on KeyPress
            {
                if (this.ModList.GetSelected(0) & (this.NewAssignmentTxtBox.Text != ReadSettings.ProcessSettings(6)) & (this.NewAssignmentTxtBox.Text != "")) // Toggle Loft Key
                {
                    SaveChanges(7, KeyConversion.VirtualKey(this.NewAssignmentTxtBox.Text));
                }
                if (this.ModList.GetSelected(1) & (this.NewAssignmentTxtBox.Text != ReadSettings.ProcessSettings(7)) & (this.NewAssignmentTxtBox.Text != "")) // Add Volume Key
                {
                    SaveChanges(8, KeyConversion.VirtualKey(this.NewAssignmentTxtBox.Text));
                }
                if (this.ModList.GetSelected(2) & (this.NewAssignmentTxtBox.Text != ReadSettings.ProcessSettings(8)) & (this.NewAssignmentTxtBox.Text != "")) // Decrease Volume Key
                {
                    SaveChanges(9, KeyConversion.VirtualKey(this.NewAssignmentTxtBox.Text));
                }
                if (this.ModList.GetSelected(3) & (this.NewAssignmentTxtBox.Text != ReadSettings.ProcessSettings(9)) & (this.NewAssignmentTxtBox.Text != "")) // Show Song Timer Key
                {
                    SaveChanges(10, KeyConversion.VirtualKey(this.NewAssignmentTxtBox.Text));
                }
                if (this.ModList.GetSelected(4) & (this.NewAssignmentTxtBox.Text != ReadSettings.ProcessSettings(10)) & (this.NewAssignmentTxtBox.Text != "")) // Force ReEnumeration Key
                {
                    SaveChanges(11, KeyConversion.VirtualKey(this.NewAssignmentTxtBox.Text));
                }
                if (this.ModList.GetSelected(5) & (this.NewAssignmentTxtBox.Text != ReadSettings.ProcessSettings(16)) & (this.NewAssignmentTxtBox.Text != "")) // Rainbow Strings Key
                {
                    SaveChanges(12, KeyConversion.VirtualKey(this.NewAssignmentTxtBox.Text));
                }
            }
            RefreshForm();
        }

        private void ToggleLoftCheckbox_CheckedChanged(object sender, EventArgs e) // Toggle Loft Enabled/ Disabled
        {
            if (this.ToggleLoftCheckbox.Checked)
            {
                    SaveChanges(13, "true");
                    this.ToggleLoftCheckbox.Checked = true;
                    this.ToggleLoftWhenStartupRadio.Visible = true;
                    this.ToggleLoftWhenManualRadio.Visible = true;
                    this.ToggleLoftWhenSongRadio.Visible = true;
                    this.ToggleLoftOffWhenBox.Visible = true;
            }
            else
            {
                SaveChanges(13, "false");
                this.ToggleLoftCheckbox.Checked = false;
                this.ToggleLoftWhenStartupRadio.Visible = false;
                this.ToggleLoftWhenManualRadio.Visible = false;
                this.ToggleLoftWhenSongRadio.Visible = false;
                this.ToggleLoftOffWhenBox.Visible = false;
            }
        }

        private void AddVolumeCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if(this.AddVolumeCheckbox.Checked)
                SaveChanges(14, "true");
            else
                SaveChanges(14, "false");
        }

        private void DecreaseVolumeCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (this.DecreaseVolumeCheckbox.Checked)
                SaveChanges(15, "true");
            else
                SaveChanges(15, "false");
        }

        private void SongTimerCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (this.SongTimerCheckbox.Checked)
                SaveChanges(16, "true");
            else
                SaveChanges(16, "false");
        }

        private void ForceEnumerationCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (ForceEnumerationCheckbox.Checked)
            {
                this.ForceEnumerationCheckbox.Checked = true;
                this.ForceEnumerationAutomaticRadio.Visible = true;
                this.ForceEnumerationManualRadio.Visible = true;
                this.HowToEnumerateBox.Visible = true;
                }
            else
            {
                SaveChanges(17, "false");
                this.ForceEnumerationCheckbox.Checked = false;
                this.ForceEnumerationAutomaticRadio.Visible = false;
                this.ForceEnumerationManualRadio.Visible = false;
                this.HowToEnumerateBox.Visible = false;
            }
        }

        private void EnumerateEveryXMS_ValueChanged(object sender, EventArgs e)
        {
            SaveChanges(44, (this.EnumerateEveryXMS.Value * 1000).ToString());
        }

        private void ForceEnumerationAutomaticRadio_CheckedChanged(object sender, EventArgs e)
        {
            this.CheckEveryXmsText.Visible = true;
            this.EnumerateEveryXMS.Visible = true;
            SaveChanges(17, "automatic");
        }

        private void ForceEnumerationManualRadio_CheckedChanged(object sender, EventArgs e)
        {
            this.CheckEveryXmsText.Visible = false;
            this.EnumerateEveryXMS.Visible = false;
            SaveChanges(17, "manual");
        }

        private void RainbowStringsEnabled_CheckedChanged(object sender, EventArgs e)
        {
            if (RainbowStringsEnabled.Checked)
                SaveChanges(18, "true");
            else
                SaveChanges(18, "false");
        }

        private void ExtendedRangeEnabled_CheckedChanged(object sender, EventArgs e)
        {
            if(ExtendedRangeEnabled.Checked)
            {
                this.ExtendedRangeTuningBox.Visible = true;
                this.ExtendedRangeTunings.Visible = true;
                SaveChanges(19, "true");
            }
            else
            {
                this.ExtendedRangeTuningBox.Visible = false;
                this.ExtendedRangeTunings.Visible = false;
                SaveChanges(19, "false");
            }
        }

        private void customColorsCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (customColorsCheckbox.Checked)
            {
                SaveChanges(20, "2");
                this.customColorsCheckbox.Checked = true;
                this.ChangeStringColorsBox.Visible = true;
            }
            else
            {
                SaveChanges(20, "0");
                this.customColorsCheckbox.Checked = false;
                this.ChangeStringColorsBox.Visible = false;
            }
        }

        private void DiscoModeCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (DiscoModeCheckbox.Checked)
                SaveChanges(21, "true");
            else
                SaveChanges(21, "false");
        }

        private void HeadstockCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (HeadstockCheckbox.Checked)
                SaveChanges(22, "true");
            else
                SaveChanges(22, "false");
        }

        private void RemoveSkylineCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (RemoveSkylineCheckbox.Checked)
                SaveChanges(23, "true");
            else
                SaveChanges(23, "false");
        }

        private void GreenScreenWallCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (GreenScreenWallCheckbox.Checked)
                SaveChanges(24, "true");
            else
                SaveChanges(24, "false");
        }

        private void AutoLoadProfileCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (AutoLoadProfileCheckbox.Checked)
                SaveChanges(25, "true");
            else
                SaveChanges(25, "false");
        }

        private void FretlessModeCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (FretlessModeCheckbox.Checked)
                SaveChanges(26, "true");
            else
                SaveChanges(26, "false");
        }

        private void RemoveInlaysCheckbox_CheckedChanged(object sender, EventArgs e)
        {
            if (RemoveInlaysCheckbox.Checked)
                SaveChanges(27, "true");
            else
                SaveChanges(27, "false");
        }

        private void ToggleLoftWhenManualRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (ToggleLoftWhenManualRadio.Checked)
                SaveChanges(28, "manual");
        }

        private void ToggleLoftWhenSongRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (ToggleLoftWhenSongRadio.Checked)
                SaveChanges(28, "song");
        }

        private void ToggleLoftWhenStartupRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (ToggleLoftWhenStartupRadio.Checked)
                SaveChanges(28, "startup");
        }

        private void RemoveLineMarkersCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (RemoveLineMarkersCheckBox.Checked)
                SaveChanges(29, "true");
            else
                SaveChanges(29, "false");
        }

        private void ToggleSkylineSongRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (ToggleSkylineSongRadio.Checked)
                SaveChanges(30, "song");
        }

        private void ToggleSkylineStartupRadio_CheckedChanged(object sender, EventArgs e)
        {
            if (ToggleSkylineStartupRadio.Checked)
                SaveChanges(30, "startup");
        }

        private void ExtendedRangeTunings_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.ExtendedRangeTunings.GetSelected(0))
            {
                SaveChanges(43, "-2");
            }
            if (this.ExtendedRangeTunings.GetSelected(1))
            {
                SaveChanges(43, "-3");
            }
            if (this.ExtendedRangeTunings.GetSelected(2))
            {
                SaveChanges(43, "-4");
            }
            if (this.ExtendedRangeTunings.GetSelected(3))
            {
                SaveChanges(43, "-5");
            }
            if (this.ExtendedRangeTunings.GetSelected(4))
            {
                SaveChanges(43, "-6");
            }
            if (this.ExtendedRangeTunings.GetSelected(5))
            {
                SaveChanges(43, "-7");
            }
            if (this.ExtendedRangeTunings.GetSelected(6))
            {
                SaveChanges(43, "-8");
            }
            if (this.ExtendedRangeTunings.GetSelected(7))
            {
                SaveChanges(43, "-9");
            }
            if (this.ExtendedRangeTunings.GetSelected(8))
            {
                SaveChanges(43, "-10");
            }
            if (this.ExtendedRangeTunings.GetSelected(9))
            {
                SaveChanges(43, "-11");
            }
            if (this.ExtendedRangeTunings.GetSelected(10))
            {
                SaveChanges(43, "-12");
            }
        }

    }
}