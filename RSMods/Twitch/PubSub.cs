using RSMods.Util;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
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
            
            pubSub.ListenToVideoPlayback(TwitchSettings.Get.Username);
            pubSub.Connect();
        }

        private void OnPubSubServiceConnected(object sender, System.EventArgs e)
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
