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
using System.Net;
using System.Net.Http;
using System.ComponentModel;
using Newtonsoft.Json.Linq;
using System.Net.Http.Headers;
using RocksmithToolkitLib.DLCPackage;
using RocksmithToolkitLib.DLCPackage.Manifest2014.Tone;
using RocksmithToolkitLib.Ogg;
using NAudio.Wave;
using NAudio.CoreAudioApi;
using SevenZip;
using Rocksmith2014PsarcLib.Psarc.Models.Json;
using ArrangementTuning = Rocksmith2014PsarcLib.Psarc.Models.Json.SongArrangement.ArrangementAttributes.ArrangementTuning;
using static RSMods.RsModsSettings;
using RSMods.ASIO;
using RSMods.Rocksmith;

namespace RSMods
{
    public partial class MainForm : Form
    {
        /// <summary>
        /// Leaving this boolean in-case we need to quickly disable the Profiles tab due to a bug.
        /// </summary>
        private readonly bool shipProfileEdits = true;

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

        private readonly bool AllowSaving;

        public MainForm()
        {
            Startup_LocateRocksmith(GenUtil.GetRSDirectory());

            Startup_ReadIniOrCreateDefault();

            RsModsSettings.SettingChanged += OnSettingChanged;
            RocksmithSettings.SettingChanged += OnSettingChanged;
            AsioSettings.SettingChanged += OnSettingChanged;

            PrepTwitch_LoadSettings();

            Startup_InitWinForms();

            Startup_LocateSaves(GenUtil.GetSaveFolder());

            ExeUtil.CheckInstallIntegrity(GenUtil.GetRSDirectory());
            Constants.SaveBaseSettings();

            Twitch_Setup();

            Startup_FixLegacySonglistBug();

            Startup_LoadKeybindingModNames();
            Startup_LoadAudioKeybindings();
            Startup_ShowCurrentKeybindingValues();
            Startup_ShowCurrentAudioKeybindingValues();

            GuitarSpeak_ResetPresets();

            StringColors_LoadDefaultStringColors();
            StringColors_LoadDefaultNoteColors();
            NotewayColors_LoadDefaultStringColors();

            CustomTheme_LoadCustomColors();

            // Load Input Devices for Override Input Device Volume mod
            Startup_LoadInputDevices();

            Midi_LoadDevices();

            Startup_VerifyInstallOfASIO();
            PriorSettings_LoadASIOSettings();

            PriorSettings_LoadRocksmithSettings();

            Startup_LoadRocksmithProfiles();

            Startup_UnpackCachePsarc();

            SetForget_LoadSetAndForgetMods();

            Fonts_Load();

            // Backup Profiles Just In Case
            Startup_BackupProfiles();

            // Load Checkbox Values From RSMods.ini
            PriorSettings_LoadModSettings();

            // Delete Old Backups To Save Space (if user specifies)
            Startup_DeleteOldBackups(GUISettings.NumberOfBackups);

            // Lock the profile edits tab if backups are disabled
            Startup_LockProfileEdits();

            // Get list of all backups so we can revert to one if needed
            Startup_ListAllBackups();

            GithubUpdater.CallGithubAPI();

            Startup_ShowUpdateButton();

            Startup_CheckStatusAudioPsarc();

            // Load SoundPacks Result Voice Over list
            SoundPacks_LoadResultVoiceOverList();

            AllowSaving = true;
        }

        #region Startup Functions

        private void Startup_ReadIniOrCreateDefault()
        {
            RsModsSettings.LoadSettingsFromINI();
        }

        private void Startup_InitWinForms()
        {
            InitializeComponent();
            Text = $"{Text}-{Assembly.GetExecutingAssembly().GetName().Version}"; // Show version number in the title of the application.
        }

        private void Startup_FixLegacySonglistBug()
        {
            if (string.IsNullOrEmpty(GetSongListTitle(1)))
            {
                SetSongListTitle(1, "Define Song List 1 Here");
            }
        }

        private void Startup_LocateRocksmith(string rocksmithFolder)
        {
            if (!string.IsNullOrEmpty(rocksmithFolder) && !Directory.Exists(rocksmithFolder))
            {
                MessageBox.Show("It looks like your current Rocksmith2014 install folder cannot be found. Please tell us where it is located!",
                                "Error: Rocksmith Location Not Found", MessageBoxButtons.OK, MessageBoxIcon.Error);

                rocksmithFolder = string.Empty;
            }

            if (string.IsNullOrEmpty(rocksmithFolder))
            {
                rocksmithFolder = GenUtil.AskUserForRSFolder();

                if (string.IsNullOrEmpty(rocksmithFolder))
                {
                    MessageBox.Show("We cannot detect where you have Rocksmith located. Please try reinstalling your game on Steam.",
                                    "Error: Rocksmith Location Not Found", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    Environment.Exit(1);
                }
            }

            Constants.RSFolder = rocksmithFolder;
        }

        private string PromptForSavePath()
        {
            MessageBox.Show("It looks like your Rocksmith 2014 save folder cannot be found. Please tell us where it is located!\nThis can be found in your Steam install folder.\n<Path To Steam Install>/userdata/#/221680/remote", "Error: SavePath Not Found", MessageBoxButtons.OK, MessageBoxIcon.Error);
            string path = GenUtil.AskUserForSavePath();

            if (string.IsNullOrEmpty(path))
            {
                MessageBox.Show("We cannot detect where your Rocksmith 2014 saves are located.", "Error: SavePath Not Found", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Environment.Exit(1);
            }

            return path;
        }

        private void Startup_LocateSaves(string savePath)
        {
            if (Constants.SavePathDeclined)
            {
                button_SetSavePath.Visible = true;
                return;
            }

            if (string.IsNullOrEmpty(savePath))
            {
                savePath = PromptForSavePath();
            }
            else if (!Directory.Exists(savePath))
            {
                string fromRegistry = Profiles.GetSaveDirectory(true);
                savePath = string.IsNullOrEmpty(fromRegistry) ? PromptForSavePath() : fromRegistry;
            }

            Constants.SavePath = savePath;
            Constants.SavePathDeclined = false;
        }

        private void Startup_LoadSonglists()
        {
            foreach (string songlist in Dictionaries.RefreshSongLists())
                listBox_Songlist.Items.Add(songlist);
        }

        private void Startup_LoadKeybindingModNames()
        {
            foreach (var mod in Dictionaries.ModKeybinds)
                listBox_Modlist_MODS.Items.Add(mod.DisplayName);
        }

        private void Startup_LoadAudioKeybindings()
        {
            foreach (var volume in Dictionaries.AudioKeybinds)
                listBox_Modlist_AUDIO.Items.Add(volume.DisplayName);
        }

        private void Startup_ShowCurrentKeybindingValues()
        {
            label_ToggleLoftKey.Text = "Toggle Loft: " + KeyConversion.VKeyToUI(Keybinds.ToggleLoftKey);
            label_SongTimerKey.Text = "Show Song Timer: " + KeyConversion.VKeyToUI(Keybinds.ShowSongTimerKey);
            label_ReEnumerationKey.Text = "Force ReEnumeration: " + KeyConversion.VKeyToUI(Keybinds.ForceReEnumerationKey);
            label_RainbowStringsKey.Text = "Rainbow Strings: " + KeyConversion.VKeyToUI(Keybinds.RainbowStringsKey);
            label_RainbowNotesKey.Text = "Rainbow Notes: " + KeyConversion.VKeyToUI(Keybinds.RainbowNotesKey);
            label_RemoveLyricsKey.Text = "Remove Lyrics: " + KeyConversion.VKeyToUI(Keybinds.RemoveLyricsKey);
            label_RRSpeedKey.Text = "RR Speed: " + KeyConversion.VKeyToUI(Keybinds.RRSpeedKey);
            label_TuningOffsetKey.Text = "Tuning Offset: " + KeyConversion.VKeyToUI(Keybinds.TuningOffsetKey);
            label_ToggleExtendedRangeKey.Text = "Toggle Extended Range: " + KeyConversion.VKeyToUI(Keybinds.ToggleExtendedRangeKey);
            label_LoopStartKey.Text = "Start Loop: " + KeyConversion.VKeyToUI(Keybinds.LoopStartKey);
            label_LoopEndKey.Text = "End Loop: " + KeyConversion.VKeyToUI(Keybinds.LoopEndKey);
            label_RewindKey.Text = "Rewind Song: " + KeyConversion.VKeyToUI(Keybinds.RewindKey);
        }

        private void Startup_ShowCurrentAudioKeybindingValues()
        {
            label_MasterVolumeKey.Text = "Master Volume: " + KeyConversion.VKeyToUI(AudioKeybindings.MasterVolumeKey);
            label_SongVolumeKey.Text = "Song Volume: " + KeyConversion.VKeyToUI(AudioKeybindings.SongVolumeKey);
            label_Player1VolumeKey.Text = "Player1 Volume: " + KeyConversion.VKeyToUI(AudioKeybindings.Player1VolumeKey);
            label_Player2VolumeKey.Text = "Player2 Volume: " + KeyConversion.VKeyToUI(AudioKeybindings.Player2VolumeKey);
            label_MicrophoneVolumeKey.Text = "Microphone Volume: " + KeyConversion.VKeyToUI(AudioKeybindings.MicrophoneVolumeKey);
            label_VoiceOverVolumeKey.Text = "Voice-Over Volume: " + KeyConversion.VKeyToUI(AudioKeybindings.VoiceOverVolumeKey);
            label_SFXVolumeKey.Text = "SFX Volume: " + KeyConversion.VKeyToUI(AudioKeybindings.SFXVolumeKey);
            label_DisplayMixerKey.Text = "Display Mixer: " + KeyConversion.VKeyToUI(AudioKeybindings.DisplayMixerKey);
            label_MutePlayer1Key.Text = "Mute / Unmute Player1: " + KeyConversion.VKeyToUI(AudioKeybindings.MutePlayer1Key);
            label_MutePlayer2Key.Text = "Mute / Unmute Player2: " + KeyConversion.VKeyToUI(AudioKeybindings.MutePlayer2Key);
        }

        private void Startup_LoadInputDevices()
        {
            MMDeviceEnumerator enumerator = new();

            foreach (MMDevice device in enumerator.EnumerateAudioEndPoints(DataFlow.Capture, DeviceState.Active))
            {
                listBox_AvailableInputDevices.Items.Add(device.FriendlyName);
            }
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
            if (!AsioSettings.SettingsExist)
                TabController.TabPages.Remove(tab_RSASIO);
            else
                Startup_LoadASIODevices();
        }

        private void Startup_LoadRocksmithProfiles()
        {
            Dictionary<string, string> AvailableProfiles = Profiles.AvailableProfiles();

            foreach (KeyValuePair<string, string> profileData in AvailableProfiles)
            {
                listBox_AutoLoadProfiles.Items.Add(profileData.Key);
                listBox_Profiles_AvailableProfiles.Items.Add(profileData.Key);
            }

            try
            {
                if (AvailableProfiles?.Count > 0)
                {
                    int MaxSongLists = 6;
                    foreach (string prf in AvailableProfiles.Keys)
                    {
                        JObject decPrf = JObject.Parse(Profiles.DecryptProfiles(Profiles_GetProfilePathFromName(prf)));

                        // Check how many song lists
                        int TotalSongLists = decPrf["SongListsRoot"]["SongLists"].ToObject<List<List<string>>>().Count;
                        decPrf = null;

                        if (TotalSongLists > MaxSongLists)
                            MaxSongLists = TotalSongLists;
                    }

                    label_TotalSonglists.Text = MaxSongLists.ToString();

                    Profiles_Helper_GenerateValidSonglists(MaxSongLists);
                }
                // We cannot load the Rocksmith profiles.
                else
                {
                    Startup_LoadSonglists();
                }
            }
            catch
            {
                Startup_LoadSonglists();
            }
        }

        private void Startup_DeleteOldBackups(int maxAmountOfBackups)
        {
            if (maxAmountOfBackups == 0) // User says they want all the backups.
                return;

            string backupFolder = Path.Combine(RSMods.Data.Constants.RSFolder, "Profile_Backups");

            if (!Directory.Exists(backupFolder))
                return;

            DirectoryInfo[] backups = [.. new DirectoryInfo(backupFolder).GetDirectories().OrderBy(f => f.LastWriteTime)];

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
            if (GUISettings.BackupProfile)
                Profiles.SaveProfile();
        }

        private void Startup_UnlockProfileEdits()
        {
            if (GUISettings.BackupProfile && shipProfileEdits)
                TabController.TabPages.Insert(ProfileEditsTabIndex, tab_Profiles);
        }

        private void Startup_LockProfileEdits()
        {
            ProfileEditsTab = tab_Profiles;
            ProfileEditsTabIndex = TabController.TabPages.IndexOf(ProfileEditsTab);

            if (!GUISettings.BackupProfile || !shipProfileEdits || !Constants.SavePath.IsSavePath())
                TabController.TabPages.Remove(tab_Profiles);
        }

        private void Startup_ListAllBackups()
        {
            try
            {
                List<string> backups = [];
                foreach (string backup in Directory.GetDirectories(Path.Combine(GenUtil.GetRSDirectory(), "Profile_Backups")))
                {
                    string folderName = Path.GetFileNameWithoutExtension(backup);
                    string date = folderName.Split('_')[0];
                    string time = folderName.Split('_')[1];

                    int month = Convert.ToInt32(date.Split('-')[0]);
                    int day = Convert.ToInt32(date.Split('-')[1]);
                    int year = Convert.ToInt32(date.Split('-')[2]);

                    string userFriendlyName = CultureInfo.CurrentCulture.DateTimeFormat.GetAbbreviatedMonthName(month) + " " + day + " " + year + " @ " + time.Replace('-', ':');

                    backups.Add(userFriendlyName);
                }
                backups.Reverse();
                backups.ForEach(b => listBox_Profiles_ListBackups.Items.Add(b));
            }
            catch // Folder doesn't exist
            {
            }
        }

        private void Startup_ShowUpdateButton() => button_UpdateRSMods.Visible = GithubUpdater.IsUpdateAvailable();
    
        private void CheckForUpdates_UpdateRSMods(object sender, EventArgs e) => GithubUpdater.DownloadAndRunInstaller();

        private void Startup_CheckStatusAudioPsarc() => SoundPacks_ChangeUIForUnpackedFolder(Directory.Exists("audio_psarc"));

        private void Startup_UnpackCachePsarc() => SetAndForgetMods.UnpackCachePsarc();

        #endregion
        #region Show Prior Settings In GUI
        private void PriorSettings_LoadModSettings()
        {
            if (Toggles.ToggleLoft)
            {
                checkBox_ToggleLoft.Checked = true;
                radio_LoftAlwaysOff.Visible = true;
                radio_LoftOffHotkey.Visible = true;
                radio_LoftOffInSong.Visible = true;
                groupBox_LoftOffWhen.Visible = true;

                if (Toggles.ToggleLoftWhen == LoftMode.Startup)
                    radio_LoftAlwaysOff.Checked = true;
                else if (Toggles.ToggleLoftWhen == LoftMode.Manual)
                    radio_LoftOffHotkey.Checked = true;
                else if (Toggles.ToggleLoftWhen == LoftMode.Song)
                    radio_LoftOffInSong.Checked = true;
            }

            if (Toggles.VolumeControl)
            {
                checkBox_ControlVolume.Checked = true;
                groupBox_Keybindings_AUDIO.Visible = true;
                groupBox_ControlVolumeIncrement.Visible = true;

                nUpDown_VolumeInterval.Value = GenUtil.Clamp(ModSettings.VolumeControlInterval, (int)nUpDown_VolumeInterval.Minimum, (int)nUpDown_VolumeInterval.Maximum);
            }

            if (Toggles.ShowSongTimer)
            {
                checkBox_SongTimer.Checked = true;
                groupBox_SongTimer.Visible = true;
                if (Toggles.ShowSongTimerWhen == SongTimerWhen.Automatic)
                    radio_SongTimerAlways.Checked = true;
                else
                    radio_SongTimerManual.Checked = true;
            }

            if (Toggles.ForceReEnumeration != EnumerationMode.Off)
            {
                radio_ForceEnumerationAutomatic.Visible = true;
                radio_ForceEnumerationManual.Visible = true;
                groupBox_HowToEnumerate.Visible = true;
                if (Toggles.ForceReEnumeration == EnumerationMode.Automatic)
                    radio_ForceEnumerationAutomatic.Checked = true;
                else
                    radio_ForceEnumerationManual.Checked = true;
            }

            if (Toggles.ExtendedRange)
            {
                checkBox_ExtendedRange.Checked = true;
                groupBox_ExtendedRangeWhen.Visible = true;
                listBox_ExtendedRangeTunings.Visible = true;

                checkBox_ExtendedRangeDrop.Checked = Toggles.ExtendedRangeDropTuning;
            }

            if (Toggles.CustomStringColors != CustomStringColorMode.Off)
            {
                checkBox_CustomColors.Checked = true;
                groupBox_StringColors.Visible = true;
            }

            if (Toggles.Headstock)
            {
                checkBox_RemoveHeadstock.Checked = true;
                groupBox_ToggleHeadstockOffWhen.Visible = true;

                if (Toggles.RemoveHeadstockWhen == HeadstockMode.Startup)
                    radio_HeadstockAlwaysOff.Checked = true;
                else if (Toggles.RemoveHeadstockWhen == HeadstockMode.Song)
                    radio_HeadstockOffInSong.Checked = true;
            }

            if (Toggles.Skyline)
            {
                checkBox_RemoveSkyline.Checked = true;
                groupBox_ToggleSkylineWhen.Visible = true;

                if (Toggles.ToggleSkylineWhen == SkylineMode.Song) // On Song Load
                    radio_SkylineOffInSong.Checked = true;
                else if (Toggles.ToggleSkylineWhen == SkylineMode.Startup) // On Game Startup
                    radio_SkylineAlwaysOff.Checked = true;
            }

            if (Toggles.ForceProfileLoad) // Force Load Profile On Game Boot
            {
                checkBox_AutoLoadProfile.Checked = true;
                if (Toggles.ProfileToLoad != "")
                    listBox_AutoLoadProfiles.SelectedItem = Toggles.ProfileToLoad;
            }

            if (Toggles.Lyrics)
            {
                checkBox_RemoveLyrics.Checked = true;
                groupBox_ToggleLyricsOffWhen.Visible = true;

                if (Toggles.RemoveLyricsWhen == LyricsMode.Startup)
                    radio_LyricsAlwaysOff.Checked = true;
                else if (Toggles.RemoveLyricsWhen == LyricsMode.Manual)
                    radio_LyricsOffHotkey.Checked = true;
            }

            if (Toggles.GuitarSpeak)
            {
                checkBox_GuitarSpeak.Checked = true;
                groupBox_GuitarSpeak.Visible = true;
                checkBox_GuitarSpeakWhileTuning.Visible = true;
            }

            if (Toggles.RRSpeedAboveOneHundred)
            {
                checkBox_RiffRepeaterSpeedAboveOneHundred.Checked = true;
                groupBox_RRSpeed.Visible = true;

                nUpDown_RiffRepeaterSpeed.Value = GenUtil.Clamp(ModSettings.RRSpeedInterval, nUpDown_RiffRepeaterSpeed.Minimum, nUpDown_RiffRepeaterSpeed.Maximum);
            }

            if (Toggles.AutoTuneForSong)
            {
                checkBox_useMidiAutoTuning.Checked = true;
                groupBox_MidiAutoTuneDevice.Visible = true;
                label_SelectedMidiOutDevice.Text = "Midi Device: " + Toggles.AutoTuneForSongDevice;
                groupBox_MidiAutoTuningOffset.Visible = true;
                listBox_MidiAutoTuningOffset.SelectedIndex = ModSettings.TuningOffset + 3;
                groupBox_MidiAutoTuningWhen.Visible = true;
                //groupBox_MidiInDevice.Visible = true;
                label_SelectedMidiInDevice.Text = "Midi Device: " + Toggles.MidiInDevice;

                switch (ModSettings.TuningPedal)
                {
                    case TuningPedalDevice.WhammyDT:
                        radio_WhammyDT.Checked = true;
                        break;
                    case TuningPedalDevice.WhammyBass:
                        radio_WhammyBass.Checked = true;
                        checkBox_WhammyFiveChordsMode.Visible = true;
                        break;
                    case TuningPedalDevice.WhammyFive:
                        radio_WhammyFive.Checked = true;
                        checkBox_WhammyFiveChordsMode.Visible = true;
                        break;
                    case TuningPedalDevice.SoftwarePedal:
                        radio_SoftwarePedal.Checked = true;
                        break;
                }

                switch (Toggles.AutoTuneForSongWhen)
                {
                    default: // Intentional fall-through
                    case AutoTuneWhen.Manual:
                        radio_AutoTuningWhenManual.Checked = true;
                        break;
                    case AutoTuneWhen.Tuner:
                        radio_AutoTuningWhenTuner.Checked = true;
                        break;
                }
            }

            if (GUISettings.BackupProfile)
            {
                nUpDown_NumberOfBackups.Value = GenUtil.Clamp(GUISettings.NumberOfBackups, (int)nUpDown_NumberOfBackups.Minimum, (int)nUpDown_NumberOfBackups.Maximum);
                groupBox_Backups.Visible = true;
            }

            if (Toggles.OverrideInputVolumeEnabled)
            {
                checkBox_OverrideInputVolume.Checked = true;
                groupBox_OverrideInputVolume.Visible = true;
            }

            checkBox_EnableLooping.Checked = Toggles.AllowLooping;
            groupBox_LoopingLeadUp.Visible = checkBox_EnableLooping.Checked;
            nUpDown_LoopingLeadUp.Value = GenUtil.Clamp(ModSettings.LoopingLeadUp / 1000m, nUpDown_LoopingLeadUp.Minimum, nUpDown_LoopingLeadUp.Maximum);
            listBox_ExtendedRangeTunings.SelectedIndex = (ModSettings.ExtendedRangeModeAt * -1) - 2; // Loads old ER tuning settings
            trackBar_FontSize.Value = Math.Max(Toggles.OnScreenFontSize, trackBar_FontSize.Minimum);
            checkBox_GuitarSpeakWhileTuning.Checked = RsModsSettings.GuitarSpeak.GuitarSpeakWhileTuning;
            checkBox_ChangeTheme.Checked = GUISettings.CustomTheme;
            checkBox_ScreenShotScores.Checked = Toggles.ScreenShotScores;
            checkBox_Fretless.Checked = Toggles.Fretless;
            checkBox_RemoveInlays.Checked = Toggles.Inlays;
            checkBox_RemoveLaneMarkers.Checked = Toggles.LaneMarkers;
            checkBox_GreenScreen.Checked = Toggles.GreenScreenWall;
            checkBox_RainbowStrings.Checked = Toggles.RainbowStrings;
            checkBox_RainbowNotes.Checked = Toggles.RainbowNotes;
            checkBox_WhammyFiveChordsMode.Checked = Toggles.ChordsMode;
            checkBox_ShowCurrentNote.Checked = Toggles.ShowCurrentNoteOnScreen;
            checkBox_CustomHighway.Checked = HighwayColors.CustomHighwayColors;
            checkBox_SecondaryMonitor.Checked = Toggles.SecondaryMonitor == OnOffMode.On;
            checkBox_NoteColors_UseRocksmithColors.Checked = ModSettings.SeparateNoteColorsMode == NoteColorMode.RocksmithColors;
            checkBox_FixBadBassTuning.Checked = Toggles.ExtendedRangeFixBassTuning;
            checkBox_RemoveSongPreviews.Checked = Toggles.SongPreviews;
            checkBox_AllowAudioInBackground.Checked = Toggles.AllowAudioInBackground;
            checkBox_BypassTwoRTCMessageBox.Checked = Toggles.BypassTwoRTCMessageBox;
            checkBox_LinearRiffRepeater.Checked = Toggles.LinearRiffRepeater;
            checkBox_UseAltSampleRate_Output.Checked = Toggles.AltOutputSampleRate;
            groupBox_SampleRateOutput.Visible = checkBox_UseAltSampleRate_Output.Checked;
            listBox_AltSampleRatesOutput.SelectedItem = $"{ModSettings.AlternativeOutputSampleRate} Hz";
            nUpDown_ForceEnumerationXMS.Value = GenUtil.Clamp(ModSettings.CheckForNewSongsInterval / 1000, (int)nUpDown_ForceEnumerationXMS.Minimum, (int)nUpDown_ForceEnumerationXMS.Maximum); // Loads old settings for enumeration every x ms
            listBox_AvailableInputDevices.SelectedItem = Toggles.OverrideInputVolumeDevice;
            nUpDown_OverrideInputVolume.Value = GenUtil.Clamp(ModSettings.OverrideInputVolume, (int)nUpDown_OverrideInputVolume.Minimum, (int)nUpDown_OverrideInputVolume.Maximum);
            checkBox_ER_SeparateNoteColors.Checked = Toggles.SeparateNoteColors == OnOffMode.On;
            groupBox_NoteColors.Visible = checkBox_ER_SeparateNoteColors.Checked;
            checkBox_BackupProfile.Checked = GUISettings.BackupProfile;
            checkBox_ModsLog.Checked = File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "RSMods_debug.txt"));
            checkBox_TurnOffAllMods.Checked = !File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll")) && File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll.off"));
            checkBox_ForceEnumeration.Checked = Toggles.ForceReEnumeration != EnumerationMode.Off;
            checkBox_AllowRewind.Checked = Toggles.AllowRewind;
            groupBox_RewindBy.Visible = checkBox_AllowRewind.Checked;
            groupBox_RewindLeadup.Visible = checkBox_AllowRewind.Checked;
            nUpDown_RewindBy.Value = GenUtil.Clamp(ModSettings.RewindBy / 1000m, nUpDown_RewindBy.Minimum, nUpDown_RewindBy.Maximum);
            nUpDown_RewindLeadup.Value = GenUtil.Clamp(ModSettings.RewindLeadup / 1000m, nUpDown_RewindLeadup.Minimum, nUpDown_RewindLeadup.Maximum);
            checkBox_FixOculusCrash.Checked = Toggles.FixOculusCrash;
            checkBox_FixBrokenTones.Checked = Toggles.FixBrokenTones;
            checkBox_CustomNSPTimer.Checked = Toggles.UseCustomNSPTimer;
            checkBox_DisplayCurrentAccuracy.Checked = Toggles.DisplayCurrentAccuracy;
            checkBox_PreventMidSongPause.Checked = Toggles.PreventMidSongPause;
            checkBox_RemoveFingerprints.Checked = Toggles.RemoveFingerprints;
            groupBox_NSPTimer.Visible = checkBox_CustomNSPTimer.Checked;
            nUpDown_NSPTimer.Value = GenUtil.Clamp(ModSettings.CustomNSPTimeLimit / 1000m, nUpDown_NSPTimer.Minimum, nUpDown_NSPTimer.Maximum);
        }

        private void PriorSettings_LoadASIOSettings()
        {
            if (!AsioSettings.SettingsExist)
                return;

            // Config
            checkBox_ASIO_WASAPI_Output.Checked = AsioSettings.Config.EnableWasapiOutputs;
            checkBox_ASIO_WASAPI_Input.Checked = AsioSettings.Config.EnableWasapiInputs;
            checkBox_ASIO_ASIO.Checked = AsioSettings.Config.EnableAsio;

            // Asio Buffer
            switch (AsioSettings.AsioSection.BufferSizeMode)
            {
                case "custom":
                    radio_ASIO_BufferSize_Custom.Checked = true;
                    nUpDown_ASIO_CustomBufferSize.Value = GenUtil.Clamp(AsioSettings.AsioSection.CustomBufferSize, (int)nUpDown_ASIO_CustomBufferSize.Minimum, (int)nUpDown_ASIO_CustomBufferSize.Maximum);
                    break;
                case "driver":
                    radio_ASIO_BufferSize_Driver.Checked = true;
                    break;
                case "host":
                    radio_ASIO_BufferSize_Host.Checked = true;
                    break;
            }

            // Output
            nUpDown_ASIO_Output_BaseChannel.Value = GenUtil.Clamp(AsioSettings.Output.BaseChannel, (int)nUpDown_ASIO_Output_BaseChannel.Minimum, (int)nUpDown_ASIO_Output_BaseChannel.Maximum);
            nUpDown_ASIO_Output_AltBaseChannel.Value = GenUtil.Clamp(AsioSettings.Output.AltBaseChannel, (int)nUpDown_ASIO_Output_AltBaseChannel.Minimum, (int)nUpDown_ASIO_Output_AltBaseChannel.Maximum);
            checkBox_ASIO_Output_ControlEndpointVolume.Checked = AsioSettings.Output.EnableSoftwareEndpointVolumeControl;
            checkBox_ASIO_Output_ControlMasterVolume.Checked = AsioSettings.Output.EnableSoftwareMasterVolumeControl;
            nUpDown_ASIO_Output_MaxVolume.Value = GenUtil.Clamp(AsioSettings.Output.SoftwareMasterVolumePercent, (int)nUpDown_ASIO_Output_MaxVolume.Minimum, (int)nUpDown_ASIO_Output_MaxVolume.Maximum);
            checkBox_ASIO_Output_Disabled.Checked = AsioSettings.Output.Disabled;
            listBox_AvailableASIODevices_Output.SelectedItem = AsioSettings.Output.Driver;
            checkBox_ASIO_Output_EnableRefHack.Checked = AsioSettings.Output.EnableRefCountHack;

            // Input0
            nUpDown_ASIO_Input0_Channel.Value = GenUtil.Clamp(AsioSettings.Input0.Channel, (int)nUpDown_ASIO_Input0_Channel.Minimum, (int)nUpDown_ASIO_Input0_Channel.Maximum);
            checkBox_ASIO_Input0_ControlEndpointVolume.Checked = AsioSettings.Input0.EnableSoftwareEndpointVolumeControl;
            checkBox_ASIO_Input0_ControlMasterVolume.Checked = AsioSettings.Input0.EnableSoftwareMasterVolumeControl;
            nUpDown_ASIO_Input0_MaxVolume.Value = GenUtil.Clamp(AsioSettings.Input0.SoftwareMasterVolumePercent, (int)nUpDown_ASIO_Input0_MaxVolume.Minimum, (int)nUpDown_ASIO_Input0_MaxVolume.Maximum);
            checkBox_ASIO_Input0_Disabled.Checked = AsioSettings.Input0.Disabled;
            listBox_AvailableASIODevices_Input0.SelectedItem = AsioSettings.Input0.Driver;
            checkBox_ASIO_Input0_EnableRefHack.Checked = AsioSettings.Input0.EnableRefCountHack;

            // Input1
            nUpDown_ASIO_Input1_Channel.Value = GenUtil.Clamp(AsioSettings.Input1.Channel, (int)nUpDown_ASIO_Input1_Channel.Minimum, (int)nUpDown_ASIO_Input1_Channel.Maximum);
            checkBox_ASIO_Input1_ControlEndpointVolume.Checked = AsioSettings.Input1.EnableSoftwareEndpointVolumeControl;
            checkBox_ASIO_Input1_ControlMasterVolume.Checked = AsioSettings.Input1.EnableSoftwareMasterVolumeControl;
            nUpDown_ASIO_Input1_MaxVolume.Value = GenUtil.Clamp(AsioSettings.Input1.SoftwareMasterVolumePercent, (int)nUpDown_ASIO_Input1_MaxVolume.Minimum, (int)nUpDown_ASIO_Input1_MaxVolume.Maximum);
            checkBox_ASIO_Input1_Disabled.Checked = AsioSettings.Input1.Disabled;
            listBox_AvailableASIODevices_Input1.SelectedItem = AsioSettings.Input1.Driver;
            checkBox_ASIO_Input1_EnableRefHack.Checked = AsioSettings.Input1.EnableRefCountHack;

            // InputMic
            nUpDown_ASIO_InputMic_Channel.Value = GenUtil.Clamp(AsioSettings.InputMic.Channel, (int)nUpDown_ASIO_InputMic_Channel.Minimum, (int)nUpDown_ASIO_InputMic_Channel.Maximum);
            checkBox_ASIO_InputMic_ControlEndpointVolume.Checked = AsioSettings.InputMic.EnableSoftwareEndpointVolumeControl;
            checkBox_ASIO_InputMic_ControlMasterVolume.Checked = AsioSettings.InputMic.EnableSoftwareMasterVolumeControl;
            nUpDown_ASIO_InputMic_MaxVolume.Value = GenUtil.Clamp(AsioSettings.InputMic.SoftwareMasterVolumePercent, (int)nUpDown_ASIO_InputMic_MaxVolume.Minimum, (int)nUpDown_ASIO_InputMic_MaxVolume.Maximum);
            checkBox_ASIO_InputMic_Disabled.Checked = AsioSettings.InputMic.Disabled;
            listBox_AvailableASIODevices_InputMic.SelectedItem = AsioSettings.InputMic.Driver;
            checkBox_ASIO_InputMic_EnableRefHack.Checked = AsioSettings.InputMic.EnableRefCountHack;
        }

        private void PriorSettings_LoadRocksmithSettings()
        {
            // Audio Settings
            checkBox_Rocksmith_EnableMicrophone.Checked = RocksmithSettings.Audio.EnableMicrophone;
            checkBox_Rocksmith_ExclusiveMode.Checked = RocksmithSettings.Audio.ExclusiveMode;
            if (RocksmithSettings.Audio.LatencyBuffer <= RocksmithSettings.Audio.MinLatencyBuffer || RocksmithSettings.Audio.LatencyBuffer > RocksmithSettings.Audio.MaxLatencyBuffer)
                RocksmithSettings.Audio.LatencyBuffer = RocksmithSettings.Audio.DefaultLatencyBuffer;
            nUpDown_Rocksmith_LatencyBuffer.Value = RocksmithSettings.Audio.LatencyBuffer;
            checkBox_Rocksmith_ForceWDM.Checked = RocksmithSettings.Audio.ForceWDM;
            checkBox_Rocksmith_ForceDirextXSink.Checked = RocksmithSettings.Audio.ForceDirectXSink;
            checkBox_Rocksmith_DumpAudioLog.Checked = RocksmithSettings.Audio.DumpAudioLog;
            if (RocksmithSettings.Audio.MaxOutputBufferSize != 0)
                nUpDown_Rocksmith_MaxOutputBuffer.Value = RocksmithSettings.Audio.MaxOutputBufferSize;
            else
                checkBox_Rocksmith_Override_MaxOutputBufferSize.Checked = true;
            checkBox_Rocksmith_RTCOnly.Checked = RocksmithSettings.Audio.RealToneCableOnly;
            checkBox_Rocksmith_LowLatencyMode.Checked = RocksmithSettings.Audio.Win32UltraLowLatencyMode;

            // Visual Settings
            checkBox_Rocksmith_GamepadUI.Checked = RocksmithSettings.RendererWin32.ShowGamepadUI;
            nUpDown_Rocksmith_ScreenWidth.Value = GenUtil.Clamp(RocksmithSettings.RendererWin32.ScreenWidth, (int)nUpDown_Rocksmith_ScreenWidth.Minimum, (int)nUpDown_Rocksmith_ScreenWidth.Maximum);
            nUpDown_Rocksmith_ScreenHeight.Value = GenUtil.Clamp(RocksmithSettings.RendererWin32.ScreenHeight, (int)nUpDown_Rocksmith_ScreenHeight.Minimum, (int)nUpDown_Rocksmith_ScreenHeight.Maximum);
            switch (RocksmithSettings.RendererWin32.Fullscreen)
            {
                case FullscreenMode.Windowed:
                    radio_Rocksmith_Windowed.Checked = true;
                    break;
                case FullscreenMode.NonExclusive:
                    radio_Rocksmith_NonExclusiveFullScreen.Checked = true;
                    break;
                case FullscreenMode.Exclusive:
                    radio_Rocksmith_ExclusiveFullScreen.Checked = true;
                    break;
                default:
                    break;
            }
            nUpDown_Rocksmith_RenderWidth.Value = GenUtil.Clamp(RocksmithSettings.RendererWin32.RenderingWidth, (int)nUpDown_Rocksmith_RenderWidth.Minimum, (int)nUpDown_Rocksmith_RenderWidth.Maximum);
            nUpDown_Rocksmith_RenderHeight.Value = GenUtil.Clamp(RocksmithSettings.RendererWin32.RenderingHeight, (int)nUpDown_Rocksmith_RenderHeight.Minimum, (int)nUpDown_Rocksmith_RenderHeight.Maximum);
            checkBox_Rocksmith_PostEffects.Checked = RocksmithSettings.RendererWin32.EnablePostEffects;
            checkBox_Rocksmith_Shadows.Checked = RocksmithSettings.RendererWin32.EnableShadows;
            checkBox_Rocksmith_HighResScope.Checked = RocksmithSettings.RendererWin32.EnableHighResScope;
            checkBox_Rocksmith_DepthOfField.Checked = RocksmithSettings.RendererWin32.EnableDepthOfField;
            checkBox_Rocksmith_PerPixelLighting.Checked = RocksmithSettings.RendererWin32.EnablePerPixelLighting;
            checkBox_Rocksmith_MSAASamples.Checked = RocksmithSettings.RendererWin32.MsaaSamples == MsaaMode.X4;
            checkBox_Rocksmith_DisableBrowser.Checked = RocksmithSettings.RendererWin32.DisableBrowser;
            checkBox_Rocksmith_EnableRenderRes.Checked = RocksmithSettings.RendererWin32.RenderingWidth != 0 || RocksmithSettings.RendererWin32.RenderingHeight != 0;

            switch (RocksmithSettings.RendererWin32.VisualQuality)
            {
                case VisualQualityMode.Low:
                    radio_Rocksmith_LowQuality.Checked = true;
                    break;
                case VisualQualityMode.Medium:
                    radio_Rocksmith_MediumQuality.Checked = true;
                    break;
                case VisualQualityMode.High:
                    radio_Rocksmith_HighQuality.Checked = true;
                    break;
                case VisualQualityMode.Custom:
                    radio_Rocksmith_CustomQuality.Checked = true;
                    break;
                default:
                    break;
            }

            // Network Settings
            checkBox_Rocksmith_UseProxy.Checked = RocksmithSettings.Net.UseProxy;
        }

        #endregion
        #region Custom Themes

        // Not taken from here :O https://stackoverflow.com/a/3419209
        private readonly List<Control> ControlList = []; // Don't make this readonly
        private void GetAllControls(Control container)
        {
            foreach (Control c in container.Controls)
            {
                GetAllControls(c);
                if (c is ListBox || c is GroupBox || c is TabPage || c is Button)
                    ControlList.Add(c);
            }
        }

        private void CustomTheme_ChangeTheme(Color backgroundColor, Color textColor, Color buttonColor)
        {
            GetAllControls(TabController);
            BackColor = backgroundColor; // MainForm BackColor
            ForeColor = textColor; // MainForm ForeColor

            foreach (Control controlToChange in ControlList)
            {
                controlToChange.ForeColor = textColor;

                if (controlToChange is Button)
                    controlToChange.BackColor = buttonColor;
                else
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
            Color backColor = Constants.DefaultBackgroundColor, foreColor = Constants.DefaultTextColor, buttonColor = Constants.DefaultButtonColor;

            if (GUISettings.CustomTheme)
            {
                if (GUISettings.ThemeBackgroundColor != String.Empty)
                    backColor = ColorTranslator.FromHtml("#" + GUISettings.ThemeBackgroundColor);

                if (GUISettings.ThemeTextColor != String.Empty)
                    foreColor = ColorTranslator.FromHtml("#" + GUISettings.ThemeTextColor);

                if (GUISettings.ThemeButtonColor != String.Empty)
                    buttonColor = ColorTranslator.FromHtml("#" + GUISettings.ThemeButtonColor);
            }

            textBox_ChangeBackgroundColor.BackColor = backColor;
            textBox_ChangeTextColor.BackColor = foreColor;
            textBox_ChangeButtonColor.BackColor = buttonColor;

            CustomTheme_ChangeTheme(textBox_ChangeBackgroundColor.BackColor, textBox_ChangeTextColor.BackColor, textBox_ChangeButtonColor.BackColor);
        }

        private void CustomTheme_ChangeTheme(object sender, EventArgs e)
        {
            GUISettings.CustomTheme = checkBox_ChangeTheme.Checked;
            groupBox_ChangeTheme.Visible = checkBox_ChangeTheme.Checked;

            if (!checkBox_ChangeTheme.Checked) // Turning off custom themes
                CustomTheme_ChangeTheme(Constants.DefaultBackgroundColor, Constants.DefaultTextColor, Constants.DefaultButtonColor);
        }

        private void CustomTheme_ChangeBackgroundColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new()
            {
                AllowFullOpen = true,
                ShowHelp = false,
                Color = Constants.DefaultBackgroundColor
            };

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                GUISettings.ThemeBackgroundColor = (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6");
                textBox_ChangeBackgroundColor.BackColor = colorDialog.Color;
            }
        }

        private void CustomTheme_ChangeTextColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new()
            {
                AllowFullOpen = true,
                ShowHelp = false,
                Color = Constants.DefaultTextColor
            };

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                GUISettings.ThemeTextColor = (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6");
                textBox_ChangeTextColor.BackColor = colorDialog.Color;
            }
        }

        private void CustomTheme_ChangeButtonColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new()
            {
                AllowFullOpen = true,
                ShowHelp = false,
                Color = Constants.DefaultButtonColor
            };

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                GUISettings.ThemeButtonColor = (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6");
                textBox_ChangeButtonColor.BackColor = colorDialog.Color;
            }
        }

        private void CustomTheme_Apply(object sender, EventArgs e) => CustomTheme_ChangeTheme(textBox_ChangeBackgroundColor.BackColor, textBox_ChangeTextColor.BackColor, textBox_ChangeButtonColor.BackColor);

        private void CustomTheme_Reset(object sender, EventArgs e)
        {
            CustomTheme_ChangeTheme(Constants.DefaultBackgroundColor, Constants.DefaultTextColor, Constants.DefaultButtonColor);

            GUISettings.ThemeBackgroundColor = (Constants.DefaultBackgroundColor.ToArgb() & 0x00ffffff).ToString("X6");
            GUISettings.ThemeTextColor = (Constants.DefaultTextColor.ToArgb() & 0x00ffffff).ToString("X6");
            GUISettings.ThemeButtonColor = (Constants.DefaultButtonColor.ToArgb() & 0x00ffffff).ToString("X6");

            textBox_ChangeBackgroundColor.BackColor = Constants.DefaultBackgroundColor;
            textBox_ChangeTextColor.BackColor = Constants.DefaultTextColor;
            textBox_ChangeButtonColor.BackColor = Constants.DefaultButtonColor;
        }

        #endregion
        #region Check For Keypresses (Keybindings)
        private void Keypress_CheckDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                e.SuppressKeyPress = true;
                Save_Songlists_Keybindings(sender, e);
                return;
            }

            if (sender is TextBox textBox &&
               (textBox == textBox_NewKeyAssignment_MODS || textBox == textBox_NewKeyAssignment_AUDIO))
            {
                HandleKeyAssignment(textBox, e);
            }
        }

        private void HandleKeyAssignment(TextBox textBox, KeyEventArgs e)
        {
            e.SuppressKeyPress = true;

            if (KeyConversion.KeyDownDictionary.Contains(e.KeyCode))
            {
                textBox.Text = e.KeyCode.ToString();
            }
            else if (KeyConversion.IsRocksmithReservedKey(e.KeyCode))
            {
                var result = MessageBox.Show(
                    "The key you entered is currently used by Rocksmith and may interfere with being able to use the game properly. Are you sure you want to use this keybinding?",
                    "Keybinding Warning!",
                    MessageBoxButtons.OKCancel,
                    MessageBoxIcon.Warning);

                if (result == DialogResult.OK)
                {
                    textBox.Text = e.KeyCode.ToString();
                }
                else
                {
                    textBox.Text = string.Empty;
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

        private void Keypress_LoadKeys(object sender, EventArgs e)
        {
            if (listBox_Modlist_MODS.SelectedIndex != -1)
            {
                textBox_NewKeyAssignment_MODS.Text = KeyConversion.VKeyToUI(Dictionaries.ModKeybinds[listBox_Modlist_MODS.SelectedIndex].GetKey());
            }
        }

        private void Keypress_LoadVolumes(object sender, EventArgs e)
        {
            if (listBox_Modlist_AUDIO.SelectedIndex != -1)
            {
                textBox_NewKeyAssignment_AUDIO.Text = KeyConversion.VKeyToUI(Dictionaries.AudioKeybinds[listBox_Modlist_AUDIO.SelectedIndex].GetKey());
            }
        }
        #endregion
        #region Reset To Default
        private void Reset_DefaultSettings(object sender, EventArgs e)
        {
            if (MessageBox.Show("Are you sure you want to reset your mod settings to their defaults?", "WARNING: RESET TO DEFAULT?", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
            {
                File.Delete(Path.Combine(GenUtil.GetRSDirectory(), "RSMods.ini"));
                Reset_RefreshForm();
            }
            else
            {
                MessageBox.Show("All your settings have been saved, and nothing was reset");
            }
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
        private void SaveSettings_ShowLabel()
        {
            label_SettingsSaved.Visible = true;
            System.Windows.Forms.Timer timer = new()
            {
                Interval = 1500
            };
            timer.Tick += (source, e) => { label_SettingsSaved.Visible = false; timer.Stop(); };
            timer.Start();
        }

        private void Save_Songlists_Keybindings(object sender, EventArgs e) // Save Songlists and Keybindings when pressing Enter
        {
            if (!AllowSaving)
                return;

            TextBox textBox = ((TextBox)sender);

            // Song Lists
            if (textBox.Name == textBox_NewSonglistName.Name)
            {
                if (string.IsNullOrEmpty(textBox_NewSonglistName.Text.Trim()))
                {
                    MessageBox.Show("You cannot save a blank song list name as the game will break", "Warning!", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
                else if (listBox_Songlist.SelectedIndex >= 0)
                {
                    RsModsSettings.SetSongListTitle(listBox_Songlist.SelectedIndex + 1, textBox_NewSonglistName.Text);
                    listBox_Songlist.Items[listBox_Songlist.SelectedIndex] = textBox_NewSonglistName.Text;
                }

                Profiles_RefreshSonglistNames();
            }

            // Mod Keybindings
            if (textBox.Name == textBox_NewKeyAssignment_MODS.Name)
            {
                if (string.IsNullOrEmpty(textBox_NewKeyAssignment_MODS.Text))
                {
                    MessageBox.Show("You cannot set a blank keybind", "Warning!", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
                else if (listBox_Modlist_MODS.SelectedIndex >= 0)
                {
                    Dictionaries.ModKeybinds[listBox_Modlist_MODS.SelectedIndex].SetKey(KeyConversion.VirtualKey(textBox_NewKeyAssignment_MODS.Text));
                }

                textBox_NewKeyAssignment_MODS.Text = String.Empty;
            }

            // Audio Keybindings
            if (textBox.Name == textBox_NewKeyAssignment_AUDIO.Name)
            {
                if (string.IsNullOrEmpty(textBox_NewKeyAssignment_AUDIO.Text))
                {
                    MessageBox.Show("You cannot set a blank keybind", "Warning!", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
                else if (listBox_Modlist_AUDIO.SelectedIndex >= 0)
                {
                    Dictionaries.AudioKeybinds[listBox_Modlist_AUDIO.SelectedIndex].SetKey(KeyConversion.VirtualKey(textBox_NewKeyAssignment_AUDIO.Text));
                }

                textBox_NewKeyAssignment_AUDIO.Text = String.Empty;
            }
            Startup_ShowCurrentKeybindingValues();
            Startup_ShowCurrentAudioKeybindingValues();
        }


        #endregion
        #region String Colors

        private void StringColors_ChangeStringColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new()
            {
                AllowFullOpen = true,
                ShowHelp = false
            };
            bool isNormalStrings = radio_DefaultStringColors.Checked; // True = Normal, False = Colorblind
            int stringNumber = -1;
            string senderText = sender.ToString();

            if (senderText.Contains("E String")) stringNumber = 0;
            else if (senderText.Contains("A String")) stringNumber = 1;
            else if (senderText.Contains("D String")) stringNumber = 2;
            else if (senderText.Contains("G String")) stringNumber = 3;
            else if (senderText.Contains("B String")) stringNumber = 4;
            else if (senderText.Contains("e String")) stringNumber = 5;

            if (stringNumber == -1) return;

            StringColors_FillStringNumberToColorDictionary();

            colorDialog.Color = ColorTranslator.FromHtml("#" + StringColors.GetStringColor(stringNumber, isNormalStrings));

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                Toggles.CustomStringColors = CustomStringColorMode.Custom;
                string newColor = (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6");
                StringColors.SetStringColor(stringNumber, isNormalStrings, newColor);
                stringNumberToColorTextBox[stringNumber].BackColor = colorDialog.Color;
            }
        }

        private void OnSettingChanged()
        {
            if (!AllowSaving)
                return;

            RsModsSettings.Save();

            if (this.InvokeRequired)
            {
                this.Invoke(new Action(SaveSettings_ShowLabel));
            }
            else
            {
                SaveSettings_ShowLabel();
            }

            WinMsgUtil.SendMsgToRS("update all");
        }

        private void StringColors_ChangeNoteColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new()
            {
                AllowFullOpen = true,
                ShowHelp = false
            };
            bool isNormalNotes = radio_DefaultNoteColors.Checked; // True = Normal, False = Colorblind
            int noteNumber = -1;
            string senderText = sender.ToString();

            if (senderText.Contains("E String")) noteNumber = 0;
            else if (senderText.Contains("A String")) noteNumber = 1;
            else if (senderText.Contains("D String")) noteNumber = 2;
            else if (senderText.Contains("G String")) noteNumber = 3;
            else if (senderText.Contains("B String")) noteNumber = 4;
            else if (senderText.Contains("e String")) noteNumber = 5;

            if (noteNumber == -1) return;

            StringColors_FillNoteNumberToColorDictionary();

            colorDialog.Color = ColorTranslator.FromHtml("#" + RsModsSettings.StringColors.GetNoteColor(noteNumber, isNormalNotes));

            if (colorDialog.ShowDialog() == DialogResult.OK)
            {
                ModSettings.SeparateNoteColorsMode = NoteColorMode.Custom;
                string newColor = (colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6");
                StringColors.SetNoteColor(noteNumber, isNormalNotes, newColor);
                stringNumberToColorTextBox[noteNumber].BackColor = colorDialog.Color;
            }
        }

        private void StringColors_LoadDefaultStringColors(bool colorBlind = false)
        {
            if (string.IsNullOrEmpty(StringColors.GetNoteColor(0, true))) // Fixes a small use case where the GUI moves faster than the writing of the INI.
            {
                RsModsSettings.Save();
                return;
            }

            var stringColorTextboxes = new[]
            {
                textBox_String0Color, textBox_String1Color, textBox_String2Color,
                textBox_String3Color, textBox_String4Color, textBox_String5Color
            };

            for (int i = 0; i < stringColorTextboxes.Length; i++)
            {
                string hexColor = $"#{StringColors.GetStringColor(i, !colorBlind)}";
                stringColorTextboxes[i].BackColor = ColorTranslator.FromHtml(hexColor);
            }
        }

        private void StringColors_LoadDefaultNoteColors(bool colorBlind = false)
        {
            if (string.IsNullOrEmpty(StringColors.GetNoteColor(0, true))) // Fixes a small use case where the GUI moves faster than the writing of the INI.
            {
                RsModsSettings.Save();
                return;
            }

            var noteColorTextBoxes = new[]
            {
                    textBox_Note0Color, textBox_Note1Color, textBox_Note2Color,
                    textBox_Note3Color, textBox_Note4Color, textBox_Note5Color
                };

            for (int i = 0; i < noteColorTextBoxes.Length; i++)
            {
                string hexColor = $"#{StringColors.GetNoteColor(i, !colorBlind)}";
                noteColorTextBoxes[i].BackColor = ColorTranslator.FromHtml(hexColor);
            }
        }

        private void StringColors_DefaultStringColors(object sender, EventArgs e) => StringColors_LoadDefaultStringColors();

        private void StringColors_ColorBlindStringColors(object sender, EventArgs e) => StringColors_LoadDefaultStringColors(true);

        private void StringColors_DefaultNoteColors(object sender, EventArgs e) => StringColors_LoadDefaultNoteColors();

        private void StringColors_ColorBlindNoteColors(object sender, EventArgs e) => StringColors_LoadDefaultNoteColors(true);

        #endregion
        #region Noteway Colors
        private void NotewayColors_ChangeNotewayColor(object sender, EventArgs e)
        {
            ColorDialog colorDialog = new()
            {
                AllowFullOpen = true,
                ShowHelp = false
            };

            NotewayColors_FillNotewayButtonToColorDictionary();

            string senderStr = sender.ToString();
            var matchedColor = Dictionaries.NotewayColors.FirstOrDefault(c => senderStr.Contains(c.DisplayName));

            if (matchedColor != null)
            {
                string currentColor = matchedColor.GetColor();
                if (!string.IsNullOrEmpty(currentColor))
                    colorDialog.Color = ColorTranslator.FromHtml("#" + currentColor);

                if (colorDialog.ShowDialog() == DialogResult.OK)
                {
                    matchedColor.SetColor((colorDialog.Color.ToArgb() & 0x00ffffff).ToString("X6"));
                    notewayButtonToColorTextbox[(Button)sender].BackColor = colorDialog.Color;
                }
            }
        }

        private void NotewayColors_LoadDefaultStringColors()
        {
            if (HighwayColors.CustomHighwayNumbered != "")
                textBox_ShowNumberedFrets.BackColor = ColorTranslator.FromHtml("#" + HighwayColors.CustomHighwayNumbered);
            if (HighwayColors.CustomHighwayUnNumbered != "")
                textBox_ShowUnNumberedFrets.BackColor = ColorTranslator.FromHtml("#" + HighwayColors.CustomHighwayUnNumbered);
            if (HighwayColors.CustomHighwayGutter != "")
                textBox_ShowNotewayGutter.BackColor = ColorTranslator.FromHtml("#" + HighwayColors.CustomHighwayGutter);
            if (HighwayColors.CustomFretNubmers != "")
                textBox_ShowFretNumber.BackColor = ColorTranslator.FromHtml("#" + HighwayColors.CustomFretNubmers);
        }

        #endregion
        #region Prep Set And Forget Mods
        // SetAndForget Mods

        private void SetForget_FillUI()
        {
            listBox_Tunings.Items.Clear();
            SetAndForgetMods.LoadTuningsCollection();

            listBox_Tunings.Items.Add("<New>");
            foreach (var key in SetAndForgetMods.TuningsCollection.Keys)
                listBox_Tunings.Items.Add(key);
        }

        private TuningDefinitionInfo SetForget_GetCurrentTuningInfo()
        {
            var tuningDefinition = new TuningDefinitionInfo();
            var strings = new Dictionary<string, int>();

            for (int strIdx = 0; strIdx < 6; strIdx++)
                strings[$"string{strIdx}"] = (int)((NumericUpDown)tabPage_SetAndForget_CustomTunings.Controls[$"nUpDown_String{strIdx}"]).Value;

            tuningDefinition.Strings = strings;
            tuningDefinition.UIName = String.Format("$[{0}]{1}", nUpDown_UIIndex.Value.ToString(), textBox_UIName.Text);

            return tuningDefinition;
        }
        private void SetForget_LoadSetAndForgetMods()
        {
            SetAndForgetMods.LoadDefaultFiles();
            SetAndForgetMods.UnpackCachePsarc(); // We need to unpack the cache AGAIN in-case the user resets their psarc, and we don't know.
            SetForget_FillUI();
            SetForget_SetTunerColors();
        }
        #endregion
        #region Set And Forget UI Functions
        private void SetForget_SetTunerColors(int string_num = -1, bool extendedRange = false)
        {
            NumericUpDown[] strings = [nUpDown_String0, nUpDown_String1, nUpDown_String2, nUpDown_String3, nUpDown_String4, nUpDown_String5];
            if (string_num >= 0 && string_num < strings.Length)
            {
                strings[string_num].BackColor = ColorTranslator.FromHtml("#" + StringColors.GetStringColor(string_num, !extendedRange));
            }
            else
            {
                for (int i = 0; i < strings.Length; i++)
                    strings[i].BackColor = ColorTranslator.FromHtml("#" + StringColors.GetStringColor(i, !extendedRange));
            }
        }

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

        private void SetForget_TurnItUpToEleven(object sender, EventArgs e) => SetAndForgetMods.AddIncreasedVolumeWwiseBank();

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
                listBox_SetAndForget_SongsWithSelectedTuning.Items.Clear();
                return;
            }

            var selectedTuning = SetAndForgetMods.TuningsCollection[selectedItem];
            var uiName = SetAndForgetMods.SplitTuningUIName(selectedTuning.UIName);

            textBox_InternalTuningName.Text = selectedItem;
            nUpDown_UIIndex.Value = Convert.ToInt32(uiName.Item1);
            textBox_UIName.Text = uiName.Item2;

            for (int strIdx = 0; strIdx < 6; strIdx++) // If you are lazy and don't want to list each string separately, just do this sexy two-liner
                ((NumericUpDown)tabPage_SetAndForget_CustomTunings.Controls[$"nUpDown_String{strIdx}"]).Value = selectedTuning.Strings[$"string{strIdx}"];

            SetForget_FillSongsWithSelectedTuningList();
        }

        private void SetForget_SaveTuningChanges(object sender, EventArgs e)
        {
            if (listBox_Tunings.SelectedIndex != -1) // If we are saving a change to the currently selected tuning, perform a change in the collection, otherwise directly go to saving
            {
                string selectedItem = listBox_Tunings.SelectedItem.ToString();

                if (selectedItem != "<New>")
                {
                    SetAndForgetMods.TuningsCollection[selectedItem] = SetForget_GetCurrentTuningInfo();

                    if (listBox_SetAndForget_SongsWithCustomTuning.Items.Count > 0)
                        SetForget_LoadSongsToWorkOn(sender, e);
                }
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

            if (listBox_SetAndForget_SongsWithCustomTuning.Items.Count > 0)
                SetForget_LoadSongsToWorkOn(sender, e);
        }

        private void SetForget_AddTuning(object sender, EventArgs e)
        {
            if (listBox_Tunings.SelectedIndex == -1)
                listBox_Tunings.SelectedIndex = 0;

            if (listBox_Tunings.SelectedItem.ToString() != "<New>")
                return;

            var currTuning = SetForget_GetCurrentTuningInfo();
            string internalName = textBox_InternalTuningName.Text;

            if (internalName.Trim().Length == 0)
            {
                MessageBox.Show("You cannot have a blank internal name.");
                return;
            }

            if (!SetAndForgetMods.TuningsCollection.ContainsKey(internalName)) // Unlikely to happen, but still... prevent users accidentaly trying to add existing stuff
            {
                SetAndForgetMods.TuningsCollection.Add(internalName, currTuning);
                listBox_Tunings.Items.Add(internalName);

                if (listBox_SetAndForget_SongsWithCustomTuning.Items.Count > 0)
                    SetForget_LoadSongsToWorkOn(sender, e);
            }
            else
            {
                MessageBox.Show("You already have a tuning with the same internal name");
            }
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

        private void SetForget_AssignNewGuitarArcadeTone(object sender, EventArgs e)
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

        private void SetForget_LoadSongsToWorkOn(object sender, EventArgs e)
        {
            Songs = SongManager.ExtractSongData(progressBar_FillSongsWithCustomTunings); // Load all the data from the songs

            SetForget_ShowLoadedSongs(); // Makes the listboxes and labels visible for songs with tunings.
            SetForget_FillDefinedTunings(); // Get a list of all of our non-"Custom Tuning"s.
            SetForget_FillCustomTuningList(); // Get a list of all song & arrangement combos that will show up as "Custom Tuning" if not dealt with.
            // SetForget_FillSongsWithBadBassTuningsList(); // DISABLED: Not integrated with the automated fixing | Get a list of all song & arrangement combos that have a bass tuning that is not in the Rocksmith tuning format.
            SetForget_FillSongsWithSelectedTuningList(); // Get a list of all song & arrangement combos that have the same tuning as selected in listBox_Tunings.
        }

        private void SetForget_ShowLoadedSongs()
        {
            // Songs with Selected Tuning
            label_SongsWithSelectedTuning.Visible = true;
            listBox_SetAndForget_SongsWithSelectedTuning.Visible = true;

            // Songs that may show up as "Custom Tuning".
            label_SetAndForget_MayShowUpAsCustomTuning.Visible = true;
            listBox_SetAndForget_SongsWithCustomTuning.Visible = true;
        }

        private ArrangementTuning SetForget_ConvertTuningStandards(TuningDefinitionInfo tuning)
        {
            var s = tuning.Strings;
            return new ArrangementTuning
            {
                String0 = s["string0"],
                String1 = s["string1"],
                String2 = s["string2"],
                String3 = s["string3"],
                String4 = s["string4"],
                String5 = s["string5"]
            };
        }

        private readonly List<ArrangementTuning> definedTunings = [];

        private void SetForget_FillDefinedTunings()
        {
            definedTunings.Clear();

            foreach (TuningDefinitionInfo tuningDefinition in SetAndForgetMods.TuningsCollection.Values)
            {
                definedTunings.Add(SetForget_ConvertTuningStandards(tuningDefinition));
            }
        }

        private void SetForget_FillCustomTuningList()
        {
            customTunings.Clear();

            foreach (SongData song in Songs)
            {
                foreach (SongArrangement arrangement in song.Arrangements)
                {
                    string formatting = string.Empty;

                    if (arrangement.Attributes.ArrangementProperties.Represent == 0)
                        formatting += "Alt ";
                    else if (arrangement.Attributes.ArrangementProperties.BonusArr == 1)
                        formatting += "Bonus ";

                    formatting += arrangement.Attributes.ArrangementName + " for " + song.Artist + " - " + song.Title;
                    if (!definedTunings.Contains(arrangement.Attributes.Tuning) && !customTunings.ContainsKey(formatting))
                    {
                        customTunings.Add(formatting, arrangement.Attributes.Tuning);
                    }
                }
            }

            listBox_SetAndForget_SongsWithCustomTuning.Items.Clear();
            listBox_SetAndForget_SongsWithCustomTuning.Items.AddRange([.. customTunings.Keys]);
        }

        private void SetForget_FillSongsWithSelectedTuningList()
        {
            listBox_SetAndForget_SongsWithSelectedTuning.Items.Clear();

            List<string> songsWithTuning = [];

            if (listBox_Tunings.SelectedIndex == -1 || listBox_Tunings.SelectedItem.ToString() == "<New>")
                return;

            ArrangementTuning selectedTuning = SetForget_ConvertTuningStandards(SetAndForgetMods.TuningsCollection[listBox_Tunings.SelectedItem.ToString()]);

            foreach (SongData song in Songs)
            {
                foreach (SongArrangement arrangement in song.Arrangements)
                {
                    string formatting = string.Empty;

                    if (arrangement.Attributes.ArrangementProperties.Represent == 0)
                        formatting += "Alt ";
                    else if (arrangement.Attributes.ArrangementProperties.BonusArr == 1)
                        formatting += "Bonus ";

                    formatting += arrangement.Attributes.ArrangementName + " for " + song.Artist + " - " + song.Title;
                    if (arrangement.Attributes.Tuning.Equals(selectedTuning))
                    {
                        songsWithTuning.Add(formatting);
                    }
                }
            }

            songsWithTuning.Sort();
            listBox_SetAndForget_SongsWithSelectedTuning.Items.AddRange([.. songsWithTuning]);
        }

        private void SetForget_FillSongsWithBadBassTuningsList()
        {
            listBox_SetAndForget_SongsWithBadBassTuning.Items.Clear();
            List<string> songsBeingChanged = [];

            foreach (SongData song in Songs)
            {
                foreach (SongArrangement arrangement in song.Arrangements)
                {

                    string formatting = string.Empty;

                    if (arrangement.Attributes.ArrangementProperties.Represent == 0)
                        formatting += "Alt ";
                    else if (arrangement.Attributes.ArrangementProperties.BonusArr == 1)
                        formatting += "Bonus ";

                    formatting += arrangement.Attributes.ArrangementName + " for " + song.Artist + " - " + song.Title;
                    ArrangementTuning arrangementTuning = arrangement.Attributes.Tuning;
                    if (arrangement.Attributes.ArrangementName.ToLower().Contains("bass")) // Should be formatted this way or we will lose alt / bonus bass.
                    {
                        if (!SetForget_IsTuningStandard(arrangement.Attributes.Tuning) && !SetForget_IsTuningDrop(arrangement.Attributes.Tuning) && !songsBeingChanged.Contains(formatting))
                        {
                            if (!song.ODLC && !(arrangementTuning.String0 == 0 || arrangementTuning.String1 == 0 || arrangementTuning.String2 == 0 || arrangementTuning.String3 == 0) && ((arrangementTuning.String4 == 0 && arrangementTuning.String5 == 0) || (arrangementTuning.String4 == 12 && arrangementTuning.String5 == 12)))
                            {
                                songsBeingChanged.Add(formatting);
                            }
                        }
                    }
                }
            }
            listBox_SetAndForget_SongsWithBadBassTuning.Items.AddRange([.. songsBeingChanged]);
        }

        private bool SetForget_IsTuningStandard(object tuning, bool forceBass = false) => tuning switch
        {
            ArrangementTuning t => t.String0 == t.String1 && t.String1 == t.String2 && t.String2 == t.String3 &&
                                   (forceBass || (t.String3 == t.String4 && t.String4 == t.String5)),
            TuningDefinitionInfo i => SetForget_IsTuningStandard(SetForget_ConvertTuningStandards(i), forceBass),
            _ => false
        };

        private bool SetForget_IsTuningDrop(object tuning, bool forceBass = false) => tuning switch
        {
            ArrangementTuning t => t.String0 + 2 == t.String1 && t.String1 == t.String2 && t.String2 == t.String3 &&
                                   (forceBass || (t.String3 == t.String4 && t.String4 == t.String5)),
            TuningDefinitionInfo i => SetForget_IsTuningDrop(SetForget_ConvertTuningStandards(i), forceBass),
            _ => false
        };

        private readonly SortedDictionary<string, ArrangementTuning> customTunings = [];

        private void SetForget_LoadCustomTuningFromSong(object sender, EventArgs e)
        {
            if (listBox_SetAndForget_SongsWithCustomTuning.SelectedIndex < 0)
                return;

            ArrangementTuning customTuning = customTunings[listBox_SetAndForget_SongsWithCustomTuning.SelectedItem.ToString()];

            nUpDown_String0.Value = customTuning.String0;
            nUpDown_String1.Value = customTuning.String1;
            nUpDown_String2.Value = customTuning.String2;
            nUpDown_String3.Value = customTuning.String3;
            nUpDown_String4.Value = customTuning.String4;
            nUpDown_String5.Value = customTuning.String5;

            listBox_Tunings.SelectedIndex = 0; // "<New>"
        }

        private void SetForget_TuningOffsets(object sender, EventArgs e)
        {
            string nupName = ((NumericUpDown)sender).Name;
            int stringNumber = Int32.Parse(nupName[nupName.Length - 1].ToString()); // Returns the current sender's name.
            switch (stringNumber)
            {
                case 0:
                    int offset = 40; // E2 (Midi)
                    label_CustomTuningLowEStringLetter.Text = GuitarSpeak.GuitarSpeakNoteOctaveMath($"{(int)nUpDown_String0.Value + offset}");
                    break;
                case 1:
                    offset = 45; // A2 (Midi)
                    label_CustomTuningAStringLetter.Text = GuitarSpeak.GuitarSpeakNoteOctaveMath($"{(int)nUpDown_String1.Value + offset}");
                    break;
                case 2:
                    offset = 50; // D3 (Midi)
                    label_CustomTuningDStringLetter.Text = GuitarSpeak.GuitarSpeakNoteOctaveMath($"{(int)nUpDown_String2.Value + offset}");
                    break;
                case 3:
                    offset = 55; // G3 (Midi)
                    label_CustomTuningGStringLetter.Text = GuitarSpeak.GuitarSpeakNoteOctaveMath($"{(int)nUpDown_String3.Value + offset}");
                    break;
                case 4:
                    offset = 59; // B3 (Midi)
                    label_CustomTuningBStringLetter.Text = GuitarSpeak.GuitarSpeakNoteOctaveMath($"{(int)nUpDown_String4.Value + offset}");
                    break;
                case 5:
                    offset = 64; // E4 (Midi)
                    label_CustomTuningHighEStringLetter.Text = GuitarSpeak.GuitarSpeakNoteOctaveMath($"{(int)nUpDown_String5.Value + offset}").ToLower();
                    break;
                default: // Yeah we don't know wtf happened here
                    MessageBox.Show("Invalid String Number! Please report this to the GUI devs!");
                    break;
            }

            // Change string color if the user if it would be "extended range" of that string.
            if (Toggles.ExtendedRange && ModSettings.ExtendedRangeModeAt >= (int)((NumericUpDown)sender).Value)
                SetForget_SetTunerColors(stringNumber, true);
            else
                SetForget_SetTunerColors(stringNumber);
        }

        #endregion
        #region Save Setting Middleware

        private void Save_ToggleLoft(object sender, EventArgs e)
        {
            bool isChecked = checkBox_ToggleLoft.Checked;
            Toggles.ToggleLoft = isChecked;

            groupBox_LoftOffWhen.Visible = isChecked;
        }

        private void Save_ToggleLoftWhen(object sender, EventArgs e) => Toggles.ToggleLoftWhen = radio_LoftOffHotkey.Checked ? LoftMode.Manual
                                                                       : radio_LoftOffInSong.Checked ? LoftMode.Song
                                                                       : LoftMode.Startup;
        private void Save_SongTimer(object sender, EventArgs e)
        {
            Toggles.ShowSongTimer = checkBox_SongTimer.Checked;
            groupBox_SongTimer.Visible = checkBox_SongTimer.Checked;
        }

        private void Save_ForceEnumeration(object sender, EventArgs e)
        {
            radio_ForceEnumerationAutomatic.Visible = checkBox_ForceEnumeration.Checked;
            radio_ForceEnumerationManual.Visible = checkBox_ForceEnumeration.Checked;
            groupBox_HowToEnumerate.Visible = checkBox_ForceEnumeration.Checked;

            Toggles.ForceReEnumeration = checkBox_ForceEnumeration.Checked ? EnumerationMode.Manual : EnumerationMode.Off;
        }

        private void Save_EnumerateEveryXMS(object sender, EventArgs e) => ModSettings.CheckForNewSongsInterval = (int)(nUpDown_ForceEnumerationXMS.Value * 1000);

        private void Save_ForceEnumerationAutomatic(object sender, EventArgs e)
        {
            label_ForceEnumerationXMS.Visible = true;
            nUpDown_ForceEnumerationXMS.Visible = true;
            Toggles.ForceReEnumeration = EnumerationMode.Automatic;
        }

        private void Save_ForceEnumerationManual(object sender, EventArgs e)
        {
            label_ForceEnumerationXMS.Visible = false;
            nUpDown_ForceEnumerationXMS.Visible = false;
            Toggles.ForceReEnumeration = EnumerationMode.Manual;
        }

        private void Save_RainbowStrings(object sender, EventArgs e) => Toggles.RainbowStrings = checkBox_RainbowStrings.Checked;

        private void Save_RainbowNotes(object sender, EventArgs e) => Toggles.RainbowNotes = checkBox_RainbowNotes.Checked;

        private void Save_ExtendedRange(object sender, EventArgs e)
        {
            groupBox_ExtendedRangeWhen.Visible = checkBox_ExtendedRange.Checked;
            listBox_ExtendedRangeTunings.Visible = checkBox_ExtendedRange.Checked;
            checkBox_CustomColors.Checked = checkBox_ExtendedRange.Checked;

            Toggles.ExtendedRange = checkBox_ExtendedRange.Checked;
            Toggles.CustomStringColors = checkBox_ExtendedRange.Checked ? CustomStringColorMode.Custom : CustomStringColorMode.Off;
        }

        private void Save_CustomStringColors(object sender, EventArgs e)
        {
            groupBox_StringColors.Visible = checkBox_CustomColors.Checked;

            Toggles.CustomStringColors = checkBox_CustomColors.Checked ? CustomStringColorMode.Custom : CustomStringColorMode.Off;
        }

        private void Save_RemoveHeadstockCheckbox(object sender, EventArgs e)
        {
            Toggles.Headstock = checkBox_RemoveHeadstock.Checked;
            groupBox_ToggleHeadstockOffWhen.Visible = checkBox_RemoveHeadstock.Checked;
        }

        private void Save_RemoveSkyline(object sender, EventArgs e)
        {
            Toggles.Skyline = checkBox_RemoveSkyline.Checked;
            groupBox_ToggleSkylineWhen.Visible = checkBox_RemoveSkyline.Checked;
        }

        private void Save_GreenScreenWall(object sender, EventArgs e) => Toggles.GreenScreenWall = checkBox_GreenScreen.Checked;

        private void Save_AutoLoadLastProfile(object sender, EventArgs e)
        {
            Toggles.ForceProfileLoad = checkBox_AutoLoadProfile.Checked;
            groupBox_AutoLoadProfiles.Visible = checkBox_AutoLoadProfile.Checked;
        }

        private void Save_Fretless(object sender, EventArgs e) => Toggles.Fretless = checkBox_Fretless.Checked;

        private void Save_RemoveInlays(object sender, EventArgs e) => Toggles.Inlays = checkBox_RemoveInlays.Checked;

        private void Save_RemoveLaneMarkers(object sender, EventArgs e) => Toggles.LaneMarkers = checkBox_RemoveLaneMarkers.Checked;

        private void Save_ToggleSkylineSong(object sender, EventArgs e)
        {
            if (radio_SkylineOffInSong.Checked) Toggles.ToggleSkylineWhen = SkylineMode.Song;
        }

        private void Save_ToggleSkylineStartup(object sender, EventArgs e)
        {
            if (radio_SkylineAlwaysOff.Checked) Toggles.ToggleSkylineWhen = SkylineMode.Startup;
        }

        private void Save_ExtendedRangeTuningAt(object sender, EventArgs e) => ModSettings.ExtendedRangeModeAt = (listBox_ExtendedRangeTunings.SelectedIndex * -1) - 2;

        private void Delete_Keybind_MODS(object sender, EventArgs e)
        {
            textBox_NewKeyAssignment_MODS.Text = "";

            if (listBox_Modlist_MODS.SelectedIndex >= 0)
            {
                Dictionaries.ModKeybinds[listBox_Modlist_MODS.SelectedIndex].SetKey("");
            }
            Startup_ShowCurrentKeybindingValues();
        }

        private void Delete_Keybind_AUDIO(object sender, EventArgs e)
        {
            textBox_NewKeyAssignment_AUDIO.Text = "";

            if (listBox_Modlist_AUDIO.SelectedIndex >= 0)
            {
                Dictionaries.AudioKeybinds[listBox_Modlist_AUDIO.SelectedIndex].SetKey("");
            }
            Startup_ShowCurrentAudioKeybindingValues();
        }

        private void Save_RemoveLyrics(object sender, EventArgs e)
        {
            Toggles.Lyrics = checkBox_RemoveLyrics.Checked;
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
            if (radio_LyricsAlwaysOff.Checked) Toggles.RemoveLyricsWhen = LyricsMode.Startup;
        }

        private void Save_ToggleLyricsManual(object sender, EventArgs e)
        {
            if (radio_LyricsOffHotkey.Checked) Toggles.RemoveLyricsWhen = LyricsMode.Manual;
        }

        private void Save_VolumeControls(object sender, EventArgs e)
        {
            groupBox_Keybindings_AUDIO.Visible = checkBox_ControlVolume.Checked;
            groupBox_ControlVolumeIncrement.Visible = checkBox_ControlVolume.Checked;
            Toggles.VolumeControl = checkBox_ControlVolume.Checked;
        }

        private void Save_RiffRepeaterSpeedInterval(object sender, EventArgs e) => ModSettings.RRSpeedInterval = nUpDown_RiffRepeaterSpeed.Value;

        private void Save_RiffRepeaterSpeedAboveOneHundred(object sender, EventArgs e)
        {
            Toggles.RRSpeedAboveOneHundred = checkBox_RiffRepeaterSpeedAboveOneHundred.Checked;
            groupBox_RRSpeed.Visible = checkBox_RiffRepeaterSpeedAboveOneHundred.Checked;
        }

        private void Save_UseMidiAutoTuning(object sender, EventArgs e)
        {
            Toggles.AutoTuneForSong = checkBox_useMidiAutoTuning.Checked;
            groupBox_MidiAutoTuneDevice.Visible = checkBox_useMidiAutoTuning.Checked;
            groupBox_MidiAutoTuningOffset.Visible = checkBox_useMidiAutoTuning.Checked;
            groupBox_MidiAutoTuningWhen.Visible = checkBox_useMidiAutoTuning.Checked;
        }

        private void Save_AutoTuneDevice(object sender, EventArgs e)
        {
            if (listBox_ListMidiOutDevices.SelectedItem != null)
            {
                Toggles.AutoTuneForSongDevice = listBox_ListMidiOutDevices.SelectedItem.ToString();
                label_SelectedMidiOutDevice.Text = "Midi Device: " + listBox_ListMidiOutDevices.SelectedItem;
            }
        }

        private void Save_MidiInDevice(object sender, EventArgs e)
        {
            if (listBox_ListMidiInDevices.SelectedItem != null)
            {
                Toggles.MidiInDevice = listBox_ListMidiInDevices.SelectedItem.ToString();
                label_SelectedMidiInDevice.Text = "Midi Device: " + listBox_ListMidiInDevices.SelectedItem;
            }
        }

        private void Save_WhammyDT(object sender, EventArgs e) => ModSettings.TuningPedal = TuningPedalDevice.WhammyDT;

        private void Save_WhammyBass(object sender, EventArgs e)
        {
            ModSettings.TuningPedal = TuningPedalDevice.WhammyBass;
            checkBox_WhammyFiveChordsMode.Visible = radio_WhammyBass.Checked;
        }

        private void Save_WhammyFive(object sender, EventArgs e)
        {
            ModSettings.TuningPedal = TuningPedalDevice.WhammyFive;
            checkBox_WhammyFiveChordsMode.Visible = radio_WhammyFive.Checked;
        }

        private void Save_WhammyFour(object sender, EventArgs e) => ModSettings.TuningPedal = TuningPedalDevice.WhammyFour;
        private void Save_SoftwarePedal(object sender, EventArgs e) => ModSettings.TuningPedal = TuningPedalDevice.SoftwarePedal;
        private void Save_WhammyChordsMode(object sender, EventArgs e) => Toggles.ChordsMode = checkBox_WhammyFiveChordsMode.Checked;
        private void Save_ExtendedRangeDrop(object sender, EventArgs e) => Toggles.ExtendedRangeDropTuning = checkBox_ExtendedRangeDrop.Checked;
        private void Save_ShowCurrentNote(object sender, EventArgs e) => Toggles.ShowCurrentNoteOnScreen = checkBox_ShowCurrentNote.Checked;
        private void Save_ScreenShotScores(object sender, EventArgs e) => Toggles.ScreenShotScores = checkBox_ScreenShotScores.Checked;

        private void Save_HeadStockAlwaysOffButton(object sender, EventArgs e)
        {
            if (radio_HeadstockAlwaysOff.Checked) Toggles.RemoveHeadstockWhen = HeadstockMode.Startup;
        }
        private void Save_HeadstockOffInSongOnlyButton(object sender, EventArgs e)
        {
            if (radio_HeadstockOffInSong.Checked) Toggles.RemoveHeadstockWhen = HeadstockMode.Song;
        }

        private void Save_VolumeInterval(object sender, EventArgs e) => ModSettings.VolumeControlInterval = (int)nUpDown_VolumeInterval.Value;

        private void Save_AutoLoadProfile(object sender, EventArgs e)
        {
            Toggles.ProfileToLoad = listBox_AutoLoadProfiles.SelectedIndex == -1 ? "" : listBox_AutoLoadProfiles.SelectedItem.ToString();
        }

        private void AutoLoadProfile_ClearSelection(object sender, EventArgs e) => listBox_AutoLoadProfiles.ClearSelected();
        private void Save_BackupProfile(object sender, EventArgs e)
        {
            GUISettings.BackupProfile = checkBox_BackupProfile.Checked;
            groupBox_Backups.Visible = checkBox_BackupProfile.Checked;


            if (checkBox_BackupProfile.Checked)
            {
                Profiles.SaveProfile();

                if (string.IsNullOrEmpty(Profiles.GetSaveDirectory()))
                {
                    MessageBox.Show("It looks like your profile(s) can't be found :(\nWe are disabling the Backup Profile mod so it doesn't look like we're lying to you.");
                    checkBox_BackupProfile.Checked = false;
                }
                else
                {
                    Startup_UnlockProfileEdits();
                }
            }
            else if (AllowSaving)
            {
                Startup_LockProfileEdits();
            }
        }

        private void UnlimitedBackups(object sender, EventArgs e)
        {
            if (checkBox_UnlimitedBackups.Checked)
            {
                nUpDown_NumberOfBackups.Value = 0;
            }
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
            GUISettings.NumberOfBackups = (int)nUpDown_NumberOfBackups.Value;
        }

        private void Save_CustomHighway(object sender, EventArgs e)
        {
            HighwayColors.CustomHighwayColors = checkBox_CustomHighway.Checked;
            groupBox_CustomHighway.Visible = checkBox_CustomHighway.Checked;
        }

        private void ResetNotewayColors(object sender, EventArgs e)
        {
            HighwayColors.CustomHighwayNumbered = "";
            HighwayColors.CustomHighwayUnNumbered = "";
            HighwayColors.CustomHighwayGutter = "";
            HighwayColors.CustomFretNubmers = "";


            textBox_ShowNumberedFrets.BackColor = SystemColors.Control;
            textBox_ShowUnNumberedFrets.BackColor = SystemColors.Control;
            textBox_ShowNotewayGutter.BackColor = SystemColors.Control;
            textBox_ShowFretNumber.BackColor = SystemColors.Control;
        }

        private void Save_SongTimerAlways(object sender, EventArgs e)
        {
            if (radio_SongTimerAlways.Checked) Toggles.ShowSongTimerWhen = SongTimerWhen.Automatic;
        }

        private void Save_SongTimerManual(object sender, EventArgs e)
        {
            if (radio_SongTimerManual.Checked) Toggles.ShowSongTimerWhen = SongTimerWhen.Manual;
        }

        private void Save_SecondaryMonitorStartPosition(object sender, EventArgs e)
        {
            Process guiProcess = Process.GetProcessesByName("RSMods")[0];
            IntPtr ptr = guiProcess.MainWindowHandle;
            Rect guiLocation = new();
            GetWindowRect(ptr, ref guiLocation);

            ModSettings.SecondaryMonitorXPosition = guiLocation.Left + 8;
            ModSettings.SecondaryMonitorYPosition = guiLocation.Top + 8;
        }

        private void Save_SecondaryMonitor(object sender, EventArgs e)
        {
            button_SecondaryMonitorStartPos.Visible = checkBox_SecondaryMonitor.Checked;
            Toggles.SecondaryMonitor = checkBox_SecondaryMonitor.Checked ? OnOffMode.On : OnOffMode.Off;
        }

        private void Save_ER_SeparateNoteColors(object sender, EventArgs e)
        {
            Toggles.SeparateNoteColors = checkBox_ER_SeparateNoteColors.Checked ? OnOffMode.On : OnOffMode.Off;
            groupBox_NoteColors.Visible = checkBox_ER_SeparateNoteColors.Checked;
            ModSettings.SeparateNoteColorsMode = checkBox_ER_SeparateNoteColors.Checked ? NoteColorMode.Custom : NoteColorMode.Off;
        }

        private void Save_NoteColors_UseRocksmithColors(object sender, EventArgs e)
        {
            ModSettings.SeparateNoteColorsMode = checkBox_NoteColors_UseRocksmithColors.Checked ? NoteColorMode.RocksmithColors : NoteColorMode.Custom;

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
            {
                StringColors_LoadDefaultNoteColors(radio_colorBlindERNoteColors.Checked);
            }

            radio_DefaultNoteColors.Enabled = !checkBox_NoteColors_UseRocksmithColors.Checked;
            radio_colorBlindERNoteColors.Enabled = !checkBox_NoteColors_UseRocksmithColors.Checked;
        }

        private void Save_DumpRSModsLogToFile(object sender, EventArgs e)
        {
            if (!AllowSaving)
                return;

            if (checkBox_ModsLog.Checked)
            {
                File.Create(Path.Combine(GenUtil.GetRSDirectory(), "RSMods_debug.txt"));
            }
            else
            {
                // First we have to do garbage cleanup since it keeps the file in use way too long.
                GC.Collect();
                GC.WaitForPendingFinalizers();

                // We can finally delete the file :)
                File.Delete(Path.Combine(GenUtil.GetRSDirectory(), "RSMods_debug.txt"));
            }
        }

        private void Save_MidiAutoTuningOffset(object sender, EventArgs e)
        {
            if (listBox_MidiAutoTuningOffset.SelectedIndex > -1)
            {
                ModSettings.TuningOffset = listBox_MidiAutoTuningOffset.SelectedIndex - 3;
            }
        }

        private void Save_AutoTuningWhenManual(object sender, EventArgs e) => Toggles.AutoTuneForSongWhen = AutoTuneWhen.Manual;

        private void Save_AutoTuningWhenTuner(object sender, EventArgs e) => Toggles.AutoTuneForSongWhen = AutoTuneWhen.Tuner;

        private void Save_TurnOffAllMods(object sender, EventArgs e)
        {
            if (File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll")) && !File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll.off"))) // Has DLL enabled and doesn't have DLL turned off
            {
                File.Move(Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll"), Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll.off"));
            }
            else if (File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll.off")) && !File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll"))) // Has DLL turned off and doesn't have DLL enabled
            {
                File.Move(Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll.off"), Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll"));
            }
            else if (File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll")) && File.Exists(Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll.off"))) // Has DLL enabled AND turned off.
            {
                File.Delete(Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll.off"));

                File.Move(Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll"), Path.Combine(GenUtil.GetRSDirectory(), "xinput1_3.dll.off"));
            }
        }

        private void Save_ERFixBadBassTuning(object sender, EventArgs e) => Toggles.ExtendedRangeFixBassTuning = checkBox_FixBadBassTuning.Checked;

        private void Save_RemoveSongPreviews(object sender, EventArgs e) => Toggles.SongPreviews = checkBox_RemoveSongPreviews.Checked;

        private void Save_OverrideInputVolumeEnabled(object sender, EventArgs e)
        {
            groupBox_OverrideInputVolume.Visible = checkBox_OverrideInputVolume.Checked;
            Toggles.OverrideInputVolumeEnabled = checkBox_OverrideInputVolume.Checked;
        }

        private void Save_OverrideInputVolume(object sender, EventArgs e) => ModSettings.OverrideInputVolume = (int)nUpDown_OverrideInputVolume.Value;

        private void Save_OverrideInputVolumeDevice(object sender, EventArgs e)
        {
            if (listBox_AvailableInputDevices.SelectedItem != null)
            {
                Toggles.OverrideInputVolumeDevice = listBox_AvailableInputDevices.SelectedItem.ToString();
            }
        }

        private void Save_AllowAudioInBackground(object sender, EventArgs e) => Toggles.AllowAudioInBackground = checkBox_AllowAudioInBackground.Checked;

        private void Save_BypassTwoRTCMessageBox(object sender, EventArgs e) => Toggles.BypassTwoRTCMessageBox = checkBox_BypassTwoRTCMessageBox.Checked;
        private void Save_LinearRiffRepeater(object sender, EventArgs e) => Toggles.LinearRiffRepeater = checkBox_LinearRiffRepeater.Checked;

        private void Save_UseAlternativeSampleRate_Output(object sender, EventArgs e)
        {
            groupBox_SampleRateOutput.Visible = checkBox_UseAltSampleRate_Output.Checked;
            Toggles.AltOutputSampleRate = checkBox_UseAltSampleRate_Output.Checked;
        }

        private void Save_AltSampleRatesOutput(object sender, EventArgs e)
        {
            if (listBox_AltSampleRatesOutput.SelectedItem != null)
            {
                ModSettings.AlternativeOutputSampleRate = int.Parse(listBox_AltSampleRatesOutput.SelectedItem.ToString().Split(' ')[0]);
            }
        }

        private void Save_EnableLooping(object sender, EventArgs e)
        {
            Toggles.AllowLooping = checkBox_EnableLooping.Checked;
            groupBox_LoopingLeadUp.Visible = checkBox_EnableLooping.Checked;
        }

        private void Save_LoopingLeadUp(object sender, EventArgs e) => ModSettings.LoopingLeadUp = (int)(nUpDown_LoopingLeadUp.Value * 1000);

        private void Save_AllowRewind(object sender, EventArgs e)
        {
            Toggles.AllowRewind = checkBox_AllowRewind.Checked;
            groupBox_RewindBy.Visible = checkBox_AllowRewind.Checked;
            groupBox_RewindLeadup.Visible = checkBox_AllowRewind.Checked;
        }

        private void Save_RewindBy(object sender, EventArgs e) => ModSettings.RewindBy = (int)(nUpDown_RewindBy.Value * 1000);
        private void Save_RewindLeadup(object sender, EventArgs e) => ModSettings.RewindLeadup = (int)(nUpDown_RewindLeadup.Value * 1000);

        private void Save_FixOculusCrash(object sender, EventArgs e) => Toggles.FixOculusCrash = checkBox_FixOculusCrash.Checked;

        private void Save_FixBrokenTones(object sender, EventArgs e) => Toggles.FixBrokenTones = checkBox_FixBrokenTones.Checked;

        private void Save_UseCustomNSPTimer(object sender, EventArgs e)
        {
            Toggles.UseCustomNSPTimer = checkBox_CustomNSPTimer.Checked;
            groupBox_NSPTimer.Visible = checkBox_CustomNSPTimer.Checked;
        }

        private void Save_DisplaySongAccuracy(object sender, EventArgs e) => Toggles.DisplayCurrentAccuracy = checkBox_DisplayCurrentAccuracy.Checked;
        private void Save_PreventMidSongPause(object sender, EventArgs e) => Toggles.PreventMidSongPause = checkBox_PreventMidSongPause.Checked;

        private void Save_NSPTimer(object sender, EventArgs e) => ModSettings.CustomNSPTimeLimit = (int)(nUpDown_NSPTimer.Value * 1000);

        private void Save_RemoveFingerprints(object sender, EventArgs e) => Toggles.RemoveFingerprints = checkBox_RemoveFingerprints.Checked;

        private void Save_SetSavePath(object sender, EventArgs e)
        {
            string saveFolder = GenUtil.GetSaveFolder(true);

            if (saveFolder.Length == 0)
            {
                MessageBox.Show("We did not save a Save Folder");
                return;
            }

            // Backup profiles & prevent the user from pressing this button again
            checkBox_BackupProfile.Checked = true;
            button_SetSavePath.Visible = false;

            Constants.SavePath = saveFolder;
            Constants.SavePathDeclined = false;
            Constants.SaveBaseSettings();

            // Refresh the Profile Edits UI, now that we have the information we need.
            Startup_LoadRocksmithProfiles();
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
            }
        }

        public ToolTip currentTooltip = new(); // Fixes toolTip duplication glitch.

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
            groupBox_GuitarSpeak.Visible = checkBox_GuitarSpeak.Checked;
            checkBox_GuitarSpeakWhileTuning.Visible = checkBox_GuitarSpeak.Checked;
            Toggles.GuitarSpeak = checkBox_GuitarSpeak.Checked;
        }

        private void GuitarSpeak_Save(object sender, EventArgs e)
        {
            if (listBox_GuitarSpeakNote.SelectedIndex >= 0 && listBox_GuitarSpeakOctave.SelectedIndex >= 0 && listBox_GuitarSpeakKeypress.SelectedIndex >= 0)
            {
                int inputNote = listBox_GuitarSpeakNote.SelectedIndex + 36; // We skip the first 3 octaves to give an accurate representation of the notes being played
                int inputOctave = listBox_GuitarSpeakOctave.SelectedIndex - 3; // -1 for the offset, and -2 for octave offset in DLL.
                int outputNoteOctave = inputNote + (inputOctave * 12);

                MessageBox.Show($"{listBox_GuitarSpeakNote.SelectedItem}{listBox_GuitarSpeakOctave.SelectedItem} was saved to {listBox_GuitarSpeakKeypress.SelectedItem}", "Note Saved!", MessageBoxButtons.OK, MessageBoxIcon.Information);

                int index = listBox_GuitarSpeakKeypress.SelectedIndex;
                Dictionaries.GuitarSpeakKeybinds[index].SetKey(outputNoteOctave.ToString());
                GuitarSpeak_ResetPresets();

                listBox_GuitarSpeakNote.ClearSelected();
                listBox_GuitarSpeakOctave.ClearSelected();
                listBox_GuitarSpeakKeypress.ClearSelected();
            }
            else
            {
                MessageBox.Show("One, or more, of the Guitar Speak boxes not selected", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void GuitarSpeak_WhileTuning(object sender, EventArgs e) => RsModsSettings.GuitarSpeak.GuitarSpeakWhileTuning = checkBox_GuitarSpeakWhileTuning.Checked;

        private void GuitarSpeak_Help(object sender, EventArgs e) => Process.Start("https://pastebin.com/raw/PZ0FQTn0");

        private void GuitarSpeak_ResetPresets()
        {
            listBox_GuitarSpeakSaved.Items.Clear();

            foreach (var keybind in Dictionaries.GuitarSpeakKeybinds)
                listBox_GuitarSpeakSaved.Items.Add($"{keybind.DisplayName}: {GuitarSpeak.GuitarSpeakNoteOctaveMath(keybind.GetKey())}");
        }

        private void GuitarSpeak_ClearSavedValue(object sender, EventArgs e)
        {
            int valueToRemove = listBox_GuitarSpeakSaved.SelectedIndex;

            if (valueToRemove == -1)
                return;

            listBox_GuitarSpeakSaved.SelectedIndex = -1;

            Dictionaries.GuitarSpeakKeybinds[valueToRemove].SetKey("");

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

                    if (!string.IsNullOrEmpty(selectedReward.AdditionalMsg) && selectedReward.AdditionalMsg != "Random")
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

            Binding listeningToTwitchBinding = new("Text", TwitchSettings.Get, "Authorized");
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
            ImplicitAuth auth = new();

            string authRes = auth.MakeAuthRequest();

            if (!authRes.Equals("OK"))
            {
                MessageBox.Show($"Please open the following link in your browser: {authRes}", "Can't open your browser!");
            }
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

        private async Task Twitch_SaveRewards()
        {
            await Task.Run(() =>
            {
                XmlSerializer xs = new(TwitchSettings.Get.Rewards.GetType());
                using var sww = new StringWriter();
                using XmlWriter writer = XmlWriter.Create(sww, new XmlWriterSettings { Indent = true });

                xs.Serialize(writer, TwitchSettings.Get.Rewards);

                string exePath = AppDomain.CurrentDomain.BaseDirectory;
                string effectListPath = Path.Combine(exePath, "TwitchEnabledEffects.xml");

                File.WriteAllText(effectListPath, sww.ToString());
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
            if (reward is BitsReward bitsReward)
                dgv_EnabledRewards.Rows.Add(reward.Enabled, reward.Name, reward.Length, bitsReward.BitsAmount, "Bits", bitsReward.BitsID);
            else if (reward is ChannelPointsReward channelPointsReward)
                dgv_EnabledRewards.Rows.Add(reward.Enabled, reward.Name, reward.Length, channelPointsReward.PointsAmount, "Points", channelPointsReward.PointsID);
            else if (reward is SubReward subReward)
                dgv_EnabledRewards.Rows.Add(reward.Enabled, reward.Name, reward.Length, 1, "Sub", subReward.SubID);
        }

        private TwitchReward Twitch_GetSelectedReward(DataGridViewRow selectedRow)
        {
            if (selectedRow.Cells["colEnabledRewardsType"].Value.ToString() == "Bits")
                return TwitchSettings.Get.Rewards.FirstOrDefault(r => r is BitsReward bitsReward && bitsReward.BitsID.ToString() == selectedRow.Cells["colEnabledRewardsID"].Value.ToString());
            else if (selectedRow.Cells["colEnabledRewardsType"].Value.ToString() == "Sub")
                return TwitchSettings.Get.Rewards.FirstOrDefault(r => r is SubReward subReward && subReward.SubID.ToString() == selectedRow.Cells["colEnabledRewardsID"].Value.ToString());
            else
                return TwitchSettings.Get.Rewards.FirstOrDefault(r => r is ChannelPointsReward channelPointsReward && channelPointsReward.PointsID.ToString() == selectedRow.Cells["colEnabledRewardsID"].Value.ToString());
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
            if (selectedRow.Cells["colEnabledRewardsLength"].Value == null || !Int32.TryParse(selectedRow.Cells["colEnabledRewardsLength"].Value.ToString(), out int rewardLength))
            {
                selectedRow.Cells["colEnabledRewardsLength"].Value = 0;
                MessageBox.Show("You need to put a number, not a text value.");
                return;
            }

            if (selectedRow.Cells["colEnabledRewardsAmount"].Value == null || !Int32.TryParse(selectedRow.Cells["colEnabledRewardsAmount"].Value.ToString(), out int rewardAmount))
            {
                selectedRow.Cells["colEnabledRewardsAmount"].Value = 0;
                MessageBox.Show("You need to put a number, not a text value.");
                return;
            }

            selectedReward.Length = rewardLength;

            if (selectedReward is BitsReward bitsReward)
                bitsReward.BitsAmount = Convert.ToInt32(selectedRow.Cells["colEnabledRewardsAmount"].Value);
            else if (selectedReward is ChannelPointsReward channelPointsReward)
                channelPointsReward.PointsAmount = Convert.ToInt32(selectedRow.Cells["colEnabledRewardsAmount"].Value);

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

            if (string.IsNullOrEmpty(selectedReward.AdditionalMsg) || selectedReward.AdditionalMsg == "Random")
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
            ColorDialog colorDialog = new()
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
            {
                PubSub.Get.Resub();
            }
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
                string exePath = AppDomain.CurrentDomain.BaseDirectory;
                string logPath = Path.Combine(exePath, "twitchLog.txt");

                File.WriteAllText(logPath, TwitchSettings.Get.Log);
                MessageBox.Show("Saved log to RS folder/RSMods/twitchLog.txt!", "Saved!");
            }
            catch (IOException ioex)
            {
                MessageBox.Show($"Unable to save log, error: {ioex.Message}");
            }
        }

        private void Twitch_CopyCredentialsForDevs(object sender, MouseEventArgs e) => Clipboard.SetText("Send to RSMod Developers ( Discord Ffio#2221 or LovroM8#9999 )\nUsername: " + TwitchSettings.Get.Username + "\nChannel ID: " + TwitchSettings.Get.ChannelID + "\nAccess Token: " + TwitchSettings.Get.AccessToken);
        #endregion
        #region Custom Fonts
        private void Fonts_Load() // Not modified from here: https://stackoverflow.com/a/8657854 :eyes:
        {
            InstalledFontCollection fontList = new();

            foreach (FontFamily font in fontList.Families)
            {
                listBox_AvailableFonts.Items.Add(font.Name);
            }

            listBox_AvailableFonts.SelectedItem = Toggles.OnScreenFont;
        }

        private void Fonts_Change(object sender, EventArgs e)
        {
            string fontName = listBox_AvailableFonts.SelectedItem.ToString();
            Font newFontSelected = new(fontName, 10.0f, Font.Style, Font.Unit);
            label_FontTestCAPITALS.Font = newFontSelected;
            label_FontTestlowercase.Font = newFontSelected;
            label_FontTestNumbers.Font = newFontSelected;

            Toggles.OnScreenFont = fontName;
        }
        #endregion
        #region RS_ASIO

        // Config
        private void ASIO_WASAPI_Output(object sender, EventArgs e) => AsioSettings.Config.EnableWasapiOutputs = checkBox_ASIO_WASAPI_Output.Checked;
        private void ASIO_WASAPI_Input(object sender, EventArgs e) => AsioSettings.Config.EnableWasapiInputs = checkBox_ASIO_WASAPI_Input.Checked;
        private void ASIO_ASIO(object sender, EventArgs e) => AsioSettings.Config.EnableAsio = checkBox_ASIO_ASIO.Checked;

        // Driver
        private void ASIO_ListAvailableInput0(object sender, EventArgs e)
        {
            if (listBox_AvailableASIODevices_Input0.SelectedItem != null)
                AsioSettings.Input0.Driver = listBox_AvailableASIODevices_Input0.SelectedItem.ToString();
        }

        private void ASIO_ListAvailableInput1(object sender, EventArgs e)
        {
            if (listBox_AvailableASIODevices_Input1.SelectedItem != null)
                AsioSettings.Input1.Driver = listBox_AvailableASIODevices_Input1.SelectedItem.ToString();
        }

        private void ASIO_ListAvailableOutput(object sender, EventArgs e)
        {
            if (listBox_AvailableASIODevices_Output.SelectedItem != null)
                AsioSettings.Output.Driver = listBox_AvailableASIODevices_Output.SelectedItem.ToString();
        }

        private void ASIO_ListAvailableInputMic(object sender, EventArgs e)
        {
            if (listBox_AvailableASIODevices_InputMic.SelectedItem != null)
                AsioSettings.InputMic.Driver = listBox_AvailableASIODevices_InputMic.SelectedItem.ToString();
        }

        // Disable / Comment Out Driver
        private void ASIO_Output_Disable(object sender, EventArgs e)
        {
            AsioSettings.Output.Disabled = checkBox_ASIO_Output_Disabled.Checked;
            if (!checkBox_ASIO_Output_Disabled.Checked && listBox_AvailableASIODevices_Output.SelectedItem != null)
                AsioSettings.Output.Driver = listBox_AvailableASIODevices_Output.SelectedItem.ToString();
        }

        private void ASIO_Input0_Disable(object sender, EventArgs e)
        {
            AsioSettings.Input0.Disabled = checkBox_ASIO_Input0_Disabled.Checked;
            if (!checkBox_ASIO_Input0_Disabled.Checked && listBox_AvailableASIODevices_Input0.SelectedItem != null)
                AsioSettings.Input0.Driver = listBox_AvailableASIODevices_Input0.SelectedItem.ToString();
        }

        private void ASIO_Input1_Disable(object sender, EventArgs e)
        {
            AsioSettings.Input1.Disabled = checkBox_ASIO_Input1_Disabled.Checked;
            if (!checkBox_ASIO_Input1_Disabled.Checked && listBox_AvailableASIODevices_Input1.SelectedItem != null)
                AsioSettings.Input1.Driver = listBox_AvailableASIODevices_Input1.SelectedItem.ToString();
        }

        private void ASIO_InputMic_Disable(object sender, EventArgs e)
        {
            AsioSettings.InputMic.Disabled = checkBox_ASIO_InputMic_Disabled.Checked;
            if (!checkBox_ASIO_InputMic_Disabled.Checked && listBox_AvailableASIODevices_InputMic.SelectedItem != null)
                AsioSettings.InputMic.Driver = listBox_AvailableASIODevices_InputMic.SelectedItem.ToString();
        }

        // Buffer Size
        private void ASIO_BufferSize_Driver(object sender, EventArgs e)
        {
            if (radio_ASIO_BufferSize_Driver.Checked)
                AsioSettings.AsioSection.BufferSizeMode = "driver";
        }

        private void ASIO_BufferSize_Host(object sender, EventArgs e)
        {
            if (radio_ASIO_BufferSize_Host.Checked)
                AsioSettings.AsioSection.BufferSizeMode = "host";
        }

        private void ASIO_BufferSize_Custom(object sender, EventArgs e)
        {
            label_ASIO_CustomBufferSize.Visible = radio_ASIO_BufferSize_Custom.Checked;
            nUpDown_ASIO_CustomBufferSize.Visible = radio_ASIO_BufferSize_Custom.Checked;
            AsioSettings.AsioSection.BufferSizeMode = "custom";
        }

        private void ASIO_CustomBufferSize(object sender, EventArgs e) => AsioSettings.AsioSection.CustomBufferSize = (int)nUpDown_ASIO_CustomBufferSize.Value;

        // Input0 Settings
        private void ASIO_Input0_Channel(object sender, EventArgs e) => AsioSettings.Input0.Channel = (int)nUpDown_ASIO_Input0_Channel.Value;
        private void ASIO_Input0_MaxVolume(object sender, EventArgs e) => AsioSettings.Input0.SoftwareMasterVolumePercent = (int)nUpDown_ASIO_Input0_MaxVolume.Value;
        private void ASIO_Input0_MasterVolume(object sender, EventArgs e)
        {
            AsioSettings.Input0.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_Input0_ControlMasterVolume.Checked;
            label_ASIO_Input0_MaxVolume.Visible = checkBox_ASIO_Input0_ControlMasterVolume.Checked;
            nUpDown_ASIO_Input0_MaxVolume.Visible = checkBox_ASIO_Input0_ControlMasterVolume.Checked;
        }
        private void ASIO_Input0_EndpointVolume(object sender, EventArgs e) => AsioSettings.Input0.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_Input0_ControlEndpointVolume.Checked;
        private void ASIO_Input0_EnableRefHack(object sender, EventArgs e) => AsioSettings.Input0.EnableRefCountHack = checkBox_ASIO_Input0_EnableRefHack.Checked;

        // Input1 Settings
        private void ASIO_Input1_Channel(object sender, EventArgs e) => AsioSettings.Input1.Channel = (int)nUpDown_ASIO_Input1_Channel.Value;
        private void ASIO_Input1_MaxVolume(object sender, EventArgs e) => AsioSettings.Input1.SoftwareMasterVolumePercent = (int)nUpDown_ASIO_Input1_MaxVolume.Value;
        private void ASIO_Input1_MasterVolume(object sender, EventArgs e)
        {
            AsioSettings.Input1.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_Input1_ControlMasterVolume.Checked;
            label_ASIO_Input1_MaxVolume.Visible = checkBox_ASIO_Input1_ControlMasterVolume.Checked;
            nUpDown_ASIO_Input1_MaxVolume.Visible = checkBox_ASIO_Input1_ControlMasterVolume.Checked;
        }
        private void ASIO_Input1_EndpointVolume(object sender, EventArgs e) => AsioSettings.Input1.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_Input1_ControlEndpointVolume.Checked;
        private void ASIO_Input1_EnableRefHack(object sender, EventArgs e) => AsioSettings.Input1.EnableRefCountHack = checkBox_ASIO_Input1_EnableRefHack.Checked;

        // Output Settings
        private void ASIO_Output_BaseChannel(object sender, EventArgs e) => AsioSettings.Output.BaseChannel = (int)nUpDown_ASIO_Output_BaseChannel.Value;
        private void ASIO_Output_AltBaseChannel(object sender, EventArgs e) => AsioSettings.Output.AltBaseChannel = (int)nUpDown_ASIO_Output_AltBaseChannel.Value;
        private void ASIO_Output_MaxVolume(object sender, EventArgs e) => AsioSettings.Output.SoftwareMasterVolumePercent = (int)nUpDown_ASIO_Output_MaxVolume.Value;
        private void ASIO_Output_MasterVolume(object sender, EventArgs e)
        {
            AsioSettings.Output.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_Output_ControlMasterVolume.Checked;
            label_ASIO_Output_MaxVolume.Visible = checkBox_ASIO_Output_ControlMasterVolume.Checked;
            nUpDown_ASIO_Output_MaxVolume.Visible = checkBox_ASIO_Output_ControlMasterVolume.Checked;
        }
        private void ASIO_Output_EndpointVolume(object sender, EventArgs e) => AsioSettings.Output.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_Output_ControlEndpointVolume.Checked;
        private void ASIO_Output_EnableRefHack(object sender, EventArgs e) => AsioSettings.Output.EnableRefCountHack = checkBox_ASIO_Output_EnableRefHack.Checked;

        // InputMic Settings
        private void ASIO_InputMic_Channel(object sender, EventArgs e) => AsioSettings.InputMic.Channel = (int)nUpDown_ASIO_InputMic_Channel.Value;
        private void ASIO_InputMic_MaxVolume(object sender, EventArgs e) => AsioSettings.InputMic.SoftwareMasterVolumePercent = (int)nUpDown_ASIO_InputMic_MaxVolume.Value;
        private void ASIO_InputMic_MasterVolume(object sender, EventArgs e)
        {
            AsioSettings.InputMic.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_InputMic_ControlMasterVolume.Checked;
            label_ASIO_InputMic_MaxVolume.Visible = checkBox_ASIO_InputMic_ControlMasterVolume.Checked;
            nUpDown_ASIO_InputMic_MaxVolume.Visible = checkBox_ASIO_InputMic_ControlMasterVolume.Checked;
        }
        private void ASIO_InputMic_EndpointVolume(object sender, EventArgs e) => AsioSettings.InputMic.EnableSoftwareEndpointVolumeControl = checkBox_ASIO_InputMic_ControlEndpointVolume.Checked;
        private void ASIO_InputMic_EnableRefHack(object sender, EventArgs e) => AsioSettings.InputMic.EnableRefCountHack = checkBox_ASIO_InputMic_EnableRefHack.Checked;

        // Clear Selection
        private void ASIO_ClearSelectedDevice(ListBox deviceList, EventHandler e, Action clearDriverSetting)
        {
            deviceList.SelectedIndexChanged -= e;
            deviceList.SelectedIndex = -1;
            clearDriverSetting();
            deviceList.SelectedIndexChanged += e;
        }
        private void ASIO_Input0_ClearSelection(object sender, EventArgs e) => ASIO_ClearSelectedDevice(listBox_AvailableASIODevices_Input0, ASIO_ListAvailableInput0, () => AsioSettings.Input0.Driver = "");
        private void ASIO_Input1_ClearSelection(object sender, EventArgs e) => ASIO_ClearSelectedDevice(listBox_AvailableASIODevices_Input1, ASIO_ListAvailableInput1, () => AsioSettings.Input1.Driver = "");
        private void ASIO_Output_ClearSelection(object sender, EventArgs e) => ASIO_ClearSelectedDevice(listBox_AvailableASIODevices_Output, ASIO_ListAvailableOutput, () => AsioSettings.Output.Driver = "");
        private void ASIO_InputMic_ClearSelection(object sender, EventArgs e) => ASIO_ClearSelectedDevice(listBox_AvailableASIODevices_InputMic, ASIO_ListAvailableInputMic, () => AsioSettings.InputMic.Driver = "");
        private void ASIO_OpenGithub(object sender, EventArgs e) => Process.Start("https://github.com/mdias/rs_asio");

        #endregion
        #region Rocksmith Settings
        // Audio Settings
        private void Rocksmith_EnableMicrophone(object sender, EventArgs e) => RocksmithSettings.Audio.EnableMicrophone = checkBox_Rocksmith_EnableMicrophone.Checked;
        private void Rocksmith_ExclusiveMode(object sender, EventArgs e) => RocksmithSettings.Audio.ExclusiveMode = checkBox_Rocksmith_ExclusiveMode.Checked;
        private void Rocksmith_LatencyBuffer(object sender, EventArgs e) => RocksmithSettings.Audio.LatencyBuffer = (int)nUpDown_Rocksmith_LatencyBuffer.Value;
        private void Rocksmith_ForceWDM(object sender, EventArgs e) => RocksmithSettings.Audio.ForceWDM = checkBox_Rocksmith_ForceWDM.Checked;
        private void Rocksmith_ForceDirextXSink(object sender, EventArgs e) => RocksmithSettings.Audio.ForceDirectXSink = checkBox_Rocksmith_ForceDirextXSink.Checked;
        private void Rocksmith_DumpAudioLog(object sender, EventArgs e) => RocksmithSettings.Audio.DumpAudioLog = checkBox_Rocksmith_DumpAudioLog.Checked;
        private void Rocksmith_MaxBufferSize(object sender, EventArgs e)
        {
            RocksmithSettings.Audio.MaxOutputBufferSize = (int)nUpDown_Rocksmith_MaxOutputBuffer.Value;
            if (nUpDown_Rocksmith_MaxOutputBuffer.Value == 0)
                checkBox_Rocksmith_Override_MaxOutputBufferSize.Checked = true;
        }
        private void Rocksmith_RTCOnly(object sender, EventArgs e) => RocksmithSettings.Audio.RealToneCableOnly = checkBox_Rocksmith_RTCOnly.Checked;
        private void Rocksmith_LowLatencyMode(object sender, EventArgs e) => RocksmithSettings.Audio.Win32UltraLowLatencyMode = checkBox_Rocksmith_LowLatencyMode.Checked;

        private void Rocksmith_AutomateMaxBufferSize(object sender, EventArgs e)
        {
            nUpDown_Rocksmith_MaxOutputBuffer.Enabled = !checkBox_Rocksmith_Override_MaxOutputBufferSize.Checked;
            nUpDown_Rocksmith_MaxOutputBuffer.Value = checkBox_Rocksmith_Override_MaxOutputBufferSize.Checked ? 0 : 32;
        }

        // Visual Settings
        private void Rocksmith_GamepadUI(object sender, EventArgs e) => RocksmithSettings.RendererWin32.ShowGamepadUI = checkBox_Rocksmith_GamepadUI.Checked;
        private void Rocksmith_ScreenWidth(object sender, EventArgs e) => RocksmithSettings.RendererWin32.ScreenWidth = (int)nUpDown_Rocksmith_ScreenWidth.Value;
        private void Rocksmith_ScreenHeight(object sender, EventArgs e) => RocksmithSettings.RendererWin32.ScreenHeight = (int)nUpDown_Rocksmith_ScreenHeight.Value;
        private void Rocksmith_Windowed(object sender, EventArgs e) => RocksmithSettings.RendererWin32.Fullscreen = FullscreenMode.Windowed;
        private void Rocksmith_NonExclusiveFullScreen(object sender, EventArgs e) => RocksmithSettings.RendererWin32.Fullscreen = FullscreenMode.NonExclusive;
        private void Rocksmith_ExclusiveFullScreen(object sender, EventArgs e) => RocksmithSettings.RendererWin32.Fullscreen = FullscreenMode.Exclusive;
        private void Rocksmith_LowQuality(object sender, EventArgs e)
        {
            checkBox_Rocksmith_DepthOfField.Checked = false;
            checkBox_Rocksmith_PostEffects.Checked = true;
            checkBox_Rocksmith_HighResScope.Checked = true;

            checkBox_Rocksmith_DepthOfField.Enabled = false;
            checkBox_Rocksmith_PostEffects.Enabled = false;
            checkBox_Rocksmith_HighResScope.Enabled = false;

            RocksmithSettings.RendererWin32.VisualQuality = VisualQualityMode.Low;
        }
        private void Rocksmith_MediumQuality(object sender, EventArgs e)
        {
            checkBox_Rocksmith_DepthOfField.Checked = true;
            checkBox_Rocksmith_PostEffects.Checked = true;
            checkBox_Rocksmith_HighResScope.Checked = true;

            checkBox_Rocksmith_DepthOfField.Enabled = false;
            checkBox_Rocksmith_PostEffects.Enabled = false;
            checkBox_Rocksmith_HighResScope.Enabled = false;

            RocksmithSettings.RendererWin32.VisualQuality = VisualQualityMode.Medium;
        }
        private void Rocksmith_HighQuality(object sender, EventArgs e)
        {
            checkBox_Rocksmith_DepthOfField.Checked = true;
            checkBox_Rocksmith_PostEffects.Checked = true;
            checkBox_Rocksmith_HighResScope.Checked = true;

            checkBox_Rocksmith_DepthOfField.Enabled = false;
            checkBox_Rocksmith_PostEffects.Enabled = false;
            checkBox_Rocksmith_HighResScope.Enabled = false;

            RocksmithSettings.RendererWin32.VisualQuality = VisualQualityMode.High;
        }

        private void Rocksmith_CustomQuality(object sender, EventArgs e)
        {
            checkBox_Rocksmith_DepthOfField.Enabled = true;
            checkBox_Rocksmith_PostEffects.Enabled = true;
            checkBox_Rocksmith_HighResScope.Enabled = true;

            RocksmithSettings.RendererWin32.VisualQuality = VisualQualityMode.Custom;
        }
        private void Rocksmith_RenderWidth(object sender, EventArgs e) => RocksmithSettings.RendererWin32.RenderingWidth = (int)nUpDown_Rocksmith_RenderWidth.Value;
        private void Rocksmith_RenderHeight(object sender, EventArgs e) => RocksmithSettings.RendererWin32.RenderingHeight = (int)nUpDown_Rocksmith_RenderHeight.Value;
        private void Rocksmith_PostEffects(object sender, EventArgs e) => RocksmithSettings.RendererWin32.EnablePostEffects = checkBox_Rocksmith_PostEffects.Checked;
        private void Rocksmith_Shadows(object sender, EventArgs e) => RocksmithSettings.RendererWin32.EnableShadows = checkBox_Rocksmith_Shadows.Checked;
        private void Rocksmith_HighResScope(object sender, EventArgs e) => RocksmithSettings.RendererWin32.EnableHighResScope = checkBox_Rocksmith_HighResScope.Checked;
        private void Rocksmith_DepthOfField(object sender, EventArgs e) => RocksmithSettings.RendererWin32.EnableDepthOfField = checkBox_Rocksmith_DepthOfField.Checked;
        private void Rocksmith_PerPixelLighting(object sender, EventArgs e) => RocksmithSettings.RendererWin32.EnablePerPixelLighting = checkBox_Rocksmith_PerPixelLighting.Checked;
        private void Rocksmith_MSAA(object sender, EventArgs e) => RocksmithSettings.RendererWin32.MsaaSamples = checkBox_Rocksmith_MSAASamples.Checked ? MsaaMode.X4 : MsaaMode.Off;
        private void Rocksmith_DisableBrowser(object sender, EventArgs e) => RocksmithSettings.RendererWin32.DisableBrowser = checkBox_Rocksmith_DisableBrowser.Checked;

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
        private void Rocksmith_UseProxy(object sender, EventArgs e) => RocksmithSettings.Net.UseProxy = checkBox_Rocksmith_UseProxy.Checked;

        #endregion
        #region Profiles

        List<SongData> Songs = [];

        private string currentUnpackedProfile = String.Empty;

        private void Profiles_RefreshSonglistNames()
        {
            const int startIndex = 3;

            for (int i = 0; i < Dictionaries.SonglistCount; i++)
            {
                dgv_Profiles_Songlists.Columns[startIndex + i].HeaderText = RsModsSettings.GetSongListTitle(i + 1);
            }
        }

        private void Profiles_LoadSongs(object sender, EventArgs e)
        {
            dgv_Profiles_Songlists.ClearSelection();
            dgv_Profiles_Songlists.Rows.Clear();

            dgv_Profiles_Songlists.Visible = false;
            button_Profiles_SaveSonglist.Visible = false;

            Songs = SongManager.ExtractSongData(progressBar_Profiles_LoadPsarcs);

            // Add RS1 owned DLC
            List<string> ownedRS1DLC = [];
            List<JToken> DLCTags = [.. Profiles.DecryptedProfile["Stats"]["DLCTag"]];
            foreach (JProperty DLCTag in DLCTags.Cast<JProperty>())
            {
                ownedRS1DLC.Add(DLCTag.Name);
            }

            List<List<string>> dlcKeyArrayList = [];
            dlcKeyArrayList.Add(Profiles.DecryptedProfile["FavoritesListRoot"]["FavoritesList"].ToObject<List<string>>());

            List<List<string>> SongLists = Profiles.DecryptedProfile["SongListsRoot"]["SongLists"].ToObject<List<List<string>>>();

            dlcKeyArrayList.AddRange(SongLists);

            foreach (SongData song in Songs.ToList())
            {
                if ((song.RS1AppID != 0 && !ownedRS1DLC.Contains(song.RS1AppID.ToString())) || string.IsNullOrEmpty(song.Artist) || string.IsNullOrEmpty(song.Title) || !song.Shipping)
                {
                    Songs.Remove(song);
                    continue;
                }

                // [0] = artist
                // [1] = title
                // [favorites + song list 1-6] = default profile songlists
                // [7-20] = modified profile lists

                object[] rowValues = new object[23];
                rowValues[0] = song.Artist;
                rowValues[1] = song.Title;

                for (int i = 2; i < 23; i++)
                {
                    rowValues[i] = false;
                }

                for (int i = 0; i < dlcKeyArrayList.Count; i++)
                {
                    if (i <= 20 && dlcKeyArrayList[i].Contains(song.DLCKey))
                    {
                        rowValues[i + 2] = true;
                    }
                }

                dgv_Profiles_Songlists.Rows.Add(rowValues);
            }

            Profiles_RefreshSonglistNames();

            // Hide songlists that the user has not enabled yet.
            // Unhide songlists that the user has enabled.
            for (int songlist = 20; songlist > SongLists.Count; songlist--)
            {
                dgv_Profiles_Songlists.Columns[$"SongList{songlist}"].Visible = false;
            }
            for (int songlist = 1; songlist <= SongLists.Count; songlist++)
            {
                dgv_Profiles_Songlists.Columns[$"SongList{songlist}"].Visible = true;
            }

            dgv_Profiles_Songlists.Visible = true;
            button_Profiles_SaveSonglist.Visible = true;
        }

        private void Profiles_UnpackProfile()
        {
            if (listBox_Profiles_AvailableProfiles.SelectedItem != null && currentUnpackedProfile != listBox_Profiles_AvailableProfiles.SelectedItem.ToString())
            {
                currentUnpackedProfile = listBox_Profiles_AvailableProfiles.SelectedItem.ToString();

                Profiles.DecryptedProfile = JObject.Parse(Profiles.DecryptProfiles(Profiles_GetProfilePathFromName(listBox_Profiles_AvailableProfiles.SelectedItem.ToString())));
            }
        }

        private void Profiles_ChangeSelectedProfile(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedItem == null) return;

            button_Profiles_LoadSongs.Visible = true;
            groupBox_Profiles_Rewards.Visible = true;
            groupBox_Profile_MoreSongLists.Visible = true;
            groupBox_ImportJsonTones.Visible = true;

            Profiles_UnpackProfile();

            List<List<string>> SongLists = Profiles.DecryptedProfile["SongListsRoot"]["SongLists"].ToObject<List<List<string>>>();
            label_TotalSonglists.Text = SongLists.Count.ToString();
        }

        private string Profiles_GetProfilePathFromName(string profileName) => Path.Combine(Profiles.GetSaveDirectory(), Profiles.AvailableProfiles()[profileName] + "_PRFLDB");

        private void Profiles_SaveSonglists(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex < 0)
                return;

            Profiles_ENCRYPT();
            MessageBox.Show("Your songlists and favorites have been saved!");
        }

        private void Profiles_UnlockAllRewards(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex <= -1)
            {
                MessageBox.Show("Make sure you have a profile selected!");
                return;
            }

            if (MessageBox.Show("Are you sure you want to unlock all rewards?\nThat defeats the grind for in-game rewards.", "Are you sure?", MessageBoxButtons.YesNo, MessageBoxIcon.Information) == DialogResult.Yes)
            {
                Profiles.ChangeRewardStatus(true);
                Profiles_SaveRewardsToProfile();
            }
        }

        private void Profiles_LockAllRewards(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex <= -1)
            {
                MessageBox.Show("Make sure you have a profile selected!");
                return;
            }

            if (MessageBox.Show("Are you sure you want to lock all rewards?\nThis will remove all access to in-game rewards.", "Are you sure?", MessageBoxButtons.YesNo, MessageBoxIcon.Information) == DialogResult.Yes)
            {
                Profiles.ChangeRewardStatus(false);
                Profiles_SaveRewardsToProfile();
            }
        }

        private void Profile_AddNewSongList(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex > -1)
            {
                List<List<string>> SongLists = Profiles.DecryptedProfile["SongListsRoot"]["SongLists"].ToObject<List<List<string>>>();

                if (SongLists.Count >= 20)
                {
                    MessageBox.Show("We cannot complete your request!\nHaving more than 20 song lists is extremely unrealistic.\nPlease reach out to the RSMods dev team and we can change this restriction");
                    return;
                }

                SongLists.Add([]);

                Profiles.DecryptedProfile["SongListsRoot"]["SongLists"] = JToken.FromObject(SongLists);

                Profiles_GenerateNewSonglistsLists();

                // Reload the song list readout to show the new songlist.
                if (dgv_Profiles_Songlists.Visible)
                {
                    Profiles_LoadSongs(sender, e);
                }

                label_TotalSonglists.Text = SongLists.Count.ToString();

                Profiles_ENCRYPT();
                MessageBox.Show("Your new song list is present in game!");
            }
            else
            {
                MessageBox.Show("Make sure you have a profile selected!");
            }
        }

        private void Profile_RemoveNewestSongList(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex > -1)
            {
                List<List<string>> SongLists = Profiles.DecryptedProfile["SongListsRoot"]["SongLists"].ToObject<List<List<string>>>();

                if (SongLists.Count <= 6)
                {
                    MessageBox.Show("We cannot remove anymore songlists.");
                    return;
                }

                SongLists.Remove(SongLists[SongLists.Count - 1]);

                Profiles.DecryptedProfile["SongListsRoot"]["SongLists"] = JToken.FromObject(SongLists);

                Profiles_GenerateNewSonglistsLists();

                // Reload the song list readout to hide the removed songlist.
                if (dgv_Profiles_Songlists.Visible)
                {
                    Profiles_LoadSongs(sender, e);
                }

                label_TotalSonglists.Text = SongLists.Count.ToString();

                Profiles_ENCRYPT();
                MessageBox.Show("The newest songlist has been removed!");
            }
            else
            {
                MessageBox.Show("Make sure you have a profile selected!");
            }
        }

        private void Profiles_GenerateNewSonglistsLists()
        {
            if (Profiles.DecryptedProfile != null)
            {
                List<List<string>> SongLists = Profiles.DecryptedProfile["SongListsRoot"]["SongLists"].ToObject<List<List<string>>>();

                Profiles_Helper_GenerateValidSonglists(SongLists.Count);
            }
        }

        private void Profiles_Helper_GenerateValidSonglists(int TotalSonglists)
        {
            Dictionaries.SonglistCount = TotalSonglists;

            Dictionaries.RefreshSongLists();
            listBox_Songlist.Items.Clear();

            foreach (string SongList in Dictionaries.songlists)
            {
                listBox_Songlist.Items.Add(SongList);
            }
        }

        private void Profiles_ENCRYPT()
        {
            Profiles.EncryptProfile(Profiles.DecryptedProfile.ToString(Newtonsoft.Json.Formatting.None), Profiles_GetProfilePathFromName(currentUnpackedProfile));
        }

        private void Profiles_SaveRewardsToProfile()
        {
            Profiles_ENCRYPT();
            MessageBox.Show("Changes to Rewards have been saved!");
        }

        private void Profiles_SongToSonglist(int songlistNumber, bool add = true)
        {
            int rowIndex = dgv_Profiles_Songlists.SelectedCells[0].RowIndex;
            string commonName = $"{dgv_Profiles_Songlists[0, rowIndex].Value} - {dgv_Profiles_Songlists[1, rowIndex].Value}";
            string DLCKey = Songs.FirstOrDefault(song => song.CommonName == commonName).DLCKey;

            List<string> SongList = Profiles.DecryptedProfile["SongListsRoot"]["SongLists"][songlistNumber - 1].ToObject<List<string>>();

            if (add && !SongList.Contains(DLCKey))
                SongList.Add(DLCKey);
            else if (!add && SongList.Contains(DLCKey))
                SongList.Remove(DLCKey);

            Profiles.DecryptedProfile["SongListsRoot"]["SongLists"][songlistNumber - 1] = JToken.FromObject(SongList);
        }

        private void Profiles_SongToFavorites(bool add = true)
        {
            int rowIndex = dgv_Profiles_Songlists.SelectedCells[0].RowIndex;
            string commonName = $"{dgv_Profiles_Songlists[0, rowIndex].Value} - {dgv_Profiles_Songlists[1, rowIndex].Value}";
            string DLCKey = Songs.FirstOrDefault(song => song.CommonName == commonName).DLCKey;

            List<string> FavoritesList = Profiles.DecryptedProfile["FavoritesListRoot"]["FavoritesList"].ToObject<List<string>>();

            if (add && !FavoritesList.Contains(DLCKey))
                FavoritesList.Add(DLCKey);
            else if (!add && FavoritesList.Contains(DLCKey))
                FavoritesList.Remove(DLCKey);

            Profiles.DecryptedProfile["FavoritesListRoot"]["FavoritesList"] = JToken.FromObject(FavoritesList);
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

                // Favorites
                if (e.ColumnIndex == 8)
                {
                    Profiles_SongToFavorites(isChecked);
                }
                // Songlists
                else
                {
                    Profiles_SongToSonglist(e.ColumnIndex - 1, isChecked);
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

        private void Profiles_ImportToneManifest(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex == -1)
            {
                MessageBox.Show("Please select a profile!");
                return;
            }

            List<string> filenames = [];

            using (OpenFileDialog fileDialog = new())
            {
                fileDialog.Filter = "JSON|*.json";

                if (checkBox_ImportTonesBulk.Checked)
                {
                    fileDialog.Multiselect = true;
                }

                if (fileDialog.ShowDialog() == DialogResult.OK)
                {
                    filenames = [.. fileDialog.FileNames];
                }
            }

            if (filenames.Count == 0)
            {
                MessageBox.Show("No JSON manifests were imported.");
                return;
            }

            List<object> tonesToImport_Guitar = [];
            List<object> tonesToImport_Bass = [];

            foreach (string filename in filenames)
            {
                string manifestContents = File.ReadAllText(filename);

                JToken manifest = JToken.Parse(manifestContents);

                if (manifest["Entries"] == null)
                {
                    MessageBox.Show($"Input Tone Manifest doesn't have ENTRIES.\nFilename: {filename}");
                    continue;
                }

                JToken entries = manifest["Entries"];

                if (entries.First == null)
                {
                    MessageBox.Show($"Input Tone Manifest ENTRIES doesn't have children.\nFilename: {filename}");
                    continue;
                }

                string arrId = string.Empty;

                foreach (var pair in JObject.Parse(entries.ToString()))
                {
                    arrId = pair.Key;
                }

                if (string.IsNullOrEmpty(arrId))
                {
                    MessageBox.Show($"Input Tone Manifest has no ArrangementId.\nFilename: {filename}");
                    continue;
                }
                if (entries[arrId] == null)
                {
                    MessageBox.Show($"Input Tone Manifest has invalid ArrangementId.\nFilename: {filename}");
                    continue;
                }

                JToken arrangement = entries[arrId];

                if (arrangement["Attributes"] == null)
                {
                    MessageBox.Show($"Input Tone Manifest has no arrangement attributes.\nFilename: {filename}");
                    continue;
                }

                JToken attributes = arrangement["Attributes"];

                if (attributes["ArrangementName"] == null)
                {
                    MessageBox.Show($"Input Tone Manifest has no arrangement name.\nFilename: {filename}");
                    continue;
                }

                string arrangementName = attributes["ArrangementName"].ToString();

                if (attributes["Tones"] == null)
                {
                    MessageBox.Show($"Input Tone Manifest has no tones.\nFilename: {filename}");
                    continue;
                }

                JToken tones = attributes["Tones"];
                foreach (object tone in tones.ToObject<List<object>>())
                {
                    if (arrangementName.Contains("Bass"))
                    {
                        tonesToImport_Bass.Add(tone);
                    }
                    else
                    {
                        tonesToImport_Guitar.Add(tone);
                    }
                }
            }

            List<object> GuitarTones = Profiles.DecryptedProfile["CustomTones"].ToObject<List<object>>();
            List<object> BassTones = Profiles.DecryptedProfile["BassTones"].ToObject<List<object>>();

            GuitarTones.AddRange(tonesToImport_Guitar);
            BassTones.AddRange(tonesToImport_Bass);

            Profiles.DecryptedProfile["CustomTones"] = JToken.FromObject(GuitarTones);
            Profiles.DecryptedProfile["BassTones"] = JToken.FromObject(BassTones);

            Profiles_ENCRYPT();

            MessageBox.Show($"Added {tonesToImport_Guitar.Count + tonesToImport_Bass.Count} tone(s) to profile!");
        }

        private void Profiles_ImportTone2014(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex == -1)
            {
                MessageBox.Show("Please select a profile!");
                return;
            }

            List<string> filenames = [];

            using (OpenFileDialog fileDialog = new())
            {
                fileDialog.Filter = "XML|*.tone2014.xml";

                if (checkBox_ImportTonesBulk.Checked)
                {
                    fileDialog.Multiselect = true;
                }

                if (fileDialog.ShowDialog() == DialogResult.OK)
                {
                    filenames = [.. fileDialog.FileNames];
                }
            }

            if (filenames.Count == 0)
            {
                MessageBox.Show("No XML tones were imported.");
                return;
            }

            List<Tone2014> tonesToImport_Guitar = [];
            List<Tone2014> tonesToImport_Bass = [];

            MessageBoxManager.OK = "Guitar";
            MessageBoxManager.Cancel = "Bass";
            MessageBoxManager.Register();

            foreach (string filename in filenames)
            {
                Tone2014 tone = Tone2014.LoadFromXmlTemplateFile(filename);

                DialogResult arrangementResult = MessageBox.Show($"Do you want to save {tone.Name} as a guitar tone, or a bass tone?", "Question", MessageBoxButtons.OKCancel, MessageBoxIcon.Question);

                if (arrangementResult == DialogResult.Cancel)
                {
                    tonesToImport_Bass.Add(tone);
                }
                else
                {
                    tonesToImport_Guitar.Add(tone);
                }
            }

            MessageBoxManager.Unregister();
            MessageBoxManager.OK = "OK";
            MessageBoxManager.Cancel = "Cancel";

            List<object> GuitarTones = Profiles.DecryptedProfile["CustomTones"].ToObject<List<object>>();
            List<object> BassTones = Profiles.DecryptedProfile["BassTones"].ToObject<List<object>>();

            GuitarTones.AddRange(tonesToImport_Guitar);
            BassTones.AddRange(tonesToImport_Bass);


            Profiles.DecryptedProfile["CustomTones"] = JToken.FromObject(GuitarTones);
            Profiles.DecryptedProfile["BassTones"] = JToken.FromObject(BassTones);

            Profiles_ENCRYPT();

            MessageBox.Show($"Added {tonesToImport_Guitar.Count + tonesToImport_Bass.Count} tone(s) to your profile!");
        }

        #endregion
        #region Sound Packs

        private const string soundPackLocationPrefix = "audio_psarc\\audio_psarc_RS2014_Pc\\audio\\windows\\";
        private const string soundPackEnglishPrefix = "english(us)\\";
        private const string voiceLine_BadPerformance = "2066953778.wem";
        private const string voiceLine_DisappointingPerformance = "2067218742.wem";
        private const string voiceLine_SubparPerformance = "2066826048.wem";
        private const string voiceLine_CouldBeBetter = "2068001585.wem";
        private const string voiceLine_DecentPerformance = "2068133176.wem";
        private const string voiceLine_AlrightPerformance = "2068002869.wem";
        private const string voiceLine_ExcellentPerformance = "2067285052.wem";
        private const string voiceLine_TopNotchPerformance = "2067281979.wem";
        private const string voiceLine_SuperbPerformance = "2067350856.wem";
        private const string voiceLine_DazzlingPerformance = "2068132687.wem";
        private const string voiceLine_YoureGonnaBeASuperstar = "2068199486.wem";
        private const string voiceLine_WonderfulPerformance = "2067154245.wem";
        private const string voiceLine_ExceptionalPerformance = "2067153482.wem";
        private const string voiceLine_AmazingPerformance = "2067871807.wem";
        private const string voiceLine_ExemplaryPerformance = "2067022644.wem";
        private const string voiceLine_MasterfulPerformance_98 = "2068137287.wem";
        private const string voiceLine_MasterfulPerformance_99 = "2067870540.wem";
        private const string voiceLine_FlawlessPerformance = "2068002100.wem";

        private void SoundPacks_UnpackAudioPsarc(object sender, EventArgs e)
        {
            if (MessageBox.Show("For us to do song packs we need to unpack a huge game file. This will take up about 1.3 gigabytes.\nPress OK if you are fine with that, or Cancel if you are not.", "Please Read!!!", MessageBoxButtons.OKCancel, MessageBoxIcon.Information) != DialogResult.OK)
                return;

            SoundPacks_PleaseWaitMessage(true);

            string audioPsarcLocation = Path.Combine(GenUtil.GetRSDirectory(), "audio.psarc");

            GlobalExtension.UpdateProgress = progressBar_RepackAudioPsarc;
            GlobalExtension.CurrentOperationLabel = label_AudioPsarcPleaseWait;

            Packer.Unpack(audioPsarcLocation, Path.Combine(GenUtil.GetRSDirectory(), "RSMods/", "audio_psarc"));

            SoundPacks_PleaseWaitMessage(false);
            SoundPacks_ChangeUIForUnpackedFolder(true);
            MessageBox.Show("You may now mess around with custom sound packs");
        }

        private void SoundPacks_DownloadWwise(object sender, EventArgs e)
        {
            Process.Start("https://ignition4.customsforge.com/cfsm/wwise/");
            MessageBox.Show("After you download and install Wwise, make sure to open it at least once to ensure the EULA is agreed to!", "Wwise EULA", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private bool SoundPacks_PleaseWaitMessage(bool show)
        {
            label_AudioPsarcPleaseWait.Visible = show;
            progressBar_RepackAudioPsarc.Visible = show;

            if (show)
                progressBar_RepackAudioPsarc.Value = 0;

            return show;
        }

        private void SoundPacks_RepackAudioPsarc(object sender, EventArgs e)
        {
            if (!Directory.Exists(Path.Combine(GenUtil.GetRSDirectory(), "RSMods/", "audio_psarc\\audio_psarc_RS2014_Pc")))
            {
                MessageBox.Show("We detect no audio.psarc is decompiled. Give us some time to try to fix that.");
                SoundPacks_UnpackAudioPsarc(sender, e);
            }

            MessageBox.Show("This will take a couple minutes!\nGo do something while this is working it's magic.\nIf RSMods looks like it crashed, it didn't, do NOT attempt to close it or you may need to verify your game files");
            SoundPacks_PleaseWaitMessage(true);
            GlobalExtension.CurrentOperationLabel = label_AudioPsarcPleaseWait;
            GlobalExtension.UpdateProgress = progressBar_RepackAudioPsarc;
            GlobalExtension.UpdateProgress.Maximum = 110;
            Packer.Pack(Path.Combine(Application.StartupPath, "audio_psarc\\audio_psarc_RS2014_Pc"), Path.Combine(GenUtil.GetRSDirectory(), "audio.psarc"));
            GlobalExtension.UpdateProgress.Value = 0;
            GlobalExtension.UpdateProgress.Maximum = 100;
            SoundPacks_PleaseWaitMessage(false);
            MessageBox.Show("Open your game, and see if the sound works!");
            GC.Collect(); // We use a lot of memory here, so let's take out the garbage.
        }

        private void SoundPacks_RemoveUnpackedAudioPsarc(object sender, EventArgs e)
        {
            Directory.Delete("audio_psarc", true);
            SoundPacks_ChangeUIForUnpackedFolder(false);
        }

        private void SoundPacks_ChangeUIForUnpackedFolder(bool isUnpacked)
        {
            button_UnpackAudioPsarc.Visible = !isUnpacked;
            groupBox_SoundPacks.Visible = isUnpacked;
        }

        private void SoundPacks_ReplaceSound(string soundToReplace)
        {

            if (!Directory.Exists(Path.Combine(Application.StartupPath, "audio_psarc")))
            {
                MessageBox.Show("Audio PSARC not unpacked");
                SoundPacks_ChangeUIForUnpackedFolder(false);
                return;
            }

            using OpenFileDialog fileDialog = new();
            fileDialog.Filter = "Mp3 Files|*.mp3|Ogg Files|*.ogg|Wav Files|*.wav|Wem Files|*.wem";
            fileDialog.RestoreDirectory = true;

            if (fileDialog.ShowDialog() == DialogResult.OK)
            {
                if (Path.GetExtension(fileDialog.FileName) == ".mp3")
                    fileDialog.FileName = SoundPacks_ConvertMP3ToWav(fileDialog.FileName);
                if (Path.GetExtension(fileDialog.FileName) == ".ogg")
                    fileDialog.FileName = SoundPacks_ConvertOGGToWem(fileDialog.FileName);
                if (Path.GetExtension(fileDialog.FileName) == ".wav")
                    fileDialog.FileName = SoundPacks_ConvertWAVToWem(fileDialog.FileName);

                if (fileDialog.FileName != "null")
                {
                    File.Delete(Path.Combine(Application.StartupPath, soundToReplace));
                    GC.Collect(); // Need to take out the garbage or it'll crash
                    File.Move(fileDialog.FileName, Path.Combine(Application.StartupPath, soundToReplace));
                    MessageBox.Show("Don't forget to hit \"Repack Audio Psarc\" when you're done.");
                }
                else
                {
                    MessageBox.Show("An error occured when converting your file.\nPlease contact the RSMods devs.");
                }
            }
        }

        private string SoundPacks_ConvertMP3ToWav(string mp3File)
        {
            string wavFile = Path.Combine(Path.GetDirectoryName(mp3File), Path.GetFileNameWithoutExtension(mp3File) + ".wav");

            using (Mp3FileReader mp3FileReader = new(mp3File))
            {
                WaveFileWriter.CreateWaveFile(wavFile, mp3FileReader);
            }

            return wavFile;
        }

        private void SoundPacks_Beta(object sender, EventArgs e) => Process.Start("https://github.com/Lovrom8/RSMods/issues/new");

        private string SoundPacks_ConvertWAVToWem(string wavFile)
        {
            string wemFile = "null";
            try
            {
                string previewWav = Path.Combine(Path.GetDirectoryName(wavFile), Path.GetFileNameWithoutExtension(wavFile) + "_preview.wav"); // Previw needs to be made or Wav2Wem crashes.

                if (File.Exists(previewWav))
                    File.Delete(previewWav);

                File.Copy(wavFile, previewWav);
                Wwise.Wav2Wem(wavFile, Path.Combine(Application.StartupPath, Path.GetFileNameWithoutExtension(wavFile) + ".wem"), 4);
                wemFile = Path.Combine(Application.StartupPath, Path.GetFileNameWithoutExtension(wavFile) + ".wem");
                File.Delete(previewWav);
                File.Delete(Path.Combine(Path.GetDirectoryName(wemFile), Path.GetFileNameWithoutExtension(wemFile) + "_preview.wem"));
            }
            catch (DirectoryNotFoundException ex)
            {
                MessageBox.Show($"Your WWISEROOT environment variable can't be found.\nPlease change it to a folder that exists.\nThe error is: {ex.Message}.\nPlease reboot your computer after you fix this.");
            }

            return wemFile;
        }

        private string SoundPacks_ConvertOGGToWem(string oggFile)
        {
            SoundPacks_PleaseWaitMessage(true);
            string wemFile = "null";
            try
            {
                string oggFileName = Path.GetFileNameWithoutExtension(oggFile);
                string oggFullPath = Path.Combine(Path.GetDirectoryName(oggFile), oggFileName);
                string directory = Path.GetDirectoryName(oggFile);
                string oggPreviewName = oggFullPath + "_preview";

                wemFile = OggFile.Convert2Wem(oggFile);

                File.Delete(oggFullPath + ".wav");
                File.Delete(oggPreviewName + ".ogg");
                File.Delete(oggPreviewName + ".wav");
                File.Delete(oggPreviewName + ".wem");

                if (File.Exists(Path.Combine(Application.StartupPath, Path.GetFileNameWithoutExtension(wemFile) + ".wem")))
                    File.Delete(Path.Combine(Application.StartupPath, Path.GetFileNameWithoutExtension(wemFile) + ".wem"));

                GC.Collect(); // Gotta clean up the garbage or it'll try to crash.

                File.Move(wemFile, Path.Combine(Application.StartupPath, Path.GetFileNameWithoutExtension(wemFile) + ".wem"));
                wemFile = Path.Combine(Application.StartupPath, Path.GetFileNameWithoutExtension(wemFile) + ".wem");
            }
            catch (DirectoryNotFoundException ex)
            {
                MessageBox.Show($"Your WWISEROOT environment variable can't be found.\nPlease change it to a folder that exists.\nThe error is: {ex.Message}.\nPlease reboot your computer after you fix this.");
            }

            SoundPacks_PleaseWaitMessage(false);
            return wemFile;
        }

        private void SoundPacks_Import_Dialog(object sender, EventArgs e)
        {
            using OpenFileDialog fileDialog = new()
            {
                RestoreDirectory = true,
                Filter = "RS2014 Soundpack|*.rs_soundpack"
            };

            if (fileDialog.ShowDialog() == DialogResult.OK)
                SoundPacks_Import_File(fileDialog.FileName);
        }

        private void SoundPacks_Import_File(string fileName)
        {
            SevenZipExtractor.SetLibraryPath("7z64.dll");
            using SevenZipExtractor extractor = new(fileName);
            extractor.ExtractArchive(soundPackLocationPrefix);
            MessageBox.Show("Don't forget to hit \"Repack Audio Psarc\" when you're done.");
        }

        private void SoundPacks_Export_Dialog(object sender, EventArgs e)
        {
            SaveFileDialog fileDialog = new()
            {
                Filter = "RS2014 Soundpack|*.rs_soundpack",
                InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.Desktop)
            };

            if (fileDialog.ShowDialog() == DialogResult.OK)
                SoundPacks_Export_File(fileDialog.FileName);
        }

        private void SoundPacks_Export_File(string fileName)
        {
            SevenZipCompressor.SetLibraryPath("7z64.dll");

            SevenZipCompressor compressor = new()
            {
                CompressionMethod = CompressionMethod.Deflate,
                CompressionLevel = SevenZip.CompressionLevel.Normal,
                CompressionMode = SevenZip.CompressionMode.Create,
                DirectoryStructure = true,
                PreserveDirectoryRoot = false,
                ArchiveFormat = OutArchiveFormat.Zip
            };

            Dictionary<string, string> exportedFiles = new()
                {
                    { soundPackEnglishPrefix + voiceLine_BadPerformance, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_BadPerformance },
                    { soundPackEnglishPrefix + voiceLine_DisappointingPerformance, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_DisappointingPerformance },
                    { soundPackEnglishPrefix + voiceLine_SubparPerformance, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_SubparPerformance },
                    { soundPackEnglishPrefix + voiceLine_CouldBeBetter, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_CouldBeBetter },
                    { soundPackEnglishPrefix + voiceLine_DecentPerformance, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_DecentPerformance },
                    { soundPackEnglishPrefix + voiceLine_AlrightPerformance, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_AlrightPerformance },
                    { soundPackEnglishPrefix + voiceLine_ExcellentPerformance, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_ExcellentPerformance },
                    { soundPackEnglishPrefix + voiceLine_TopNotchPerformance, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_TopNotchPerformance },
                    { soundPackEnglishPrefix + voiceLine_SuperbPerformance, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_SuperbPerformance },
                    { soundPackEnglishPrefix + voiceLine_DazzlingPerformance, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_DazzlingPerformance },
                    { soundPackEnglishPrefix + voiceLine_YoureGonnaBeASuperstar, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_YoureGonnaBeASuperstar },
                    { soundPackEnglishPrefix + voiceLine_WonderfulPerformance, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_WonderfulPerformance },
                    { soundPackEnglishPrefix + voiceLine_ExceptionalPerformance, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_ExceptionalPerformance },
                    { soundPackEnglishPrefix + voiceLine_AmazingPerformance, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_AmazingPerformance },
                    { soundPackEnglishPrefix + voiceLine_ExemplaryPerformance, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_ExemplaryPerformance },
                    { soundPackEnglishPrefix + voiceLine_MasterfulPerformance_98, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_MasterfulPerformance_98 },
                    { soundPackEnglishPrefix + voiceLine_MasterfulPerformance_99, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_MasterfulPerformance_99 },
                    { soundPackEnglishPrefix + voiceLine_FlawlessPerformance, soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_FlawlessPerformance },
                };

            compressor.CompressFileDictionary(exportedFiles, fileName);
            MessageBox.Show("You can now share your sound pack with your friends!\nTell them to open this menu and click \"Import Soundpack\" button, and point to this file.");
        }

        private void SoundPacks_Reset(object sender, EventArgs e)
        {
            GenUtil.ExtractEmbeddedResource(Application.StartupPath, Assembly.GetExecutingAssembly(), "RSMods.Resources", ["original.rs_soundpack"]);
            SoundPacks_Import_File("original.rs_soundpack");
            File.Delete("original.rs_soundpack");
        }

        private void SoundPacks_ReplaceBadPerformance(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_BadPerformance);
        private void SoundPacks_ReplaceDisappointingPerformance(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_DisappointingPerformance);
        private void SoundPacks_ReplaceSubparPerformance(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_SubparPerformance);
        private void SoundPacks_ReplaceCouldBeBetter(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_CouldBeBetter);
        private void SoundPacks_ReplaceDecentPerformance(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_DecentPerformance);
        private void SoundPacks_ReplaceAlrightPerformance(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_AlrightPerformance);
        private void SoundPacks_ReplaceExcellentPerformance(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_ExcellentPerformance);
        private void SoundPacks_ReplaceTopNotchPerformance(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_TopNotchPerformance);
        private void SoundPacks_ReplaceSuperbPerformance(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_SuperbPerformance);
        private void SoundPacks_ReplaceDazzlingPerformance(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_DazzlingPerformance);
        private void SoundPacks_ReplaceSuperstar(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_YoureGonnaBeASuperstar);
        private void SoundPacks_ReplaceWonderfulPerformance(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_WonderfulPerformance);
        private void SoundPacks_ReplaceExceptionalPerformance(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_ExceptionalPerformance);
        private void SoundPacks_ReplaceAmazingPerformance(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_AmazingPerformance);
        private void SoundPacks_ReplaceExemplaryPerformance(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_ExemplaryPerformance);
        private void SoundPacks_ReplaceMasterfulPerformance_98(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_MasterfulPerformance_98);
        private void SoundPacks_ReplaceMasterfulPerformance_99(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_MasterfulPerformance_99);
        private void SoundPacks_ReplaceFlawlessPerformance(object sender, EventArgs e) => SoundPacks_ReplaceSound(soundPackLocationPrefix + soundPackEnglishPrefix + voiceLine_FlawlessPerformance);


        private void SoundPacks_LoadResultVoiceOverList()
        {
            listBox_Result_VOs.Items.Clear();
            Dictionaries.ResultVoiceOverDictionary.Keys.ToList().ForEach(key => listBox_Result_VOs.Items.Add(key));
        }

        private void SoundPacks_PlayResultVoiceOver(object sender, EventArgs e)
        {
            if (listBox_Result_VOs.SelectedIndex == -1)
                return;

            WinMsgUtil.SendMsgToRS($"WwiseEvent {Dictionaries.ResultVoiceOverDictionary[listBox_Result_VOs.SelectedItem.ToString()]}");
        }
        #endregion
        #region Midi

        private void Midi_LoadDevices()
        {
            this.listBox_ListMidiOutDevices.Items.Clear();
            this.listBox_ListMidiInDevices.Items.Clear();

            uint numberOfMidiOutDevices = Midi.midiOutGetNumDevs();
            uint numberOfMidiInDevices = Midi.midiInGetNumDevs();

            for (uint deviceNumber = 0; deviceNumber < numberOfMidiOutDevices; deviceNumber++)
            {
                Midi.MIDIOUTCAPS temp = new();
                Midi.midiOutGetDevCaps(deviceNumber, ref temp, (uint)Marshal.SizeOf(typeof(Midi.MIDIOUTCAPS)));
                this.listBox_ListMidiOutDevices.Items.Add(temp.szPname);
            }

            for (uint deviceNumber = 0; deviceNumber < numberOfMidiInDevices; deviceNumber++)
            {
                Midi.MIDIINCAPS temp = new();
                Midi.midiInGetDevCaps(deviceNumber, ref temp, (uint)Marshal.SizeOf(typeof(Midi.MIDIINCAPS)));
                this.listBox_ListMidiInDevices.Items.Add(temp.szPname);
            }

            if (Toggles.AutoTuneForSongDevice != "")
                listBox_ListMidiOutDevices.SelectedItem = Toggles.AutoTuneForSongDevice;

            if (Toggles.MidiInDevice != "")
                listBox_ListMidiInDevices.SelectedItem = Toggles.MidiInDevice;
        }

        private void MidiInProc(int hMidiIn, Midi.Responses wMsg, uint dwInstance, uint midiMessage, uint timeStamp)
        {
            switch (wMsg)
            {
                case Midi.Responses.MIM_OPEN:
                    Debug.WriteLine("wMsg=MIM_OPEN");
                    break;
                case Midi.Responses.MIM_CLOSE:
                    Debug.WriteLine("wMsg=MIM_CLOSE");
                    break;
                case Midi.Responses.MIM_DATA:
                    byte[] Data = BitConverter.GetBytes(midiMessage);
                    byte[] Timestamp = BitConverter.GetBytes(timeStamp);

                    // This should always be the case, but we should check just in case.
                    // 0xFE - Keep-Alive signal. We don't need to log that.
                    if (Data.Length == 4)
                    {
                        Midi.Status Status = (Midi.Status)Data[0];
                        byte Channel = (byte)(Data[0] % 16);

                        // Note Off
                        if (Status >= Midi.Status.NoteOff && Status < Midi.Status.NoteOn)
                        {
                            Debug.WriteLine($"Note Off received on channel {Channel}. Key = {Data[1]}. Velocity = {Data[2]}");
                        }

                        // Note On
                        else if (Status >= Midi.Status.NoteOn && Status < Midi.Status.AfterTouch)
                        {
                            Debug.WriteLine($"Note On received on channel {Channel}. Key = {Data[1]}. Velocity = {Data[2]}");
                        }

                        // Aftertouch
                        else if (Status >= Midi.Status.AfterTouch && Status < Midi.Status.CC)
                        {
                            Debug.WriteLine($"Aftertouch received on channel {Channel}. Key = {Data[1]}. Touch = {Data[2]}");
                        }

                        // CC
                        else if (Status >= Midi.Status.CC && Status < Midi.Status.PC)
                        {
                            Debug.WriteLine($"CC received on channel {Channel}. Bank = {Data[1]}. Value = {Data[2]}");
                        }

                        // PC
                        else if (Status >= Midi.Status.PC && Status < Midi.Status.Pressure)
                        {
                            Debug.WriteLine($"PC received on channel {Channel}. Program = {Data[1]}");
                        }

                        // Pressure
                        else if (Status >= Midi.Status.Pressure && Status < Midi.Status.PitchBend)
                        {
                            Debug.WriteLine($"Pressure received on channel {Channel}.");
                        }

                        // Pitch Bend
                        else if (Status >= Midi.Status.PitchBend && Status < Midi.Status.SystemEx)
                        {
                            Debug.WriteLine($"Pitch Bend received on channel {Channel}. LSB = {Data[1]}. MSB = {Data[2]}");
                        }

                        // SystemEx
                        else if (Status >= Midi.Status.SystemEx)
                        {
                            // Keep-Alive status. Don't log this as it will spam the console.
                            if (Status == (Midi.Status)0xFE)
                            {
                                return;
                            }

                            Debug.WriteLine($"SystemEX received on channel {Channel}. Data1 = {Data[1]}. Data2 = {Data[2]}");
                        }

                        // Unknown status
                        else
                        {
                            Debug.WriteLine($"Unknown MIDI status received on channel {Channel}! Status = {Data[0]}. Data1 = {Data[1]}. Data2 = {Data[2]}");
                        }
                    }

                    break;
                case Midi.Responses.MIM_LONGDATA:
                    Debug.WriteLine("wMsg=MIM_LONGDATA");
                    break;
                case Midi.Responses.MIM_ERROR:
                    Debug.WriteLine("wMsg=MIM_ERROR");
                    break;
                case Midi.Responses.MIM_LONGERROR:
                    Debug.WriteLine("wMsg=MIM_LONGERROR");
                    break;
                case Midi.Responses.MIM_MOREDATA:
                    Debug.WriteLine("wMsg=MIM_MOREDATA");
                    break;
                default:
                    Debug.WriteLine("wMsg = unknown");
                    break;
            }
        }

        private void CheckBox_EnabledMidiIn_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox_EnabledMidiIn.Checked)
            {
                if (listBox_ListMidiInDevices.SelectedIndex == -1)
                    return;

                uint numberOfMidiInDevices = Midi.midiInGetNumDevs();

                Debug.WriteLine("Looking for device");

                for (uint deviceNumber = 0; deviceNumber < numberOfMidiInDevices; deviceNumber++)
                {
                    Midi.MIDIINCAPS temp = new();
                    Midi.midiInGetDevCaps(deviceNumber, ref temp, (uint)Marshal.SizeOf(typeof(Midi.MIDIINCAPS)));

                    if (temp.szPname == listBox_ListMidiInDevices.SelectedItem.ToString())
                    {
                        Debug.WriteLine($"Found device: {temp.szPname}");
                        Midi.SelectedMidiInDeviceId = deviceNumber;
                        break;
                    }
                }

                if (Midi.SelectedMidiInDeviceId == 2014)
                    return;

                Midi.MidiInProcessing = MidiInProc;
                Debug.WriteLine(Midi.midiInOpen(ref Midi.MidiInHandle, Midi.SelectedMidiInDeviceId, Midi.MidiInProcessing, 0, 0x30000));

                Debug.WriteLine(Midi.midiInStart(Midi.MidiInHandle));

                Debug.WriteLine("Set up Midi In");
            }
            else
            {
                Midi.midiInStop(Midi.MidiInHandle);
                Midi.midiInClose(Midi.MidiInHandle);

                Midi.MidiInHandle = (IntPtr)0;

                Debug.WriteLine("Shutdown Midi In");
            }
        }

        private void TrackBar_FontSize_Scroll(object sender, EventArgs e) => lblCurrentFontSize.Text = $"Current font size: {trackBar_FontSize.Value}";

        private void BtnSaveFontSize_Click(object sender, EventArgs e)
        {
            Toggles.OnScreenFontSize = trackBar_FontSize.Value;
        }
    }

    public class Midi
    {
        public enum Status : byte
        {
            NoteOff = 0x80,
            NoteOn = 0x90,
            AfterTouch = 0xA0,
            CC = 0xB0,
            PC = 0xC0,
            Pressure = 0xD0,
            PitchBend = 0xE0,
            SystemEx = 0xF0
        }


        public static IntPtr MidiInHandle = (IntPtr)0;
        public static uint SelectedMidiInDeviceId = 2014;
        public static MidiInProc MidiInProcessing = null;

        public enum Responses : uint
        {
            MIM_OPEN = 0x3C1,
            MIM_CLOSE = 0x3C2,
            MIM_DATA = 0x3C3,
            MIM_LONGDATA = 0x3C4,
            MIM_ERROR = 0x3C5,
            MIM_LONGERROR = 0x3C6,
            MIM_MOREDATA = 0x3CC
        }
        public enum MMRESULT : uint
        {
            MMSYSERR_NOERROR,
            MMSYSERR_ERROR,
            MMSYSERR_BADDEVICEID,
            MMSYSERR_NOTENABLED,
            MMSYSERR_ALLOCATED,
            MMSYSERR_INVALHANDLE,
            MMSYSERR_NODRIVER,
            MMSYSERR_NOMEM,
            MMSYSERR_NOTSUPPORTED,
            MMSYSERR_BADERRNUM,
            MMSYSERR_INVALFLAG,
            MMSYSERR_INVALPARAM,
            MMSYSERR_HANDLEBUSY,
            MMSYSERR_INVALIDALIAS,
            MMSYSERR_BADDB,
            MMSYSERR_KEYNOTFOUND,
            MMSYSERR_READERROR,
            MMSYSERR_WRITEERROR,
            MMSYSERR_DELETEERROR,
            MMSYSERR_VALNOTFOUND,
            MMSYSERR_NODRIVERCB,
            WAVERR_BADFORMAT = 32,
            WAVERR_STILLPLAYING = 33,
            WAVERR_UNPREPARED = 34
        }

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void MidiInProc(int hMidiIn, Midi.Responses wMsg, uint dwInstance, uint dwParam1, uint dwParam2);

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

        [StructLayout(LayoutKind.Sequential)]
        public struct MIDIINCAPS
        {
            public ushort wMid;
            public ushort wPid;
            public uint vDriverVersion;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string szPname;
            public uint dwSupport;
        }

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern MMRESULT midiOutGetDevCaps(uint uDeviceID, ref MIDIOUTCAPS lpMidiOutCaps, uint cbMidiOutCaps);

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern uint midiOutGetNumDevs();

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern MMRESULT midiInGetDevCaps(uint uDeviceID, ref MIDIINCAPS pmic, uint cbmic);

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern uint midiInGetNumDevs();

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern uint midiInOpen(ref IntPtr hmi, uint uDeviceID, [MarshalAs(UnmanagedType.FunctionPtr)] MidiInProc dwCallback, uint dwInstance, uint fdwOpen);

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern uint midiInStart(IntPtr hmi);

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern uint midiInStop(IntPtr hmi);

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern uint midiInClose(IntPtr hmi);
    }
    #endregion
}