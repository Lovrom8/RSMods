using System;
using System.Windows.Forms;
using System.Runtime;
using System.IO;
using System.Linq;
using System.Drawing;

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
                if (ImportPriorSettings()[12].ToString() == "true") // Toggle Loft Enabled / Disabled
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

                if (ImportPriorSettings()[13].ToString() == "true") // Add Volume Enabled / Disabled
                {
                    this.AddVolumeCheckbox.Checked = true;
                }
                else
                {
                    this.AddVolumeCheckbox.Checked = false;
                }
                if (ImportPriorSettings()[14].ToString() == "true") // Decrease Volume Enabled / Disabled
                {
                    this.DecreaseVolumeCheckbox.Checked = true;
                }
                else
                {
                    this.DecreaseVolumeCheckbox.Checked = false;
                }
                if (ImportPriorSettings()[15].ToString() == "true") // Show Song Timer Enabled / Disabled
                {
                    this.SongTimerCheckbox.Checked = true;
                }
                else
                {
                    this.SongTimerCheckbox.Checked = false;
                }
                if (ImportPriorSettings()[17].ToString() == "true") // Rainbow String Enabled / Disabled
                {
                    this.RainbowStringsEnabled.Checked = true;
                }
                else
                {
                    this.RainbowStringsEnabled.Checked = false;
                }
                if (ImportPriorSettings()[16].ToString() == "automatic" ^ ImportPriorSettings()[16].ToString() == "manual") // Force ReEnumeration Manual / Automatic / False
                {
                    this.ForceEnumerationCheckbox.Checked = true;
                    this.ForceEnumerationAutomaticRadio.Visible = true;
                    this.ForceEnumerationManualRadio.Visible = true;
                    this.HowToEnumerateBox.Visible = true;

                    if (ImportPriorSettings()[16].ToString() == "automatic")
                    {
                        this.ForceEnumerationAutomaticRadio.Checked = true;
                    }
                    if (ImportPriorSettings()[16].ToString() == "manual")
                    {
                        this.ForceEnumerationManualRadio.Checked = true;
                    }
                }
                if (ImportPriorSettings()[18].ToString() == "true") // Extended Range Enabled / Disabled
                {
                    this.ExtendedRangeEnabled.Checked = true;
                    this.ExtendedRangeTuningBox.Visible = true;
                    this.ExtendedRangeTunings.Visible = true;
                }
                else
                {
                    this.ExtendedRangeEnabled.Checked = false;
                }

                if (ImportPriorSettings()[20].ToString() == "true") // Disco Mode Enabled / Disabled
                {
                    this.DiscoModeCheckbox.Checked = true;
                }
                else
                {
                    this.DiscoModeCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[21].ToString() == "true") // Remove Headstock Enabled / Disabled
                {
                    this.HeadstockCheckbox.Checked = true;
                }
                else
                {
                    this.HeadstockCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[22].ToString() == "true") // Remove Skyline Enabled / Disabled
                {
                    this.RemoveSkylineCheckbox.Checked = true;
                }
                else
                {
                    this.RemoveSkylineCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[23].ToString() == "true") // Greenscreen Wall Enabled / Disabled
                {
                    this.GreenScreenWallCheckbox.Checked = true;
                }
                else
                {
                    this.GreenScreenWallCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[24].ToString() == "true") // Force Load Profile On Game Boot Enabled / Disabled
                {
                    this.AutoLoadProfileCheckbox.Checked = true;
                }
                else
                {
                    this.AutoLoadProfileCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[25].ToString() == "true") // Fretless Mode Enabled / Disabled
                {
                    this.FretlessModeCheckbox.Checked = true;
                }
                else
                {
                    this.FretlessModeCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[26].ToString() == "true") // Remove Inlay Markers Enabled / Disabled
                {
                    this.RemoveInlaysCheckbox.Checked = true;
                }
                else
                {
                    this.RemoveInlaysCheckbox.Checked = false;
                }

                if (ImportPriorSettings()[28].ToString() == "true") // Remove Line Markers Enabled / Disabled
                {
                    this.RemoveLineMarkersCheckBox.Checked = true;
                }
                else
                {
                    this.RemoveLineMarkersCheckBox.Checked = false;
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
                Save_SaveChanges(sender, e);
                e.SuppressKeyPress = true;
            }
        }

        private void SaveAndQuitFunction(object sender, EventArgs e)
        {
            Save_SaveChanges(sender, e);
            Application.Exit();
        }

        private void ResetToDefaultSettings(object sender, EventArgs e)
        {
            File.Delete(@WriteSettings.dumpLocation);
            RefreshForm();
        }

        private void Save_SaveChanges(object sender, EventArgs e) // Save button
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
           
            // Toggle Mods
            {
                if (this.ToggleLoftCheckbox.Checked.ToString() != ReadSettings.ProcessSettings(11)) // Toggle Loft Enabled/ Disabled
                {
                    SaveChanges(13, this.ToggleLoftCheckbox.Checked.ToString().ToLower());
                }

                if (this.AddVolumeCheckbox.Checked.ToString() != ReadSettings.ProcessSettings(13)) // Add Volume Enabled/ Disabled
                {
                    SaveChanges(14, this.AddVolumeCheckbox.Checked.ToString().ToLower());
                }
                if (this.DecreaseVolumeCheckbox.Checked.ToString() != ReadSettings.ProcessSettings(14)) // Decrease Volume Enabled / Disabled
                {
                    SaveChanges(15, this.DecreaseVolumeCheckbox.Checked.ToString().ToLower());
                }
                if (this.SongTimerCheckbox.Checked.ToString() != ReadSettings.ProcessSettings(15)) // Show Song Timer Enabled / Disabled
                {
                    SaveChanges(16, this.SongTimerCheckbox.Checked.ToString().ToLower());
                }
                if (this.ForceEnumerationCheckbox.Checked == true) // Force Enumeration Manual / Automatic / Disabled
                {
                    if (this.ForceEnumerationAutomaticRadio.Checked == true)
                    {
                        SaveChanges(17, "automatic");
                    }
                    else
                    {
                        SaveChanges(17, "manual");
                    }
                }
                else
                {
                    SaveChanges(17, "false");
                }
                if (this.RainbowStringsEnabled.Checked.ToString() != ReadSettings.ProcessSettings(17)) // Rainbow Strings Enabled / Disabled
                {
                    SaveChanges(18, this.RainbowStringsEnabled.Checked.ToString().ToLower());
                }
                if (this.ExtendedRangeEnabled.Checked.ToString() != ReadSettings.ProcessSettings(18)) // Extended Range Mode Enabled / Disabled
                {
                    SaveChanges(19, this.ExtendedRangeEnabled.Checked.ToString().ToLower());
                }
                if (this.DiscoModeCheckbox.Checked.ToString() != ReadSettings.ProcessSettings(20)) // Disco Mode Enabled / Disabled
                {
                    SaveChanges(21, this.DiscoModeCheckbox.Checked.ToString().ToLower());
                }
                if (this.HeadstockCheckbox.Checked.ToString() != ReadSettings.ProcessSettings(21)) // Remove Headstock Enabled / Disabled
                {
                    SaveChanges(22, this.HeadstockCheckbox.Checked.ToString().ToLower());
                }
                if (this.RemoveSkylineCheckbox.Checked.ToString() != ReadSettings.ProcessSettings(22)) // Remove Skyline Enabled / Disabled
                {
                    SaveChanges(23, this.RemoveSkylineCheckbox.Checked.ToString().ToLower());
                }
                if (this.GreenScreenWallCheckbox.Checked.ToString() != ReadSettings.ProcessSettings(23)) // Green Screen Wall Enabled / Disabled
                {
                    SaveChanges(24, this.GreenScreenWallCheckbox.Checked.ToString().ToLower());
                }
                if (this.AutoLoadProfileCheckbox.Checked.ToString() != ReadSettings.ProcessSettings(24)) // Force Load Profile On Game Boot Enabled / Disabled
                {
                    SaveChanges(25, this.AutoLoadProfileCheckbox.Checked.ToString().ToLower());
                }
                if (this.FretlessModeCheckbox.Checked.ToString() != ReadSettings.ProcessSettings(25)) // Fretless Mode Enabled / Disabled
                {
                    SaveChanges(26, this.FretlessModeCheckbox.Checked.ToString().ToLower());
                }
                if (this.RemoveInlaysCheckbox.Checked.ToString() != ReadSettings.ProcessSettings(26)) // Remove Inlay Markers Enabled / Disabled
                {
                    SaveChanges(27, this.RemoveInlaysCheckbox.Checked.ToString().ToLower());
                }
                if (this.ToggleLoftCheckbox.Checked == true)
                {
                    if (this.ToggleLoftWhenManualRadio.Checked == true) // Toggle Loft On Keypress
                    {
                        SaveChanges(28, "manual");
                    }
                    if (this.ToggleLoftWhenSongRadio.Checked == true) // Toggle Loft On Song Load
                    {
                        SaveChanges(28, "song");
                    }
                    if (this.ToggleLoftWhenStartupRadio.Checked == true) // Toggle Loft On Game Startup
                    {
                        SaveChanges(28, "startup");
                    }
                }
                if (this.RemoveLineMarkersCheckBox.Checked.ToString() != ReadSettings.ProcessSettings(28)) // Remove Line Markers Enabled / Disabled
                {
                    SaveChanges(29, this.RemoveLineMarkersCheckBox.Checked.ToString().ToLower());
                }
            }
            
            // Extended Range
            {
                if (this.ExtendedRangeTunings.GetSelected(0))
                {
                    SaveChanges(20, "-2");
                }
                if (this.ExtendedRangeTunings.GetSelected(1))
                {
                    SaveChanges(20, "-3");
                }
                if (this.ExtendedRangeTunings.GetSelected(2))
                {
                    SaveChanges(20, "-4");
                }
                if (this.ExtendedRangeTunings.GetSelected(3))
                {
                    SaveChanges(20, "-5");
                }
                if (this.ExtendedRangeTunings.GetSelected(4))
                {
                    SaveChanges(20, "-6");
                }
                if (this.ExtendedRangeTunings.GetSelected(5))
                {
                    SaveChanges(20, "-7");
                }
                if (this.ExtendedRangeTunings.GetSelected(6))
                {
                    SaveChanges(20, "-8");
                }
                if (this.ExtendedRangeTunings.GetSelected(7))
                {
                    SaveChanges(20, "-9");
                }
                if (this.ExtendedRangeTunings.GetSelected(8))
                {
                    SaveChanges(20, "-10");
                }
                if (this.ExtendedRangeTunings.GetSelected(9))
                {
                    SaveChanges(20, "-11");
                }
                if (this.ExtendedRangeTunings.GetSelected(10))
                {
                    SaveChanges(20, "-12");
                }
            }

            // String Colors
            {

            }
            
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
                    if (ElementToChange == 20) // Extended Range Tuning Name
                    {
                        StringArray[22] = ReadSettings.ExtendedRangeTuningIdentifier + ChangedSettingValue;
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
                }
                // String Colors (Normal {N} & Colorblind {CB})
                {
                    // Normal String Colors
                    {
                        if (ElementToChange == 31)
                        {
                            StringArray[33] = ReadSettings.String0Color_N_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 32)
                        {
                            StringArray[34] = ReadSettings.String1Color_N_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 33)
                        {
                            StringArray[35] = ReadSettings.String2Color_N_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 34)
                        {
                            StringArray[36] = ReadSettings.String3Color_N_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 35)
                        {
                            StringArray[37] = ReadSettings.String4Color_N_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 36)
                        {
                            StringArray[38] = ReadSettings.String5Color_N_Identifier + ChangedSettingValue;
                        }
                    }
                    // Colorblind String Colors
                    {
                        if (ElementToChange == 37)
                        {
                            StringArray[39] = ReadSettings.String0Color_CB_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 38)
                        {
                            StringArray[40] = ReadSettings.String1Color_CB_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 39)
                        {
                            StringArray[41] = ReadSettings.String2Color_CB_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 40)
                        {
                            StringArray[42] = ReadSettings.String3Color_CB_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 41)
                        {
                            StringArray[43] = ReadSettings.String4Color_CB_Identifier + ChangedSettingValue;
                        }
                        if (ElementToChange == 42)
                        {
                            StringArray[44] = ReadSettings.String5Color_CB_Identifier + ChangedSettingValue;
                        }
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
            object[] priorSettings = new object[42];
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
            priorSettings[19] = ReadSettings.ProcessSettings(19); // Extended Range At X Tuning
            priorSettings[20] = ReadSettings.ProcessSettings(20); // Disco Mode Enabled / Disabled
            priorSettings[21] = ReadSettings.ProcessSettings(21); // Remove Headstock Enabled / Disabled
            priorSettings[22] = ReadSettings.ProcessSettings(22); // Remove Skyline Enabled / Disabled
            priorSettings[23] = ReadSettings.ProcessSettings(23); // Greenscreen Wall Enabled / Disabled
            priorSettings[24] = ReadSettings.ProcessSettings(24); // Force Load Profile On Game Boot Enabled / Disabled
            priorSettings[25] = ReadSettings.ProcessSettings(25); // Fretless Mode Enabled / Disabled
            priorSettings[26] = ReadSettings.ProcessSettings(26); // Remove Inlay Markers Enabled / Disabled
            priorSettings[27] = ReadSettings.ProcessSettings(27); // Toggle Loft When Startup / Manual / Song
            priorSettings[28] = ReadSettings.ProcessSettings(28); // Remove Lane Markers Enabled / Disabled
            //priorSettings[29] = ReadSettings.ProcessSettings(29); // Unused for right now

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
            return priorSettings;
        }

        private void ChangeString0ColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog();
            colorDialog.AllowFullOpen = true;
            colorDialog.ShowHelp = false;
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
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
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
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
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
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
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
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
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
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
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
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
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
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
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
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
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
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
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
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
            if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
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
                if (DefaultStringColorsRadio.Checked.ToString().ToLower() == "true")
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
    }
}