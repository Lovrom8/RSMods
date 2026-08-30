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

        private void UpdateProgressBarSafe(int value)
        {
            if (progressBar_RepackAudioPsarc.InvokeRequired)
            {
                progressBar_RepackAudioPsarc.Invoke(new Action(() => UpdateProgressBarSafe(value)));
                return;
            }

            if (value >= progressBar_RepackAudioPsarc.Minimum && value <= progressBar_RepackAudioPsarc.Maximum)
                progressBar_RepackAudioPsarc.Value = value;
        }

        private void UpdateLabelSafe(string text)
        {
            if (label_AudioPsarcPleaseWait.InvokeRequired)
            {
                label_AudioPsarcPleaseWait.Invoke(new Action(() => UpdateLabelSafe(text)));
                return;
            }

            label_AudioPsarcPleaseWait.Text = text;
        }

        private async Task<bool> SoundPacks_ExecuteAudioPsarcTaskAsync(Action backgroundTask, string errorPrefix)
        {
            GlobalProgress.ProgressValueChanged += UpdateProgressBarSafe;
            GlobalProgress.CurrentOperationChanged += UpdateLabelSafe;

            SoundPacks_PleaseWaitMessage(true);

            try
            {
                await Task.Run(backgroundTask);
                return true;
            }
            catch (Exception ex)
            {
                MessageBox.Show($"{errorPrefix}: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return false;
            }
            finally
            {
                GlobalProgress.ProgressValueChanged -= UpdateProgressBarSafe;
                GlobalProgress.CurrentOperationChanged -= UpdateLabelSafe;

                SoundPacks_PleaseWaitMessage(false);

                GCSettings.LargeObjectHeapCompactionMode = GCLargeObjectHeapCompactionMode.CompactOnce;
                GC.Collect(2, GCCollectionMode.Forced, blocking: true, compacting: true);
            }
        }

        private async Task<bool> SoundPacks_PerformUnpackAsync()
        {
            if (MessageBox.Show("For us to do song packs we need to unpack a huge game file. This will take up about 1.3 gigabytes on your hard drive.\n\nPress OK if you are fine with that, or Cancel if you are not.", "Please Read!!!", MessageBoxButtons.OKCancel, MessageBoxIcon.Warning) != DialogResult.OK)
                return false;

            string audioPsarcLocation = Path.Combine(GenUtil.GetRSDirectory(), "audio.psarc");
            string unpackDestination = Path.Combine(GenUtil.GetRsModsPath(), "audio_psarc");

            bool success = await SoundPacks_ExecuteAudioPsarcTaskAsync(
                () => Packer.Unpack(audioPsarcLocation, unpackDestination),
                "An error occurred while unpacking"
            );

            if (success)
            {
                SoundPacks_ChangeUIForUnpackedFolder(true);
                MessageBox.Show("Success! You may now mess around with custom sound packs.", "Unpacked Successfully", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }

            return success;
        }

        private async Task SoundPacks_PerformRepackAsync()
        {
            string unpackPath = Path.Combine(GenUtil.GetRsModsPath(), "audio_psarc", "audio_psarc_RS2014_Pc");
            string packPath = Path.Combine(GenUtil.GetRSDirectory(), "audio.psarc");

            if (!Directory.Exists(unpackPath))
            {
                MessageBox.Show("We detect no audio.psarc is decompiled. Give us some time to try to fix that.", "Missing Files", MessageBoxButtons.OK, MessageBoxIcon.Information);

                bool unpackSuccess = await SoundPacks_PerformUnpackAsync();

                if (!unpackSuccess || !Directory.Exists(unpackPath)) return;
            }

            MessageBox.Show("This will take a couple of minutes depending on your hard drive speed.\nFeel free to minimize the app while it works its magic.", "Packing Audio", MessageBoxButtons.OK, MessageBoxIcon.Information);

            bool success = await SoundPacks_ExecuteAudioPsarcTaskAsync(
                () => Packer.Pack(unpackPath, packPath),
                "An error occurred while packing"
            );

            if (success)
            {
                MessageBox.Show("Open your game, and see if the sound works!", "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }

        private async void SoundPacks_UnpackAudioPsarc(object sender, EventArgs e)
        {
            await SoundPacks_PerformUnpackAsync();
        }
        private async void SoundPacks_RepackAudioPsarc(object sender, EventArgs e)
        {
            await SoundPacks_PerformRepackAsync();
        }

        private void SoundPacks_RemoveUnpackedAudioPsarc(object sender, EventArgs e)
        {
            string unpackRoot = Path.Combine(GenUtil.GetRsModsPath(), "audio_psarc");

            if (Directory.Exists(unpackRoot))
            {
                try
                {
                    Directory.Delete(unpackRoot, true);
                    MessageBox.Show("Unpacked files removed successfully.", "Clean Up", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                catch (Exception ex)
                {
                    MessageBox.Show($"Could not delete the folder. It might be in use by another program.\nError: {ex.Message}", "Delete Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }

            SoundPacks_ChangeUIForUnpackedFolder(false);
        }

        private void SoundPacks_ChangeUIForUnpackedFolder(bool isUnpacked)
        {
            button_UnpackAudioPsarc.Visible = !isUnpacked;
            groupBox_SoundPacks.Visible = isUnpacked;
        }

        private void SoundPacks_ReplaceSound(string soundToReplace)
        {
            string rsModsPath = GenUtil.GetRsModsPath();

            if (!Directory.Exists(Path.Combine(rsModsPath, "audio_psarc")))
            {
                MessageBox.Show("Audio PSARC not unpacked. Please unpack it first.", "Missing Files", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                SoundPacks_ChangeUIForUnpackedFolder(false);
                return;
            }

            using OpenFileDialog fileDialog = new OpenFileDialog();
            fileDialog.Filter = "Sound Files|*.mp3;*.ogg;*.wav;*.wem";
            fileDialog.RestoreDirectory = true;

            if (fileDialog.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    Soundpacks.ConvertSoundAndReplace(soundToReplace, fileDialog.FileName, rsModsPath);
                    MessageBox.Show("Sound replaced successfully!\nDon't forget to hit \"Repack Audio Psarc\" when you're done.", "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                /*catch (DirectoryNotFoundException)
                {
                    MessageBox.Show("Wwise (WWISEROOT) environment variable not found. Please check your installation.", "Wwise Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }*/
                catch (Exception ex)
                {
                    MessageBox.Show($"An error occurred: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private void SoundPacks_Beta(object sender, EventArgs e) => Process.Start("https://github.com/Lovrom8/RSMods/issues/new");

        private void SoundPacks_Import_Dialog(object sender, EventArgs e)
        {
            using OpenFileDialog fileDialog = new()
            {
                RestoreDirectory = true,
                Filter = "RS2014 Soundpack|*.rs_soundpack"
            };

            if (fileDialog.ShowDialog() == DialogResult.OK)
            {
                Soundpacks.ImportSoundFile(fileDialog.FileName);
                MessageBox.Show("Don't forget to hit \"Repack Audio Psarc\" when you're done.");
            }
        }

        private void SoundPacks_Export_Dialog(object sender, EventArgs e)
        {
            SaveFileDialog fileDialog = new()
            {
                Filter = "RS2014 Soundpack|*.rs_soundpack",
                InitialDirectory = Environment.GetFolderPath(Environment.SpecialFolder.Desktop)
            };

            if (fileDialog.ShowDialog() == DialogResult.OK)
            {
                Soundpacks.ExportSoundFile(fileDialog.FileName);
                MessageBox.Show("You can now share your sound pack with your friends!\nTell them to open this menu and click \"Import Soundpack\" button, and point to this file.");
            }
        }

        private void SoundPacks_Reset(object sender, EventArgs e)
        {
            Soundpacks.Reset();
            MessageBox.Show("Don't forget to hit \"Repack Audio Psarc\" when you're done.");
        }

        private void ReplaceSound(string soundToReplace) => SoundPacks_ReplaceSound(Soundpacks.SoundPackLocationPrefix + Soundpacks.SoundPackEnglishPrefix + soundToReplace);

        private void SoundPacks_ReplaceBadPerformance(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.BadPerformance);
        private void SoundPacks_ReplaceDisappointingPerformance(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.DisappointingPerformance);
        private void SoundPacks_ReplaceSubparPerformance(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.SubparPerformance);
        private void SoundPacks_ReplaceCouldBeBetter(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.CouldBeBetter);
        private void SoundPacks_ReplaceDecentPerformance(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.DecentPerformance);
        private void SoundPacks_ReplaceAlrightPerformance(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.AlrightPerformance);
        private void SoundPacks_ReplaceExcellentPerformance(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.ExcellentPerformance);
        private void SoundPacks_ReplaceTopNotchPerformance(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.TopNotchPerformance);
        private void SoundPacks_ReplaceSuperbPerformance(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.SuperbPerformance);
        private void SoundPacks_ReplaceDazzlingPerformance(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.DazzlingPerformance);
        private void SoundPacks_ReplaceSuperstar(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.YoureGonnaBeASuperstar);
        private void SoundPacks_ReplaceWonderfulPerformance(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.WonderfulPerformance);
        private void SoundPacks_ReplaceExceptionalPerformance(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.ExceptionalPerformance);
        private void SoundPacks_ReplaceAmazingPerformance(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.AmazingPerformance);
        private void SoundPacks_ReplaceExemplaryPerformance(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.ExemplaryPerformance);
        private void SoundPacks_ReplaceMasterfulPerformance_98(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.MasterfulPerformance_98);
        private void SoundPacks_ReplaceMasterfulPerformance_99(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.MasterfulPerformance_99);
        private void SoundPacks_ReplaceFlawlessPerformance(object sender, EventArgs e) => ReplaceSound(Soundpacks.VoiceLines.FlawlessPerformance);

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
    }
}
