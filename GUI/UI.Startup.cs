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
using System.Threading.Tasks;
using System.Diagnostics;
using RocksmithToolkitLib.DLCPackage;
using NAudio.CoreAudioApi;
using Rocksmith2014PsarcLib.Psarc.Models.Json;
using ArrangementTuning = Rocksmith2014PsarcLib.Psarc.Models.Json.SongArrangement.ArrangementAttributes.ArrangementTuning;
using static RSMods.RsModsSettings;
using RSMods.ASIO;
using RSMods.Rocksmith;
using System.Runtime;

namespace RSMods
{
    public partial class MainForm : Form
    {
        private void Startup_ReadIniOrCreateDefault()
        {
            RsModsSettings.LoadSettingsFromINI();
        }

        private void Startup_FixLegacySonglistBug()
        {
            if (string.IsNullOrEmpty(GetSongListTitle(1)))
            {
                SetSongListTitle(1, "Define Song List 1 Here");
            }
        }

        /// <summary>
        /// Surfaces the "Set Save Path" button when the save folder is unresolved (declined or invalid).
        /// The detection / prompting itself lives in <see cref="RSLocationResolver"/>; this only reflects the
        /// resolved state onto the form.
        /// </summary>
        private void Startup_ApplySaveFolderUI()
        {
            if (Constants.SavePathDeclined || !Constants.SavePath.IsSavePath())
                button_SetSavePath.Visible = true;
        }

        private void Startup_LoadSonglists()
        {
            foreach (string songlist in RsModsSettings.RefreshSongListTitles(Profiles.SongListCount))
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
            if (!_asioSettings.SettingsExist)
                TabController.TabPages.Remove(tab_RSASIO);
            else
                Startup_LoadASIODevices();
        }

        private void Startup_LoadRocksmithProfiles()
        {
            try
            {
                var profiles = Profiles.AvailableProfiles();

                if (profiles == null || profiles.Count == 0)
                {
                    Startup_LoadSonglists();
                    return;
                }

                string[] keys = profiles.Keys.ToArray();
                listBox_AutoLoadProfiles.Items.AddRange(keys);
                listBox_Profiles_AvailableProfiles.Items.AddRange(keys);

                label_TotalSonglists.Text = Math.Max(6, keys.Max(Profiles.GetSongListCount)).ToString();

                Profiles_Helper_GenerateValidSonglists();
            }
            catch
            {
                Startup_LoadSonglists();
            }
        }

        private void Startup_DeleteOldBackups(int maxAmountOfBackups) => Profiles.DeleteOldBackups(maxAmountOfBackups);

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
            ProfileEditsTabIndex = TabController.TabPages.IndexOf(tab_Profiles);

            if (!GUISettings.BackupProfile || !shipProfileEdits || !Constants.SavePath.IsSavePath())
                TabController.TabPages.Remove(tab_Profiles);
        }

        private void Startup_ListAllBackups()
        {
            try
            {
                var backups = Profiles.GetFormattedBackupNames().ToArray();

                listBox_Profiles_ListBackups.BeginUpdate();
                listBox_Profiles_ListBackups.Items.Clear();
                listBox_Profiles_ListBackups.Items.AddRange(backups);
                listBox_Profiles_ListBackups.EndUpdate();
            }
            catch { /* Folder doesn't exist / access error  */ }
        }

        private void Startup_CheckStatusAudioPsarc() => SoundPacks_ChangeUIForUnpackedFolder(Directory.Exists("audio_psarc"));

        private void Startup_UnpackCachePsarc() => SetAndForgetMods.UnpackCachePsarc();
    }
}
