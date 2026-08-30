using System;
using System.Windows.Forms;
using System.IO;
using System.Linq;
using System.Drawing;
using System.Drawing.Text;
using RSMods.Core;
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
            WinMsgUtil.GetWindowRect(ptr, ref guiLocation);

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
                using var _ = File.Create(Path.Combine(GenUtil.GetRSDirectory(), "RSMods_debug.txt"));
            }
            else
            {
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

        private void Save_TurnOffAllMods(object sender, EventArgs e) => ExeUtil.ToggleMods(GenUtil.GetRSDirectory());

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

        private async void Save_SetSavePath(object sender, EventArgs e)
        {
            string saveFolder = await RSLocationResolver.ResolveSaveFolderAsync(AppServices.Dialogs, forcePrompt: true);

            if (saveFolder.Length == 0)
            {
                await AppServices.Dialogs.ShowInfoAsync("We did not save a Save Folder");
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
    }
}
