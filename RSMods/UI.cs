using System;
using System.Windows.Forms;
using System.IO;
using System.Linq;
using System.Drawing;
using RSMods.Data;
using RSMods.Util;
using System.Reflection;
using System.Collections.Generic;
using RocksmithToolkitLib.DLCPackage.Manifest2014.Tone;
using RocksmithToolkitLib.Extensions;
using RSMods.Twitch;
using System.Threading;
using System.Xml.Serialization;
using System.Xml;
using System.Threading.Tasks;

namespace RSMods
{
    public partial class MainForm : Form
    {
        // Global Vars
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

            // Load saved credidentials and enable PubSub
            LoadTwitchSettings();

            InitializeComponent();
            Text = $"{Text}-{Assembly.GetExecutingAssembly().GetName().Version}";

            // Setup bindings for Twitch events
            SetupTwitchTab();

            // Fix Songlist Bug
            if (ReadSettings.ProcessSettings(ReadSettings.Songlist1Identifier) == String.Empty)
                SaveChanges(ReadSettings.Songlist1Identifier, "Define Song List 1 Here");

            // Fill Songlist List
            foreach (string songlist in Dictionaries.refreshSonglistOrSavedKeysForModTogglesLists("songlists"))
                listBox_Songlist.Items.Add(songlist);

            // Fill Modlist List
            foreach (string mod in Dictionaries.currentModKeypressList)
                listBox_Modlist.Items.Add(mod);

            // Load Keybinding Values
            ShowCurrentKeybindingValues();

            // Load Guitar Speak Preset Values
            RefreshGuitarSpeakPresets();

            // Load Checkbox Values From RSMods.ini
            RefreshModsSelections();

            // Load Default String Colors
            LoadDefaultStringColors();

            // Load Colors Saved as Theme Colors.
            LoadCustomThemeColors();

            // Load Twitch Colors
            LoadTwitchDefaultColors();

            // Load Mod Settings
            LoadModSettings();

            // Load Set And Forget Mods
            LoadSetAndForgetMods();
        }

        private void ModList_SelectedIndexChanged(object sender, EventArgs e) => textBox_NewKeyAssignment.Text = Dictionaries.refreshSonglistOrSavedKeysForModTogglesLists("savedKeysForModToggles")[listBox_Modlist.SelectedIndex];

        private void LoadModSettings()
        {
            // Disable changes while we change them. This prevents us from saving a value we already know.
            listBox_ExtendedRangeTunings.SelectedIndexChanged -= new System.EventHandler(ExtendedRangeTunings_SelectedIndexChanged);
            nUpDown_ForceEnumerationXMS.ValueChanged -= new System.EventHandler(EnumerateEveryXMS_ValueChanged);

            // Now we can change things without saving.
            nUpDown_ForceEnumerationXMS.Value = Decimal.Parse(ReadSettings.ProcessSettings(ReadSettings.CheckForNewSongIntervalIdentifier)) / 1000; // Loads old settings for enumeration every x ms
            listBox_ExtendedRangeTunings.SelectedIndex = (Convert.ToInt32(ReadSettings.ProcessSettings(ReadSettings.ExtendedRangeTuningIdentifier)) * -1) - 2; // Loads old ER tuning settings

            // Re-enable the saving of the values now that we've done our work.
            listBox_ExtendedRangeTunings.SelectedIndexChanged += new System.EventHandler(ExtendedRangeTunings_SelectedIndexChanged);
            nUpDown_ForceEnumerationXMS.ValueChanged += new System.EventHandler(EnumerateEveryXMS_ValueChanged);
        }

        private void RefreshModsSelections()
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

        // Not taken from here :O https://stackoverflow.com/a/3419209
        private List<Control> ControlList = new List<Control>(); // Don't make this readonly
        private void GetAllControls(Control container)
        {
            foreach (Control c in container.Controls)
            {
                GetAllControls(c);
                if (c is ListBox || c is GroupBox || c is TabPage)
                    ControlList.Add(c);
            }
        }

        private void ChangeTheme(Color backgroundColor, Color textColor)
        {
            GetAllControls(TabController);
            BackColor = backgroundColor; // MainForm BackColor
            ForeColor = textColor; // MainForm ForeColor

            foreach (Control backColorToChange in ControlList)
                backColorToChange.BackColor = backgroundColor;
            foreach (Control foreColorToChange in ControlList)
                foreColorToChange.ForeColor = textColor;
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

                else if ((e.KeyValue > 47 && e.KeyValue < 60) || (e.KeyValue > 64 && e.KeyValue < 91)) // Number or Letter was pressed (Will be overrided by text input)
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

        private void SaveChanges(string IdentifierToChange, string ChangedSettingValue)
        {
            // Right before launch, we switched from the boolean names of (true / false) to (on / off) for users to be able to edit the mods without the GUI (by hand).
            if (ChangedSettingValue == "true")
                ChangedSettingValue = "on";
            else if (ChangedSettingValue == "false")
                ChangedSettingValue = "off";

            foreach (string section in WriteSettings.saveSettings.Keys)
            {
                foreach (KeyValuePair<string, string> entry in WriteSettings.saveSettings[section])
                {
                    if (IdentifierToChange == entry.Key)
                    {
                        WriteSettings.saveSettings[section][IdentifierToChange] = ChangedSettingValue;
                        break; // We found what we need, so let's leave.
                    }
                }
            }
            WriteSettings.WriteINI(WriteSettings.saveSettings);
        }

        private void ChangeStringColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog
            {
                AllowFullOpen = true,
                ShowHelp = false
            };
            bool isNormalStrings = radio_DefaultStringColors.Checked; // True = Normal, False = Colorblind
            string stringColorButtonIdentifier = String.Empty;
            int stringNumber = 0;
            FillStringNumberToColorDictionary();

            foreach (KeyValuePair<string, string> stringColorButton in Dictionaries.stringColorButtonsToSettingIdentifiers[isNormalStrings])
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
                Dictionaries.stringNumberToColorTextBox[stringNumber].BackColor = colorDialog.Color;
            }
        }

        private void FillStringNumberToColorDictionary()
        {
            Dictionaries.stringNumberToColorTextBox.Clear();

            Dictionaries.stringNumberToColorTextBox.Add(0, textBox_String0Color);
            Dictionaries.stringNumberToColorTextBox.Add(1, textBox_String1Color);
            Dictionaries.stringNumberToColorTextBox.Add(2, textBox_String2Color);
            Dictionaries.stringNumberToColorTextBox.Add(3, textBox_String3Color);
            Dictionaries.stringNumberToColorTextBox.Add(4, textBox_String4Color);
            Dictionaries.stringNumberToColorTextBox.Add(5, textBox_String5Color);
        }

        private void LoadDefaultStringColors(bool colorBlind = false)
        {
            if (ReadSettings.ProcessSettings(ReadSettings.String0Color_N_Identifier) != String.Empty) // Fixes a small use case where the GUI moves faster than the writing of the INI.
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
            else
                WriteSettings.WriteINI(WriteSettings.Settings);
        }

        private void LoadTwitchDefaultColors()
        {
            if (ReadSettings.ProcessSettings(ReadSettings.TwitchSolidNoteColorIdentifier) != "random" && ReadSettings.ProcessSettings(ReadSettings.TwitchSolidNoteColorIdentifier) != String.Empty)
                textBox_SolidNoteColorPicker.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.TwitchSolidNoteColorIdentifier));
            else
            {
                textBox_SolidNoteColorPicker.BackColor = Color.White;
                //  textBox_SolidNoteColorPicker.Text = "Random";
            }

        }

        private void LoadCustomThemeColors()
        {
            Color backColor = WriteSettings.defaultBackgroundColor, foreColor = WriteSettings.defaultTextColor;

            if (ReadSettings.ProcessSettings(ReadSettings.CustomGUIThemeIdentifier) == "on") // Users uses a custom theme.
            {
                if (ReadSettings.ProcessSettings(ReadSettings.CustomGUIBackgroundColorIdentifier) != String.Empty)
                    backColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.CustomGUIBackgroundColorIdentifier));

                if (ReadSettings.ProcessSettings(ReadSettings.CustomGUITextColorIdentifier) != String.Empty)
                    foreColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.CustomGUITextColorIdentifier));
            }

            textBox_ChangeBackgroundColor.BackColor = backColor;
            textBox_ChangeTextColor.BackColor = foreColor;
        }

        private void DefaultStringColorsRadio_CheckedChanged(object sender, EventArgs e) => LoadDefaultStringColors();

        private void ColorBlindStringColorsRadio_CheckedChanged(object sender, EventArgs e) => LoadDefaultStringColors(true);

        // SetAndForget Mods

        private void FillUI()
        {
            listBox_Tunings.Items.Clear();
            SetAndForgetMods.TuningsCollection = SetAndForgetMods.LoadTuningsCollection();

            listBox_Tunings.Items.Add("<New>");
            foreach (var key in SetAndForgetMods.TuningsCollection.Keys)
                listBox_Tunings.Items.Add(key);
        }

        private TuningDefinitionInfo GetCurrentTuningInfo()
        {
            var tuningDefinition = new TuningDefinitionInfo();
            var strings = new Dictionary<string, int>();

            for (int strIdx = 0; strIdx < 6; strIdx++)
                strings[$"string{strIdx}"] = (int)((NumericUpDown)groupBox_SetAndForget.Controls[$"nUpDown_String{strIdx}"]).Value;

            tuningDefinition.Strings = strings;
            tuningDefinition.UIName = String.Format("$[{0}]{1}", nUpDown_UIIndex.Value.ToString(), textBox_UIName.Text);

            return tuningDefinition;
        }

        private void BtnRestoreDefaults_Click(object sender, EventArgs e)
        {
            if (SetAndForgetMods.RestoreDefaults())
                FillUI();
        }

        private void BtnUnpackCacheAgain_Click(object sender, EventArgs e) => SetAndForgetMods.CleanUnpackedCache();

        private void BtnAddCustomTunings_Click(object sender, EventArgs e) => SetAndForgetMods.AddCustomTunings();

        private void BtnAddFastLoadMod_Click(object sender, EventArgs e) => SetAndForgetMods.AddFastLoadMod();

        private void LoadSetAndForgetMods()
        {
            SetAndForgetMods.LoadDefaultFiles();
            FillUI();
        }

        private void ListTunings_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (sender == null || listBox_Tunings.SelectedItem == null)
                return;

            string selectedItem = listBox_Tunings.SelectedItem.ToString();

            if (selectedItem == "<New>")
            {
                textBox_InternalTuningName.Text = "";
                nUpDown_UIIndex.Value = 0;
                textBox_UIName.Text = "";
                return;
            }

            var selectedTuning = SetAndForgetMods.TuningsCollection[selectedItem];
            var uiName = SetAndForgetMods.SplitTuningUIName(selectedTuning.UIName);

            textBox_InternalTuningName.Text = selectedItem;
            nUpDown_UIIndex.Value = Convert.ToInt32(uiName.Item1);
            textBox_UIName.Text = uiName.Item2;

            for (int strIdx = 0; strIdx < 6; strIdx++) // If you are lazy and don't want to list each string separately, just do this sexy two-liner
                ((NumericUpDown)groupBox_SetAndForget.Controls[$"nUpDown_String{strIdx}"]).Value = selectedTuning.Strings[$"string{strIdx}"];
        }

        private void BtnSaveTuningChanges_Click(object sender, EventArgs e)
        {
            if (listBox_Tunings.SelectedItem != null) // If we are saving a change to the currently selected tuning, perform a change in the collection, otherwise directly go to saving
            {
                string selectedItem = listBox_Tunings.SelectedItem.ToString();

                if (selectedItem != "<New>")
                    SetAndForgetMods.TuningsCollection[selectedItem] = GetCurrentTuningInfo();
            }

            SetAndForgetMods.SaveTuningsJSON();

            MessageBox.Show("Saved current tuning, don't forget to press \"Add Custom Tunings\" button when you are done!", "Success");
        }

        private void BtnRemoveTuning_Click(object sender, EventArgs e)
        {
            if (listBox_Tunings.SelectedItem == null)
                return;

            string selectedItem = listBox_Tunings.SelectedItem.ToString();

            if (selectedItem == "<New>")
                return;

            SetAndForgetMods.TuningsCollection.Remove(selectedItem); // I guess we would be better here using BindingSource on Listbox + ObservableCollection instead of Dict to make changes reflect automatically, but... one day
            listBox_Tunings.Items.Remove(selectedItem);
        }

        private void BtnAddTuning_Click(object sender, EventArgs e)
        {
            if (listBox_Tunings.SelectedItem.ToString() != "<New>")
                return;

            var currTuning = GetCurrentTuningInfo();
            string internalName = textBox_InternalTuningName.Text;

            if (!SetAndForgetMods.TuningsCollection.ContainsKey(internalName)) // Unlikely to happen, but still... prevent users accidentaly trying to add existing stuff
            {
                SetAndForgetMods.TuningsCollection.Add(internalName, currTuning);
                listBox_Tunings.Items.Add(internalName);
            }
        }

        private void BtnAddCustomMenu_Click(object sender, EventArgs e) => SetAndForgetMods.AddCustomMenuOptions();

        private void BtnRemoveTempFolders_Click(object sender, EventArgs e) => SetAndForgetMods.RemoveTempFolders();

        private void BtnSetDefaultTones_Click(object sender, EventArgs e)
        {
            if (listBox_ProfileTones.SelectedItem == null)
                return;

            int selectedToneType = -1;
            if (radio_DefaultRhythmTone.Checked)
                selectedToneType = 0;
            else if (radio_DefaultLeadTone.Checked)
                selectedToneType = 1;
            else if (radio_DefaultBassTone.Checked)
                selectedToneType = 2;

            string selectedToneName = listBox_ProfileTones.SelectedItem.ToString();

            SetAndForgetMods.SetDefaultTones(selectedToneName, selectedToneType);
        }

        private void BtnLoadTonesFromProfiles_Click(object sender, EventArgs e)
        {
            var profileTones = SetAndForgetMods.GetSteamProfilesFolder();

            if (profileTones.Count > 0)
            {
                listBox_ProfileTones.Items.Clear();

                profileTones.ForEach(t => listBox_ProfileTones.Items.Add(t));
            }
        }

        private void BtnImportExistingSettings_Click(object sender, EventArgs e)
        {
            if (SetAndForgetMods.ImportExistingSettings())
                FillUI();
        }

        private void Save_Songlists_Keybindings(object sender, EventArgs e) // Save Songlists and Keybindings when pressing Enter
        {
            string currentTab = TabController.SelectedTab.Text.ToString();

            if (currentTab == "Song Lists")
            {
                foreach (KeyValuePair<int, string> currentSongList in Dictionaries.SongListIndexToINISetting)
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
                foreach (KeyValuePair<int, string> currentKeybinding in Dictionaries.KeybindingsIndexToINISetting)
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

        private void AddVolumeCheckbox_CheckedChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.AddVolumeEnabledIdentifier, AddVolumeCheckbox.Checked.ToString().ToLower());

        private void DecreaseVolumeCheckbox_CheckedChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.DecreaseVolumeEnabledIdentifier, DecreaseVolumeCheckbox.Checked.ToString().ToLower());

        private void SongTimerCheckbox_CheckedChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.ShowSongTimerEnabledIdentifier, checkBox_SongTimer.Checked.ToString().ToLower());

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

        private void RainbowStringsEnabled_CheckedChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.RainbowStringsEnabledIdentifier, checkBox_RainbowStrings.Checked.ToString().ToLower());

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
        // private void DiscoModeCheckbox_CheckedChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.DiscoModeIdentifier, DiscoModeCheckbox.Checked.ToString().ToLower());

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

        private void GreenScreenWallCheckbox_CheckedChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.GreenScreenWallIdentifier, checkBox_GreenScreen.Checked.ToString().ToLower());

        private void AutoLoadProfileCheckbox_CheckedChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.ForceProfileEnabledIdentifier, checkBox_AutoLoadProfile.Checked.ToString().ToLower());

        private void FretlessModeCheckbox_CheckedChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.FretlessModeEnabledIdentifier, checkBox_Fretless.Checked.ToString().ToLower());

        private void RemoveInlaysCheckbox_CheckedChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.RemoveInlaysIdentifier, checkBox_RemoveInlays.Checked.ToString().ToLower());

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

        private void RemoveLineMarkersCheckBox_CheckedChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.RemoveLaneMarkersIdentifier, checkBox_RemoveLineMarkers.Checked.ToString().ToLower());
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

            foreach (KeyValuePair<int, string> currentMod in Dictionaries.KeybindingsIndexToINISetting)
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
            label_ChangeSelectedVolumeKey.Text = "Change Selected Volume: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ChangeSelectedVolumeKeyIdentifier));
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

        private void FillToolTipDictionary()
        {
            // INI Edits
            // Checkboxes
            Dictionaries.TooltipDictionary.Add(checkBox_ToggleLoft, "Disables the game background, amps and noise reactive speaker rings.\nBest used with Venue Mode off (setting in game).\nUsed by a lot of Rocksmith Streamers to make it easy to Luma Key out the game background.\nPlayer just sees an all black background when this is enabled.\nOptions for turning the loft off only when in a song, when the game first starts up, or on a key press.");
            //TooltipDictionary.Add(AddVolumeCheckbox, "Experimental"); // Changed to "VolumeControlsCheckbox"
            //TooltipDictionary.Add(DecreaseVolumeCheckbox, "Experimental"); // Changed to "VolumeControlsCheckbox"
            Dictionaries.TooltipDictionary.Add(checkBox_SongTimer, "Experimental.\nIntent is to show a box with your timestamp position through the song.");
            Dictionaries.TooltipDictionary.Add(checkBox_ExtendedRange, "Alters the string and note colors to make it easier to play a 5 string bass or 7 string guitar.");
            Dictionaries.TooltipDictionary.Add(checkBox_ForceEnumeration, "Game will automatically start an Enumeration sequence when a new psarc (CDLC) file is detected as having been added to the dlc folder.\nNot necesary to enable if you're already using Rocksniffer to do the same thing.");
            Dictionaries.TooltipDictionary.Add(checkBox_RemoveHeadstock, "Stops the Headstock of the guitar being drawn.\n“Headless” guitar mode. Just cleans up some more of the UI.");
            Dictionaries.TooltipDictionary.Add(checkBox_RemoveSkyline, "Removes the purple and orange bars from the top of the display in LAS.\nUse in conjunction with No Loft for a cleaner UI.\nOptions for always off, only off when in a song, or only when toggled by key press.");
            Dictionaries.TooltipDictionary.Add(checkBox_GreenScreen, "Changes just a section of the game background to all black, amusing for a selective “green screen” stream experience.\nInvalidated by \"No Loft\".");
            Dictionaries.TooltipDictionary.Add(checkBox_AutoLoadProfile, "Essentially holds down the ENTER key until the game has reached the main menu.\nLets you auto load the last used profile without needing to interact with the game at all.");
            Dictionaries.TooltipDictionary.Add(checkBox_Fretless, "Removes the Fret Wire from the neck, making your instrument appear to be fretless.");
            Dictionaries.TooltipDictionary.Add(checkBox_RemoveInlays, "Disables the guitar neck inlay display entirely.\nNote: This only works with the standard dot inlays.");
            Dictionaries.TooltipDictionary.Add(checkBox_ControlVolume, "Allows you to control how loud the song is using the in-game mixer without needing to open it.");
            Dictionaries.TooltipDictionary.Add(checkBox_GuitarSpeak, "Use your guitar to control the menus!");
            Dictionaries.TooltipDictionary.Add(checkBox_RemoveLyrics, "Disables the display of song lyrics while in Learn-A-Song mode.");
            Dictionaries.TooltipDictionary.Add(checkBox_RainbowStrings, "Experimental.\nHow Pro are you? This makes the players guitar strings constantly cycling through colors.");
            Dictionaries.TooltipDictionary.Add(checkBox_CustomColors, "Lets you define the string / note colors you want.\nSaves a normal set and a Colorblind mode set.");
            Dictionaries.TooltipDictionary.Add(checkBox_RemoveLineMarkers, "Removes the additional lane marker lines seen in the display.\nWhen used with No Loft, provides a cleaner Luma Key.");
            Dictionaries.TooltipDictionary.Add(checkBox_ChangeTheme, "Use this feature to customize the colors used in this GUI.");

            // Mods
            Dictionaries.TooltipDictionary.Add(groupBox_HowToEnumerate, "Choose to Enumerate on key press,\nor automatically scan for changes every X seconds and start enumeration if a new file has been added.");
            Dictionaries.TooltipDictionary.Add(groupBox_ExtendedRangeWhen, "Mod is enabled when the lowest string is tuned to the note defined here.\nSee the Custom Colors - Color Blind mode for the colors that will be used while in ER mode.");
            Dictionaries.TooltipDictionary.Add(groupBox_LoftOffWhen, "Turn the loft off via hotkey, as soon as the game starts up or only when in a song.");
            Dictionaries.TooltipDictionary.Add(radio_colorBlindERColors, "When ER mode is enabled, these are the colors that the strings will be changed to.");
            Dictionaries.TooltipDictionary.Add(groupBox_ToggleSkylineWhen, "Turn the skyline (Purple and Orange DD level bars) as soon as the game starts up, or only when in a song.");
            Dictionaries.TooltipDictionary.Add(groupBox_ToggleLyricsOffWhen, "How or when do you want the lyric display disabled, always, or toggled by a hotkey only?");
            Dictionaries.TooltipDictionary.Add(radio_LyricsAlwaysOff, "Lyrics display will always be disabled in Learn-A-Song game mode.");
            Dictionaries.TooltipDictionary.Add(radio_LyricsOffHotkey, "Lyrics can be toggled on or off by a defined hotkey.");
            Dictionaries.TooltipDictionary.Add(checkbox_GuitarSpeakWhileTuning, "For Advanced Users Only!\nUse Guitar Speak in tuning menus.\nThis can potentially stop you from tuning, or playing songs if setup improperly.");

            // Misc
            Dictionaries.TooltipDictionary.Add(groupBox_Songlist, "Custom names for the 6 \"SONG LISTS\" shown in game.");
            Dictionaries.TooltipDictionary.Add(groupBox_Keybindings, "Set key binds for the toggle on / off by keypress modifications.\nYou need to press ENTER after setting teh key for it to be saved.");
            Dictionaries.TooltipDictionary.Add(button_ResetModsToDefault, "Resets all RSMods values to defaults");

            // Set & Forget Mods (Cache.psarc Modifications)
            // Tones
            Dictionaries.TooltipDictionary.Add(label_ChangeTonesHeader, "This section lets you change the default menu tone for Lead, Rhythm Or Bass.\nYou need to have the tone you want to set saved in your profile first,\nthen you can load it here and set it as the default tone that will be used when you start up Rocksmith.");
            Dictionaries.TooltipDictionary.Add(button_LoadTones, "Step 1.\nClick this to load the tones that are saved in your profile.");
            Dictionaries.TooltipDictionary.Add(listBox_ProfileTones, "Step2.\n Highlight a tone name.");
            Dictionaries.TooltipDictionary.Add(radio_DefaultRhythmTone, "Set Highlighted Tone As New Default Rhythm Tone.");
            Dictionaries.TooltipDictionary.Add(radio_DefaultLeadTone, "Set Highlighted Tone As New Default Lead Tone.");
            Dictionaries.TooltipDictionary.Add(radio_DefaultBassTone, "Set Highlighted Tone As New Default Bass Tone.");
            Dictionaries.TooltipDictionary.Add(button_AssignNewDefaultTone, "Assign the currently highlighted tone to the chosen path.");

            // Custom Tuning
            Dictionaries.TooltipDictionary.Add(listBox_Tunings, "Shows the list of tuning definitions currently in Rocksmith.");
            Dictionaries.TooltipDictionary.Add(button_AddTuning, "Adds the tuning as defined above.");
            Dictionaries.TooltipDictionary.Add(button_RemoveTuning, "Removes the highlighted tuning.");
            Dictionaries.TooltipDictionary.Add(nUpDown_String0, "Set the offset for the low-E string.");
            Dictionaries.TooltipDictionary.Add(nUpDown_String1, "Set the offset for the A string.");
            Dictionaries.TooltipDictionary.Add(nUpDown_String2, "Set the offset for the D string.");
            Dictionaries.TooltipDictionary.Add(nUpDown_String3, "Set the offset for the G string.");
            Dictionaries.TooltipDictionary.Add(nUpDown_String4, "Set the offset for the B string.");
            Dictionaries.TooltipDictionary.Add(nUpDown_String5, "Set the offset for the high-E string.");
            Dictionaries.TooltipDictionary.Add(button_SaveTuningChanges, "Saves the tuning list to Rocksmith.");

            // One Click Mods
            Dictionaries.TooltipDictionary.Add(button_AddDCExitGame, "Adds the Direct Connect mode - microphone mode with tone simulations.\nAlso replaces UPLAY on the main menu with an EXIT GAME option.");
            Dictionaries.TooltipDictionary.Add(button_AddCustomTunings, "Adds some preset definitions for the most common Custom Tunings.");
            Dictionaries.TooltipDictionary.Add(button_AddFastLoad, "SSD drive or faster or may cause the game to not launch properly, skips some of the intro sequences.\nCombined with Auto Load Last Profile and huzzah!");

            // Misc
            Dictionaries.TooltipDictionary.Add(button_RemoveTemp, "Removes the temporary files used by RSMods.");
            Dictionaries.TooltipDictionary.Add(button_RestoreCacheBackup, "Restores the original cache.psarc file\nUndoes all \"Set-and-forget\" mods.");
            Dictionaries.TooltipDictionary.Add(button_CleanUpUnpackedCache, "Removes temporary files and un-packs cache.psarc as it is being used now, again.");
        }

        private void HideToolTips(object sender, EventArgs e)
        {
            if (ActiveForm != null) // This fixes a glitch where if you are hovering over a Control that calls the tooltip, and alt-tab, the program will crash since ActiveFrame turns to null... If the user is highlighting something, and the window becomes null, we need to refrain from trying to hide the tooltip that "does not exist".
            {
                ToolTip.Hide(ActiveForm);
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

            Dictionaries.TooltipDictionary.Clear();
            FillToolTipDictionary();

            foreach (Control ControlHoveredOver in Dictionaries.TooltipDictionary.Keys)
            {
                if (ControlHoveredOver == sender)
                {
                    Dictionaries.TooltipDictionary.TryGetValue(ControlHoveredOver, out string toolTipString);
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

                foreach (KeyValuePair<string, string> entry in Dictionaries.GuitarSpeakKeyPressDictionary)
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
                        listBox_GuitarSpeakPresets.Items.Add(listBox_GuitarSpeakKeypress.SelectedItem.ToString() + ": " + GuitarSpeak.GuitarSpeakNoteOctaveMath(outputNoteOctave.ToString()));
                        RefreshGuitarSpeakPresets();
                    }
                }

                listBox_GuitarSpeakNote.ClearSelected();
                listBox_GuitarSpeakOctave.ClearSelected();
                listBox_GuitarSpeakKeypress.ClearSelected();
            }
            else
                MessageBox.Show("One, or more, of the Guitar Speak boxes not selected", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

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
            string nupName = ((NumericUpDown)sender).Name;
            int stringNumber = Int32.Parse(nupName[nupName.Length - 1].ToString()); // Returns the current sender's name.
            switch (stringNumber)
            {
                case 0:
                    int offset = 28; // Offset from (24 - 1) + 5
                    label_CustomTuningLowEStringLetter.Text = GuitarSpeak.IntToNote(Convert.ToInt32(nUpDown_String0.Value) + offset);
                    break;
                case 1:
                    offset = 33; // Offset from (24 - 1) + 10
                    label_CustomTuningAStringLetter.Text = GuitarSpeak.IntToNote(Convert.ToInt32(nUpDown_String1.Value) + offset);
                    break;
                case 2:
                    offset = 26; // Offset from (24 - 1) + 3
                    label_CustomTuningDStringLetter.Text = GuitarSpeak.IntToNote(Convert.ToInt32(nUpDown_String2.Value) + offset);
                    break;
                case 3:
                    offset = 31;// Offset from (24 - 1) + 8
                    label_CustomTuningGStringLetter.Text = GuitarSpeak.IntToNote(Convert.ToInt32(nUpDown_String3.Value) + offset);
                    break;
                case 4:
                    offset = 35; // Offset from (24 - 1) + 12
                    label_CustomTuningBStringLetter.Text = GuitarSpeak.IntToNote(Convert.ToInt32(nUpDown_String4.Value) + offset);
                    break;
                case 5:
                    offset = 28; // Offset from (24 - 1) + 5
                    label_CustomTuningHighEStringLetter.Text = GuitarSpeak.IntToNote(Convert.ToInt32(nUpDown_String5.Value) + offset);
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

        private void GuitarSpeakWhileTuningBox_CheckedChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.GuitarSpeakTuningIdentifier, checkbox_GuitarSpeakWhileTuning.Checked.ToString().ToLower());

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

            foreach (KeyValuePair<string, string> guitarSpeakKeypress in Dictionaries.RefreshGuitarSpeakPresets())
                listBox_GuitarSpeakPresets.Items.Add(guitarSpeakKeypress.Key + guitarSpeakKeypress.Value);
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            Color backColor = WriteSettings.defaultBackgroundColor, foreColor = WriteSettings.defaultTextColor;

            if (ReadSettings.ProcessSettings(ReadSettings.CustomGUIThemeIdentifier) == "on") // User wants to use a custom theme
            {
                if (ReadSettings.ProcessSettings(ReadSettings.CustomGUIBackgroundColorIdentifier) != String.Empty)
                    backColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.CustomGUIBackgroundColorIdentifier));

                if (ReadSettings.ProcessSettings(ReadSettings.CustomGUITextColorIdentifier) != String.Empty)
                    foreColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.CustomGUITextColorIdentifier));
            }

            ChangeTheme(backColor, foreColor);
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

        private void Button_TwitchReAuthorize_Click(object sender, EventArgs e)
        {
            ImplicitAuth auth = new ImplicitAuth();
            auth.MakeAuthRequest();

            // string authToken = TwitchSettings.Get.AccessToken;
            // while (TwitchSettings.Get.AccessToken == authToken || TwitchSettings.Get.Username == String.Empty) {} // We want to get the new value so we are waiting until this breaks
            // label_AuthorizedAs.Text = $"{TwitchSettings.Get.Username} with channel ID: {TwitchSettings.Get.ChannelID} and access token: {TwitchSettings.Get.AccessToken}";
        }

        private void LoadTwitchSettings()
        {
            TwitchSettings.Get._context = SynchronizationContext.Current;
            TwitchSettings.Get.LoadSettings();
            TwitchSettings.Get.LoadDefaultEffects();
            TwitchSettings.Get.LoadEnabledEffects();
        }

        private void EnableTwitchTab()
        {
            foreach (Control ctrl in tab_Twitch.Controls)
                ctrl.Visible = true;
        }

        private void SetupTwitchTab()
        {
            label_TwitchUsernameVal.DataBindings.Add(new Binding("Text", TwitchSettings.Get, "Username", false, DataSourceUpdateMode.OnPropertyChanged));
            label_TwitchChannelIDVal.DataBindings.Add(new Binding("Text", TwitchSettings.Get, "ChannelID", false, DataSourceUpdateMode.OnPropertyChanged));
            label_TwitchAccessTokenVal.DataBindings.Add(new Binding("Text", TwitchSettings.Get, "AccessToken", false, DataSourceUpdateMode.OnPropertyChanged));
            textBox_TwitchLog.DataBindings.Add(new Binding("Text", TwitchSettings.Get, "Log"));

            Binding listeningToTwitchBinding = new Binding("Text", TwitchSettings.Get, "Authorized");
            listeningToTwitchBinding.Format += (s, e) =>
            {
                if ((bool)e.Value && TwitchSettings.Get.Reauthorized) // If we are authorized
                {
                    PubSub.Get.SetUp(); // Well... this is probably not the best place since it's called a lot, but wing it
                    TwitchSettings.Get.Reauthorized = false;
                    EnableTwitchTab();
                }

                e.Value = (bool)e.Value ? "Listening to Twitch events" : "Not listening to twitch events";
            };
            label_IsListeningToEvents.DataBindings.Add(listeningToTwitchBinding);

            foreach (var defaultReward in TwitchSettings.Get.DefaultRewards) // BindingList... yeah, not yet
                dgv_DefaultRewards.Rows.Add(defaultReward.Name, defaultReward.Description);

            foreach (var enabledReward in TwitchSettings.Get.Rewards)
                AddToSelectedRewards(enabledReward);
        }

        private void Button_ChangeBackgroundColor_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog
            {
                AllowFullOpen = true,
                ShowHelp = false,
                Color = WriteSettings.defaultBackgroundColor
            };

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                SaveChanges(ReadSettings.CustomGUIBackgroundColorIdentifier, (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6"));
                textBox_ChangeBackgroundColor.BackColor = colorDialog.Color;
            }
        }

        private void Button_ChangeTextColor_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog
            {
                AllowFullOpen = true,
                ShowHelp = false,
                Color = WriteSettings.defaultTextColor
            };

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                SaveChanges(ReadSettings.CustomGUITextColorIdentifier, (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6"));
                textBox_ChangeTextColor.BackColor = colorDialog.Color;
            }
        }

        private void Button_SaveThemeColors_Click(object sender, EventArgs e) => ChangeTheme(textBox_ChangeBackgroundColor.BackColor, textBox_ChangeTextColor.BackColor);

        private async void SaveEnabledRewardsToFile()
        {
            await Task.Run(async () =>
            {
                XmlSerializer xs = new XmlSerializer(TwitchSettings.Get.Rewards.GetType());
                using (var sww = new StringWriter())
                {
                    using (XmlWriter writer = XmlWriter.Create(sww, new XmlWriterSettings { Indent = true }))
                    {
                        xs.Serialize(writer, TwitchSettings.Get.Rewards);
                        File.WriteAllText("TwitchEnabledEffects.xml", sww.ToString());
                    }
                }
            });
        }

        private void button_AddSelectedReward_Click(object sender, EventArgs e)
        {
            if (dgv_DefaultRewards.SelectedRows.Count < 1)
                return;

            var selectedRow = dgv_DefaultRewards.SelectedRows[0];
            var selectedReward = TwitchSettings.Get.DefaultRewards.FirstOrDefault(r => r.Name == selectedRow.Cells["colDefaultRewardsName"].Value.ToString());
            int rewardID = -1;

            if (selectedReward == null)
                return;

            if (dgv_EnabledRewards.Rows.Count == 0)
                rewardID = 1;
            else
                rewardID = Convert.ToInt32(dgv_EnabledRewards.Rows[dgv_EnabledRewards.Rows.Count - 1].Cells["colEnabledRewardsID"].Value) + 1;

            if (selectedReward.Name == "Solid color notes")
                selectedReward.AdditionalMsg = (textBox_SolidNoteColorPicker.BackColor.ToArgb() & 0x00ffffff).ToString("X6");

            if (MessageBox.Show("Do you wish to add selected reward for bits or channel points? (bits = Yes, points = No)", "Bits or Channel points?", MessageBoxButtons.YesNo) == DialogResult.Yes)
            {
                var reward = new BitsReward();
                reward.Map(selectedReward);
                reward.BitsID = rewardID;

                TwitchSettings.Get.Rewards.Add(reward);
                AddToSelectedRewards(reward);
            }
            else
            {
                var reward = new ChannelPointsReward();
                reward.Map(selectedReward);
                reward.PointsID = rewardID;

                TwitchSettings.Get.Rewards.Add(reward);
                AddToSelectedRewards(reward);
            }
        }

        private void AddToSelectedRewards(TwitchReward reward) // Just imagine this was a bound list :P
        {
            if (reward is BitsReward)
                dgv_EnabledRewards.Rows.Add(reward.Enabled, reward.Name, reward.Length, ((BitsReward)reward).BitsAmount, "Bits", ((BitsReward)reward).BitsID);
            else if (reward is ChannelPointsReward)
                dgv_EnabledRewards.Rows.Add(reward.Enabled, reward.Name, reward.Length, ((ChannelPointsReward)reward).PointsAmount, "Points", ((ChannelPointsReward)reward).PointsID);
            else if (reward is SubReward)
                dgv_EnabledRewards.Rows.Add(reward.Enabled, reward.Name, reward.Length, 1, "Sub", ((SubReward)reward).SubID);
        }

        private TwitchReward GetSelectedReward(DataGridViewRow selectedRow)
        {
            TwitchReward selectedReward;

            if (selectedRow.Cells["colEnabledRewardsType"].Value.ToString() == "Bits")
                selectedReward = TwitchSettings.Get.Rewards.FirstOrDefault(r => r is BitsReward && ((BitsReward)r).BitsID.ToString() == selectedRow.Cells["colEnabledRewardsID"].Value.ToString());
            else if (selectedRow.Cells["colEnabledRewardsType"].Value.ToString() == "Sub")
                selectedReward = TwitchSettings.Get.Rewards.FirstOrDefault(r => r is SubReward && ((SubReward)r).SubID.ToString() == selectedRow.Cells["colEnabledRewardsID"].Value.ToString());
            else
                selectedReward = TwitchSettings.Get.Rewards.FirstOrDefault(r => r is ChannelPointsReward && ((ChannelPointsReward)r).PointsID.ToString() == selectedRow.Cells["colEnabledRewardsID"].Value.ToString());

            return selectedReward;
        }

        private void dgv_EnabledRewards_CellEndEdit(object sender, DataGridViewCellEventArgs e)
        {
            var selectedRow = dgv_EnabledRewards.SelectedRows[0];
            var selectedReward = GetSelectedReward(selectedRow);

            selectedReward.Enabled = Convert.ToBoolean(selectedRow.Cells["colEnabledRewardsEnabled"].Value);
            selectedReward.Length = Convert.ToInt32(selectedRow.Cells["colEnabledRewardsLength"].Value);

            if (selectedReward is BitsReward)
                ((BitsReward)selectedReward).BitsAmount = Convert.ToInt32(selectedRow.Cells["colEnabledRewardsAmount"].Value);
            else
                ((ChannelPointsReward)selectedReward).PointsAmount = Convert.ToInt32(selectedRow.Cells["colEnabledRewardsAmount"].Value);

            SaveEnabledRewardsToFile();
        }

        private void button_RemoveReward_Click(object sender, EventArgs e)
        {
            if (dgv_EnabledRewards.SelectedRows.Count < 1)
                return;

            var selectedRow = dgv_EnabledRewards.SelectedRows[0];
            var selectedReward = GetSelectedReward(selectedRow);

            if (selectedReward != null)
                TwitchSettings.Get.Rewards.Remove(selectedReward);

            dgv_EnabledRewards.Rows.RemoveAt(selectedRow.Index);

            SaveEnabledRewardsToFile();
        }

        private void SetAdditionalMessage(string msg)
        {
            var selectedRow = dgv_EnabledRewards.SelectedRows[0];
            var selectedReward = GetSelectedReward(selectedRow);

            if (selectedReward.Name != "Solid color notes")
                return;

            selectedReward.AdditionalMsg = msg;
        }

        private void button_SolidNoteColorPicker_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog
            {
                AllowFullOpen = true,
                ShowHelp = false
            };

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                string colorHex = (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6");
                SaveChanges(ReadSettings.TwitchSolidNoteColorIdentifier, colorHex);
                textBox_SolidNoteColorPicker.BackColor = colorDialog.Color;
                textBox_SolidNoteColorPicker.Text = String.Empty;

                SetAdditionalMessage(colorHex);
                SaveEnabledRewardsToFile();
            }
        }

        private void button_SolidNoteColorResetToRandom_Click(object sender, EventArgs e)
        {
            textBox_SolidNoteColorPicker.BackColor = Color.White;
            textBox_SolidNoteColorPicker.Text = "Random";
            SaveChanges(ReadSettings.TwitchSolidNoteColorIdentifier, "random");

            SetAdditionalMessage("Random");
            SaveEnabledRewardsToFile();
        }

        private async Task WaitUntilRewardEnds(int seconds) => await Task.Delay(seconds * 1000);

        private async void SendFakeTwitchReward()
        {
            if (dgv_EnabledRewards.CurrentCell == null)
                return;

            var reward = TwitchSettings.Get.Rewards[dgv_EnabledRewards.CurrentCell.RowIndex];

            if (reward.AdditionalMsg != "")
                WinMsgUtil.SendMsgToRS($"{reward.InternalMsgEnable} {reward.AdditionalMsg}");
            else
                WinMsgUtil.SendMsgToRS(reward.InternalMsgEnable);

            TwitchSettings.Get.AddToLog($"Enabling: {reward.Name}");

            await WaitUntilRewardEnds(reward.Length);

            WinMsgUtil.SendMsgToRS(reward.InternalMsgDisable);
            TwitchSettings.Get.AddToLog($"Disabling: {reward.Name}");
        }

        private void TestTwitchReward_Click(object sender, EventArgs e) => SendFakeTwitchReward();

        private void dgv_EnabledRewards_SelectionChanged(object sender, EventArgs e)
        {
            if (dgv_EnabledRewards.SelectedRows.Count < 1)
                return;

            var selectedRow = dgv_EnabledRewards.SelectedRows[0];
            var selectedReward = GetSelectedReward(selectedRow);

            if (selectedReward.Name != "Solid color notes")
                return;

            if (selectedReward.AdditionalMsg == null || selectedReward.AdditionalMsg == string.Empty || selectedReward.AdditionalMsg == "Random")
            {
                textBox_SolidNoteColorPicker.BackColor = Color.White;
                textBox_SolidNoteColorPicker.Text = "Random";
            }
            else
            {
                textBox_SolidNoteColorPicker.BackColor = ColorTranslator.FromHtml("#" + selectedReward.AdditionalMsg);
                textBox_SolidNoteColorPicker.Text = "";
            }
        }
    }
}