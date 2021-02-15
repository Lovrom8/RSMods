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
using RocksmithToolkitLib.Extensions;
using RSMods.Twitch;
using System.Threading;
using System.Xml.Serialization;
using System.Xml;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Data;
using System.Globalization;

namespace RSMods
{
    public partial class MainForm : Form
    {
        bool shipProfileEdits = false;

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        static extern IntPtr FindWindow(string strClassName, string strWindowName);

        [DllImport("user32.dll")]
        static extern bool GetWindowRect(IntPtr hwnd, ref Rect rectangle);
        struct Rect
        {
            public int Left { get; set; }
            public int Top { get; set; }
            public int Right { get; set; }
            public int Bottom { get; set; }
        }

        TabPage ProfileEditsTab;
        int ProfileEditsTabIndex;

        public MainForm()
        {

            // Locate Rocksmith Folder
            Startup_LocateRocksmith(GenUtil.GetRSDirectory());

            // Check if the GUI settings, and DLL settings already exist
            Startup_VerifyGUIInstall();

            // Load saved credidentials and enable PubSub
            PrepTwitch_LoadSettings();

            // Initialize WinForms
            Startup_InitWinForms();

            // Setup bindings for Twitch events
            Twitch_Setup();

            // Fix Legacy Songlist Bug
            Startup_FixLegacySonglistBug();

            // Fill Songlist List
            Startup_LoadSonglists();

            // Fill Mod Keybindings List (Fill list box)
            Startup_LoadKeybindingModNames();

            // Fill Audio Keybinding List
            Startup_LoadAudioKeybindings();

            // Load Mod Keybinding Values
            Startup_ShowCurrentKeybindingValues();

            // Load Audio Keybinding Values
            Startup_ShowCurrentAudioKeybindingValues();

            // Load Guitar Speak Preset Values
            GuitarSpeak_ResetPresets();

            // Load Default String Colors
            StringColors_LoadDefaultStringColors();

            // Load Default Note Colors
            StringColors_LoadDefaultNoteColors();

            // Load Default Noteway Colors
            NotewayColors_LoadDefaultStringColors();

            // Load Colors Saved as Theme Colors.
            CustomTheme_LoadCustomColors();

            // Load RS_ASIO
            Startup_VerifyInstallOfASIO();

            // Load RS_ASIO Settings
            PriorSettings_LoadASIOSettings();

            // Load Rocksmith Settings
            PriorSettings_LoadRocksmithSettings();

            // Load All Available Rocksmith Profiles
            Startup_LoadRocksmithProfiles();

            // Prevent some double saving
            PriorSettings_PreventDoubleSave();

            // Load Set And Forget Mods
            SetForget_LoadSetAndForgetMods();

            // Load All System Fonts
            Fonts_Load();

            // Backup Profiles Just In Case
            Startup_BackupProfiles();

            // Load Checkbox Values From RSMods.ini
            PriorSettings_LoadModSettings();

            // Delete Old Backups To Save Space (if user specifies)
            Startup_DeleteOldBackups(GenUtil.StrToIntDef(ReadSettings.ProcessSettings(ReadSettings.NumberOfBackupsIdentifier), 50));

            // Lock the profile edits tab if backups are disabled
            Startup_LockProfileEdits();

            // Get list of all backups so we can revert to one if needed
            Startup_ListAllBackups();
        }

        #region Startup Functions

        private void Startup_InitWinForms()
        {
            InitializeComponent();
            Text = $"{Text}-{Assembly.GetExecutingAssembly().GetName().Version}"; // Show version number in the title of the application.
        }

        private void Startup_FixLegacySonglistBug()
        {
            if (ReadSettings.ProcessSettings(ReadSettings.Songlist1Identifier) == String.Empty)
                SaveSettings_Save(ReadSettings.Songlist1Identifier, "Define Song List 1 Here");
        }

        private void Startup_LocateRocksmith(string RSFolder)
        {

            if (RSFolder == String.Empty)
            {
                MessageBox.Show("We cannot detect where you have Rocksmith located. Please try reinstalling your game on Steam.", "Error: RSLocation Not Found", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Environment.Exit(1);
            }
            else
                Constants.RSFolder = RSFolder;
        }
        private void Startup_VerifyGUIInstall()
        {
            WriteSettings.IsVoid(GenUtil.GetRSDirectory());
            if (!File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini")))
                WriteSettings.WriteINI(WriteSettings.saveSettingsOrDefaults); // Creates Settings File

            if (!File.Exists(Constants.SettingsPath))
                File.WriteAllText(Constants.SettingsPath, "RSPath = " + Constants.RSFolder);
        }

        private void Startup_LoadSonglists()
        {
            foreach (string songlist in Dictionaries.refreshSonglists())
                listBox_Songlist.Items.Add(songlist);
        }

        private void Startup_LoadKeybindingModNames()
        {
            foreach (string mod in Dictionaries.currentModKeypressList)
                listBox_Modlist_MODS.Items.Add(mod);
        }

        private void Startup_LoadAudioKeybindings()
        {
            foreach (string volume in Dictionaries.currentAudioKeypressList)
                listBox_Modlist_AUDIO.Items.Add(volume);
        }

        private void Startup_ShowCurrentKeybindingValues()
        {
            label_ToggleLoftKey.Text = "Toggle Loft: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ToggleLoftIdentifier));
            label_SongTimerKey.Text = "Show Song Timer: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerIdentifier));
            label_ReEnumerationKey.Text = "Force ReEnumeration: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.ForceReEnumerationIdentifier));
            label_RainbowStringsKey.Text = "Rainbow Strings: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RainbowStringsIdentifier));
            label_RainbowNotesKey.Text = "Rainbow Notes: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RainbowNotesIdentifier));
            label_RemoveLyricsKey.Text = "Remove Lyrics: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RemoveLyricsKeyIdentifier));
            label_RRSpeedKey.Text = "RR Speed: " + KeyConversion.VKeyToUI(ReadSettings.ProcessSettings(ReadSettings.RRSpeedKeyIdentifier));
        }

        private void Startup_ShowCurrentAudioKeybindingValues()
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

        private void Startup_LoadASIODevices()
        {
            foreach (ASIO.Devices.DriverInfo device in ASIO.Devices.FindDevices())
            {
                listBox_AvailableASIODevices_Input0.Items.Add(device.deviceName);
                listBox_AvailableASIODevices_Input1.Items.Add(device.deviceName);
                listBox_AvailableASIODevices_Output.Items.Add(device.deviceName);
                listBox_AvailableASIODevices_InputMic.Items.Add(device.deviceName);
            }
        }

        private void Startup_VerifyInstallOfASIO()
        {
            if (!ASIO.ReadSettings.VerifySettingsExist())
                TabController.TabPages.Remove(tab_RSASIO);
            else
                Startup_LoadASIODevices();
        }

        private void Startup_LoadRocksmithProfiles()
        {
            foreach (KeyValuePair<string, string> profileData in Profiles.AvailableProfiles())
            {
                listBox_AutoLoadProfiles.Items.Add(profileData.Key);
                listBox_Profiles_AvailableProfiles.Items.Add(profileData.Key);
            }
        }

        private void Startup_DeleteOldBackups(int maxAmountOfBackups)
        {

            if (maxAmountOfBackups == 0) // User says they want all the backups.
                return;

            string backupFolder = Path.Combine(RSMods.Data.Constants.RSFolder, "Profile_Backups");

            if (!Directory.Exists(backupFolder))
                return;

            DirectoryInfo[] backups = new DirectoryInfo(backupFolder).GetDirectories().OrderBy(f => f.LastWriteTime).ToArray();

            int foldersLeftToRemove = backups.Length - maxAmountOfBackups;

            foreach (DirectoryInfo backup in backups)
            {
                if (foldersLeftToRemove == 0)
                    break;

                if (Array.IndexOf(backups, backup.Name) < backups.Length - maxAmountOfBackups)
                {
                    foreach (string file in Directory.GetFiles(backup.FullName))
                    {
                        File.Delete(file);
                    }
                    Directory.Delete(backup.FullName);
                    foldersLeftToRemove--;
                }

            }
        }

        private void Startup_BackupProfiles()
        {
            if (ReadSettings.ProcessSettings(ReadSettings.BackupProfileIdentifier) == "on")
                Profiles.SaveProfile();
        }

        private void Startup_LockProfileEdits()
        {
            ProfileEditsTab = tab_Profiles;
            ProfileEditsTabIndex = TabController.TabPages.IndexOf(ProfileEditsTab);

            if (ReadSettings.ProcessSettings(ReadSettings.BackupProfileIdentifier) != "on" || !shipProfileEdits)
                TabController.TabPages.Remove(tab_Profiles);
        }

        private void Startup_ListAllBackups()
        {
            try
            {
                foreach (string backup in Directory.GetDirectories(Path.Combine(GenUtil.GetRSDirectory(), "Profile_Backups")))
                {
                    string folderName = Path.GetFileNameWithoutExtension(backup);
                    string date = folderName.Split('_')[0];
                    string time = folderName.Split('_')[1];

                    int month = Convert.ToInt32(date.Split('-')[0]);
                    int day = Convert.ToInt32(date.Split('-')[1]);
                    int year = Convert.ToInt32(date.Split('-')[2]);

                    string userFriendlyName = CultureInfo.CurrentCulture.DateTimeFormat.GetAbbreviatedMonthName(month) + " " + day + " " + year + " @ " + time.Replace('-', ':');

                    listBox_Profiles_ListBackups.Items.Add(userFriendlyName);
                }
                    
            }
            catch // Folder doesn't exist
            {
            }
        }

        #endregion
        #region Show Prior Settings In GUI
        private void PriorSettings_LoadModSettings()
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
                groupBox_ControlVolumeIncrement.Visible = true;
                groupBox_ControlSongVolumeWhen.Visible = true;

                string valStr = ReadSettings.ProcessSettings(ReadSettings.VolumeControlIntervalIdentifier);
                int intVal = 0;

                if (int.TryParse(valStr, out intVal))
                    nUpDown_VolumeInterval.Value = intVal;
            }

            if (ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerEnabledIdentifier) == "on") // Show Song Timer Enabled / Disabled
            {
                checkBox_SongTimer.Checked = true;
                groupBox_SongTimer.Visible = true;
                if (ReadSettings.ProcessSettings(ReadSettings.ShowSongTimerWhenIdentifier) == "automatic")
                    radio_SongTimerAlways.Checked = true;
                else
                    radio_SongTimerManual.Checked = true;
            }

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

            if (ReadSettings.ProcessSettings(ReadSettings.RainbowNotesEnabledIdentifier) == "on") // Rainbow Notes Enabled / Disabled
                checkBox_RainbowNotes.Checked = true;

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

            if (ReadSettings.ProcessSettings(ReadSettings.ForceProfileEnabledIdentifier) == "on")
            { // Force Load Profile On Game Boot Enabled / Disabled
                checkBox_AutoLoadProfile.Checked = true;
                if (ReadSettings.ProcessSettings(ReadSettings.ProfileToLoadIdentifier) != "")
                    listBox_AutoLoadProfiles.SelectedItem = ReadSettings.ProcessSettings(ReadSettings.ProfileToLoadIdentifier);
            }

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
                checkBox_GuitarSpeakWhileTuning.Visible = true;
            }

            if (ReadSettings.ProcessSettings(ReadSettings.GuitarSpeakTuningIdentifier) == "on")
                checkBox_GuitarSpeakWhileTuning.Checked = true;

            if (ReadSettings.ProcessSettings(ReadSettings.CustomGUIThemeIdentifier) == "on")
                checkBox_ChangeTheme.Checked = true;

            if (ReadSettings.ProcessSettings(ReadSettings.RiffRepeaterAboveHundredIdentifier) == "on")
            {
                checkBox_RiffRepeaterSpeedAboveOneHundred.Checked = true;
                groupBox_RRSpeed.Visible = true;

                string val = ReadSettings.ProcessSettings(ReadSettings.RiffRepeaterSpeedIntervalIdentifier);
                decimal decimalVal = 0;

                if (Decimal.TryParse(val, out decimalVal))
                {
                    if (decimalVal < 2)
                        decimalVal = 2;
                    nUpDown_RiffRepeaterSpeed.Value = decimalVal;
                    SaveSettings_Save(ReadSettings.RiffRepeaterSpeedIntervalIdentifier, decimalVal.ToString());
                }

            }

            if (ReadSettings.ProcessSettings(ReadSettings.MidiAutoTuningIdentifier) == "on")
            {
                checkBox_useMidiAutoTuning.Checked = true;
                groupBox_MidiAutoTuneDevice.Visible = true;
                label_SelectedMidiDevice.Text = "Midi Device: " + ReadSettings.ProcessSettings(ReadSettings.MidiAutoTuningDeviceIdentifier);

                if (ReadSettings.ProcessSettings(ReadSettings.TuningPedalIdentifier) != "")
                {
                    int tuningPedal = GenUtil.StrToIntDef(ReadSettings.ProcessSettings(ReadSettings.TuningPedalIdentifier), 0);

                    switch (tuningPedal)
                    {
                        case 1:
                            radio_WhammyDT.Checked = true;
                            break;
                        case 2:
                            radio_WhammyBass.Checked = true;
                            checkBox_WhammyChordsMode.Visible = true;
                            break;
                        case 3:
                            radio_Whammy.Checked = true;
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

            if (ReadSettings.ProcessSettings(ReadSettings.BackupProfileIdentifier) == "on")
            {
                nUpDown_NumberOfBackups.Value = GenUtil.StrToIntDef(ReadSettings.ProcessSettings(ReadSettings.NumberOfBackupsIdentifier), 50);
                groupBox_Backups.Visible = true;
            }

            if (ReadSettings.ProcessSettings(ReadSettings.CustomHighwayColorsIdentifier) == "on")
                checkBox_CustomHighway.Checked = true;

            if (ReadSettings.ProcessSettings(ReadSettings.ShowSelectedVolumeWhenIdentifier) == "automatic")
                radio_ControlSongVolumeAlways.Checked = true;
            else if (ReadSettings.ProcessSettings(ReadSettings.ShowSelectedVolumeWhenIdentifier) == "song")
                radio_ControlSongVolumeInSong.Checked = true;
            else
                radio_ControlSongVolumeManual.Checked = true;

            if (ReadSettings.ProcessSettings(ReadSettings.SecondaryMonitorIdentifier) == "on")
                checkBox_SecondaryMonitor.Checked = true;

            checkBox_NoteColors_UseRocksmithColors.Checked = ReadSettings.ProcessSettings(ReadSettings.SeparateNoteColorsIdentifier) == "1";

            checkBox_ModsLog.Checked = File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "RSMods_debug.txt"));
        }

        private void PriorSettings_LoadASIOSettings()
        {
            if (!ASIO.ReadSettings.VerifySettingsExist())
                return;

            // Config
            checkBox_ASIO_WASAPI_Output.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.EnableWasapiOutputsIdentifier, ASIO.ReadSettings.Sections.Config), 0));
            checkBox_ASIO_WASAPI_Input.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.EnableWasapiInputsIdentifier, ASIO.ReadSettings.Sections.Config), 0));
            checkBox_ASIO_ASIO.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.EnableAsioIdentifier, ASIO.ReadSettings.Sections.Config), 0));

            // Asio
            if (ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.BufferSizeModeIdentifier, ASIO.ReadSettings.Sections.Asio) == "custom")
            {
                radio_ASIO_BufferSize_Custom.Checked = true;
                nUpDown_ASIO_CustomBufferSize.Value = GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.CustomBufferSizeIdentifier, ASIO.ReadSettings.Sections.Asio), 0);
            }
            if (ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.BufferSizeModeIdentifier, ASIO.ReadSettings.Sections.Asio) == "driver")
                radio_ASIO_BufferSize_Driver.Checked = true;
            if (ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.BufferSizeModeIdentifier, ASIO.ReadSettings.Sections.Asio) == "host")
                radio_ASIO_BufferSize_Host.Checked = true;

            // Output
            nUpDown_ASIO_Output_BaseChannel.Value = GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.BaseChannelIdentifier, ASIO.ReadSettings.Sections.Output), 0);
            nUpDown_ASIO_Output_AltBaseChannel.Value = GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.AltBaseChannelIdentifier, ASIO.ReadSettings.Sections.Output), 0);
            checkBox_ASIO_Output_ControlEndpointVolume.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ASIO.ReadSettings.Sections.Output), 0));
            checkBox_ASIO_Output_ControlMasterVolume.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ASIO.ReadSettings.Sections.Output), 0));
            nUpDown_ASIO_Output_MaxVolume.Value = GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.SoftwareMasterVolumePercentIdentifier, ASIO.ReadSettings.Sections.Output), 0);
            // checkBox_ASIO_Output_Disabled.Checked = ASIO.ReadSettings.DisabledOutput;

            // Input0
            nUpDown_ASIO_Input0_Channel.Value = GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.ChannelIdentifier, ASIO.ReadSettings.Sections.Input0), 0);
            checkBox_ASIO_Input0_ControlEndpointVolume.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ASIO.ReadSettings.Sections.Input0), 0));
            checkBox_ASIO_Input0_ControlMasterVolume.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ASIO.ReadSettings.Sections.Input0), 0));
            nUpDown_ASIO_Input0_MaxVolume.Value = GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.SoftwareMasterVolumePercentIdentifier, ASIO.ReadSettings.Sections.Input0), 0);
            checkBox_ASIO_Input0_Disabled.Checked = ASIO.ReadSettings.DisabledInput0;

            // Input1
            nUpDown_ASIO_Input1_Channel.Value = GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.ChannelIdentifier, ASIO.ReadSettings.Sections.Input1), 0);
            checkBox_ASIO_Input1_ControlEndpointVolume.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ASIO.ReadSettings.Sections.Input1), 0));
            checkBox_ASIO_Input1_ControlMasterVolume.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ASIO.ReadSettings.Sections.Input1), 0));
            nUpDown_ASIO_Input1_MaxVolume.Value = GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.SoftwareMasterVolumePercentIdentifier, ASIO.ReadSettings.Sections.Input1), 0);
            checkBox_ASIO_Input1_Disabled.Checked = ASIO.ReadSettings.DisabledInput1;

            // InputMic
            nUpDown_ASIO_InputMic_Channel.Value = GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.ChannelIdentifier, ASIO.ReadSettings.Sections.InputMic), 0);
            checkBox_ASIO_InputMic_ControlEndpointVolume.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ASIO.ReadSettings.Sections.InputMic), 0));
            checkBox_ASIO_InputMic_ControlMasterVolume.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ASIO.ReadSettings.Sections.InputMic), 0));
            nUpDown_ASIO_InputMic_MaxVolume.Value = GenUtil.StrToIntDef(ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.SoftwareMasterVolumePercentIdentifier, ASIO.ReadSettings.Sections.InputMic), 0);
            checkBox_ASIO_InputMic_Disabled.Checked = ASIO.ReadSettings.DisabledInputMic;
        }

        private void PriorSettings_LoadRocksmithSettings()
        {
            if (!Rocksmith.ReadSettings.VerifySettingsINI())
                return;
            // Convert.ToDecimal(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.));
            // Audio Settings

            checkBox_Rocksmith_EnableMicrophone.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.EnableMicrophoneIdentifier), 0));
            checkBox_Rocksmith_ExclusiveMode.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.ExclusiveModeIdentifier), 0));
            if (GenUtil.StrToDecDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.LatencyBufferIdentifier), 4) <= 0)
                SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.LatencyBufferIdentifier, "4");
            nUpDown_Rocksmith_LatencyBuffer.Value = GenUtil.StrToDecDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.LatencyBufferIdentifier), 4);
            checkBox_Rocksmith_ForceWDM.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.ForceWDMIdentifier), 0));
            checkBox_Rocksmith_ForceDirextXSink.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.ForceDirectXSinkIdentifier), 0));
            checkBox_Rocksmith_DumpAudioLog.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.DumpAudioLogIdentifier), 0));
            if (Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.MaxOutputBufferSizeIdentifier), 0)))
                nUpDown_Rocksmith_MaxOutputBuffer.Value = GenUtil.StrToDecDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.MaxOutputBufferSizeIdentifier), 0);
            else
                checkBox_Rocksmith_Override_MaxOutputBufferSize.Checked = true;
            checkBox_Rocksmith_RTCOnly.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.RealToneCableOnlyIdentifier), 0));
            checkBox_Rocksmith_LowLatencyMode.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.Win32UltraLowLatencyModeIdentifier), 0));
            // Visual Settings

            checkBox_Rocksmith_GamepadUI.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.ShowGamepadUIIdentifier), 0));
            nUpDown_Rocksmith_ScreenWidth.Value = GenUtil.StrToDecDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.ScreenWidthIdentifier), 0);
            nUpDown_Rocksmith_ScreenHeight.Value = GenUtil.StrToDecDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.ScreenHeightIdentifier), 0);
            switch (GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.FullscreenIdentifier), 0))
            {
                case 0:
                    radio_Rocksmith_Windowed.Checked = true;
                    break;
                case 1:
                    radio_Rocksmith_NonExclusiveFullScreen.Checked = true;
                    break;
                case 2:
                    radio_Rocksmith_ExclusiveFullScreen.Checked = true;
                    break;
                default:
                    break;
            }
            nUpDown_Rocksmith_RenderWidth.Value = GenUtil.StrToDecDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.RenderingWidthIdentifier), 0);
            nUpDown_Rocksmith_RenderHeight.Value = GenUtil.StrToDecDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.RenderingHeightIdentifier), 0);
            checkBox_Rocksmith_PostEffects.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.EnablePostEffectsIdentifier), 0));
            checkBox_Rocksmith_Shadows.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.EnableShadowsIdentifier), 0));
            checkBox_Rocksmith_HighResScope.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.EnableHighResScopeIdentifier), 0));
            checkBox_Rocksmith_DepthOfField.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.EnableDepthOfFieldIdentifier), 0));
            checkBox_Rocksmith_PerPixelLighting.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.EnablePerPixelLightingIdentifier), 0));
            checkBox_Rocksmith_MSAASamples.Checked = GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.MsaaSamplesIdentifier), 0) == 4;
            checkBox_Rocksmith_DisableBrowser.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.DisableBrowserIdentifier), 0));
            checkBox_Rocksmith_EnableRenderRes.Checked = (Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.RenderingWidthIdentifier) != "0" || Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.RenderingHeightIdentifier) != "0");

            switch (GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.VisualQualityIdentifier), 0))
            {
                case 0:
                    radio_Rocksmith_LowQuality.Checked = true;
                    break;
                case 1:
                    radio_Rocksmith_MediumQuality.Checked = true;
                    break;
                case 2:
                    radio_Rocksmith_HighQuality.Checked = true;
                    break;
                case 3:
                    radio_Rocksmith_CustomQuality.Checked = true;
                    break;
                default:
                    break;
            }

            // Network Settings
            checkBox_Rocksmith_UseProxy.Checked = Convert.ToBoolean(GenUtil.StrToIntDef(Rocksmith.ReadSettings.ProcessSettings(Rocksmith.ReadSettings.UseProxyIdentifier), 0));
        }

        private void PriorSettings_PreventDoubleSave()
        {
            // Disable changes while we change them. This prevents us from saving a value we already know.
            listBox_ExtendedRangeTunings.SelectedIndexChanged -= new System.EventHandler(Save_ExtendedRangeTuningAt);
            nUpDown_ForceEnumerationXMS.ValueChanged -= new System.EventHandler(Save_EnumerateEveryXMS);
            listBox_AvailableASIODevices_Input0.SelectedIndexChanged -= new System.EventHandler(ASIO_ListAvailableInput0);
            listBox_AvailableASIODevices_Input1.SelectedIndexChanged -= new System.EventHandler(ASIO_ListAvailableInput1);
            listBox_AvailableASIODevices_Output.SelectedIndexChanged -= new System.EventHandler(ASIO_ListAvailableOutput);
            listBox_AvailableASIODevices_InputMic.SelectedIndexChanged -= new System.EventHandler(ASIO_ListAvailableInputMic);
            checkBox_ASIO_Output_Disabled.CheckedChanged -= new System.EventHandler(ASIO_Output_Disable);
            checkBox_ASIO_Input0_Disabled.CheckedChanged -= new System.EventHandler(ASIO_Input0_Disable);
            checkBox_ASIO_Input1_Disabled.CheckedChanged -= new System.EventHandler(ASIO_Input1_Disable);
            checkBox_ASIO_InputMic_Disabled.CheckedChanged -= new System.EventHandler(ASIO_InputMic_Disable);
            checkBox_ER_SeparateNoteColors.CheckedChanged -= new System.EventHandler(Save_ER_SeparateNoteColors);
            checkBox_BackupProfile.CheckedChanged -= new System.EventHandler(Save_BackupProfile);


            // Now we can change things without saving.
            nUpDown_ForceEnumerationXMS.Value = GenUtil.StrToIntDef(ReadSettings.ProcessSettings(ReadSettings.CheckForNewSongIntervalIdentifier), 5000) / 1000; // Loads old settings for enumeration every x ms
            listBox_ExtendedRangeTunings.SelectedIndex = (GenUtil.StrToIntDef(ReadSettings.ProcessSettings(ReadSettings.ExtendedRangeTuningIdentifier), 0) * -1) - 2; // Loads old ER tuning settings
            listBox_AvailableASIODevices_Input0.SelectedItem = ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.Input0);
            listBox_AvailableASIODevices_Input1.SelectedItem = ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.Input1);
            listBox_AvailableASIODevices_Output.SelectedItem = ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.Output);
            listBox_AvailableASIODevices_InputMic.SelectedItem = ASIO.ReadSettings.ProcessSettings(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.InputMic);
            checkBox_ASIO_Output_Disabled.Checked = ASIO.ReadSettings.DisabledOutput;
            checkBox_ASIO_Input0_Disabled.Checked = ASIO.ReadSettings.DisabledInput0;
            checkBox_ASIO_Input1_Disabled.Checked = ASIO.ReadSettings.DisabledInput1;
            checkBox_ASIO_InputMic_Disabled.Checked = ASIO.ReadSettings.DisabledInputMic;
            checkBox_ER_SeparateNoteColors.Checked = ReadSettings.ProcessSettings(ReadSettings.SeparateNoteColorsIdentifier) == "1" || ReadSettings.ProcessSettings(ReadSettings.SeparateNoteColorsIdentifier) == "2";
            groupBox_NoteColors.Visible = checkBox_ER_SeparateNoteColors.Checked;
            checkBox_BackupProfile.Checked = ReadSettings.ProcessSettings(ReadSettings.BackupProfileIdentifier) == "on";

            // Re-enable the saving of the values now that we've done our work.
            listBox_ExtendedRangeTunings.SelectedIndexChanged += new System.EventHandler(Save_ExtendedRangeTuningAt);
            nUpDown_ForceEnumerationXMS.ValueChanged += new System.EventHandler(Save_EnumerateEveryXMS);
            listBox_AvailableASIODevices_Input0.SelectedIndexChanged += new System.EventHandler(ASIO_ListAvailableInput0);
            listBox_AvailableASIODevices_Input1.SelectedIndexChanged += new System.EventHandler(ASIO_ListAvailableInput1);
            listBox_AvailableASIODevices_Output.SelectedIndexChanged += new System.EventHandler(ASIO_ListAvailableOutput);
            listBox_AvailableASIODevices_InputMic.SelectedIndexChanged += new System.EventHandler(ASIO_ListAvailableInputMic);
            checkBox_ASIO_Output_Disabled.CheckedChanged += new System.EventHandler(ASIO_Output_Disable);
            checkBox_ASIO_Input0_Disabled.CheckedChanged += new System.EventHandler(ASIO_Input0_Disable);
            checkBox_ASIO_Input1_Disabled.CheckedChanged += new System.EventHandler(ASIO_Input1_Disable);
            checkBox_ASIO_InputMic_Disabled.CheckedChanged += new System.EventHandler(ASIO_InputMic_Disable);
            checkBox_ER_SeparateNoteColors.CheckedChanged += new System.EventHandler(Save_ER_SeparateNoteColors);
            checkBox_BackupProfile.CheckedChanged += new System.EventHandler(Save_BackupProfile);
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
            CustomTheme_DataGridView(dgv_Profiles_Songlists, backgroundColor, textColor);

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
            SaveSettings_Save(ReadSettings.CustomGUIThemeIdentifier, checkBox_ChangeTheme.Checked.ToString().ToLower());
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
                SaveSettings_Save(ReadSettings.CustomGUIBackgroundColorIdentifier, (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6"));
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
                SaveSettings_Save(ReadSettings.CustomGUITextColorIdentifier, (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6"));
                textBox_ChangeTextColor.BackColor = colorDialog.Color;
            }
        }

        private void CustomTheme_Apply(object sender, EventArgs e) => CustomTheme_ChangeTheme(textBox_ChangeBackgroundColor.BackColor, textBox_ChangeTextColor.BackColor);

        #endregion
        #region Check For Keypresses (Keybindings)
        private void Keypress_CheckDown(object sender, KeyEventArgs e)
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

        private void Keypress_CheckUp(object sender, KeyEventArgs e)
        {
            if (KeyConversion.KeyUpDictionary.Contains(e.KeyCode))
            {
                if (sender == textBox_NewKeyAssignment_MODS)
                    textBox_NewKeyAssignment_MODS.Text = e.KeyCode.ToString();
                else if (sender == textBox_NewKeyAssignment_AUDIO)
                    textBox_NewKeyAssignment_AUDIO.Text = e.KeyCode.ToString();
            }

        }

        private void Keypress_CheckMouse(object sender, MouseEventArgs e)
        {
            if (KeyConversion.MouseButtonDictionary.Contains(e.Button))
            {
                if (sender == textBox_NewKeyAssignment_MODS)
                    textBox_NewKeyAssignment_MODS.Text = e.Button.ToString();
                else if (sender == textBox_NewKeyAssignment_AUDIO)
                    textBox_NewKeyAssignment_AUDIO.Text = e.Button.ToString();
            }

        }

        private void Keypress_LoadKeys(object sender, EventArgs e) => textBox_NewKeyAssignment_MODS.Text = Dictionaries.refreshKeybindingList()[listBox_Modlist_MODS.SelectedIndex];
        private void Keypress_LoadVolumes(object sender, EventArgs e) => textBox_NewKeyAssignment_AUDIO.Text = Dictionaries.refreshAudioKeybindingList()[listBox_Modlist_AUDIO.SelectedIndex];
        #endregion
        #region Reset To Default
        private void Reset_DefaultSettings(object sender, EventArgs e)
        {
            if (MessageBox.Show("Are you sure you want to reset your mod settings to their defaults?", "WARNING: RESET TO DEFAULT?", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
            {
                File.Delete(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini"));
                Reset_RefreshForm();
                WriteSettings.WriteINI(WriteSettings.saveSettingsOrDefaults); // Refresh Form will regenerate all the settings, so we need to overwrite them.
            }
            else
                MessageBox.Show("All your settings have been saved, and nothing was reset");
        }

        private void Reset_RefreshForm()
        {
            Hide();
            var newForm = new MainForm();
            newForm.Closed += (s, args) => Close();
            newForm.Show();
        }
        #endregion
        #region Save Settings
        private void SaveSettings_Save(string IdentifierToChange, string ChangedSettingValue)
        {
            // Right before launch, we switched from the boolean names of (true / false) to (on / off) for users to be able to edit the mods without the GUI (by hand).
            if (ChangedSettingValue == "true")
                ChangedSettingValue = "on";
            else if (ChangedSettingValue == "false")
                ChangedSettingValue = "off";

            foreach (string section in WriteSettings.saveSettingsOrDefaults.Keys)
            {
                foreach (KeyValuePair<string, string> entry in WriteSettings.saveSettingsOrDefaults[section])
                {
                    if (IdentifierToChange == entry.Key)
                    {
                        WriteSettings.saveSettingsOrDefaults[section][IdentifierToChange] = ChangedSettingValue;
                        break; // We found what we need, so let's leave.
                    }
                }
            }

            WriteSettings.WriteINI(WriteSettings.saveSettingsOrDefaults);
            SaveSettings_ShowLabel();
            WinMsgUtil.SendMsgToRS("update all");
        }

        private void SaveSettings_ShowLabel()
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
                        SaveSettings_Save(currentSongList, textBox_NewSonglistName.Text);
                        listBox_Songlist.Items[index] = textBox_NewSonglistName.Text;
                        break;
                    };
                }

                Profiles_RefreshSonglistNames();
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
                        SaveSettings_Save(currentKeybinding, KeyConversion.VirtualKey(textBox_NewKeyAssignment_MODS.Text));
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
                        SaveSettings_Save(currentKeybinding, KeyConversion.VirtualKey(textBox_NewKeyAssignment_AUDIO.Text));
                        break;
                    }
                }

                textBox_NewKeyAssignment_AUDIO.Text = String.Empty;
            }
            Startup_ShowCurrentKeybindingValues();
            Startup_ShowCurrentAudioKeybindingValues();
        }

        private void SaveSettings_ASIO_Middleware(string identifierToChange, ASIO.ReadSettings.Sections section, string ChangedSettingValue)
        {
            ASIO.WriteSettings.SaveChanges(identifierToChange, section, ChangedSettingValue, checkBox_ASIO_Output_Disabled.Checked, checkBox_ASIO_Input0_Disabled.Checked, checkBox_ASIO_Input1_Disabled.Checked, checkBox_ASIO_InputMic_Disabled.Checked);
            SaveSettings_ShowLabel();
        }

        private void SaveSettings_Rocksmith_Middleware(string identifierToChange, string ChangedSettingValue)
        {
            Rocksmith.WriteSettings.SaveChanges(identifierToChange, ChangedSettingValue);
            SaveSettings_ShowLabel();
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
                SaveSettings_Save(ReadSettings.CustomStringColorNumberIndetifier, "2"); // Tell the game to use custom colors
                SaveSettings_Save(stringColorButtonIdentifier, (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6"));
                stringNumberToColorTextBox[stringNumber].BackColor = colorDialog.Color;
            }
        }

        private void StringColors_ChangeNoteColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog
            {
                AllowFullOpen = true,
                ShowHelp = false
            };
            bool isNormalNotes = radio_DefaultNoteColors.Checked; // True = Normal, False = Colorblind
            string noteColorButtonIdentifier = String.Empty;
            int noteNumber = 0;

            StringColors_FillNoteNumberToColorDictionary();

            foreach (KeyValuePair<string, string> noteColorButton in Dictionaries.noteColorButtonsToSettingIdentifiers[isNormalNotes])
            {
                if (sender.ToString().Contains(noteColorButton.Key.ToString()))
                {
                    noteColorButtonIdentifier = noteColorButton.Value.ToString();
                    break; // We have the one value we need, so we can leave.
                }
                noteNumber++;
            }

            colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(noteColorButtonIdentifier));

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                SaveSettings_Save(ReadSettings.SeparateNoteColorsIdentifier, "2"); // Tell the game to use custom note colors
                SaveSettings_Save(noteColorButtonIdentifier, (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6"));
                stringNumberToColorTextBox[noteNumber].BackColor = colorDialog.Color;
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
                WriteSettings.WriteINI(WriteSettings.saveSettingsOrDefaults);
        }

        private void StringColors_LoadDefaultNoteColors(bool colorBlind = false)
        {
            if (ReadSettings.ProcessSettings(ReadSettings.Note0Color_N_Identifier) != String.Empty) // Fixes a small use case where the GUI moves faster than the writing of the INI.
            {
                if (!colorBlind)
                {
                    textBox_Note0Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.Note0Color_N_Identifier));
                    textBox_Note1Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.Note1Color_N_Identifier));
                    textBox_Note2Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.Note2Color_N_Identifier));
                    textBox_Note3Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.Note3Color_N_Identifier));
                    textBox_Note4Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.Note4Color_N_Identifier));
                    textBox_Note5Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.Note5Color_N_Identifier));
                }
                else
                {
                    textBox_Note0Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.Note0Color_CB_Identifier));
                    textBox_Note1Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.Note1Color_CB_Identifier));
                    textBox_Note2Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.Note2Color_CB_Identifier));
                    textBox_Note3Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.Note3Color_CB_Identifier));
                    textBox_Note4Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.Note4Color_CB_Identifier));
                    textBox_Note5Color.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.Note5Color_CB_Identifier));
                }
            }
            else
                WriteSettings.WriteINI(WriteSettings.saveSettingsOrDefaults);
        }

        private void StringColors_DefaultStringColors(object sender, EventArgs e) => StringColors_LoadDefaultStringColors();

        private void StringColors_ColorBlindStringColors(object sender, EventArgs e) => StringColors_LoadDefaultStringColors(true);

        private void StringColors_DefaultNoteColors(object sender, EventArgs e) => StringColors_LoadDefaultNoteColors();

        private void StringColors_ColorBlindNoteColors(object sender, EventArgs e) => StringColors_LoadDefaultNoteColors(true);

        #endregion
        #region Noteway Colors
        private void NotewayColors_ChangeNotewayColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new ColorDialog
            {
                AllowFullOpen = true,
                ShowHelp = false
            };

            string notewayColorButtonIdentifier = String.Empty;
            int notewayObject = 0;
            NotewayColors_FillNotewayButtonToColorDictionary();

            foreach (KeyValuePair<string, string> notewayColorButton in Dictionaries.notewayColorButtonsToSettingIdentifier)
            {
                if (sender.ToString().Contains(notewayColorButton.Key.ToString()))
                {
                    notewayColorButtonIdentifier = notewayColorButton.Value.ToString();
                    break; // We have the one value we need, so we can leave.
                }
                notewayObject++;
            }

            if (ReadSettings.ProcessSettings(notewayColorButtonIdentifier) != "")
                colorDialog.Color = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(notewayColorButtonIdentifier));

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                SaveSettings_Save(notewayColorButtonIdentifier, (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6"));
                notewayButtonToColorTextbox[((Button)sender)].BackColor = colorDialog.Color;
            }
        }

        private void NotewayColors_LoadDefaultStringColors()
        {
            if (ReadSettings.ProcessSettings(ReadSettings.CustomHighwayNumberedIdentifier) != "")
                textBox_ShowNumberedFrets.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.CustomHighwayNumberedIdentifier));
            if (ReadSettings.ProcessSettings(ReadSettings.CustomHighwayUnNumberedIdentifier) != "")
                textBox_ShowUnNumberedFrets.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.CustomHighwayUnNumberedIdentifier));
            if (ReadSettings.ProcessSettings(ReadSettings.CustomHighwayGutterIdentifier) != "")
                textBox_ShowNotewayGutter.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.CustomHighwayGutterIdentifier));
            if (ReadSettings.ProcessSettings(ReadSettings.CustomFretNubmersIdentifier) != "")
                textBox_ShowFretNumber.BackColor = ColorTranslator.FromHtml("#" + ReadSettings.ProcessSettings(ReadSettings.CustomFretNubmersIdentifier));
        }

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

        private void SetForget_ResetCache(object sender, EventArgs e)
        {
            if (MessageBox.Show("Woah, hang on there!\nHave you tried pressing the \"Restore Cache Backup\" button?\nThis should be a last resort.\nWe call home to Steam to redownload all modified files.\nThis will only break the mods in this section, nothing else.", "HANG ON!", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
            {
                System.Diagnostics.Process.Start("steam://validate/221680");
                SetAndForgetMods.RemoveTempFolders();
            }

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
            var profileTones = SetAndForgetMods.GetSteamProfilesTones();

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

        private void SetAndForget_AssignNewGuitarArcadeTone(object sender, EventArgs e)
        {
            if (listBox_ProfileTones.SelectedItem == null)
                return;

            int selectedToneType = -1;

            var gaRadioControls = new List<RadioButton>() {
                radio_TempleOfBendsTone, radio_ScaleWarriorsTone, radio_StringsSkipSaloonTone,
                radio_ScaleRacerTone, radio_NinjaSlideNTone, radio_HurtlinHurdlesTone, radio_HarmonicHeistTone,
                radio_DucksReduxTone, radio_RainbowLaserTone, radio_GoneWailinTone};

            for (int i = 0; i < 10; i++)
            {
                if (gaRadioControls[i].Checked)
                {
                    selectedToneType = i;
                    break;
                }
            }

            string selectedToneName = listBox_ProfileTones.SelectedItem.ToString();

            SetAndForgetMods.SetGuitarArcadeTone(selectedToneName, selectedToneType);
        }

        #endregion
        #region Save Setting Middleware

        private void Save_ToggleLoft(object sender, EventArgs e) // Toggle Loft Enabled/ Disabled
        {
            SaveSettings_Save(ReadSettings.ToggleLoftEnabledIdentifier, checkBox_ToggleLoft.Checked.ToString().ToLower());
            checkBox_ToggleLoft.Checked = checkBox_ToggleLoft.Checked;
            radio_LoftAlwaysOff.Visible = checkBox_ToggleLoft.Checked;
            radio_LoftOffHotkey.Visible = checkBox_ToggleLoft.Checked;
            radio_LoftOffInSong.Visible = checkBox_ToggleLoft.Checked;
            groupBox_LoftOffWhen.Visible = checkBox_ToggleLoft.Checked;
        }

        private void Save_SongTimer(object sender, EventArgs e)
        {
            SaveSettings_Save(ReadSettings.ShowSongTimerEnabledIdentifier, checkBox_SongTimer.Checked.ToString().ToLower());
            groupBox_SongTimer.Visible = checkBox_SongTimer.Checked;
        }

        private void Save_ForceEnumeration(object sender, EventArgs e)
        {
            checkBox_ForceEnumeration.Checked = checkBox_ForceEnumeration.Checked;
            radio_ForceEnumerationAutomatic.Visible = checkBox_ForceEnumeration.Checked;
            radio_ForceEnumerationManual.Visible = checkBox_ForceEnumeration.Checked;
            groupBox_HowToEnumerate.Visible = checkBox_ForceEnumeration.Checked;

            if (checkBox_ForceEnumeration.Checked)
                SaveSettings_Save(ReadSettings.ForceReEnumerationEnabledIdentifier, "manual");
            else
                SaveSettings_Save(ReadSettings.ForceReEnumerationEnabledIdentifier, "false");
        }

        private void Save_EnumerateEveryXMS(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.CheckForNewSongIntervalIdentifier, (nUpDown_ForceEnumerationXMS.Value * 1000).ToString());

        private void Save_ForceEnumerationAutomatic(object sender, EventArgs e)
        {
            label_ForceEnumerationXMS.Visible = true;
            nUpDown_ForceEnumerationXMS.Visible = true;
            SaveSettings_Save(ReadSettings.ForceReEnumerationEnabledIdentifier, "automatic");
        }

        private void Save_ForceEnumerationManual(object sender, EventArgs e)
        {
            label_ForceEnumerationXMS.Visible = false;
            nUpDown_ForceEnumerationXMS.Visible = false;
            SaveSettings_Save(ReadSettings.ForceReEnumerationEnabledIdentifier, "manual");
        }

        private void Save_RainbowStrings(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.RainbowStringsEnabledIdentifier, checkBox_RainbowStrings.Checked.ToString().ToLower());

        private void Save_RainbowNotes(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.RainbowNotesEnabledIdentifier, checkBox_RainbowNotes.Checked.ToString().ToLower());

        private void Save_ExtendedRange(object sender, EventArgs e)
        {

            groupBox_ExtendedRangeWhen.Visible = checkBox_ExtendedRange.Checked;
            listBox_ExtendedRangeTunings.Visible = checkBox_ExtendedRange.Checked;
            checkBox_CustomColors.Checked = checkBox_ExtendedRange.Checked;

            SaveSettings_Save(ReadSettings.ExtendedRangeEnabledIdentifier, checkBox_ExtendedRange.Checked.ToString().ToLower());

            if (checkBox_ExtendedRange.Checked)
                SaveSettings_Save(ReadSettings.CustomStringColorNumberIndetifier, "2");
            else
                SaveSettings_Save(ReadSettings.CustomStringColorNumberIndetifier, "0");
        }

        private void Save_CustomStringColors(object sender, EventArgs e)
        {
            groupBox_StringColors.Visible = checkBox_CustomColors.Checked;

            if (checkBox_CustomColors.Checked)
                SaveSettings_Save(ReadSettings.CustomStringColorNumberIndetifier, "2");
            else
                SaveSettings_Save(ReadSettings.CustomStringColorNumberIndetifier, "0");
        }

        // private void Save_DiscoMode(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.DiscoModeIdentifier, DiscoModeCheckbox.Checked.ToString().ToLower());

        private void Save_RemoveHeadstockCheckbox(object sender, EventArgs e)
        {
            SaveSettings_Save(ReadSettings.RemoveHeadstockIdentifier, checkBox_RemoveHeadstock.Checked.ToString().ToLower());
            groupBox_ToggleHeadstockOffWhen.Visible = checkBox_RemoveHeadstock.Checked;
        }

        private void Save_RemoveSkyline(object sender, EventArgs e)
        {
            SaveSettings_Save(ReadSettings.RemoveSkylineIdentifier, checkBox_RemoveSkyline.Checked.ToString().ToLower());
            groupBox_ToggleSkylineWhen.Visible = checkBox_RemoveSkyline.Checked;
        }

        private void Save_GreenScreenWall(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.GreenScreenWallIdentifier, checkBox_GreenScreen.Checked.ToString().ToLower());

        private void Save_AutoLoadLastProfile(object sender, EventArgs e)
        {
            SaveSettings_Save(ReadSettings.ForceProfileEnabledIdentifier, checkBox_AutoLoadProfile.Checked.ToString().ToLower());
            groupBox_AutoLoadProfiles.Visible = checkBox_AutoLoadProfile.Checked;
        }

        private void Save_Fretless(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.FretlessModeEnabledIdentifier, checkBox_Fretless.Checked.ToString().ToLower());

        private void Save_RemoveInlays(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.RemoveInlaysIdentifier, checkBox_RemoveInlays.Checked.ToString().ToLower());

        private void Save_ToggleLoftWhenManual(object sender, EventArgs e)
        {
            if (radio_LoftOffHotkey.Checked)
                SaveSettings_Save(ReadSettings.ToggleLoftWhenIdentifier, "manual");
        }

        private void Save_ToggleLoftWhenSong(object sender, EventArgs e)
        {
            if (radio_LoftOffInSong.Checked)
                SaveSettings_Save(ReadSettings.ToggleLoftWhenIdentifier, "song");
        }

        private void Save_ToggleLoftWhenStartup(object sender, EventArgs e)
        {
            if (radio_LoftAlwaysOff.Checked)
                SaveSettings_Save(ReadSettings.ToggleLoftWhenIdentifier, "startup");
        }

        private void Save_RemoveLaneMarkers(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.RemoveLaneMarkersIdentifier, checkBox_RemoveLaneMarkers.Checked.ToString().ToLower());
        private void Save_ToggleSkylineSong(object sender, EventArgs e)
        {
            if (radio_SkylineOffInSong.Checked)
                SaveSettings_Save(ReadSettings.ToggleSkylineWhenIdentifier, "song");
        }

        private void Save_ToggleSkylineStartup(object sender, EventArgs e)
        {
            if (radio_SkylineAlwaysOff.Checked)
                SaveSettings_Save(ReadSettings.ToggleSkylineWhenIdentifier, "startup");
        }

        private void Save_ExtendedRangeTuningAt(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.ExtendedRangeTuningIdentifier, Convert.ToString((listBox_ExtendedRangeTunings.SelectedIndex * -1) - 2));

        private void Delete_Keybind_MODS(object sender, EventArgs e)
        {
            textBox_NewKeyAssignment_MODS.Text = "";

            foreach (string currentMod in Dictionaries.KeybindingsIndexToINISetting)
            {
                int index = Dictionaries.KeybindingsIndexToINISetting.IndexOf(currentMod);
                if (index == listBox_Modlist_MODS.SelectedIndex)
                {
                    SaveSettings_Save(currentMod, "");
                    break;
                }
            }
            Startup_ShowCurrentKeybindingValues();
        }

        private void Delete_Keybind_AUDIO(object sender, EventArgs e)
        {
            textBox_NewKeyAssignment_AUDIO.Text = "";

            foreach (string currentMod in Dictionaries.AudioKeybindingsIndexToINISetting)
            {
                int index = Dictionaries.AudioKeybindingsIndexToINISetting.IndexOf(currentMod);
                if (index == listBox_Modlist_AUDIO.SelectedIndex)
                {
                    SaveSettings_Save(currentMod, "");
                    break;
                }
            }
            Startup_ShowCurrentAudioKeybindingValues();
        }

        private void Save_RemoveLyrics(object sender, EventArgs e)
        {
            SaveSettings_Save(ReadSettings.RemoveLyricsIdentifier, checkBox_RemoveLyrics.Checked.ToString().ToLower());
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
                SaveSettings_Save(ReadSettings.RemoveLyricsWhenIdentifier, "startup");
        }

        private void Save_ToggleLyricsManual(object sender, EventArgs e)
        {
            if (radio_LyricsOffHotkey.Checked)
                SaveSettings_Save(ReadSettings.RemoveLyricsWhenIdentifier, "manual");
        }

        private void Save_VolumeControls(object sender, EventArgs e)
        {
            groupBox_Keybindings_AUDIO.Visible = checkBox_ControlVolume.Checked;
            groupBox_ControlVolumeIncrement.Visible = checkBox_ControlVolume.Checked;
            groupBox_ControlSongVolumeWhen.Visible = checkBox_ControlVolume.Checked;
            SaveSettings_Save(ReadSettings.VolumeControlEnabledIdentifier, checkBox_ControlVolume.Checked.ToString().ToLower());
        }

        private void Save_HeadstockOffInSongOnlyButton(object sender, EventArgs e)
        {
            if (radio_HeadstockOffInSong.Checked)
                SaveSettings_Save(ReadSettings.RemoveHeadstockWhenIdentifier, "song");
        }

        private void Save_RiffRepeaterSpeedInterval(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.RiffRepeaterSpeedIntervalIdentifier, nUpDown_RiffRepeaterSpeed.Value.ToString());

        private void Save_RiffRepeaterSpeedAboveOneHundred(object sender, EventArgs e)
        {
            SaveSettings_Save(ReadSettings.RiffRepeaterAboveHundredIdentifier, checkBox_RiffRepeaterSpeedAboveOneHundred.Checked.ToString().ToLower());
            groupBox_RRSpeed.Visible = checkBox_RiffRepeaterSpeedAboveOneHundred.Checked;
        }

        private void Save_useMidiAutoTuning(object sender, EventArgs e)
        {
            SaveSettings_Save(ReadSettings.MidiAutoTuningIdentifier, checkBox_useMidiAutoTuning.Checked.ToString().ToLower());
            groupBox_MidiAutoTuneDevice.Visible = checkBox_useMidiAutoTuning.Checked;
        }
        private void Save_AutoTuneDevice(object sender, EventArgs e)
        {
            if (listBox_ListMidiDevices.SelectedIndex != -1)
            {
                SaveSettings_Save(ReadSettings.MidiAutoTuningDeviceIdentifier, listBox_ListMidiDevices.SelectedItem.ToString());
                label_SelectedMidiDevice.Text = "Midi Device: " + listBox_ListMidiDevices.SelectedItem.ToString();
            }
        }

        private void Save_WhammyDT(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.TuningPedalIdentifier, "1");

        private void Save_WhammyBass(object sender, EventArgs e)
        {
            SaveSettings_Save(ReadSettings.TuningPedalIdentifier, "2");
            checkBox_WhammyChordsMode.Visible = radio_WhammyBass.Checked;
        }

        private void Save_Whammy(object sender, EventArgs e)
        {
            SaveSettings_Save(ReadSettings.TuningPedalIdentifier, "3");
            checkBox_WhammyChordsMode.Visible = radio_Whammy.Checked;
        }

        private void Save_WhammyChordsMode(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.ChordsModeIdentifier, checkBox_WhammyChordsMode.Checked.ToString().ToLower());

        private void Save_ExtendedRangeDrop(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.ExtendedRangeDropTuningIdentifier, checkBox_ExtendedRangeDrop.Checked.ToString().ToLower());

        private void Save_ShowCurrentNote(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.ShowCurrentNoteOnScreenIdentifier, checkBox_ShowCurrentNote.Checked.ToString().ToLower());

        private void Save_ScreenShotScores(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.ScreenShotScoresIdentifier, checkBox_ScreenShotScores.Checked.ToString().ToLower());

        private void Save_HeadStockAlwaysOffButton(object sender, EventArgs e)
        {
            if (radio_HeadstockAlwaysOff.Checked)
                SaveSettings_Save(ReadSettings.RemoveHeadstockWhenIdentifier, "startup");
        }

        private void Save_VolumeInterval(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.VolumeControlIntervalIdentifier, Convert.ToInt32(nUpDown_VolumeInterval.Value).ToString());

        private void Save_AutoLoadProfile(object sender, EventArgs e)
        {
            if (listBox_AutoLoadProfiles.SelectedIndex == -1)
                SaveSettings_Save(ReadSettings.ProfileToLoadIdentifier, "");
            else
                SaveSettings_Save(ReadSettings.ProfileToLoadIdentifier, listBox_AutoLoadProfiles.SelectedItem.ToString());
        }

        private void AutoLoadProfile_ClearSelection(object sender, EventArgs e) => listBox_AutoLoadProfiles.ClearSelected();
        private void Save_BackupProfile(object sender, EventArgs e)
        {
            SaveSettings_Save(ReadSettings.BackupProfileIdentifier, checkBox_BackupProfile.Checked.ToString().ToLower());
            groupBox_Backups.Visible = checkBox_BackupProfile.Checked;
            

            if (checkBox_BackupProfile.Checked)
            {
                Profiles.SaveProfile();

                TabController.TabPages.Insert(ProfileEditsTabIndex, ProfileEditsTab);

                if (Profiles.GetSaveDirectory() == String.Empty)
                {
                    MessageBox.Show("It looks like your profile(s) can't be found :(\nWe are disabling the Backup Profile mod so it doesn't look like we're lying to you.");
                    checkBox_BackupProfile.Checked = false;
                }
            }

            else
                TabController.TabPages.Remove(ProfileEditsTab);
        }

        private void UnlimitedBackups(object sender, EventArgs e)
        {
            if (checkBox_UnlimitedBackups.Checked)
                nUpDown_NumberOfBackups.Value = 0;
            else
            {
                nUpDown_NumberOfBackups.Value = 50;
                nUpDown_NumberOfBackups.Enabled = true;
            }
        }

        private void Save_NumberOfBackups(object sender, EventArgs e)
        {
            if (nUpDown_NumberOfBackups.Value == 0)
            {
                nUpDown_NumberOfBackups.Enabled = false;
                checkBox_UnlimitedBackups.Checked = true;
            }
            SaveSettings_Save(ReadSettings.NumberOfBackupsIdentifier, nUpDown_NumberOfBackups.Value.ToString());
        }

        private void Save_CustomHighway(object sender, EventArgs e)
        {
            SaveSettings_Save(ReadSettings.CustomHighwayColorsIdentifier, checkBox_CustomHighway.Checked.ToString().ToLower());
            groupBox_CustomHighway.Visible = checkBox_CustomHighway.Checked;
        }

        private void ResetNotewayColors(object sender, EventArgs e)
        {
            SaveSettings_Save(ReadSettings.CustomHighwayNumberedIdentifier, "");
            SaveSettings_Save(ReadSettings.CustomHighwayUnNumberedIdentifier, "");
            SaveSettings_Save(ReadSettings.CustomHighwayGutterIdentifier, "");
            SaveSettings_Save(ReadSettings.CustomFretNubmersIdentifier, "");

            textBox_ShowNumberedFrets.BackColor = SystemColors.Control;
            textBox_ShowUnNumberedFrets.BackColor = SystemColors.Control;
            textBox_ShowNotewayGutter.BackColor = SystemColors.Control;
            textBox_ShowFretNumber.BackColor = SystemColors.Control;
        }

        private void Save_SongTimerAlways(object sender, EventArgs e)
        {
            if (radio_SongTimerAlways.Checked)
                SaveSettings_Save(ReadSettings.ShowSongTimerWhenIdentifier, "automatic");
        }

        private void Save_SongTimerManual(object sender, EventArgs e)
        {
            if (radio_SongTimerManual.Checked)
                SaveSettings_Save(ReadSettings.ShowSongTimerWhenIdentifier, "manual");
        }

        private void Save_ControlSongVolumeManual(object sender, EventArgs e)
        {
            if (radio_ControlSongVolumeManual.Checked)
                SaveSettings_Save(ReadSettings.ShowSelectedVolumeWhenIdentifier, "manual");
        }

        private void Save_ControlSongVolumeInSong(object sender, EventArgs e)
        {
            if (radio_ControlSongVolumeInSong.Checked)
                SaveSettings_Save(ReadSettings.ShowSelectedVolumeWhenIdentifier, "song");
        }

        private void Save_ControlSongVolumeAlways(object sender, EventArgs e)
        {
            if (radio_ControlSongVolumeAlways.Checked)
                SaveSettings_Save(ReadSettings.ShowSelectedVolumeWhenIdentifier, "automatic");
        }

        private void Save_SecondaryMonitorStartPosition(object sender, EventArgs e)
        {
            Process guiProcess = Process.GetProcessesByName("RSMods")[0];
            IntPtr ptr = guiProcess.MainWindowHandle;
            Rect guiLocation = new Rect();
            GetWindowRect(ptr, ref guiLocation);

            SaveSettings_Save(ReadSettings.SecondaryMonitorXPositionIdentifier, (guiLocation.Left + 8).ToString());
            SaveSettings_Save(ReadSettings.SecondaryMonitorYPositionIdentifier, (guiLocation.Top + 8).ToString());
        }

        private void Save_SecondaryMonitor(object sender, EventArgs e)
        {
            button_SecondaryMonitorStartPos.Visible = checkBox_SecondaryMonitor.Checked;
            SaveSettings_Save(ReadSettings.SecondaryMonitorIdentifier, checkBox_SecondaryMonitor.Checked.ToString().ToLower());
        }

        private void Save_ER_SeparateNoteColors(object sender, EventArgs e)
        {
            SaveSettings_Save(ReadSettings.SeparateNoteColorsIdentifier, Convert.ToInt32(checkBox_ER_SeparateNoteColors.Checked).ToString());

            groupBox_NoteColors.Visible = checkBox_ER_SeparateNoteColors.Checked;
            checkBox_NoteColors_UseRocksmithColors.Checked = checkBox_ER_SeparateNoteColors.Checked;
        }

        private void Save_NoteColors_UseRocksmithColors(object sender, EventArgs e)
        {
            SaveSettings_Save(ReadSettings.SeparateNoteColorsIdentifier, (2 - Convert.ToInt32(checkBox_NoteColors_UseRocksmithColors.Checked)).ToString());

            button_Note0ColorButton.Enabled = !checkBox_NoteColors_UseRocksmithColors.Checked;
            button_Note1ColorButton.Enabled = !checkBox_NoteColors_UseRocksmithColors.Checked;
            button_Note2ColorButton.Enabled = !checkBox_NoteColors_UseRocksmithColors.Checked;
            button_Note3ColorButton.Enabled = !checkBox_NoteColors_UseRocksmithColors.Checked;
            button_Note4ColorButton.Enabled = !checkBox_NoteColors_UseRocksmithColors.Checked;
            button_Note5ColorButton.Enabled = !checkBox_NoteColors_UseRocksmithColors.Checked;

            if (checkBox_NoteColors_UseRocksmithColors.Checked)
            {
                textBox_Note0Color.BackColor = DefaultBackColor;
                textBox_Note1Color.BackColor = DefaultBackColor;
                textBox_Note2Color.BackColor = DefaultBackColor;
                textBox_Note3Color.BackColor = DefaultBackColor;
                textBox_Note4Color.BackColor = DefaultBackColor;
                textBox_Note5Color.BackColor = DefaultBackColor;
            }
            else
                StringColors_LoadDefaultNoteColors(radio_colorBlindERNoteColors.Checked);

            radio_DefaultNoteColors.Enabled = !checkBox_NoteColors_UseRocksmithColors.Checked;
            radio_colorBlindERNoteColors.Enabled = !checkBox_NoteColors_UseRocksmithColors.Checked;
        }

        private void Save_DumpRSModsLogToFile(object sender, EventArgs e)
        {
           if (checkBox_ModsLog.Checked)
               File.Create(Path.Combine(GenUtil.GetRSDirectory(), "RSMods_debug.txt"));
            else
            {
                // First we have to do garbage cleanup since it keeps the file in use way too long.
                GC.Collect();
                GC.WaitForPendingFinalizers();

                // We can finally delete the file :)
                File.Delete(Path.Combine(GenUtil.GetRSDirectory(), "RSMods_debug.txt"));
            }
        }

        #endregion
        #region ToolTips

        bool CreatedToolTipYet = false;

        private void ToolTips_Hide(object sender, EventArgs e)
        {
            if (ActiveForm != null) // This fixes a glitch where if you are hovering over a Control that calls the tooltip, and alt-tab, the program will crash since ActiveFrame turns to null... If the user is highlighting something, and the window becomes null, we need to refrain from trying to hide the tooltip that "does not exist".
            {
                ToolTip.Hide(ActiveForm);
                ToolTip.Active = false;
            };
        }

        public ToolTip currentTooltip = new ToolTip(); // Fixes toolTip duplication glitch.

        private void ToolTips_Show(object sender, EventArgs e)
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
        #region Guitar Speak

        private void GuitarSpeak_Enable(object sender, EventArgs e)
        {
            checkBox_GuitarSpeak.Checked = checkBox_GuitarSpeak.Checked;
            groupBox_GuitarSpeak.Visible = checkBox_GuitarSpeak.Checked;
            checkBox_GuitarSpeakWhileTuning.Visible = checkBox_GuitarSpeak.Checked;
            SaveSettings_Save(ReadSettings.GuitarSpeakIdentifier, checkBox_GuitarSpeak.Checked.ToString().ToLower());
        }

        private void GuitarSpeak_Save(object sender, EventArgs e)
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
                        SaveSettings_Save(entry.Value, outputNoteOctave.ToString());
                        listBox_GuitarSpeakSaved.ClearSelected();

                        foreach (string guitarSpeakItem in listBox_GuitarSpeakSaved.Items)
                        {
                            if (guitarSpeakItem.Contains(listBox_GuitarSpeakKeypress.SelectedItem.ToString()))
                            {
                                listBox_GuitarSpeakSaved.Items.Remove(guitarSpeakItem);
                                break;
                            }
                        }
                        listBox_GuitarSpeakSaved.Items.Add(listBox_GuitarSpeakKeypress.SelectedItem.ToString() + ": " + GuitarSpeak.GuitarSpeakNoteOctaveMath(outputNoteOctave.ToString()));
                        GuitarSpeak_ResetPresets();
                    }
                }

                listBox_GuitarSpeakNote.ClearSelected();
                listBox_GuitarSpeakOctave.ClearSelected();
                listBox_GuitarSpeakKeypress.ClearSelected();
            }
            else
                MessageBox.Show("One, or more, of the Guitar Speak boxes not selected", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        private void GuitarSpeak_TuningOffsets(object sender, EventArgs e)
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

        private void GuitarSpeak_WhileTuning(object sender, EventArgs e) => SaveSettings_Save(ReadSettings.GuitarSpeakTuningIdentifier, checkBox_GuitarSpeakWhileTuning.Checked.ToString().ToLower());

        private void GuitarSpeak_Help(object sender, EventArgs e) => System.Diagnostics.Process.Start("https://pastebin.com/raw/PZ0FQTn0");


        private void GuitarSpeak_ResetPresets()
        {
            listBox_GuitarSpeakSaved.Items.Clear();

            foreach (KeyValuePair<string, string> guitarSpeakKeypress in Dictionaries.RefreshGuitarSpeakPresets())
                listBox_GuitarSpeakSaved.Items.Add(guitarSpeakKeypress.Key + guitarSpeakKeypress.Value);
        }

        private void GuitarSpeak_ClearSavedValue(object sender, EventArgs e)
        {
            int valueToRemove = listBox_GuitarSpeakSaved.SelectedIndex;

            if (valueToRemove == -1)
                return;

            listBox_GuitarSpeakSaved.SelectedIndex = -1;

            SaveSettings_Save(Dictionaries.GuitarSpeakIndexToINISetting[valueToRemove], "");

            GuitarSpeak_ResetPresets();
        }

        #endregion
        #region Prep Twitch
        private void Twitch_Show()
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

            Twitch_SolidNoteColor_Show(false);
        }

        private void Twitch_Setup()
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
                    Twitch_Show();
                }

                e.Value = (bool)e.Value ? "Listening to Twitch events" : "Not listening to twitch events";
            };
            label_IsListeningToEvents.DataBindings.Add(listeningToTwitchBinding);

            checkBox_TwitchForceReauth.Checked = TwitchSettings.Get.ForceReauth;

            foreach (var defaultReward in TwitchSettings.Get.DefaultRewards) // BindingList... yeah, not yet
                dgv_DefaultRewards.Rows.Add(defaultReward.Name, defaultReward.Description);

            foreach (var enabledReward in TwitchSettings.Get.Rewards)
                Twitch_AddRewardToEnabled(enabledReward);

        }

        private void PrepTwitch_LoadSettings()
        {
            TwitchSettings.Get._context = SynchronizationContext.Current;
            TwitchSettings.Get.LoadSettings();
            TwitchSettings.Get.LoadDefaultEffects();
            TwitchSettings.Get.LoadEnabledEffects();
        }
        #endregion
        #region Twitch
        private void Twitch_ReAuthorize(object sender, EventArgs e)
        {
            ImplicitAuth auth = new ImplicitAuth();
            auth.MakeAuthRequest();

            // string authToken = TwitchSettings.Get.AccessToken;
            // while (TwitchSettings.Get.AccessToken == authToken || TwitchSettings.Get.Username == String.Empty) {} // We want to get the new value so we are waiting until this breaks
            // label_AuthorizedAs.Text = $"{TwitchSettings.Get.Username} with channel ID: {TwitchSettings.Get.ChannelID} and access token: {TwitchSettings.Get.AccessToken}";
        }

        private void Twitch_NewAccessToken(object sender, EventArgs e) => checkBox_RevealTwitchAuthToken.Checked = false;

        private void Twitch_AutoScrollLog(object sender, EventArgs e)
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

        private async void Twitch_SaveRewards()
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

        private void Twitch_AddReward(object sender, EventArgs e)
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
                Twitch_AddRewardToEnabled(reward);
            }
            else if (dialogResult == DialogResult.No)
            {
                var reward = new BitsReward();
                reward.Map(selectedReward);
                reward.BitsID = rewardID;

                TwitchSettings.Get.Rewards.Add(reward);
                Twitch_AddRewardToEnabled(reward);
            }
            else
            {
                var reward = new ChannelPointsReward();
                reward.Map(selectedReward);
                reward.PointsID = rewardID;

                TwitchSettings.Get.Rewards.Add(reward);
                Twitch_AddRewardToEnabled(reward);
            }

            MessageBoxManager.Unregister(); // Just making sure our custom msg buttons don't stay enabled
            Twitch_SaveRewards();
        }

        private void Twitch_AddRewardToEnabled(TwitchReward reward) // Just imagine this was a bound list :P
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

            Twitch_SaveRewards();
        }

        private void Twitch_SelectEnabledReward(object sender, EventArgs e)
        {
            if (dgv_EnabledRewards.SelectedRows.Count < 1)
                return;

            var selectedRow = dgv_EnabledRewards.SelectedRows[0];
            var selectedReward = Twitch_GetSelectedReward(selectedRow);
            Twitch_SolidNoteColor_Show(false);

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

            Twitch_SolidNoteColor_Show(true);
        }


        private void Twitch_RemoveReward(object sender, EventArgs e)
        {
            if (dgv_EnabledRewards.SelectedRows.Count < 1)
                return;

            var selectedRow = dgv_EnabledRewards.SelectedRows[0];
            var selectedReward = Twitch_GetSelectedReward(selectedRow);

            if (selectedReward.Name == "Solid color notes")
                Twitch_SolidNoteColor_Show(false);

            if (selectedReward != null)
                TwitchSettings.Get.Rewards.Remove(selectedReward);

            dgv_EnabledRewards.Rows.RemoveAt(selectedRow.Index);

            Twitch_SaveRewards();
        }

        private void Twitch_SetAdditionalMessage(string msg)
        {
            var selectedRow = dgv_EnabledRewards.SelectedRows[0];
            var selectedReward = Twitch_GetSelectedReward(selectedRow);

            if (selectedReward.Name != "Solid color notes")
                return;

            selectedReward.AdditionalMsg = msg;
        }

        private void Twitch_SolidNoteColor_Pick(object sender, EventArgs e)
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
                Twitch_SaveRewards();
            }
        }

        private void Twitch_SolidNoteColor_Random(object sender, EventArgs e)
        {
            textBox_SolidNoteColorPicker.BackColor = Color.White;
            textBox_SolidNoteColorPicker.Text = "Random";

            Twitch_SetAdditionalMessage("Random");
            Twitch_SaveRewards();

            dgv_EnabledRewards.SelectedRows[0].DefaultCellStyle.BackColor = Color.White;
        }

        private void Twitch_SendFakeReward()
        {
            if (dgv_EnabledRewards.CurrentCell == null)
                return;

            PubSub.SendMessageToRocksmith(TwitchSettings.Get.Rewards[dgv_EnabledRewards.CurrentCell.RowIndex]);
        }

        private void Twitch_TestReward(object sender, EventArgs e)
        {
            if (Process.GetProcessesByName("Rocksmith2014").Length == 0)
            {
                TwitchSettings.Get.AddToLog("The game does not appear to be running!");
                return;
            }

            Twitch_SendFakeReward();
        }
        private void Twitch_SolidNoteColor_Show(bool show)
        {
            button_SolidNoteColorPicker.Visible = show;
            textBox_SolidNoteColorPicker.Visible = show;
            button_SolidNoteColorRandom.Visible = show;
        }

        private void Twitch_timerValidate(object sender, EventArgs e)
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

        private void Twitch_ForceReauth(object sender, EventArgs e)
        {
            TwitchSettings.Get.ForceReauth = checkBox_TwitchForceReauth.Checked;
            Twitch_SaveSettings();
        }

        private void Twitch_SaveLog(object sender, EventArgs e)
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

        private void Twitch_CopyCredentialsForDevs(object sender, MouseEventArgs e) => Clipboard.SetText("Send to RSMod Developers ( Discord Ffio#2221 or LovroM8#9999 )\nUsername: " + TwitchSettings.Get.Username + "\nChannel ID: " + TwitchSettings.Get.ChannelID + "\nAccess Token: " + TwitchSettings.Get.AccessToken);

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
        #region Custom Fonts
        private void Fonts_Load() // Not modified from here: https://stackoverflow.com/a/8657854 :eyes:
        {
            InstalledFontCollection fontList = new InstalledFontCollection();
            FontFamily[] fontFamilies = fontList.Families;

            foreach (FontFamily font in fontFamilies)
            {
                listBox_AvailableFonts.Items.Add(font.Name);
            }

            listBox_AvailableFonts.SelectedItem = ReadSettings.ProcessSettings(ReadSettings.OnScreenFontIdentifier);
        }

        private void Fonts_Change(object sender, EventArgs e)
        {
            string fontName = listBox_AvailableFonts.SelectedItem.ToString();
            Font newFontSelected = new Font(fontName, 10.0f, Font.Style, Font.Unit);
            label_FontTestCAPITALS.Font = newFontSelected;
            label_FontTestlowercase.Font = newFontSelected;
            label_FontTestNumbers.Font = newFontSelected;

            SaveSettings_Save(ReadSettings.OnScreenFontIdentifier, fontName);
        }
        #endregion
        #region RS_ASIO

        // Config
        private void ASIO_WASAPI_Output(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.EnableWasapiOutputsIdentifier, ASIO.ReadSettings.Sections.Config, Convert.ToInt32(checkBox_ASIO_WASAPI_Output.Checked).ToString());
        private void ASIO_WASAPI_Input(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.EnableWasapiInputsIdentifier, ASIO.ReadSettings.Sections.Config, Convert.ToInt32(checkBox_ASIO_WASAPI_Input.Checked).ToString());
        private void ASIO_ASIO(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.EnableAsioIdentifier, ASIO.ReadSettings.Sections.Config, Convert.ToInt32(checkBox_ASIO_ASIO.Checked).ToString());

        // Driver
        private void ASIO_ListAvailableInput0(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.Input0, listBox_AvailableASIODevices_Input0.SelectedItem.ToString());
        private void ASIO_ListAvailableInput1(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.Input1, listBox_AvailableASIODevices_Input1.SelectedItem.ToString());
        private void ASIO_ListAvailableOutput(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.Output, listBox_AvailableASIODevices_Output.SelectedItem.ToString());
        private void ASIO_ListAvailableInputMic(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.InputMic, listBox_AvailableASIODevices_InputMic.SelectedItem.ToString());

        // Disable / Comment Out Driver
        private void ASIO_Output_Disable(object sender, EventArgs e)
        {
            if (listBox_AvailableASIODevices_Output.SelectedItem != null)
                SaveSettings_ASIO_Middleware(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.Output, listBox_AvailableASIODevices_Output.SelectedItem.ToString());
            else
                SaveSettings_ASIO_Middleware(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.Output, "");
        }
        private void ASIO_Input0_Disable(object sender, EventArgs e)
        {
            if (listBox_AvailableASIODevices_Input0.SelectedItem != null)
                SaveSettings_ASIO_Middleware(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.Input0, listBox_AvailableASIODevices_Input0.SelectedItem.ToString());
            else
                SaveSettings_ASIO_Middleware(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.Input0, "");
        }
        private void ASIO_Input1_Disable(object sender, EventArgs e)
        {
            if (listBox_AvailableASIODevices_Input1.SelectedItem != null)
                SaveSettings_ASIO_Middleware(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.Input1, listBox_AvailableASIODevices_Input1.SelectedItem.ToString());
            else
                SaveSettings_ASIO_Middleware(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.Input1, "");
        }
        private void ASIO_InputMic_Disable(object sender, EventArgs e)
        {
            if (listBox_AvailableASIODevices_InputMic.SelectedItem != null)
                SaveSettings_ASIO_Middleware(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.InputMic, listBox_AvailableASIODevices_InputMic.SelectedItem.ToString());
            else
                SaveSettings_ASIO_Middleware(ASIO.ReadSettings.DriverIdentifier, ASIO.ReadSettings.Sections.InputMic, "");
        }

        // Buffer Size
        private void ASIO_BufferSize_Driver(object sender, EventArgs e)
        {
            if (radio_ASIO_BufferSize_Driver.Checked)
                SaveSettings_ASIO_Middleware(ASIO.ReadSettings.BufferSizeModeIdentifier, ASIO.ReadSettings.Sections.Asio, "driver");
        }
        private void ASIO_BufferSize_Host(object sender, EventArgs e)
        {
            if (radio_ASIO_BufferSize_Host.Checked)
                SaveSettings_ASIO_Middleware(ASIO.ReadSettings.BufferSizeModeIdentifier, ASIO.ReadSettings.Sections.Asio, "host");
        }
        private void ASIO_BufferSize_Custom(object sender, EventArgs e)
        {
            label_ASIO_CustomBufferSize.Visible = radio_ASIO_BufferSize_Custom.Checked;
            nUpDown_ASIO_CustomBufferSize.Visible = radio_ASIO_BufferSize_Custom.Checked;
            SaveSettings_ASIO_Middleware(ASIO.ReadSettings.BufferSizeModeIdentifier, ASIO.ReadSettings.Sections.Asio, "custom");
        }
        private void ASIO_CustomBufferSize(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.CustomBufferSizeIdentifier, ASIO.ReadSettings.Sections.Asio, nUpDown_ASIO_CustomBufferSize.Value.ToString());

        // Input0 Settings
        private void ASIO_Input0_Channel(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.ChannelIdentifier, ASIO.ReadSettings.Sections.Input0, nUpDown_ASIO_Input0_Channel.Value.ToString());
        private void ASIO_Input0_MaxVolume(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.SoftwareMasterVolumePercentIdentifier, ASIO.ReadSettings.Sections.Input0, nUpDown_ASIO_Input0_MaxVolume.Value.ToString());
        private void ASIO_Input0_MasterVolume(object sender, EventArgs e)
        {
            SaveSettings_ASIO_Middleware(ASIO.ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ASIO.ReadSettings.Sections.Input0, Convert.ToInt32(checkBox_ASIO_Input0_ControlMasterVolume.Checked).ToString());
            label_ASIO_Input0_MaxVolume.Visible = checkBox_ASIO_Input0_ControlMasterVolume.Checked;
            nUpDown_ASIO_Input0_MaxVolume.Visible = checkBox_ASIO_Input0_ControlMasterVolume.Checked;
        }
        private void ASIO_Input0_EndpointVolume(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ASIO.ReadSettings.Sections.Input0, Convert.ToInt32(checkBox_ASIO_Input0_ControlEndpointVolume.Checked).ToString());

        // Input1 Settings
        private void ASIO_Input1_Channel(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.ChannelIdentifier, ASIO.ReadSettings.Sections.Input1, nUpDown_ASIO_Input1_Channel.Value.ToString());
        private void ASIO_Input1_MaxVolume(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.SoftwareMasterVolumePercentIdentifier, ASIO.ReadSettings.Sections.Input1, nUpDown_ASIO_Input1_MaxVolume.Value.ToString());
        private void ASIO_Input1_MasterVolume(object sender, EventArgs e)
        {
            SaveSettings_ASIO_Middleware(ASIO.ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ASIO.ReadSettings.Sections.Input1, Convert.ToInt32(checkBox_ASIO_Input1_ControlMasterVolume.Checked).ToString());
            label_ASIO_Input1_MaxVolume.Visible = checkBox_ASIO_Input1_ControlMasterVolume.Checked;
            nUpDown_ASIO_Input1_MaxVolume.Visible = checkBox_ASIO_Input1_ControlMasterVolume.Checked;
        }
        private void ASIO_Input1_EndpointVolume(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ASIO.ReadSettings.Sections.Input1, Convert.ToInt32(checkBox_ASIO_Input1_ControlEndpointVolume.Checked).ToString());

        // Output Settings
        private void ASIO_Output_BaseChannel(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.BaseChannelIdentifier, ASIO.ReadSettings.Sections.Output, nUpDown_ASIO_Output_BaseChannel.Value.ToString());
        private void ASIO_Output_AltBaseChannel(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.AltBaseChannelIdentifier, ASIO.ReadSettings.Sections.Output, nUpDown_ASIO_Output_AltBaseChannel.Value.ToString());
        private void ASIO_Output_MaxVolume(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.SoftwareMasterVolumePercentIdentifier, ASIO.ReadSettings.Sections.Output, nUpDown_ASIO_Output_MaxVolume.Value.ToString());
        private void ASIO_Output_MasterVolume(object sender, EventArgs e)
        {
            SaveSettings_ASIO_Middleware(ASIO.ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ASIO.ReadSettings.Sections.Output, Convert.ToInt32(checkBox_ASIO_Output_ControlMasterVolume.Checked).ToString());
            label_ASIO_Output_MaxVolume.Visible = checkBox_ASIO_Output_ControlMasterVolume.Checked;
            nUpDown_ASIO_Output_MaxVolume.Visible = checkBox_ASIO_Output_ControlMasterVolume.Checked;
        }
        private void ASIO_Output_EndpointVolume(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ASIO.ReadSettings.Sections.Output, Convert.ToInt32(checkBox_ASIO_Output_ControlEndpointVolume.Checked).ToString());

        // InputMic Settings
        private void ASIO_InputMic_Channel(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.ChannelIdentifier, ASIO.ReadSettings.Sections.InputMic, nUpDown_ASIO_InputMic_Channel.Value.ToString());
        private void ASIO_InputMic_MaxVolume(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.SoftwareMasterVolumePercentIdentifier, ASIO.ReadSettings.Sections.InputMic, nUpDown_ASIO_InputMic_MaxVolume.Value.ToString());
        private void ASIO_InputMic_MasterVolume(object sender, EventArgs e)
        {
            SaveSettings_ASIO_Middleware(ASIO.ReadSettings.EnableSoftwareMasterVolumeControlIdentifier, ASIO.ReadSettings.Sections.InputMic, Convert.ToInt32(checkBox_ASIO_InputMic_ControlMasterVolume.Checked).ToString());
            label_ASIO_InputMic_MaxVolume.Visible = checkBox_ASIO_InputMic_ControlMasterVolume.Checked;
            nUpDown_ASIO_InputMic_MaxVolume.Visible = checkBox_ASIO_InputMic_ControlMasterVolume.Checked;
        }
        private void ASIO_InputMic_EndpointVolume(object sender, EventArgs e) => SaveSettings_ASIO_Middleware(ASIO.ReadSettings.EnableSoftwareEndpointVolumeControlIdentifier, ASIO.ReadSettings.Sections.InputMic, Convert.ToInt32(checkBox_ASIO_InputMic_ControlEndpointVolume.Checked).ToString());

        // Clear Selection
        private void ASIO_ClearSelectedDevice(ListBox deviceList, EventHandler e, ASIO.ReadSettings.Sections section)
        {
            deviceList.SelectedIndexChanged -= e;
            deviceList.SelectedIndex = -1;
            SaveSettings_ASIO_Middleware(ASIO.ReadSettings.DriverIdentifier, section, "");
            deviceList.SelectedIndexChanged += e;
        }
        private void ASIO_Input0_ClearSelection(object sender, EventArgs e) => ASIO_ClearSelectedDevice(listBox_AvailableASIODevices_Input0, ASIO_ListAvailableInput0, ASIO.ReadSettings.Sections.Input0);
        private void ASIO_Input1_ClearSelection(object sender, EventArgs e) => ASIO_ClearSelectedDevice(listBox_AvailableASIODevices_Input1, ASIO_ListAvailableInput1, ASIO.ReadSettings.Sections.Input1);
        private void ASIO_Output_ClearSelection(object sender, EventArgs e) => ASIO_ClearSelectedDevice(listBox_AvailableASIODevices_Output, ASIO_ListAvailableOutput, ASIO.ReadSettings.Sections.Output);
        private void ASIO_InputMic_ClearSelection(object sender, EventArgs e) => ASIO_ClearSelectedDevice(listBox_AvailableASIODevices_InputMic, ASIO_ListAvailableInputMic, ASIO.ReadSettings.Sections.InputMic);

        #endregion
        #region Rocksmith Settings
        // Audio Settings
        private void Rocksmith_EnableMicrophone(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.EnableMicrophoneIdentifier, checkBox_Rocksmith_EnableMicrophone.Checked.ToString().ToLower());
        private void Rocksmith_ExclusiveMode(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.ExclusiveModeIdentifier, checkBox_Rocksmith_ExclusiveMode.Checked.ToString().ToLower());
        private void Rocksmith_LatencyBuffer(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.LatencyBufferIdentifier, nUpDown_Rocksmith_LatencyBuffer.Value.ToString());
        private void Rocksmith_ForceWDM(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.ForceWDMIdentifier, checkBox_Rocksmith_ForceWDM.Checked.ToString().ToLower());
        private void Rocksmith_ForceDirextXSink(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.ForceDirectXSinkIdentifier, checkBox_Rocksmith_ForceDirextXSink.Checked.ToString().ToLower());
        private void Rocksmith_DumpAudioLog(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.DumpAudioLogIdentifier, checkBox_Rocksmith_DumpAudioLog.Checked.ToString().ToLower());
        private void Rocksmith_MaxBufferSize(object sender, EventArgs e)
        {
            SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.MaxOutputBufferSizeIdentifier, nUpDown_Rocksmith_MaxOutputBuffer.Value.ToString());
            if (nUpDown_Rocksmith_MaxOutputBuffer.Value == 0)
                checkBox_Rocksmith_Override_MaxOutputBufferSize.Checked = true;
        }
        private void Rocksmith_RTCOnly(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.RealToneCableOnlyIdentifier, checkBox_Rocksmith_RTCOnly.Checked.ToString().ToLower());
        private void Rocksmith_LowLatencyMode(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.Win32UltraLowLatencyModeIdentifier, checkBox_Rocksmith_LowLatencyMode.Checked.ToString().ToLower());

        private void Rocksmith_AutomateMaxBufferSize(object sender, EventArgs e)
        {
            nUpDown_Rocksmith_MaxOutputBuffer.Enabled = !checkBox_Rocksmith_Override_MaxOutputBufferSize.Checked;
            if (checkBox_Rocksmith_Override_MaxOutputBufferSize.Checked)
                nUpDown_Rocksmith_MaxOutputBuffer.Value = 0;
            else
                nUpDown_Rocksmith_MaxOutputBuffer.Value = 32;
        }

        // Visual Settings
        private void Rocksmith_GamepadUI(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.ShowGamepadUIIdentifier, checkBox_Rocksmith_GamepadUI.Checked.ToString().ToLower());
        private void Rocksmith_ScreenWidth(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.ScreenWidthIdentifier, nUpDown_Rocksmith_ScreenWidth.Value.ToString());
        private void Rocksmith_ScreenHeight(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.ScreenHeightIdentifier, nUpDown_Rocksmith_ScreenHeight.Value.ToString());
        private void Rocksmith_Windowed(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.FullscreenIdentifier, "0");
        private void Rocksmith_NonExclusiveFullScreen(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.FullscreenIdentifier, "1");
        private void Rocksmith_ExclusiveFullScreen(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.FullscreenIdentifier, "2");
        private void Rocksmith_LowQuality(object sender, EventArgs e)
        {
            checkBox_Rocksmith_DepthOfField.Checked = false;
            checkBox_Rocksmith_PostEffects.Checked = true;
            checkBox_Rocksmith_HighResScope.Checked = true;

            checkBox_Rocksmith_DepthOfField.Enabled = false;
            checkBox_Rocksmith_PostEffects.Enabled = false;
            checkBox_Rocksmith_HighResScope.Enabled = false;

            SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.VisualQualityIdentifier, "0");
        }
        private void Rocksmith_MediumQuality(object sender, EventArgs e)
        {
            checkBox_Rocksmith_DepthOfField.Checked = true;
            checkBox_Rocksmith_PostEffects.Checked = true;
            checkBox_Rocksmith_HighResScope.Checked = true;

            checkBox_Rocksmith_DepthOfField.Enabled = false;
            checkBox_Rocksmith_PostEffects.Enabled = false;
            checkBox_Rocksmith_HighResScope.Enabled = false;


            SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.VisualQualityIdentifier, "1");
        }
        private void Rocksmith_HighQuality(object sender, EventArgs e)
        {
            checkBox_Rocksmith_DepthOfField.Checked = true;
            checkBox_Rocksmith_PostEffects.Checked = true;
            checkBox_Rocksmith_HighResScope.Checked = true;

            checkBox_Rocksmith_DepthOfField.Enabled = false;
            checkBox_Rocksmith_PostEffects.Enabled = false;
            checkBox_Rocksmith_HighResScope.Enabled = false;

            SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.VisualQualityIdentifier, "2");
        }

        private void Rocksmith_CustomQuality(object sender, EventArgs e)
        {
            checkBox_Rocksmith_DepthOfField.Enabled = true;
            checkBox_Rocksmith_PostEffects.Enabled = true;
            checkBox_Rocksmith_HighResScope.Enabled = true;

            SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.VisualQualityIdentifier, "3");
        }
        private void Rocksmith_RenderWidth(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.RenderingWidthIdentifier, nUpDown_Rocksmith_RenderWidth.Value.ToString());
        private void Rocksmith_RenderHeight(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.RenderingHeightIdentifier, nUpDown_Rocksmith_RenderHeight.Value.ToString());
        private void Rocksmith_PostEffects(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.EnablePostEffectsIdentifier, checkBox_Rocksmith_PostEffects.Checked.ToString().ToLower());
        private void Rocksmith_Shadows(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.EnableShadowsIdentifier, checkBox_Rocksmith_Shadows.Checked.ToString().ToLower());
        private void Rocksmith_HighResScope(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.EnableHighResScopeIdentifier, checkBox_Rocksmith_HighResScope.Checked.ToString().ToLower());
        private void Rocksmith_DepthOfField(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.EnableDepthOfFieldIdentifier, checkBox_Rocksmith_DepthOfField.Checked.ToString().ToLower());
        private void Rocksmith_PerPixelLighting(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.EnablePerPixelLightingIdentifier, checkBox_Rocksmith_PerPixelLighting.Checked.ToString().ToLower());
        private void Rocksmith_MSAA(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.MsaaSamplesIdentifier, (((Convert.ToInt32(checkBox_Rocksmith_MSAASamples.Checked) * 3) + 1).ToString()));
        private void Rocksmith_DisableBrowser(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.DisableBrowserIdentifier, checkBox_Rocksmith_DisableBrowser.Checked.ToString().ToLower());

        private void Rocksmith_EnableRenderRes(object sender, EventArgs e)
        {
            label_Rocksmith_RenderWidth.Visible = checkBox_Rocksmith_EnableRenderRes.Checked;
            label_Rocksmith_RenderHeight.Visible = checkBox_Rocksmith_EnableRenderRes.Checked;
            nUpDown_Rocksmith_RenderWidth.Visible = checkBox_Rocksmith_EnableRenderRes.Checked;
            nUpDown_Rocksmith_RenderHeight.Visible = checkBox_Rocksmith_EnableRenderRes.Checked;

            if (!checkBox_Rocksmith_EnableRenderRes.Checked)
            {
                nUpDown_Rocksmith_RenderWidth.Value = 0;
                nUpDown_Rocksmith_RenderHeight.Value = 0;
            }
        }

        // Network Settings
        private void Rocksmith_UseProxy(object sender, EventArgs e) => SaveSettings_Rocksmith_Middleware(Rocksmith.ReadSettings.UseProxyIdentifier, checkBox_Rocksmith_UseProxy.Checked.ToString().ToLower());

        #endregion
        #region Profiles

        List<SongData> Songs = new List<SongData>();

        // TODO: Add ODLC / CDLC Only buttons (radios maybe?)
        List<SongData> ODLCOnly = new List<SongData>();
        List<SongData> CDLCOnly = new List<SongData>();
        private string currentUnpackedProfile = String.Empty;

        private void Profiles_RefreshSonglistNames()
        {
            dgv_Profiles_Songlists.Columns[2].HeaderText = ReadSettings.ProcessSettings(ReadSettings.Songlist1Identifier);
            dgv_Profiles_Songlists.Columns[3].HeaderText = ReadSettings.ProcessSettings(ReadSettings.Songlist2Identifier);
            dgv_Profiles_Songlists.Columns[4].HeaderText = ReadSettings.ProcessSettings(ReadSettings.Songlist3Identifier);
            dgv_Profiles_Songlists.Columns[5].HeaderText = ReadSettings.ProcessSettings(ReadSettings.Songlist4Identifier);
            dgv_Profiles_Songlists.Columns[6].HeaderText = ReadSettings.ProcessSettings(ReadSettings.Songlist5Identifier);
            dgv_Profiles_Songlists.Columns[7].HeaderText = ReadSettings.ProcessSettings(ReadSettings.Songlist6Identifier);
        }

        private void Profiles_LoadSongs(object sender, EventArgs e)
        {
            dgv_Profiles_Songlists.ClearSelection();
            dgv_Profiles_Songlists.Rows.Clear();


            Songs = SongManager.ExtractSongData(progressBar_Profiles_LoadPsarcs);
            ODLCOnly = Songs.Where(song => song.ODLC == true).ToList();
            CDLCOnly = Songs.Where(song => song.ODLC == false).ToList();

            Profiles_UnpackProfile();

            List<string> ownedRS1DLC = Profile_Sections.Loaded_Stats.DLCTag.Keys.ToList();

            List<string[]> dlcKeyArrayList = new List<string[]>();
            dlcKeyArrayList.Add(Profile_Sections.Loaded_Songlists.SongLists[0].ToArray());
            dlcKeyArrayList.Add(Profile_Sections.Loaded_Songlists.SongLists[1].ToArray());
            dlcKeyArrayList.Add(Profile_Sections.Loaded_Songlists.SongLists[2].ToArray());
            dlcKeyArrayList.Add(Profile_Sections.Loaded_Songlists.SongLists[3].ToArray());
            dlcKeyArrayList.Add(Profile_Sections.Loaded_Songlists.SongLists[4].ToArray());
            dlcKeyArrayList.Add(Profile_Sections.Loaded_Songlists.SongLists[5].ToArray());
            dlcKeyArrayList.Add(Profile_Sections.Loaded_FavoritesList.FavoritesList.ToArray());

            foreach (SongData song in Songs.ToList())
            {

                if ((song.RS1AppID != 0 && !ownedRS1DLC.Contains(song.RS1AppID.ToString())) || song.Artist == String.Empty || song.Title == String.Empty || !song.Shipping)
                {
                    Songs.Remove(song);
                    continue;
                }

                bool inSonglist1 = false, inSonglist2 = false, inSonglist3 = false, inSonglist4 = false, inSonglist5 = false, inSonglist6 = false, inFavorites = false;

                foreach (string[] dlcKeyArray in dlcKeyArrayList)
                {
                    if (dlcKeyArray.Contains(song.DLCKey))
                    {
                        int songlist = dlcKeyArrayList.IndexOf(dlcKeyArray);

                        switch (songlist)
                        {
                            case 0:
                                inSonglist1 = true;
                                break;
                            case 1:
                                inSonglist2 = true;
                                break;
                            case 2:
                                inSonglist3 = true;
                                break;
                            case 3:
                                inSonglist4 = true;
                                break;
                            case 4:
                                inSonglist5 = true;
                                break;
                            case 5:
                                inSonglist6 = true;
                                break;
                            case 6:
                                inFavorites = true;
                                break;
                            default:
                                break;
                        }
                    }

                }


                dgv_Profiles_Songlists.Rows.Add(song.Artist, song.Title, inSonglist1, inSonglist2, inSonglist3, inSonglist4, inSonglist5, inSonglist6, inFavorites);
            }

            Profiles_RefreshSonglistNames();

            groupBox_Profiles_SongLists.Visible = true;
            button_Profiles_SaveSonglist.Visible = true;
        }

        private void Profiles_UnpackProfile()
        {
            if (currentUnpackedProfile != listBox_Profiles_AvailableProfiles.SelectedItem.ToString())
            {
                currentUnpackedProfile = listBox_Profiles_AvailableProfiles.SelectedItem.ToString();
                Profiles_OpenProfileFromProfileName(listBox_Profiles_AvailableProfiles.SelectedItem.ToString(), progressBar_Profiles_LoadPsarcs);
            }
        }

        private void Profiles_ChangeSelectedProfile(object sender, EventArgs e)
        {
            button_Profiles_LoadSongs.Visible = true;
            groupBox_Profiles_Rewards.Visible = true;
        }

        private void Profiles_OpenProfileFromProfileName(string profileName, ProgressBar progressbar = null) => Profile_Sections.LoadProfileSections(Profiles_GetProfilePathFromName(profileName), progressbar);

        private string Profiles_GetProfilePathFromName(string profileName) => Path.Combine(Profiles.GetSaveDirectory(), Profiles.AvailableProfiles()[profileName] + "_PRFLDB");

        private void Profiles_SaveSonglists(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex < 0)
                return;

            Profiles.EncryptProfiles<Profile_Sections.SongListsRoot>(Profile_Sections.Loaded_Songlists, "SongListsRoot", Profiles.DecryptProfiles(Profiles_GetProfilePathFromName(currentUnpackedProfile)), Profiles_GetProfilePathFromName(listBox_Profiles_AvailableProfiles.SelectedItem.ToString()));
            Profiles.EncryptProfiles<Profile_Sections.FavoritesListRoot>(Profile_Sections.Loaded_FavoritesList, "FavoritesListRoot", Profiles.DecryptProfiles(Profiles_GetProfilePathFromName(currentUnpackedProfile)), Profiles_GetProfilePathFromName(listBox_Profiles_AvailableProfiles.SelectedItem.ToString()));
            MessageBox.Show("Your songlists and favorites have been saved!");
        }

        private void Profiles_UnlockAllRewards(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex > -1)
            {
                if (MessageBox.Show("Are you sure you want to unlock all rewards?\nThat defeats the grind for in-game rewards.", "Are you sure?", MessageBoxButtons.YesNo, MessageBoxIcon.Information) == DialogResult.Yes)
                {
                    Profiles_UnpackProfile();

                    Profile_Sections.LockAndUnlockRewards();
                    Profiles_SaveRewardsToProfile();
                }
            }
            else
                MessageBox.Show("Make sure you have a profile selected!");
        }

        private void Profiles_LockAllRewards(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex > -1)
            {
                if (MessageBox.Show("Are you sure you want to lock all rewards?\nThis will remove all access to in-game rewards.", "Are you sure?", MessageBoxButtons.YesNo, MessageBoxIcon.Information) == DialogResult.Yes)
                {
                    Profiles_UnpackProfile();

                    Profile_Sections.LockAndUnlockRewards(false);
                    Profiles_SaveRewardsToProfile();
                }
            }
            else
                MessageBox.Show("Make sure you have a profile selected!");
        }

        private void Profiles_SaveRewardsToProfile()
        {
            Profiles.EncryptProfiles<Profile_Sections.Prizes>(Profile_Sections.Loaded_Prizes, "Prizes", Profiles.DecryptProfiles(Profiles_GetProfilePathFromName(currentUnpackedProfile)), Profiles_GetProfilePathFromName(listBox_Profiles_AvailableProfiles.SelectedItem.ToString()));
            MessageBox.Show("Changes to Rewards have been saved!");
        }

        private void Profiles_SongToSonglist(int songlistNumber, bool add = true)
        {
            int rowIndex = dgv_Profiles_Songlists.SelectedCells[0].RowIndex;
            string commonName = $"{dgv_Profiles_Songlists[0, rowIndex].Value.ToString()} - {dgv_Profiles_Songlists[1, rowIndex].Value.ToString()}";
            string DLCKey = Songs.FirstOrDefault(song => song.CommonName == commonName).DLCKey;

            if (add && !Profile_Sections.Loaded_Songlists.SongLists[songlistNumber - 1].Contains(DLCKey))
                Profile_Sections.Loaded_Songlists.SongLists[songlistNumber - 1].Add(DLCKey);
            else if (!add && Profile_Sections.Loaded_Songlists.SongLists[songlistNumber - 1].Contains(DLCKey))
                Profile_Sections.Loaded_Songlists.SongLists[songlistNumber - 1].Remove(DLCKey);

        }

        private void Profiles_SongToFavorites(bool add = true)
        {
            int rowIndex = dgv_Profiles_Songlists.SelectedCells[0].RowIndex;
            string commonName = $"{dgv_Profiles_Songlists[0, rowIndex].Value.ToString()} - {dgv_Profiles_Songlists[1, rowIndex].Value.ToString()}";
            string DLCKey = Songs.FirstOrDefault(song => song.CommonName == commonName).DLCKey;

            if (add && !Profile_Sections.Loaded_FavoritesList.FavoritesList.Contains(DLCKey))
                Profile_Sections.Loaded_FavoritesList.FavoritesList.Add(DLCKey);
            else if (!add && Profile_Sections.Loaded_FavoritesList.FavoritesList.Contains(DLCKey))
                Profile_Sections.Loaded_FavoritesList.FavoritesList.Remove(DLCKey);
        }

        private void Profiles_Songlists_DirtyState(object sender, EventArgs e)
        {
            if (dgv_Profiles_Songlists.IsCurrentCellDirty)
                dgv_Profiles_Songlists.CommitEdit(DataGridViewDataErrorContexts.Commit);
        }

        private void Profiles_Songlists_ChangedValue(object sender, DataGridViewCellEventArgs e)
        {
            if (dgv_Profiles_Songlists.Columns[e.ColumnIndex].CellType == typeof(DataGridViewCheckBoxCell) && e.RowIndex > -1)
            {
                bool isChecked = Convert.ToBoolean(dgv_Profiles_Songlists[e.ColumnIndex, e.RowIndex].Value.ToString().ToLower());

                if (e.ColumnIndex == 8)
                {
                    // Favorites
                    if (isChecked)
                        Profiles_SongToFavorites();
                    else
                        Profiles_SongToFavorites(false);
                }
                else
                {
                    // Songlists
                    if (isChecked)
                        Profiles_SongToSonglist(e.ColumnIndex - 1);
                    else
                        Profiles_SongToSonglist(e.ColumnIndex - 1, false);
                }
            }
        }

        private void Profiles_RevertToBackup(object sender, EventArgs e)
        {
            if (listBox_Profiles_ListBackups.SelectedIndex < 0)
                return;

            string localizedName = listBox_Profiles_ListBackups.SelectedItem.ToString();

            int monthNumber = DateTime.ParseExact(localizedName.Split(' ')[0], "MMM", CultureInfo.CurrentCulture).Month;

            string[] localizedSplit = localizedName.Split(' ');

            string time = localizedSplit[4];

            time = time.Replace(':', '-');

            string month = monthNumber.ToString();

            if (monthNumber < 10)
                month = "0" + monthNumber.ToString();

            string backupName = month + '-' + localizedSplit[1] + '-' + localizedSplit[2] + '_' + time;

            foreach (string profile in Directory.GetFiles(Path.Combine(GenUtil.GetRSDirectory(), "Profile_Backups", backupName)))
                File.Copy(profile, Path.Combine(Profiles.GetSaveDirectory(), Path.GetFileName(profile)), true);


            MessageBox.Show($"Reverted to the backup: {localizedName}");
        }

        #endregion
        #region Midi

        private void Midi_LoadDevices(object sender, EventArgs e)
        {
            this.listBox_ListMidiDevices.Items.Clear();

            uint numberOfMidiOutDevices = Midi.midiOutGetNumDevs();

            for (uint deviceNumber = 0; deviceNumber < numberOfMidiOutDevices; deviceNumber++)
            {
                Midi.MIDIOUTCAPS temp = new Midi.MIDIOUTCAPS { };
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