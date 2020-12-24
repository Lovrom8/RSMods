using System;
using System.Windows.Forms;
using System.IO;
using System.Linq;
using System.Drawing;
using System.Drawing.Text;
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
using RSMods.Twitch.EffectServer;
using System.Runtime.InteropServices;

namespace RSMods
{
    public partial class MainForm : Form
    {

        public MainForm()
        {

            // Locate Rocksmith Folder
            LocateRocksmith(GenUtil.GetRSDirectory());

            // Check if the GUI settings, and DLL settings already exist
            VerifyGUIInstall();

            // Load saved credidentials and enable PubSub
            LoadTwitchSettings();

            // Initialize WinForms
            InitWinForms();

            // Setup bindings for Twitch events
            SetupTwitchTab();

            // Fix Legacy Songlist Bug
            FixLegacySonglistBug();

            // Fill Songlist List
            LoadSonglists();

            // Fill Mod Keybindings List (Fill list box)
            LoadKeybindingModNames();

            // Fill Audio Keybinding List
            LoadAudioKeybindings();

            // Load Mod Keybinding Values
            ShowCurrentKeybindingValues();

            // Load Audio Keybinding Values
            ShowCurrentAudioKeybindingValues();

            // Load Guitar Speak Preset Values
            RefreshGuitarSpeakPresets();

            // Load Checkbox Values From RSMods.ini
            LoadModSettings();

            // Load Default String Colors
            StringColors_LoadDefaultStringColors();

            // Load Colors Saved as Theme Colors.
            CustomTheme_LoadCustomColors();

            // Prevent some double saving
            PreventDoubleSave();

            // Load Set And Forget Mods
            SetForget_LoadSetAndForgetMods();

            // Load All System Fonts
            LoadFonts();
        }

        #region Startup Functions

        private void InitWinForms()
        {
            InitializeComponent();
            Text = $"{Text}-{Assembly.GetExecutingAssembly().GetName().Version}"; // Show version number in the title of the application.
        }

        private void FixLegacySonglistBug()
        {
            if (ReadSettings.ProcessSettings(ReadSettings.Songlist1Identifier) == String.Empty)
                SaveChanges(ReadSettings.Songlist1Identifier, "Define Song List 1 Here");
        }

        private void LocateRocksmith(string RSFolder)
        {

            if (RSFolder == String.Empty)
            {
                MessageBox.Show("We cannot detect where you have Rocksmith located. Please try reinstalling your game on Steam.", "Error: RSLocation Not Found", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Environment.Exit(1);
            }
            else
                Constants.RSFolder = RSFolder;
        }
        private void VerifyGUIInstall()
        {
            WriteSettings.IsVoid(GenUtil.GetRSDirectory());
            if (!File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini")))
                WriteSettings.WriteINI(WriteSettings.Settings); // Creates Settings File

            if (!File.Exists(Constants.SettingsPath))
                File.WriteAllText(Constants.SettingsPath, "RSPath = " + Constants.RSFolder);
        }

        private void LoadSonglists()
        {
            foreach (string songlist in Dictionaries.refreshSonglists())
                listBox_Songlist.Items.Add(songlist);
        }

        private void LoadKeybindingModNames()
        {
            foreach (string mod in Dictionaries.currentModKeypressList)
                listBox_Modlist_MODS.Items.Add(mod);
        }

        private void LoadAudioKeybindings()
        {
            foreach (string volume in Dictionaries.currentAudioKeypressList)
                listBox_Modlist_AUDIO.Items.Add(volume);
        }

        private void PreventDoubleSave()
        {
            // Disable changes while we change them. This prevents us from saving a value we already know.
            listBox_ExtendedRangeTunings.SelectedIndexChanged -= new System.EventHandler(Save_ExtendedRangeTuningAt);
            nUpDown_ForceEnumerationXMS.ValueChanged -= new System.EventHandler(Save_EnumerateEveryXMS);

            // Now we can change things without saving.
            nUpDown_ForceEnumerationXMS.Value = Decimal.Parse(ReadSettings.ProcessSettings(ReadSettings.CheckForNewSongIntervalIdentifier)) / 1000; // Loads old settings for enumeration every x ms
            listBox_ExtendedRangeTunings.SelectedIndex = (Convert.ToInt32(ReadSettings.ProcessSettings(ReadSettings.ExtendedRangeTuningIdentifier)) * -1) - 2; // Loads old ER tuning settings

            // Re-enable the saving of the values now that we've done our work.
            listBox_ExtendedRangeTunings.SelectedIndexChanged += new System.EventHandler(Save_ExtendedRangeTuningAt);
            nUpDown_ForceEnumerationXMS.ValueChanged += new System.EventHandler(Save_EnumerateEveryXMS);
        }

        private void ShowCurrentKeybindingValues()
        {
            label_ToggleLoftKey.Text = "Toggle Loft: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ToggleLoftIdentifier));
            label_SongTimerKey.Text = "Show Song Timer: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerIdentifier));
            label_ReEnumerationKey.Text = "Force ReEnumeration: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationIdentifier));
            label_RainbowStringsKey.Text = "Rainbow Strings: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RainbowStringsIdentifier));
            label_RemoveLyricsKey.Text = "Remove Lyrics: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsKeyIdentifier));
            label_RRSpeedKey.Text = "RR Speed: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RRSpeedKeyIdentifier));
        }

        private void ShowCurrentAudioKeybindingValues()
        {
            label_MasterVolumeKey.Text = "Master Volume: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.MasterVolumeKeyIdentifier));
            label_SongVolumeKey.Text = "Song Volume: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.SongVolumeKeyIdentifier));
            label_Player1VolumeKey.Text = "Player1 Volume: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.Player1VolumeKeyIdentifier));
            label_Player2VolumeKey.Text = "Player2 Volume: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.Player2VolumeKeyIdentifier));
            label_MicrophoneVolumeKey.Text = "Microphone Volume: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.MicrophoneVolumeKeyIdentifier));
            label_VoiceOverVolumeKey.Text = "Voice-Over Volume: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.VoiceOverVolumeKeyIdentifier));
            label_SFXVolumeKey.Text = "SFX Volume: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.SFXVolumeKeyIdentifier));
            label_ChangeSelectedVolumeKey.Text = "Show Volume On Screen: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ChangeSelectedVolumeKeyIdentifier));
        }

        #endregion
        #region Show Settings In GUI
        private void LoadModSettings()
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

            if (ReadSettings.ProcessSettings(ReadSettings.VolumeControlEnabledIdentifier) == "on") // Add Volume Enabled / Disabled
            {
                checkBox_ControlVolume.Checked = true;
                groupBox_Keybindings_AUDIO.Visible = true;
                groupBox_ControlVolume.Visible = true;
                nUpDown_VolumeInterval.Value = Convert.ToInt32(ReadSettings.ProcessSettings(ReadSettings.VolumeControlIntervalIdentifier));
            }

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

                if (ReadSettings.ProcessSettings(ReadSettings.ExtendedRangeDropTuningIdentifier) == "on") // Extended Range on Drop Tuning
                    checkBox_ExtendedRangeDrop.Checked = true;
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
                checkBox_RemoveLaneMarkers.Checked = true;

            if (ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsIdentifier) == "on") // Remove Lyrics
            {
                checkBox_RemoveLyrics.Checked = true;
                groupBox_ToggleLyricsOffWhen.Visible = true;

                if (ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsWhenIdentifier) == "startup") // Remove Lyrics When ...
                    radio_LyricsAlwaysOff.Checked = true;
                else if (ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsWhenIdentifier) == "manual") // Remove Lyrics When ...
                    radio_LyricsOffHotkey.Checked = true;
            }

            if (ReadSettings.ProcessSettings(ReadSettings.ScreenShotScoresIdentifier) == "on")
                checkBox_ScreenShotScores.Checked = true;

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

            if (ReadSettings.ProcessSettings(ReadSettings.RiffRepeaterAboveHundredIdentifier) == "on")
            {
                checkBox_RiffRepeaterSpeedAboveOneHundred.Checked = true;
                groupBox_RRSpeed.Visible = true;
                nUpDown_RiffRepeaterSpeed.Value = Convert.ToDecimal(ReadSettings.ProcessSettings(ReadSettings.RiffRepeaterSpeedIntervalIdentifier));
            }

            if (ReadSettings.ProcessSettings(ReadSettings.MidiAutoTuningIdentifier) == "on")
            {
                checkBox_useMidiAutoTuning.Checked = true;
                groupBox_MidiAutoTuneDevice.Visible = true;
                label_SelectedMidiDevice.Text = "Midi Device: " + ReadSettings.ProcessSettings(ReadSettings.MidiAutoTuningDeviceIdentifier);

                if (ReadSettings.ProcessSettings(ReadSettings.TuningPedalIdentifier) != "")
                {
                    int tuningPedal = Convert.ToInt32(ReadSettings.ProcessSettings(ReadSettings.TuningPedalIdentifier));

                    switch (tuningPedal)
                    {
                        case 1:
                            radio_WhammyDT.Checked = true;
                            break;
                        case 2:
                            radio_WhammyORBass.Checked = true;
                            checkBox_WhammyChordsMode.Visible = true;
                            break;
                        default:
                            break;
                    }

                }

                if (ReadSettings.ProcessSettings(ReadSettings.ChordsModeIdentifier) == "on")
                    checkBox_WhammyChordsMode.Checked = true;
            }

            if (ReadSettings.ProcessSettings(ReadSettings.ShowCurrentNoteOnScreenIdentifier) == "on")
                checkBox_ShowCurrentNote.Checked = true;
        }

        #endregion
        #region Custom Themes

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

        private void CustomTheme_ChangeTheme(Color backgroundColor, Color textColor)
        {
            GetAllControls(TabController);
            BackColor = backgroundColor; // MainForm BackColor
            ForeColor = textColor; // MainForm ForeColor

            foreach (Control controlToChange in ControlList)
            {
                controlToChange.ForeColor = textColor;
                controlToChange.BackColor = backgroundColor;
            }

            CustomTheme_DataGridView(dgv_DefaultRewards, backgroundColor, textColor);
            CustomTheme_DataGridView(dgv_EnabledRewards, backgroundColor, textColor);

            // Twitch Log. Can't be done automatically or it will break other text boxes :(
            textBox_TwitchLog.ForeColor = textColor;
            textBox_TwitchLog.BackColor = backgroundColor;
        }

        private void CustomTheme_DataGridView(DataGridView grid, Color backgroundColor, Color textColor)
        {
            grid.EnableHeadersVisualStyles = false; // Allows us to customize the color scheme

            // Background Colors
            grid.BackgroundColor = backgroundColor;
            grid.ColumnHeadersDefaultCellStyle.BackColor = backgroundColor;
            grid.ColumnHeadersDefaultCellStyle.SelectionBackColor = backgroundColor;
            grid.DefaultCellStyle.SelectionBackColor = backgroundColor;

            // Foreground Colors
            grid.ForeColor = textColor;
            grid.DefaultCellStyle.SelectionForeColor = textColor;
            grid.ColumnHeadersDefaultCellStyle.SelectionForeColor = textColor;
            grid.ColumnHeadersDefaultCellStyle.ForeColor = textColor;
        }

        private void CustomTheme_LoadCustomColors()
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

            CustomTheme_ChangeTheme(textBox_ChangeBackgroundColor.BackColor, textBox_ChangeTextColor.BackColor);
        }

        private void CustomTheme_ChangeTheme(object sender, EventArgs e)
        {
            SaveChanges(ReadSettings.CustomGUIThemeIdentifier, checkBox_ChangeTheme.Checked.ToString().ToLower());
            groupBox_ChangeTheme.Visible = checkBox_ChangeTheme.Checked;

            if (!checkBox_ChangeTheme.Checked) // Turning off custom themes
                CustomTheme_ChangeTheme(WriteSettings.defaultBackgroundColor, WriteSettings.defaultTextColor);
        }

        private void CustomTheme_ChangeBackgroundColor(object sender, EventArgs e)
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

        private void CustomTheme_ChangeTextColor(object sender, EventArgs e)
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

        private void Apply_ThemeColors(object sender, EventArgs e) => CustomTheme_ChangeTheme(textBox_ChangeBackgroundColor.BackColor, textBox_ChangeTextColor.BackColor);

        #endregion
        #region Check For Keypresses (Keybindings)
        private void CheckKeyPressesDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter) // If enter is pressed
            {
                e.SuppressKeyPress = true; // Turns off the windows beep for pressing an invalid key.
                Save_Songlists_Keybindings(sender, e);
            }

            else if (((TextBox)sender) == textBox_NewKeyAssignment_MODS)
            {
                e.SuppressKeyPress = true; // Turns off the windows beep for pressing an invalid key.

                if (KeyConversion.KeyDownDictionary.Contains(e.KeyCode))
                    textBox_NewKeyAssignment_MODS.Text = e.KeyCode.ToString();

                else if ((e.KeyValue > 47 && e.KeyValue < 60) || (e.KeyValue > 64 && e.KeyValue < 91)) // Number or Letter was pressed (Will be overrided by text input)
                {
                    if (MessageBox.Show("The key you entered is currently used by Rocksmith and may interfere with being able to use the game properly. Are you sure you want to use this keybinding?", "Keybinding Warning!", MessageBoxButtons.OKCancel, MessageBoxIcon.Warning) == DialogResult.OK)
                        return;
                    else
                        textBox_NewKeyAssignment_MODS.Text = "";
                }
            }
            else if (((TextBox)sender) == textBox_NewKeyAssignment_AUDIO)
            {
                e.SuppressKeyPress = true; // Turns off the windows beep for pressing an invalid key.

                if (KeyConversion.KeyDownDictionary.Contains(e.KeyCode))
                    textBox_NewKeyAssignment_AUDIO.Text = e.KeyCode.ToString();

                else if ((e.KeyValue > 47 && e.KeyValue < 60) || (e.KeyValue > 64 && e.KeyValue < 91)) // Number or Letter was pressed (Will be overrided by text input)
                {
                    if (MessageBox.Show("The key you entered is currently used by Rocksmith and may interfere with being able to use the game properly. Are you sure you want to use this keybinding?", "Keybinding Warning!", MessageBoxButtons.OKCancel, MessageBoxIcon.Warning) == DialogResult.OK)
                        return;
                    else
                        textBox_NewKeyAssignment_AUDIO.Text = "";
                }
            }
        }

        private void CheckKeyPressesUp(object sender, KeyEventArgs e)
        {
            if (KeyConversion.KeyUpDictionary.Contains(e.KeyCode))
            {
                if (sender == textBox_NewKeyAssignment_MODS)
                    textBox_NewKeyAssignment_MODS.Text = e.KeyCode.ToString();
                else if (sender == textBox_NewKeyAssignment_AUDIO)
                    textBox_NewKeyAssignment_AUDIO.Text = e.KeyCode.ToString();
            }

        }

        private void CheckMouseInput(object sender, MouseEventArgs e)
        {
            if (KeyConversion.MouseButtonDictionary.Contains(e.Button))
            {
                if (sender == textBox_NewKeyAssignment_MODS)
                    textBox_NewKeyAssignment_MODS.Text = e.Button.ToString();
                else if (sender == textBox_NewKeyAssignment_AUDIO)
                    textBox_NewKeyAssignment_AUDIO.Text = e.Button.ToString();
            }

        }

        private void LoadPreviousKeyAssignment(object sender, EventArgs e) => textBox_NewKeyAssignment_MODS.Text = Dictionaries.refreshKeybindingList()[listBox_Modlist_MODS.SelectedIndex];

        private void LoadPreviousVolumeAssignment(object sender, EventArgs e) => textBox_NewKeyAssignment_AUDIO.Text = Dictionaries.refreshAudioKeybindingList()[listBox_Modlist_AUDIO.SelectedIndex];
        #endregion
        #region Reset To Default
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
        #endregion
        #region Save Settings
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
            ShowSavedSettingsLabel();
            WinMsgUtil.SendMsgToRS("update all");
        }

        private void ShowSavedSettingsLabel()
        {
            label_SettingsSaved.Visible = true;
            System.Windows.Forms.Timer timer = new System.Windows.Forms.Timer();
            timer.Interval = 1500;
            timer.Tick += (source, e) => { label_SettingsSaved.Visible = false; timer.Stop(); };
            timer.Start();
        }

        private void Save_Songlists_Keybindings(object sender, EventArgs e) // Save Songlists and Keybindings when pressing Enter
        {
            TextBox textBox = ((TextBox)sender);

            // Song Lists
            if (textBox.Name == textBox_NewSonglistName.Name)
            {

                foreach (string currentSongList in Dictionaries.SongListIndexToINISetting)
                {
                    int index = Dictionaries.SongListIndexToINISetting.IndexOf(currentSongList);

                    if (textBox_NewSonglistName.Text.Trim() == "") // The game UI will break with a blank name.
                    {
                        MessageBox.Show("You cannot save a blank song list name as the game will break", "Warning!", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        break;
                    }
                    else if (index == listBox_Songlist.SelectedIndex)
                    {
                        SaveChanges(currentSongList, textBox_NewSonglistName.Text);
                        listBox_Songlist.Items[index] = textBox_NewSonglistName.Text;
                        break;
                    };
                }
            }

            // Mod Keybindings
            if (textBox.Name == textBox_NewKeyAssignment_MODS.Name)
            {
                foreach (string currentKeybinding in Dictionaries.KeybindingsIndexToINISetting)
                {
                    int index = Dictionaries.KeybindingsIndexToINISetting.IndexOf(currentKeybinding);

                    if (textBox_NewKeyAssignment_MODS.Text == "")
                    {
                        MessageBox.Show("You cannot set a blank keybind", "Warning!", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        break;
                    }
                    else if (index == listBox_Modlist_MODS.SelectedIndex)
                    {
                        SaveChanges(currentKeybinding, KeyConversion.VirtualKey(textBox_NewKeyAssignment_MODS.Text));
                        break;
                    }
                }

                textBox_NewKeyAssignment_MODS.Text = String.Empty;
            }

            // Audio Keybindings
            if (textBox.Name == textBox_NewKeyAssignment_AUDIO.Name)
            {
                foreach (string currentKeybinding in Dictionaries.AudioKeybindingsIndexToINISetting)
                {
                    int index = Dictionaries.AudioKeybindingsIndexToINISetting.IndexOf(currentKeybinding);

                    if (textBox_NewKeyAssignment_AUDIO.Text == "")
                    {
                        MessageBox.Show("You cannot set a blank keybind", "Warning!", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        break;
                    }
                    else if (index == listBox_Modlist_AUDIO.SelectedIndex)
                    {
                        SaveChanges(currentKeybinding, KeyConversion.VirtualKey(textBox_NewKeyAssignment_AUDIO.Text));
                        break;
                    }
                }

                textBox_NewKeyAssignment_AUDIO.Text = String.Empty;
            }
            ShowCurrentKeybindingValues();
            ShowCurrentAudioKeybindingValues();
        }
        #endregion
        #region String Colors

        private void StringColors_ChangeStringColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog
            {
                AllowFullOpen = true,
                ShowHelp = false
            };
            bool isNormalStrings = radio_DefaultStringColors.Checked; // True = Normal, False = Colorblind
            string stringColorButtonIdentifier = String.Empty;
            int stringNumber = 0;
            StringColors_FillStringNumberToColorDictionary();

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
                stringNumberToColorTextBox[stringNumber].BackColor = colorDialog.Color;
            }
        }

        private void StringColors_LoadDefaultStringColors(bool colorBlind = false)
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

        private void StringColors_DefaultStringColors(object sender, EventArgs e) => StringColors_LoadDefaultStringColors();

        private void StringColors_ColorBlindStringColors(object sender, EventArgs e) => StringColors_LoadDefaultStringColors(true);

        #endregion
        #region Prep Set And Forget Mods
        // SetAndForget Mods

        private void SetForget_FillUI()
        {
            listBox_Tunings.Items.Clear();
            SetAndForgetMods.TuningsCollection = SetAndForgetMods.LoadTuningsCollection();

            listBox_Tunings.Items.Add("<New>");
            foreach (var key in SetAndForgetMods.TuningsCollection.Keys)
                listBox_Tunings.Items.Add(key);
        }

        private TuningDefinitionInfo SetForget_GetCurrentTuningInfo()
        {
            var tuningDefinition = new TuningDefinitionInfo();
            var strings = new Dictionary<string, int>();

            for (int strIdx = 0; strIdx < 6; strIdx++)
                strings[$"string{strIdx}"] = (int)((NumericUpDown)groupBox_SetAndForget.Controls[$"nUpDown_String{strIdx}"]).Value;

            tuningDefinition.Strings = strings;
            tuningDefinition.UIName = String.Format("$[{0}]{1}", nUpDown_UIIndex.Value.ToString(), textBox_UIName.Text);

            return tuningDefinition;
        }
        private void SetForget_LoadSetAndForgetMods()
        {
            SetAndForgetMods.LoadDefaultFiles();
            SetForget_FillUI();
        }
        #endregion
        #region Set And Forget UI Functions

        private void SetForget_RestoreDefaults(object sender, EventArgs e)
        {
            if (SetAndForgetMods.RestoreDefaults())
                SetForget_FillUI();
        }

        private void SetForget_UnpackCacheAgain(object sender, EventArgs e) => SetAndForgetMods.CleanUnpackedCache();

        private void SetForget_AddCustomTunings(object sender, EventArgs e) => SetAndForgetMods.AddCustomTunings();

        private void SetForget_AddFastLoadMod(object sender, EventArgs e) => SetAndForgetMods.AddFastLoadMod();

        private void SetForget_ListTunings(object sender, EventArgs e)
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

        private void SetForget_SaveTuningChanges(object sender, EventArgs e)
        {
            if (listBox_Tunings.SelectedIndex != -1) // If we are saving a change to the currently selected tuning, perform a change in the collection, otherwise directly go to saving
            {
                string selectedItem = listBox_Tunings.SelectedItem.ToString();

                if (selectedItem != "<New>")
                    SetAndForgetMods.TuningsCollection[selectedItem] = SetForget_GetCurrentTuningInfo();
            }

            SetAndForgetMods.SaveTuningsJSON();

            MessageBox.Show("Saved current tuning, don't forget to press \"Add Custom Tunings\" button when you are done!", "Success");
        }

        private void SetForget_RemoveTuning(object sender, EventArgs e)
        {
            if (listBox_Tunings.SelectedIndex == -1)
                return;

            string selectedItem = listBox_Tunings.SelectedItem.ToString();

            if (selectedItem == "<New>")
                return;

            SetAndForgetMods.TuningsCollection.Remove(selectedItem); // I guess we would be better here using BindingSource on Listbox + ObservableCollection instead of Dict to make changes reflect automatically, but... one day
            listBox_Tunings.Items.Remove(selectedItem);
        }

        private void SetForget_AddTuning(object sender, EventArgs e)
        {
            if (listBox_Tunings.SelectedIndex == -1)
                listBox_Tunings.SelectedIndex = 0;

            if (listBox_Tunings.SelectedItem.ToString() != "<New>")
                return;

            var currTuning = SetForget_GetCurrentTuningInfo();
            string internalName = textBox_InternalTuningName.Text;

            if (internalName.Trim() == "")
            {
                MessageBox.Show("You cannot have a blank internal name.");
                return;
            }

            if (!SetAndForgetMods.TuningsCollection.ContainsKey(internalName)) // Unlikely to happen, but still... prevent users accidentaly trying to add existing stuff
            {
                SetAndForgetMods.TuningsCollection.Add(internalName, currTuning);
                listBox_Tunings.Items.Add(internalName);
            }
            else
                MessageBox.Show("You already have a tuning with the same internal name");
        }

        private void SetForget_AddCustomMenu(object sender, EventArgs e) => SetAndForgetMods.AddExitGameMenuOption();

        private void SetForget_AddDCMode(object sender, EventArgs e) => SetAndForgetMods.AddDirectConnectModeOption();

        private void SetForget_RemoveTempFolders(object sender, EventArgs e) => SetAndForgetMods.RemoveTempFolders();

        private void SetForget_SetDefaultTones(object sender, EventArgs e)
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

        private void SetForget_LoadTonesFromProfiles(object sender, EventArgs e)
        {
            var profileTones = SetAndForgetMods.GetSteamProfilesFolder();

            if (profileTones.Count > 0)
            {
                listBox_ProfileTones.Items.Clear();

                profileTones.ForEach(t => listBox_ProfileTones.Items.Add(t));
            }
        }

        private void SetForget_ImportExistingSettings(object sender, EventArgs e)
        {
            if (SetAndForgetMods.ImportExistingSettings())
                SetForget_FillUI();
        }

        #endregion
        #region Save Setting Middleware

        private void Save_ToggleLoft(object sender, EventArgs e) // Toggle Loft Enabled/ Disabled
        {
            SaveChanges(ReadSettings.ToggleLoftEnabledIdentifier, checkBox_ToggleLoft.Checked.ToString().ToLower());
            checkBox_ToggleLoft.Checked = checkBox_ToggleLoft.Checked;
            radio_LoftAlwaysOff.Visible = checkBox_ToggleLoft.Checked;
            radio_LoftOffHotkey.Visible = checkBox_ToggleLoft.Checked;
            radio_LoftOffInSong.Visible = checkBox_ToggleLoft.Checked;
            groupBox_LoftOffWhen.Visible = checkBox_ToggleLoft.Checked;
        }

        private void Save_SongTimer(object sender, EventArgs e) => SaveChanges(ReadSettings.ShowSongTimerEnabledIdentifier, checkBox_SongTimer.Checked.ToString().ToLower());

        private void Save_ForceEnumeration(object sender, EventArgs e)
        {
            checkBox_ForceEnumeration.Checked = checkBox_ForceEnumeration.Checked;
            radio_ForceEnumerationAutomatic.Visible = checkBox_ForceEnumeration.Checked;
            radio_ForceEnumerationManual.Visible = checkBox_ForceEnumeration.Checked;
            groupBox_HowToEnumerate.Visible = checkBox_ForceEnumeration.Checked;

            if (checkBox_ForceEnumeration.Checked)
                SaveChanges(ReadSettings.ForceReEnumerationEnabledIdentifier, "manual");
            else
                SaveChanges(ReadSettings.ForceReEnumerationEnabledIdentifier, "false");
        }

        private void Save_EnumerateEveryXMS(object sender, EventArgs e) => SaveChanges(ReadSettings.CheckForNewSongIntervalIdentifier, (nUpDown_ForceEnumerationXMS.Value * 1000).ToString());

        private void Save_ForceEnumerationAutomatic(object sender, EventArgs e)
        {
            label_ForceEnumerationXMS.Visible = true;
            nUpDown_ForceEnumerationXMS.Visible = true;
            SaveChanges(ReadSettings.ForceReEnumerationEnabledIdentifier, "automatic");
        }

        private void Save_ForceEnumerationManual(object sender, EventArgs e)
        {
            label_ForceEnumerationXMS.Visible = false;
            nUpDown_ForceEnumerationXMS.Visible = false;
            SaveChanges(ReadSettings.ForceReEnumerationEnabledIdentifier, "manual");
        }

        private void Save_RainbowStrings(object sender, EventArgs e) => SaveChanges(ReadSettings.RainbowStringsEnabledIdentifier, checkBox_RainbowStrings.Checked.ToString().ToLower());

        private void Save_ExtendedRange(object sender, EventArgs e)
        {

            groupBox_ExtendedRangeWhen.Visible = checkBox_ExtendedRange.Checked;
            listBox_ExtendedRangeTunings.Visible = checkBox_ExtendedRange.Checked;
            checkBox_CustomColors.Checked = checkBox_ExtendedRange.Checked;

            SaveChanges(ReadSettings.ExtendedRangeEnabledIdentifier, checkBox_ExtendedRange.Checked.ToString().ToLower());

            if (checkBox_ExtendedRange.Checked)
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "2");
            else
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "0");
        }

        private void Save_CustomStringColors(object sender, EventArgs e)
        {
            groupBox_StringColors.Visible = checkBox_CustomColors.Checked;

            if (checkBox_CustomColors.Checked)
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "2");
            else
                SaveChanges(ReadSettings.CustomStringColorNumberIndetifier, "0");
        }

        // private void Save_DiscoMode(object sender, EventArgs e) => SaveChanges(ReadSettings.DiscoModeIdentifier, DiscoModeCheckbox.Checked.ToString().ToLower());

        private void Save_RemoveHeadstockCheckbox(object sender, EventArgs e)
        {
            SaveChanges(ReadSettings.RemoveHeadstockIdentifier, checkBox_RemoveHeadstock.Checked.ToString().ToLower());
            groupBox_ToggleHeadstockOffWhen.Visible = checkBox_RemoveHeadstock.Checked;
        }

        private void Save_RemoveSkyline(object sender, EventArgs e)
        {
            SaveChanges(ReadSettings.RemoveSkylineIdentifier, checkBox_RemoveSkyline.Checked.ToString().ToLower());
            groupBox_ToggleSkylineWhen.Visible = checkBox_RemoveSkyline.Checked;
        }

        private void Save_GreenScreenWall(object sender, EventArgs e) => SaveChanges(ReadSettings.GreenScreenWallIdentifier, checkBox_GreenScreen.Checked.ToString().ToLower());

        private void Save_AutoLoadProfile(object sender, EventArgs e) => SaveChanges(ReadSettings.ForceProfileEnabledIdentifier, checkBox_AutoLoadProfile.Checked.ToString().ToLower());

        private void Save_Fretless(object sender, EventArgs e) => SaveChanges(ReadSettings.FretlessModeEnabledIdentifier, checkBox_Fretless.Checked.ToString().ToLower());

        private void Save_RemoveInlays(object sender, EventArgs e) => SaveChanges(ReadSettings.RemoveInlaysIdentifier, checkBox_RemoveInlays.Checked.ToString().ToLower());

        private void Save_ToggleLoftWhenManual(object sender, EventArgs e)
        {
            if (radio_LoftOffHotkey.Checked)
                SaveChanges(ReadSettings.ToggleLoftWhenIdentifier, "manual");
        }

        private void Save_ToggleLoftWhenSong(object sender, EventArgs e)
        {
            if (radio_LoftOffInSong.Checked)
                SaveChanges(ReadSettings.ToggleLoftWhenIdentifier, "song");
        }

        private void Save_ToggleLoftWhenStartup(object sender, EventArgs e)
        {
            if (radio_LoftAlwaysOff.Checked)
                SaveChanges(ReadSettings.ToggleLoftWhenIdentifier, "startup");
        }

        private void Save_RemoveLaneMarkers(object sender, EventArgs e) => SaveChanges(ReadSettings.RemoveLaneMarkersIdentifier, checkBox_RemoveLaneMarkers.Checked.ToString().ToLower());
        private void Save_ToggleSkylineSong(object sender, EventArgs e)
        {
            if (radio_SkylineOffInSong.Checked)
                SaveChanges(ReadSettings.ToggleSkylineWhenIdentifier, "song");
        }

        private void Save_ToggleSkylineStartup(object sender, EventArgs e)
        {
            if (radio_SkylineAlwaysOff.Checked)
                SaveChanges(ReadSettings.ToggleSkylineWhenIdentifier, "startup");
        }

        private void Save_ExtendedRangeTuningAt(object sender, EventArgs e) => SaveChanges(ReadSettings.ExtendedRangeTuningIdentifier, Convert.ToString((listBox_ExtendedRangeTunings.SelectedIndex * -1) - 2));

        private void Delete_Keybind_MODS(object sender, EventArgs e)
        {
            textBox_NewKeyAssignment_MODS.Text = "";

            foreach (string currentMod in Dictionaries.KeybindingsIndexToINISetting)
            {
                int index = Dictionaries.KeybindingsIndexToINISetting.IndexOf(currentMod);
                if (index == listBox_Modlist_MODS.SelectedIndex)
                {
                    SaveChanges(currentMod, "");
                    break;
                }
            }
            ShowCurrentKeybindingValues();
        }

        private void Delete_Keybind_AUDIO(object sender, EventArgs e)
        {
            textBox_NewKeyAssignment_AUDIO.Text = "";

            foreach (string currentMod in Dictionaries.AudioKeybindingsIndexToINISetting)
            {
                int index = Dictionaries.AudioKeybindingsIndexToINISetting.IndexOf(currentMod);
                if (index == listBox_Modlist_AUDIO.SelectedIndex)
                {
                    SaveChanges(currentMod, "");
                    break;
                }
            }
            ShowCurrentAudioKeybindingValues();
        }

        private void Save_RemoveLyrics(object sender, EventArgs e)
        {
            SaveChanges(ReadSettings.RemoveLyricsIdentifier, checkBox_RemoveLyrics.Checked.ToString().ToLower());
            radio_LyricsAlwaysOff.Visible = checkBox_RemoveLyrics.Checked;
            radio_LyricsOffHotkey.Visible = checkBox_RemoveLyrics.Checked;
            groupBox_ToggleLyricsOffWhen.Visible = checkBox_RemoveLyrics.Checked;
        }

        private void Fill_Songlist_Name(object sender, EventArgs e)
        {
            if (listBox_Songlist.SelectedIndex >= 0)
                textBox_NewSonglistName.Text = listBox_Songlist.SelectedItem.ToString();
        }

        private void Save_ToggleLyricsStartup(object sender, EventArgs e)
        {
            if (radio_LyricsAlwaysOff.Checked)
                SaveChanges(ReadSettings.RemoveLyricsWhenIdentifier, "startup");
        }

        private void Save_ToggleLyricsManual(object sender, EventArgs e)
        {
            if (radio_LyricsOffHotkey.Checked)
                SaveChanges(ReadSettings.RemoveLyricsWhenIdentifier, "manual");
        }

        private void Save_VolumeControls(object sender, EventArgs e)
        {
            groupBox_Keybindings_AUDIO.Visible = checkBox_ControlVolume.Checked;
            groupBox_ControlVolume.Visible = checkBox_ControlVolume.Checked;
            SaveChanges(ReadSettings.VolumeControlEnabledIdentifier, checkBox_ControlVolume.Checked.ToString().ToLower());
        }

        private void Save_HeadstockOffInSongOnlyButton(object sender, EventArgs e)
        {
            if (radio_HeadstockOffInSong.Checked)
                SaveChanges(ReadSettings.RemoveHeadstockWhenIdentifier, "song");
        }

        private void Save_RiffRepeaterSpeedInterval(object sender, EventArgs e) => SaveChanges(ReadSettings.RiffRepeaterSpeedIntervalIdentifier, nUpDown_RiffRepeaterSpeed.Value.ToString());

        private void Save_RiffRepeaterSpeedAboveOneHundred(object sender, EventArgs e)
        {
            SaveChanges(ReadSettings.RiffRepeaterAboveHundredIdentifier, checkBox_RiffRepeaterSpeedAboveOneHundred.Checked.ToString().ToLower());
            groupBox_RRSpeed.Visible = checkBox_RiffRepeaterSpeedAboveOneHundred.Checked;
        }

        private void Save_useMidiAutoTuning(object sender, EventArgs e)
        {
            SaveChanges(ReadSettings.MidiAutoTuningIdentifier, checkBox_useMidiAutoTuning.Checked.ToString().ToLower());
            groupBox_MidiAutoTuneDevice.Visible = checkBox_useMidiAutoTuning.Checked;
        }
        private void Save_AutoTuneDevice(object sender, EventArgs e)
        {
            if (listBox_ListMidiDevices.SelectedIndex != -1)
            {
                SaveChanges(ReadSettings.MidiAutoTuningDeviceIdentifier, listBox_ListMidiDevices.SelectedItem.ToString());
                label_SelectedMidiDevice.Text = "Midi Device: " + listBox_ListMidiDevices.SelectedItem.ToString();
            }
        }

        private void Save_WhammyDT(object sender, EventArgs e) => SaveChanges(ReadSettings.TuningPedalIdentifier, "1");

        private void Save_WhammyORBass(object sender, EventArgs e)
        {
            SaveChanges(ReadSettings.TuningPedalIdentifier, "2");
            checkBox_WhammyChordsMode.Visible = radio_WhammyORBass.Checked;
        }

        private void Save_WhammyChordsMode(object sender, EventArgs e) => SaveChanges(ReadSettings.ChordsModeIdentifier, checkBox_WhammyChordsMode.Checked.ToString().ToLower());

        private void Save_ExtendedRangeDrop(object sender, EventArgs e) => SaveChanges(ReadSettings.ExtendedRangeDropTuningIdentifier, checkBox_ExtendedRangeDrop.Checked.ToString().ToLower());

        private void Save_ShowCurrentNote(object sender, EventArgs e) => SaveChanges(ReadSettings.ShowCurrentNoteOnScreenIdentifier, checkBox_ShowCurrentNote.Checked.ToString().ToLower());

        private void Save_ScreenShotScores(object sender, EventArgs e) => SaveChanges(ReadSettings.ScreenShotScoresIdentifier, checkBox_ScreenShotScores.Checked.ToString().ToLower());

        private void Save_HeadStockAlwaysOffButton(object sender, EventArgs e)
        {
            if (radio_HeadstockAlwaysOff.Checked)
                SaveChanges(ReadSettings.RemoveHeadstockWhenIdentifier, "startup");
        }

        private void Save_VolumeInterval(object sender, EventArgs e) => SaveChanges(ReadSettings.VolumeControlIntervalIdentifier, Convert.ToInt32(nUpDown_VolumeInterval.Value).ToString());
        #endregion
        #region Tooltips

        bool CreatedToolTipYet = false;

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

            TooltipDictionary.Clear();
            FillToolTipDictionary();

            foreach (Control ControlHoveredOver in TooltipDictionary.Keys)
            {
                if (ControlHoveredOver == sender)
                {
                    TooltipDictionary.TryGetValue(ControlHoveredOver, out string toolTipString);
                    currentTooltip.Show(toolTipString, ControlHoveredOver, 5000000); // Don't change the duration number, even if it's higher. It works as it is, and changing it to even Int32.MaxValue causes it to go back to the 5-second max.
                    CreatedToolTipYet = true;
                    break; // We found what we needed, now GTFO of here.
                }
            }
        }

        #endregion
        #region Guitar Speak Functions
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
                    int offset = 40; // E2 (Midi)
                    label_CustomTuningLowEStringLetter.Text = GuitarSpeak.GuitarSpeakNoteOctaveMath(Convert.ToString(Convert.ToInt32(nUpDown_String0.Value) + offset));
                    break;
                case 1:
                    offset = 45; // A2 (Midi)
                    label_CustomTuningAStringLetter.Text = GuitarSpeak.GuitarSpeakNoteOctaveMath(Convert.ToString(Convert.ToInt32(nUpDown_String1.Value) + offset));
                    break;
                case 2:
                    offset = 50; // D3 (Midi)
                    label_CustomTuningDStringLetter.Text = GuitarSpeak.GuitarSpeakNoteOctaveMath(Convert.ToString(Convert.ToInt32(nUpDown_String2.Value) + offset));
                    break;
                case 3:
                    offset = 55;// G3 (Midi)
                    label_CustomTuningGStringLetter.Text = GuitarSpeak.GuitarSpeakNoteOctaveMath(Convert.ToString(Convert.ToInt32(nUpDown_String3.Value) + offset));
                    break;
                case 4:
                    offset = 59; // B3 (Midi)
                    label_CustomTuningBStringLetter.Text = GuitarSpeak.GuitarSpeakNoteOctaveMath(Convert.ToString(Convert.ToInt32(nUpDown_String4.Value) + offset));
                    break;
                case 5:
                    offset = 64; // E4 (Midi)
                    label_CustomTuningHighEStringLetter.Text = GuitarSpeak.GuitarSpeakNoteOctaveMath(Convert.ToString(Convert.ToInt32(nUpDown_String5.Value) + offset)).ToLower();
                    break;
                default: // Yeah we don't know wtf happened here
                    MessageBox.Show("Invalid String Number! Please report this to the GUI devs!");
                    break;
            }
        }

        

        private void GuitarSpeakWhileTuningBox_CheckedChanged(object sender, EventArgs e) => SaveChanges(ReadSettings.GuitarSpeakTuningIdentifier, checkbox_GuitarSpeakWhileTuning.Checked.ToString().ToLower());

        private void GuitarSpeakHelpButton_Click(object sender, EventArgs e) => System.Diagnostics.Process.Start("https://pastebin.com/raw/PZ0FQTn0");
        

        private void RefreshGuitarSpeakPresets()
        {
            listBox_GuitarSpeakPresets.Items.Clear();

            foreach (KeyValuePair<string, string> guitarSpeakKeypress in Dictionaries.RefreshGuitarSpeakPresets())
                listBox_GuitarSpeakPresets.Items.Add(guitarSpeakKeypress.Key + guitarSpeakKeypress.Value);
        }

        #endregion
        #region Prep Twitch Tab
        private void EnableTwitchTab()
        {
            foreach (Control ctrl in tab_Twitch.Controls)
                ctrl.Visible = true;

            foreach (DataGridViewRow row in dgv_EnabledRewards.Rows)
            {
                if (row.Cells[1].Value.ToString() == "Solid color notes")
                {
                    var selectedReward = Twitch_GetSelectedReward(row);

                    if (selectedReward.AdditionalMsg != null && selectedReward.AdditionalMsg != string.Empty && selectedReward.AdditionalMsg != "Random")
                        row.DefaultCellStyle.BackColor = ColorTranslator.FromHtml("#" + selectedReward.AdditionalMsg);

                    Twitch_CheckForTurboSpeed(selectedReward);
                }
            }

            Twitch_ShowSolidNoteColorRelatedStuff(false);
        }

        private void SetupTwitchTab()
        {
            label_TwitchUsernameVal.DataBindings.Add(new Binding("Text", TwitchSettings.Get, "Username", false, DataSourceUpdateMode.OnPropertyChanged));
            label_TwitchChannelIDVal.DataBindings.Add(new Binding("Text", TwitchSettings.Get, "ChannelID", false, DataSourceUpdateMode.OnPropertyChanged));
            label_TwitchAccessTokenVal.DataBindings.Add(new Binding("Text", TwitchSettings.Get, "AccessToken", false, DataSourceUpdateMode.OnPropertyChanged));

            // Hide values by default (Security just incase the streamer is live with RSMods on screen)
            label_TwitchUsernameVal.DataBindings.Add(new Binding("Visible", checkBox_RevealTwitchAuthToken, "Checked", false, DataSourceUpdateMode.OnPropertyChanged));
            label_TwitchChannelIDVal.DataBindings.Add(new Binding("Visible", checkBox_RevealTwitchAuthToken, "Checked", false, DataSourceUpdateMode.OnPropertyChanged));
            label_TwitchAccessTokenVal.DataBindings.Add(new Binding("Visible", checkBox_RevealTwitchAuthToken, "Checked", false, DataSourceUpdateMode.OnPropertyChanged));

            textBox_TwitchLog.DataBindings.Add(new Binding("Text", TwitchSettings.Get, "Log"));

            Binding listeningToTwitchBinding = new Binding("Text", TwitchSettings.Get, "Authorized");
            listeningToTwitchBinding.Format += (s, e) =>
            {
                if ((bool)e.Value && TwitchSettings.Get.Reauthorized) // If we are authorized
                {
                    PubSub.Get.SetUp(); // Well... this is probably not the best place since it's called a lot, but wing it
                    TwitchSettings.Get.Reauthorized = false;
                    timerValidateTwitch.Enabled = true;
                    EnableTwitchTab();
                }

                e.Value = (bool)e.Value ? "Listening to Twitch events" : "Not listening to twitch events";
            };
            label_IsListeningToEvents.DataBindings.Add(listeningToTwitchBinding);

            checkBox_TwitchForceReauth.Checked = TwitchSettings.Get.ForceReauth;

            foreach (var defaultReward in TwitchSettings.Get.DefaultRewards) // BindingList... yeah, not yet
                dgv_DefaultRewards.Rows.Add(defaultReward.Name, defaultReward.Description);

            foreach (var enabledReward in TwitchSettings.Get.Rewards)
                Twitch_AddToSelectedRewards(enabledReward);

        }
        
        private void LoadTwitchSettings()
        {
            TwitchSettings.Get._context = SynchronizationContext.Current;
            TwitchSettings.Get.LoadSettings();
            TwitchSettings.Get.LoadDefaultEffects();
            TwitchSettings.Get.LoadEnabledEffects();
        }
        #endregion
        #region Twitch UI Functions
        private void Twitch_ReAuthorize_Click(object sender, EventArgs e)
        {
            ImplicitAuth auth = new ImplicitAuth();
            auth.MakeAuthRequest();

            // string authToken = TwitchSettings.Get.AccessToken;
            // while (TwitchSettings.Get.AccessToken == authToken || TwitchSettings.Get.Username == String.Empty) {} // We want to get the new value so we are waiting until this breaks
            // label_AuthorizedAs.Text = $"{TwitchSettings.Get.Username} with channel ID: {TwitchSettings.Get.ChannelID} and access token: {TwitchSettings.Get.AccessToken}";
        }

        private void Twitch_NewAccessToken(object sender, EventArgs e) => checkBox_RevealTwitchAuthToken.Checked = false;

        private void Twitch_scrollLog(object sender, EventArgs e)
        {
            textBox_TwitchLog.SelectionStart = textBox_TwitchLog.TextLength;
            textBox_TwitchLog.ScrollToCaret();
        }

        private void Twitch_CheckForTurboSpeed(TwitchReward selectedReward)
        {
            if (selectedReward.Name.Contains("TurboSpeed"))
            {
                if (selectedReward.Enabled)
                    WinMsgUtil.SendMsgToRS("enable TurboSpeed");
                else
                    WinMsgUtil.SendMsgToRS("disable TurboSpeed");
            }
        }

        private async void Twitch_SaveEnabledRewardsToFile()
        {
            await Task.Run(() =>
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

        private void Twitch_AddSelectedReward_Click(object sender, EventArgs e)
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

            MessageBoxManager.Yes = "Subs";
            MessageBoxManager.No = "Bits";
            MessageBoxManager.Cancel = "Points";
            MessageBoxManager.Register();

            var dialogResult = MessageBox.Show("Do you wish to add selected reward for subs, bits, channel points?" + Environment.NewLine + "NOTE: changing the amount of subs won't have an effect, as sub \"bombs\" are sent separately!", "Subs or Bits or Channel points?", MessageBoxButtons.YesNoCancel);
            if (dialogResult == DialogResult.Yes)
            {
                var reward = new SubReward();
                reward.Map(selectedReward);
                reward.SubID = rewardID;

                TwitchSettings.Get.Rewards.Add(reward);
                Twitch_AddToSelectedRewards(reward);
            }
            else if (dialogResult == DialogResult.No)
            {
                var reward = new BitsReward();
                reward.Map(selectedReward);
                reward.BitsID = rewardID;

                TwitchSettings.Get.Rewards.Add(reward);
                Twitch_AddToSelectedRewards(reward);
            }
            else
            {
                var reward = new ChannelPointsReward();
                reward.Map(selectedReward);
                reward.PointsID = rewardID;

                TwitchSettings.Get.Rewards.Add(reward);
                Twitch_AddToSelectedRewards(reward);
            }

            MessageBoxManager.Unregister(); // Just making sure our custom msg buttons don't stay enabled
            Twitch_SaveEnabledRewardsToFile();
        }

        private void Twitch_AddToSelectedRewards(TwitchReward reward) // Just imagine this was a bound list :P
        {
            if (reward is BitsReward)
                dgv_EnabledRewards.Rows.Add(reward.Enabled, reward.Name, reward.Length, ((BitsReward)reward).BitsAmount, "Bits", ((BitsReward)reward).BitsID);
            else if (reward is ChannelPointsReward)
                dgv_EnabledRewards.Rows.Add(reward.Enabled, reward.Name, reward.Length, ((ChannelPointsReward)reward).PointsAmount, "Points", ((ChannelPointsReward)reward).PointsID);
            else if (reward is SubReward)
                dgv_EnabledRewards.Rows.Add(reward.Enabled, reward.Name, reward.Length, 1, "Sub", ((SubReward)reward).SubID);
        }

        private TwitchReward Twitch_GetSelectedReward(DataGridViewRow selectedRow)
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

        private void Twitch_EnabledRewards_CurrentCellDirtyStateChanged(object sender, EventArgs e)
        {
            if (dgv_EnabledRewards.IsCurrentCellDirty && dgv_EnabledRewards.CurrentCell.ColumnIndex == 0 && dgv_EnabledRewards.CurrentCell.RowIndex != -1)
            {
                dgv_EnabledRewards.CommitEdit(DataGridViewDataErrorContexts.Commit);
                dgv_EnabledRewards.EndEdit();
            }
        }

        private void Twitch_EnabledRewards_CellEndEdit(object sender, DataGridViewCellEventArgs e)
        {
            var selectedRow = dgv_EnabledRewards.SelectedRows[0];
            var selectedReward = Twitch_GetSelectedReward(selectedRow);

            if (selectedReward == null)
                return;

            selectedReward.Enabled = Convert.ToBoolean(selectedRow.Cells["colEnabledRewardsEnabled"].Value);
            if (selectedRow.Cells["colEnabledRewardsLength"].Value == null || !(Int32.TryParse(selectedRow.Cells["colEnabledRewardsLength"].Value.ToString(), out int rewardLength)))
            {
                selectedRow.Cells["colEnabledRewardsLength"].Value = 0;
                MessageBox.Show("You need to put a number, not a text value.");
                return;
            }

            if (selectedRow.Cells["colEnabledRewardsAmount"].Value == null || !(Int32.TryParse(selectedRow.Cells["colEnabledRewardsAmount"].Value.ToString(), out int rewardAmount)))
            {
                selectedRow.Cells["colEnabledRewardsAmount"].Value = 0;
                MessageBox.Show("You need to put a number, not a text value.");
                return;
            }

            selectedReward.Length = rewardLength;

            if (selectedReward is BitsReward)
                ((BitsReward)selectedReward).BitsAmount = Convert.ToInt32(selectedRow.Cells["colEnabledRewardsAmount"].Value);
            else if (selectedReward is ChannelPointsReward)
                ((ChannelPointsReward)selectedReward).PointsAmount = Convert.ToInt32(selectedRow.Cells["colEnabledRewardsAmount"].Value);

            Twitch_CheckForTurboSpeed(selectedReward);

            Twitch_SaveEnabledRewardsToFile();
        }

        private void Twitch_EnabledRewards_SelectionChanged(object sender, EventArgs e)
        {
            if (dgv_EnabledRewards.SelectedRows.Count < 1)
                return;

            var selectedRow = dgv_EnabledRewards.SelectedRows[0];
            var selectedReward = Twitch_GetSelectedReward(selectedRow);
            Twitch_ShowSolidNoteColorRelatedStuff(false);

            if (selectedReward.Name != "Solid color notes")
                return;

            if (selectedReward.AdditionalMsg == null || selectedReward.AdditionalMsg == string.Empty || selectedReward.AdditionalMsg == "Random")
            {
                Twitch_SetAdditionalMessage("Random");
                textBox_SolidNoteColorPicker.BackColor = Color.White;
                textBox_SolidNoteColorPicker.Text = "Random";
                dgv_EnabledRewards.SelectedRows[0].DefaultCellStyle.BackColor = Color.White;
            }
            else
            {
                textBox_SolidNoteColorPicker.BackColor = ColorTranslator.FromHtml("#" + selectedReward.AdditionalMsg);
                textBox_SolidNoteColorPicker.Text = "";
                dgv_EnabledRewards.SelectedRows[0].DefaultCellStyle.BackColor = ColorTranslator.FromHtml("#" + selectedReward.AdditionalMsg);
            }

            Twitch_ShowSolidNoteColorRelatedStuff(true);
        }


        private void Twitch_RemoveReward_Click(object sender, EventArgs e)
        {
            if (dgv_EnabledRewards.SelectedRows.Count < 1)
                return;

            var selectedRow = dgv_EnabledRewards.SelectedRows[0];
            var selectedReward = Twitch_GetSelectedReward(selectedRow);

            if (selectedReward.Name == "Solid color notes")
                Twitch_ShowSolidNoteColorRelatedStuff(false);

            if (selectedReward != null)
                TwitchSettings.Get.Rewards.Remove(selectedReward);

            dgv_EnabledRewards.Rows.RemoveAt(selectedRow.Index);

            Twitch_SaveEnabledRewardsToFile();
        }

        private void Twitch_SetAdditionalMessage(string msg)
        {
            var selectedRow = dgv_EnabledRewards.SelectedRows[0];
            var selectedReward = Twitch_GetSelectedReward(selectedRow);

            if (selectedReward.Name != "Solid color notes")
                return;

            selectedReward.AdditionalMsg = msg;
        }

        private void Twitch_SolidNoteColorPicker_Click(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog
            {
                AllowFullOpen = true,
                ShowHelp = false
            };

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                string colorHex = (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6");
                textBox_SolidNoteColorPicker.BackColor = colorDialog.Color;
                textBox_SolidNoteColorPicker.Text = String.Empty;
                dgv_EnabledRewards.SelectedRows[0].DefaultCellStyle.BackColor = colorDialog.Color;

                Twitch_SetAdditionalMessage(colorHex);
                Twitch_SaveEnabledRewardsToFile();
            }
        }

        private void Twitch_SolidNoteColorRandom_Click(object sender, EventArgs e)
        {
            textBox_SolidNoteColorPicker.BackColor = Color.White;
            textBox_SolidNoteColorPicker.Text = "Random";

            Twitch_SetAdditionalMessage("Random");
            Twitch_SaveEnabledRewardsToFile();

            dgv_EnabledRewards.SelectedRows[0].DefaultCellStyle.BackColor = Color.White;
        }

        private void Twitch_SendFakeReward()
        {
            if (dgv_EnabledRewards.CurrentCell == null)
                return;

            PubSub.SendMessageToRocksmith(TwitchSettings.Get.Rewards[dgv_EnabledRewards.CurrentCell.RowIndex]);
        }

        private void Twitch_TestReward(object sender, EventArgs e) => Twitch_SendFakeReward();

        private void Twitch_ShowSolidNoteColorRelatedStuff(bool shouldWeShow)
        {
            button_SolidNoteColorPicker.Visible = shouldWeShow;
            textBox_SolidNoteColorPicker.Visible = shouldWeShow;
            button_SolidNoteColorRandom.Visible = shouldWeShow;
        }

        private void Twitch_timerValidate_Tick(object sender, EventArgs e)
        {
            if (checkBox_TwitchForceReauth.Checked)
            {
                TwitchSettings.Get.AddToLog("Reauthorizing...");
                TwitchSettings.Get.AddToLog("----------------");

                var auth = new ImplicitAuth(); // Force the issue
                auth.MakeAuthRequest(true); // When the request finishes, it will trigger PropertyChanged & set Reauthorized, which in turn will reset PubSub
            }
            else
                PubSub.Get.Resub();
        }

        private static void Twitch_SaveSettings() => TwitchSettings.Get.SaveSettings();

        private void Twitch_ForceReauth_CheckedChanged(object sender, EventArgs e)
        {
            TwitchSettings.Get.ForceReauth = checkBox_TwitchForceReauth.Checked;
            Twitch_SaveSettings();
        }

        private void Twitch_SaveLogToFile(object sender, EventArgs e)
        {
            try
            {
                File.WriteAllText("twitchLog.txt", TwitchSettings.Get.Log);
                MessageBox.Show("Saved log to RS folder/RSMods/twitchLog.txt!", "Saved!");
            }
            catch (IOException ioex)
            {
                MessageBox.Show($"Unable to save log, error: {ioex.Message}");
            }
        }

        private void Twitch_CopyOnClick(object sender, MouseEventArgs e) => Clipboard.SetText("Send to RSMod Developers (Discord Ffio#2221)\nUsername: " + TwitchSettings.Get.Username + "\nChannel ID: " + TwitchSettings.Get.ChannelID + "\nAccess Token: " + TwitchSettings.Get.AccessToken);

        /*private void dgv_EnabledRewards_CellMouseClick(object sender, DataGridViewCellMouseEventArgs e)
       {
           if (!(dgv_EnabledRewards.Columns[e.ColumnIndex] is DataGridViewComboBoxColumn))
               return;

           dgv_EnabledRewards.BeginEdit(false);
           var ec = dgv_EnabledRewards.EditingControl as DataGridViewComboBoxEditingControl;
           if (ec != null && ec.Width - e.X < SystemInformation.VerticalScrollBarWidth)
               ec.DroppedDown = true;

           SaveEnabledRewardsToFile();
       }*/
        #endregion
        #region Fonts
        private void LoadFonts() // Not modified from here: https://stackoverflow.com/a/8657854 :eyes:
        {
            InstalledFontCollection fontList = new InstalledFontCollection();
            FontFamily[] fontFamilies = fontList.Families;

            foreach (FontFamily font in fontFamilies)
            {
                listBox_AvailableFonts.Items.Add(font.Name);
            }

            listBox_AvailableFonts.SelectedItem = ReadSettings.ProcessSettings(ReadSettings.OnScreenFontIdentifier);
        }

        private void ChangeOnScreenFont(object sender, EventArgs e)
        {
            string fontName = listBox_AvailableFonts.SelectedItem.ToString();
            Font newFontSelected = new Font(fontName, 10.0f, Font.Style, Font.Unit);
            label_FontTestCAPITALS.Font = newFontSelected;
            label_FontTestlowercase.Font = newFontSelected;
            label_FontTestNumbers.Font = newFontSelected;

            SaveChanges(ReadSettings.OnScreenFontIdentifier, fontName);
        }
        #endregion
        #region Midi

        private void LoadMidiDeviceNames(object sender, EventArgs e)
        {
            this.listBox_ListMidiDevices.Items.Clear();

            uint numberOfMidiOutDevices = Midi.midiOutGetNumDevs();

            for(uint deviceNumber = 0; deviceNumber < numberOfMidiOutDevices; deviceNumber++)
            {
                Midi.MIDIOUTCAPS temp = new Midi.MIDIOUTCAPS {};
                Midi.midiOutGetDevCaps(deviceNumber, ref temp, (uint)Marshal.SizeOf(typeof(Midi.MIDIOUTCAPS)));
                this.listBox_ListMidiDevices.Items.Add(temp.szPname);
            }

            if (ReadSettings.ProcessSettings(ReadSettings.MidiAutoTuningDeviceIdentifier) != "")
                listBox_ListMidiDevices.SelectedItem = ReadSettings.ProcessSettings(ReadSettings.MidiAutoTuningDeviceIdentifier);
        }
    }

    public class Midi
    {
        public enum MMRESULT : uint
        {
            MMSYSERR_NOERROR = 0,
            MMSYSERR_ERROR = 1,
            MMSYSERR_BADDEVICEID = 2,
            MMSYSERR_NOTENABLED = 3,
            MMSYSERR_ALLOCATED = 4,
            MMSYSERR_INVALHANDLE = 5,
            MMSYSERR_NODRIVER = 6,
            MMSYSERR_NOMEM = 7,
            MMSYSERR_NOTSUPPORTED = 8,
            MMSYSERR_BADERRNUM = 9,
            MMSYSERR_INVALFLAG = 10,
            MMSYSERR_INVALPARAM = 11,
            MMSYSERR_HANDLEBUSY = 12,
            MMSYSERR_INVALIDALIAS = 13,
            MMSYSERR_BADDB = 14,
            MMSYSERR_KEYNOTFOUND = 15,
            MMSYSERR_READERROR = 16,
            MMSYSERR_WRITEERROR = 17,
            MMSYSERR_DELETEERROR = 18,
            MMSYSERR_VALNOTFOUND = 19,
            MMSYSERR_NODRIVERCB = 20,
            WAVERR_BADFORMAT = 32,
            WAVERR_STILLPLAYING = 33,
            WAVERR_UNPREPARED = 34
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct MIDIOUTCAPS
        {
            public ushort wMid;
            public ushort wPid;
            public uint vDriverVersion;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string szPname;
            public ushort wTechnology;
            public ushort wVoices;
            public ushort wNotes;
            public ushort wChannelMask;
            public uint dwSupport;
        }

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern MMRESULT midiOutGetDevCaps(uint uDeviceID, ref MIDIOUTCAPS lpMidiOutCaps, uint cbMidiOutCaps);

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern uint midiOutGetNumDevs();
    }
    #endregion
}