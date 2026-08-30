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

            // Hide values by default (Security just in case the streamer is live with RSMods on screen)
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

        private async void PrepTwitch_LoadSettings()
        {
            TwitchSettings.Get._context = SynchronizationContext.Current;
            await TwitchSettings.Get.LoadSettings();
            TwitchSettings.Get.LoadDefaultEffects();
            TwitchSettings.Get.LoadEnabledEffects();
        }

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

        private async Task Twitch_SaveRewards() => await TwitchSettings.Get.SaveRewards();

        private async void Twitch_AddReward(object sender, EventArgs e)
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
            await Twitch_SaveRewards();
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

        private async void Twitch_EnabledRewards_CellEndEdit(object sender, DataGridViewCellEventArgs e)
        {
            var selectedRow = dgv_EnabledRewards.SelectedRows[0];
            var selectedReward = Twitch_GetSelectedReward(selectedRow);

            if (selectedReward == null)
                return;

            selectedReward.Enabled = Convert.ToBoolean(selectedRow.Cells["colEnabledRewardsEnabled"].Value);
            if (!(selectedRow.Cells["colEnabledRewardsLength"].Value != null && int.TryParse(selectedRow.Cells["colEnabledRewardsLength"].Value.ToString(), out int rewardLength)))
            {
                selectedRow.Cells["colEnabledRewardsLength"].Value = 0;
                MessageBox.Show("You need to put a number, not a text value.");
                return;
            }

            /*if (!(selectedRow.Cells["colEnabledRewardsAmount"].Value != null && int.TryParse(selectedRow.Cells["colEnabledRewardsAmount"].Value.ToString(), out int rewardAmount)))
            {
                selectedRow.Cells["colEnabledRewardsAmount"].Value = 0;
                MessageBox.Show("You need to put a number, not a text value.");
                return;
            }*/

            selectedReward.Length = rewardLength;

            if (selectedReward is BitsReward bitsReward)
                bitsReward.BitsAmount = Convert.ToInt32(selectedRow.Cells["colEnabledRewardsAmount"].Value);
            else if (selectedReward is ChannelPointsReward channelPointsReward)
                channelPointsReward.PointsAmount = Convert.ToInt32(selectedRow.Cells["colEnabledRewardsAmount"].Value);

            Twitch_CheckForTurboSpeed(selectedReward);

            await Twitch_SaveRewards();
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


        private async void Twitch_RemoveReward(object sender, EventArgs e)
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

            await Twitch_SaveRewards();
        }

        private void Twitch_SetAdditionalMessage(string msg)
        {
            var selectedRow = dgv_EnabledRewards.SelectedRows[0];
            var selectedReward = Twitch_GetSelectedReward(selectedRow);

            if (selectedReward.Name != "Solid color notes")
                return;

            selectedReward.AdditionalMsg = msg;
        }

        private async void Twitch_SolidNoteColor_Pick(object sender, EventArgs e)
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
                await Twitch_SaveRewards();
            }
        }

        private async void Twitch_SolidNoteColor_Random(object sender, EventArgs e)
        {
            textBox_SolidNoteColorPicker.BackColor = Color.White;
            textBox_SolidNoteColorPicker.Text = "Random";

            Twitch_SetAdditionalMessage("Random");
            await Twitch_SaveRewards();

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
                string logPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "twitchLog.txt");
                TwitchSettings.Get.SaveLog(logPath);
                MessageBox.Show("Saved log to RS folder/RSMods/twitchLog.txt!", "Saved!");
            }
            catch (IOException ioex)
            {
                MessageBox.Show($"Unable to save log, error: {ioex.Message}");
            }
        }

        private void Twitch_CopyCredentialsForDevs(object sender, MouseEventArgs e) => Clipboard.SetText("Send to RSMod Developers ( Discord Ffio#2221 or LovroM8#9999 )\nUsername: " + TwitchSettings.Get.Username + "\nChannel ID: " + TwitchSettings.Get.ChannelID + "\nAccess Token: " + TwitchSettings.Get.AccessToken);
    }
}
