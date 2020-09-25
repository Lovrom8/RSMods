using RSMods.Util;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using TwitchLib.PubSub;
using TwitchLib.PubSub.Events;

namespace RSMods.Twitch
{
    public class PubSub
    {
        private TwitchPubSub pubSub;

        public void SetUp()
        {
            pubSub = new TwitchPubSub();
            pubSub.OnPubSubServiceConnected += OnPubSubServiceConnected;
            pubSub.OnPubSubServiceClosed += OnPubSubServiceClosed;
            pubSub.OnPubSubServiceError += OnPubSubServiceError;
            pubSub.OnListenResponse += OnListenResponse;

            pubSub.ListenToVideoPlayback(TwitchSettings.Get.Username);
            pubSub.Connect();
        }

        private void OnPubSubServiceConnected(object sender, EventArgs e)
        {
            TwitchSettings.Get.AddToLog(("PubSub Service Connected!"));

            pubSub.ListenToBitsEvents(TwitchSettings.Get.ChannelID);
            TwitchSettings.Get.AddToLog("Subscribing to bit events");

            pubSub.ListenToRewards(TwitchSettings.Get.ChannelID);
            TwitchSettings.Get.AddToLog("Subscribing to channel rewards");

            pubSub.OnBitsReceived += OnBitsReceived;
            pubSub.OnRewardRedeemed += OnRewardRedeemed;

            pubSub.SendTopics(TwitchSettings.Get.AccessToken);
        }

        public void Resub()
        {
            pubSub.Disconnect();
            pubSub.Connect();
            pubSub.SendTopics(TwitchSettings.Get.AccessToken);
        }

        private void OnListenResponse(object sender, OnListenResponseArgs e)
        {
            if (!e.Successful)
            {
                if (e.Response.ToString().ToLower().Contains("auth"))
                    TwitchSettings.Get.AddToLog($"Wrong auth cuz lovro dumb");
            }
            else
                TwitchSettings.Get.AddToLog($"This may even work, poggers in chat!");
        }

        private void OnPubSubServiceClosed(object sender, EventArgs e)
        {
            TwitchSettings.Get.AddToLog("Lazy fudger disconnected :(");
            Thread.Sleep(2000);

            // pubSub.Connect(); // Brute force MF-ers
            Resub();
        }

        private void OnPubSubServiceError(object sender, EventArgs e)
        {
            TwitchSettings.Get.AddToLog("Something went wrong :(");

            Thread.Sleep(2000);
            //pubSub.Connect();
            Resub();
        }

        private void OnBitsReceived(object sender, OnBitsReceivedArgs e)
        {
            TwitchSettings.Get.AddToLog($"{e.Username} cheered {e.BitsUsed} bits");

            HandleBitsRecieved(e);
        }

        private void OnRewardRedeemed(object sender, OnRewardRedeemedArgs e)
        {
            TwitchSettings.Get.AddToLog($"A viewer redeemed {e.RewardCost} points");

            HandleChannelPointsRecieved(e);
        }

        private async Task WaitUntilRewardEnds(int seconds) => await Task.Delay(seconds * 1000);

        public async void HandleBitsRecieved(OnBitsReceivedArgs e)
        {
            var reward = TwitchSettings.Get.Rewards.OfType<BitsReward>().FirstOrDefault(rew => rew.Enabled && rew.BitsAmount == e.BitsUsed);

            if (reward == null) // If there's no reward specified for this amount of bits
                return;

            if (reward.AdditionalMsg != "")
                WinMsgUtil.SendMsgToRS($"{reward.InternalMsgEnable} {reward.AdditionalMsg}");
            else
                WinMsgUtil.SendMsgToRS(reward.InternalMsgEnable);

            TwitchSettings.Get.AddToLog($"Enabling: {reward.Name}");

            await WaitUntilRewardEnds(reward.Length);

            WinMsgUtil.SendMsgToRS(reward.InternalMsgDisable);
            TwitchSettings.Get.AddToLog($"Disabling: {reward.Name}");
        }

        public async void HandleChannelPointsRecieved(OnRewardRedeemedArgs e)
        {
            var reward = TwitchSettings.Get.Rewards.OfType<ChannelPointsReward>().FirstOrDefault(rew => rew.Enabled && rew.PointsAmount == e.RewardCost);

            if (reward == null)
                return;

            if (reward.AdditionalMsg != "")
                WinMsgUtil.SendMsgToRS($"{reward.InternalMsgEnable} {reward.AdditionalMsg}");
            else
                WinMsgUtil.SendMsgToRS(reward.InternalMsgEnable);

            TwitchSettings.Get.AddToLog($"Enabling: {reward.Name}");

            await WaitUntilRewardEnds(reward.Length);

            WinMsgUtil.SendMsgToRS(reward.InternalMsgDisable);
            TwitchSettings.Get.AddToLog($"Disabling: {reward.Name}");
        }

        private PubSub() { }
        public static readonly PubSub Get = new PubSub();
    }
}
