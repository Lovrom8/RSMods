using System;
using System.Windows.Forms;
using System.IO;
using System.Linq;
using System.Drawing;
using System.Drawing.Text;
using RSMods.Core;
using RSMods.Data;
using RSMods.SetAndForget.Models;
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

        private async void SetForget_RestoreDefaults(object sender, EventArgs e)
        {
            if (await SetAndForgetMods.RestoreDefaults(AppServices.Dialogs))
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

        private static async Task RunModActionAsync(Action action, string successMessage = null, string errorPrefix = "Operation failed")
        {
            try
            {
                action();

                if (successMessage != null) {
                    await AppServices.Dialogs.ShowInfoAsync(successMessage, "Success");
                }
            }
            catch (IOException ex)
            {
                await AppServices.Dialogs.ShowErrorAsync($"{errorPrefix}{Environment.NewLine}Error: {ex.Message}");
            }
        }

        private async void SetForget_TurnItUpToEleven(object sender, EventArgs e) =>
            await RunModActionAsync(SetAndForgetMods.AddIncreasedVolumeWwiseBank, "cache.psarc repackaged successfully", "Unable to repack cache.psarc");

        private void SetForget_UnpackCacheAgain(object sender, EventArgs e) => SetAndForgetMods.CleanUnpackedCache();

        private async void SetForget_AddCustomTunings(object sender, EventArgs e) =>
            await RunModActionAsync(SetAndForgetMods.AddCustomTunings, "cache.psarc repackaged successfully", "Unable to add custom tunings");

        private async void SetForget_AddFastLoadMod(object sender, EventArgs e)
        {
            bool useNvmeFastLoad = false;

            switch (SetAndForgetMods.GetFastLoadDrivePrompt())
            {
                case SetAndForgetMods.FastLoadDrivePrompt.ConfirmHddRisk:
                    if (!await AppServices.Dialogs.ShowConfirmAsync(
                        "It appears as though Rocksmith is installed on a hard disk drive. HDDs are normally too slow to support the fast load mod and will likely result in a crash.\n\nDo you wish to proceed?",
                        "Drive too slow for fast load"))
                        return;
                    break;

                case SetAndForgetMods.FastLoadDrivePrompt.ConfirmNvme:
                    useNvmeFastLoad = await AppServices.Dialogs.ShowConfirmAsync(
                        "Can you confirm Rocksmith is installed on an NVMe drive?\nIf you are unsure, press \"No\", because Rocksmith is likely to crash if you pick the fastest option!",
                        "Is RS on an NVMe drive?");
                    break;

                case SetAndForgetMods.FastLoadDrivePrompt.ConfirmUnknown:
                    useNvmeFastLoad = await AppServices.Dialogs.ShowConfirmAsync(
                        "We were unable to detect the drive type on which Rocksmith is installed.\nIs it on an NVMe drive? (If it's not, the fastest loading option is likely to crash your game!)",
                        "Fast drive?");
                    break;
            }

            await RunModActionAsync(() => SetAndForgetMods.ApplyFastLoadMod(useNvmeFastLoad),
                "cache.psarc repackaged successfully", "Unable to modify required files");
        }

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

        private async void SetForget_SaveTuningChanges(object sender, EventArgs e)
        {
            if (listBox_Tunings.SelectedIndex != -1) // If we are saving a change to the currently selected tuning, perform a change in the collection, otherwise directly go to saving
            {
                string selectedItem = listBox_Tunings.SelectedItem.ToString();

                if (selectedItem != "<New>")
                {
                    SetAndForgetMods.TuningsCollection[selectedItem] = SetForget_GetCurrentTuningInfo();

                    if (listBox_SetAndForget_SongsWithCustomTuning.Items.Count > 0)
                        await SetForget_LoadSongsToWorkOnAsync();
                }
            }

            try
            {
                SetAndForgetMods.SaveTuningsJSON();
            }
            catch (IOException ex)
            {
                await AppServices.Dialogs.ShowErrorAsync($"Error: {ex}");
            }

            MessageBox.Show("Saved current tuning, don't forget to press \"Add Custom Tunings\" button when you are done!", "Success");
        }

        private async void SetForget_RemoveTuning(object sender, EventArgs e)
        {
            if (listBox_Tunings.SelectedIndex == -1)
                return;

            string selectedItem = listBox_Tunings.SelectedItem.ToString();

            if (selectedItem == "<New>")
                return;

            SetAndForgetMods.TuningsCollection.Remove(selectedItem); // I guess we would be better here using BindingSource on Listbox + ObservableCollection instead of Dict to make changes reflect automatically, but... one day
            listBox_Tunings.Items.Remove(selectedItem);

            if (listBox_SetAndForget_SongsWithCustomTuning.Items.Count > 0)
                await SetForget_LoadSongsToWorkOnAsync();
        }

        private async void SetForget_AddTuning(object sender, EventArgs e)
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
                    await SetForget_LoadSongsToWorkOnAsync();
            }
            else
            {
                MessageBox.Show("You already have a tuning with the same internal name");
            }
        }

        private async void SetForget_AddCustomMenu(object sender, EventArgs e) =>
            await RunModActionAsync(SetAndForgetMods.AddExitGameMenuOption, "cache.psarc repackaged successfully", "Unable to repack cache.psarc");

        private async void SetForget_AddDCMode(object sender, EventArgs e) =>
            await RunModActionAsync(SetAndForgetMods.AddDirectConnectModeOption, "cache.psarc repackaged successfully", "Unable to repack cache.psarc");

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

            var result = SetAndForgetMods.SetDefaultTones(selectedToneName, selectedToneType);
            MessageBox.Show(result.Message, "Assignment result");
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

            var result = SetAndForgetMods.SetGuitarArcadeTone(selectedToneName, selectedToneType);
            MessageBox.Show(result.Message, "Tone change result");
        }

        private async void SetForget_LoadSongsToWorkOn(object sender, EventArgs e)
        {
            await SetForget_LoadSongsToWorkOnAsync();
        }

        private async Task SetForget_LoadSongsToWorkOnAsync()
        {
            progressBar_FillSongsWithCustomTunings.Minimum = 0;
            progressBar_FillSongsWithCustomTunings.Maximum = 100;
            progressBar_FillSongsWithCustomTunings.Value = 0;
            progressBar_FillSongsWithCustomTunings.Visible = true;

            try
            {
                var progress = new Progress<int>(value => progressBar_FillSongsWithCustomTunings.Value = value);
                Songs = await SongManager.ExtractSongDataAsync(progress);

                SetForget_ShowLoadedSongs(); // Makes the listboxes and labels visible for songs with tunings.
                SetForget_FillCustomTuningList(); // Get a list of all song & arrangement combos that will show up as "Custom Tuning" if not dealt with.
                SetForget_FillSongsWithSelectedTuningList(); // Get a list of all song & arrangement combos that have the same tuning as selected in listBox_Tunings.
            }
            finally
            {
                progressBar_FillSongsWithCustomTunings.Visible = false;
                progressBar_FillSongsWithCustomTunings.Value = 0;
            }
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

        private void SetForget_FillCustomTuningList()
        {
            customTunings = SetAndForgetMods.GetUnknownTunings(Songs);
            listBox_SetAndForget_SongsWithCustomTuning.Items.Clear();
            listBox_SetAndForget_SongsWithCustomTuning.Items.AddRange([.. customTunings.Keys]);
        }

        private void SetForget_FillSongsWithSelectedTuningList()
        {
            listBox_SetAndForget_SongsWithSelectedTuning.Items.Clear();

            if (listBox_Tunings.SelectedIndex == -1 || listBox_Tunings.SelectedItem.ToString() == "<New>")
                return;

            ArrangementTuning selectedTuning = SetAndForgetMods.ToArrangementTuning(SetAndForgetMods.TuningsCollection[listBox_Tunings.SelectedItem.ToString()]);
            listBox_SetAndForget_SongsWithSelectedTuning.Items.AddRange([.. SetAndForgetMods.GetSongsWithTuning(Songs, selectedTuning)]);
        }

        private SortedDictionary<string, ArrangementTuning> customTunings = [];

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
    }
}
