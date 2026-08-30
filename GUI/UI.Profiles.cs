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
        List<SongData> Songs = [];

        private void Profiles_RefreshSonglistNames()
        {
            const int startIndex = 3;

            for (int i = 0; i < Profiles.SongListCount; i++)
            {
                dgv_Profiles_Songlists.Columns[startIndex + i].HeaderText = RsModsSettings.GetSongListTitle(i + 1);
            }
        }

        private static object[] MapSongToRow(SongData song, List<List<string>> songLists)
        {
            const int MaxColumns = 23;
            object[] row = new object[MaxColumns];

            row[0] = song.Artist;
            row[1] = song.Title;

            for (int i = 2; i < MaxColumns; i++) row[i] = false;

            // Check membership in lists (Favorites + up to 20 SongLists)
            int limit = Math.Min(songLists.Count, 21);
            for (int i = 0; i < limit; i++)
            {
                if (songLists[i].Contains(song.DLCKey))
                    row[i + 2] = true;
            }

            return row;
        }

        private void PrepareSongListUI(bool isFinished)
        {
            if (!isFinished)
            {
                dgv_Profiles_Songlists.SuspendLayout();

                dgv_Profiles_Songlists.Rows.Clear();

                Cursor = Cursors.WaitCursor;

                dgv_Profiles_Songlists.Visible = false;

                button_Profiles_SaveSonglist.Enabled = false;

                progressBar_Profiles_LoadPsarcs.Value = 0;
                progressBar_Profiles_LoadPsarcs.Visible = true;
            }
            else
            {
                dgv_Profiles_Songlists.ResumeLayout();
                Cursor = Cursors.Default;

                dgv_Profiles_Songlists.Visible = true;
                button_Profiles_SaveSonglist.Enabled = true;

                progressBar_Profiles_LoadPsarcs.Visible = false;
            }
        }

        private void Profiles_LoadSongs(object sender, EventArgs e)
        {
            PrepareSongListUI(false);

            Songs = SongManager.ExtractSongData(progressBar_Profiles_LoadPsarcs);
            var ownedRS1DLC = Profiles.GetOwnedRS1DLC();
            var allSongLists = Profiles.GetProfileSongListsWithFavorites();

            foreach (var song in Songs)
            {
                if (!Profiles.ShouldIncludeSong(song, ownedRS1DLC))
                    continue;

                object[] row = MapSongToRow(song, allSongLists);
                dgv_Profiles_Songlists.Rows.Add(row);
            }

            Profiles_RefreshSonglistNames();
            UpdateSongListColumnVisibility(allSongLists.Count - 1); // -1 because Favorites is list 0
            PrepareSongListUI(true);
        }

        private void UpdateSongListColumnVisibility(int activeSongListCount)
        {
            for (int i = 1; i <= 20; i++)
            {
                string columnName = $"SongList{i}";
                if (dgv_Profiles_Songlists.Columns.Contains(columnName))
                {
                    dgv_Profiles_Songlists.Columns[columnName].Visible = (i <= activeSongListCount);
                }
            }
        }

        private void Profiles_UnpackProfile()
        {
            string selectedProfileName = listBox_Profiles_AvailableProfiles.SelectedItem.ToString();

            if (listBox_Profiles_AvailableProfiles.SelectedItem != null && Profiles.CurrentUnpackedProfileName != selectedProfileName)
            {
                Profiles.CurrentUnpackedProfileName = selectedProfileName;
                Profiles.SetProfileAsActive(selectedProfileName);
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

            label_TotalSonglists.Text = Profiles.GetProfileSongLists().Count.ToString();
        }

        private void Profiles_SaveSonglists(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex < 0)
                return;

            Profiles.EncryptCurrentProfile();
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
                Profiles_ChangeRewardStatusAndSave(true);
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
                Profiles_ChangeRewardStatusAndSave(false);
            }
        }

        private void Profile_AddNewSongList(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex < 0)
            {
                MessageBox.Show("Make sure you have a profile selected!");
                return;
            }

            if (!Profiles.AddSongList())
            {
                MessageBox.Show("We cannot complete your request!\nHaving more than 20 song lists is extremely unrealistic.\nPlease reach out to the RSMods dev team and we can change this restriction");
                return;
            }

            Profiles_GenerateNewSonglistsLists();

            if (dgv_Profiles_Songlists.Visible)
                Profiles_LoadSongs(sender, e);

            label_TotalSonglists.Text = Profiles.SongListCount.ToString();
            MessageBox.Show("Your new song list is present in game!");
        }

        private void Profile_RemoveNewestSongList(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex < 0)
            {
                MessageBox.Show("Make sure you have a profile selected!");
                return;
            }

            if (!Profiles.RemoveSongList())
            {
                MessageBox.Show("We cannot remove anymore songlists.");
                return;
            }

            Profiles_GenerateNewSonglistsLists();

            if (dgv_Profiles_Songlists.Visible)
                Profiles_LoadSongs(sender, e);

            label_TotalSonglists.Text = Profiles.SongListCount.ToString();
            MessageBox.Show("The newest songlist has been removed!");
        }

        private void Profiles_GenerateNewSonglistsLists()
        {
            if (Profiles.DecryptedProfile != null)
            {
                Profiles_Helper_GenerateValidSonglists();
            }
        }

        private void Profiles_Helper_GenerateValidSonglists()
        {
            RsModsSettings.RefreshSongListTitles();
            listBox_Songlist.Items.Clear();

            foreach (string SongList in RsModsSettings.SongListTitles)
            {
                listBox_Songlist.Items.Add(SongList);
            }
        }

        private void Profiles_ChangeRewardStatusAndSave(bool unlockRewards)
        {
            Profiles.ChangeRewardStatus(unlockRewards);
            Profiles.EncryptCurrentProfile();
            MessageBox.Show("Changes to Rewards have been saved!");
        }

        private void Profiles_SongToSonglist(int songlistNumber, bool add = true)
        {
            int rowIndex = dgv_Profiles_Songlists.SelectedCells[0].RowIndex;
            string commonName = $"{dgv_Profiles_Songlists[0, rowIndex].Value} - {dgv_Profiles_Songlists[1, rowIndex].Value}";
            string dlcKey = Songs.FirstOrDefault(song => song.CommonName == commonName).DLCKey;

            Profiles.SetSongInList(dlcKey, songlistNumber - 1, add);
        }

        private void Profiles_SongToFavorites(bool add = true)
        {
            int rowIndex = dgv_Profiles_Songlists.SelectedCells[0].RowIndex;
            string commonName = $"{dgv_Profiles_Songlists[0, rowIndex].Value} - {dgv_Profiles_Songlists[1, rowIndex].Value}";
            string dlcKey = Songs.FirstOrDefault(song => song.CommonName == commonName).DLCKey;

            Profiles.SetSongInFavorites(dlcKey, add);
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
            if (listBox_Profiles_ListBackups.SelectedItem is not string localizedName)
                return;

            string sourceDir = Profiles.GetBackupSourceDir(localizedName);

            if (sourceDir == null)
            {
                MessageBox.Show("Could not identify the backup date format.");
                return;
            }

            if (!Directory.Exists(sourceDir))
            {
                MessageBox.Show("The selected backup folder could not be found.");
                return;
            }

            try
            {
                Profiles.RestoreBackup(sourceDir, Profiles.GetSaveDirectory());
                MessageBox.Show($"Reverted to the backup: {localizedName}");
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to restore backup: {ex.Message}");
            }
        }

        private void Profiles_ImportToneManifest(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex == -1)
            {
                MessageBox.Show("Please select a profile!", "Validation Error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            using OpenFileDialog fileDialog = new()
            {
                Filter = "JSON|*.json",
                Multiselect = checkBox_ImportTonesBulk.Checked
            };

            if (fileDialog.ShowDialog() != DialogResult.OK || fileDialog.FileNames.Length == 0)
            {
                return;
            }

            var (ImportedCount, ErrorMessages) = Profiles.ProcessToneManifests(fileDialog.FileNames);

            if (ErrorMessages.Count > 0)
            {
                string errors = string.Join("\n", ErrorMessages);
                MessageBox.Show($"Import completed with some errors:\n\n{errors}", "Import Warnings", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }

            if (ImportedCount > 0)
            {
                MessageBox.Show($"Added {ImportedCount} tone(s) to profile!", "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else if (ErrorMessages.Count == 0)
            {
                MessageBox.Show("No tones were found to import.", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }

        private void Profiles_ImportTone2014(object sender, EventArgs e)
        {
            if (listBox_Profiles_AvailableProfiles.SelectedIndex == -1)
            {
                MessageBox.Show("Please select a profile!", "Validation Error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            using OpenFileDialog fileDialog = new()
            {
                Filter = "XML|*.tone2014.xml",
                Multiselect = checkBox_ImportTonesBulk.Checked
            };

            if (fileDialog.ShowDialog() != DialogResult.OK || fileDialog.FileNames.Length == 0) return;

            MessageBoxManager.OK = "Guitar";
            MessageBoxManager.Cancel = "Bass";
            MessageBoxManager.Register();

            try
            {
                var (ImportedCount, ErrorMessages) = Profiles.ProcessXmlTones(fileDialog.FileNames, tone =>
                {
                    var dialogResult = MessageBox.Show(
                        $"Do you want to save '{tone.Name}' as a guitar tone, or a bass tone?",
                        "Tone Assignment",
                        MessageBoxButtons.OKCancel,
                        MessageBoxIcon.Question);

                    return dialogResult == DialogResult.OK; // OK = Guitar
                });

                if (ImportedCount > 0)
                {
                    MessageBox.Show($"Added {ImportedCount} tone(s) to your profile!", "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }

                if (ErrorMessages.Count > 0)
                {
                    MessageBox.Show($"Import completed with some errors:\n\n{string.Join("\n", ErrorMessages)}", "Warnings", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }
            finally
            {
                MessageBoxManager.Unregister();
                MessageBoxManager.OK = "OK";
                MessageBoxManager.Cancel = "Cancel";
            }
        }
    }
}
