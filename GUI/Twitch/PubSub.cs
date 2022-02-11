using RSMods.Twitch.EffectServer;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using TwitchLib.PubSub;
using TwitchLib.PubSub.Events;

namespace RSMods.Twitch
{
    public class PubSub
    {
        private TwitchPubSub pubSub;
        private static EffectServerTCP effectServer;
        private bool authedPoints = false, authedBits = false, authedSubs = false;
        private List<string> eventHashes = new List<string>();

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

            pubSub.ListenToSubscriptions(TwitchSettings.Get.ChannelID);
            TwitchSettings.Get.AddToLog("Subscribing to subs");

            pubSub.OnBitsReceived += OnBitsReceived;
            pubSub.OnRewardRedeemed += OnRewardRedeemed;
            pubSub.OnChannelSubscription += OnSubscriptionRecieved;
            pubSub.SendTopics(TwitchSettings.Get.AccessToken);

            if (effectServer != null)
                effectServer.StopTCPListener();

            effectServer = new EffectServerTCP();
        }

        public void Resub()
        {
            // pubSub.Disconnect(); Disconnecting/Reconnecting may cause OnPubSubServiceError loop
            // pubSub.Connect();

            TwitchSettings.Get.AddToLog("------------------------" + Environment.NewLine + "Resubing to Twitch events");
            pubSub.ListenToRewards(TwitchSettings.Get.ChannelID);
            pubSub.ListenToBitsEvents(TwitchSettings.Get.ChannelID);
            pubSub.ListenToSubscriptions(TwitchSettings.Get.ChannelID);
            pubSub.SendTopics(TwitchSettings.Get.AccessToken);
        }

        private void OnListenResponse(object sender, OnListenResponseArgs e)
        {
            if (!e.Successful)
            {
                if (e.Response.ToString().ToLower().Contains("auth"))
                    TwitchSettings.Get.AddToLog($"Wrong auth :(");
            }
            else
            {
                if (authedBits && authedPoints && authedSubs)
                    return;

                if (e.Topic.Contains("points"))
                {
                    TwitchSettings.Get.AddToLog($"Authorized channel points events, poggers in chat!");
                    authedPoints = true;
                }    
                else if (e.Topic.Contains("bits"))
                {
                    TwitchSettings.Get.AddToLog($"Authorized bit events, poggers in chat!");
                    authedBits = true;
                }
                else if (e.Topic.Contains("sub"))
                {
                    TwitchSettings.Get.AddToLog($"Authorized sub events, poggers in chat!");
                    authedSubs = true;
                }
            }
        }

        private void OnPubSubServiceClosed(object sender, EventArgs e)
        {
            TwitchSettings.Get.AddToLog("Twitch API yeeted us out. Trying to hack our way back in :P");
            Thread.Sleep(2000);

            // pubSub.Connect(); // Brute force MF-ers
            Resub();
        }

        private void OnPubSubServiceError(object sender, EventArgs e)
        {
            TwitchSettings.Get.AddToLog("Twitch API tried to download more RAM :O Deleting all traces of Rabies.");

            Thread.Sleep(2000);
            Resub();
        }

        private void OnBitsReceived(object sender, OnBitsReceivedArgs e)
        {
            TwitchSettings.Get.AddToLog($"{e.Username} cheered {e.BitsUsed} bits (time: {e.Time} )");

            string hash = e.Time + e.Username + e.BitsUsed;

            if (eventHashes.Contains(hash))
            {
                //TwitchSettings.Get.AddToLog("Triple bit event, thou shall not pass!");
                return;
            }

            eventHashes.Add(hash);
            HandleBitsRecieved(e);
        }

        private void OnRewardRedeemed(object sender, OnRewardRedeemedArgs e)
        {
            TwitchSettings.Get.AddToLog($"A viewer redeemed {e.RewardCost} points");

            HandleChannelPointsRecieved(e);
        }

        private void OnSubscriptionRecieved(object sender, OnChannelSubscriptionArgs e)
        {
            var subInfo = e.Subscription;

            if (subInfo.IsGift.HasValue && subInfo.IsGift.Value)
                TwitchSettings.Get.AddToLog($"{subInfo.RecipientDisplayName} recieved a subgift from {subInfo.DisplayName}");
            else
                TwitchSettings.Get.AddToLog($"{subInfo.DisplayName} subscribed to the channel!");

            HandleSubRecieved(e);
        }

        //private static async Task WaitUntilRewardEnds(int seconds) => await Task.Delay(seconds * 1000);

        public static void SendMessageToRocksmith(TwitchReward reward)
        {
            if (reward == null) // If there's no reward specified for this amount of bits
                return;

            effectServer.AddEffectToTheQueue(reward);

            /* TCP Server instead of WM_COPYDATA, to accomodate the way CC effect server works :(
             * if (reward.AdditionalMsg != "")
                WinMsgUtil.SendMsgToRS($"{reward.InternalMsgEnable} {reward.AdditionalMsg} {reward.Length}");
            else
                WinMsgUtil.SendMsgToRS($"{reward.InternalMsgEnable} {reward.Length}");
            */

            TwitchSettings.Get.AddToLog($"Enabling: {reward.Name}");

            /* It's now handled internally 
             
            await WaitUntilRewardEnds(reward.Length);

            WinMsgUtil.SendMsgToRS(reward.InternalMsgDisable);
            TwitchSettings.Get.AddToLog($"Disabling: {reward.Name}");*/
        }

        public void HandleBitsRecieved(OnBitsReceivedArgs e) => SendMessageToRocksmith(TwitchSettings.Get.Rewards.OfType<BitsReward>().FirstOrDefault(rew => rew.Enabled && rew.BitsAmount == e.BitsUsed));

        public void HandleChannelPointsRecieved(OnRewardRedeemedArgs e) => SendMessageToRocksmith(TwitchSettings.Get.Rewards.OfType<ChannelPointsReward>().FirstOrDefault(rew => rew.Enabled && rew.PointsAmount == e.RewardCost));

        public void HandleSubRecieved(OnChannelSubscriptionArgs e) => SendMessageToRocksmith(TwitchSettings.Get.Rewards.OfType<SubReward>().FirstOrDefault(rew => rew.Enabled));

        private PubSub() { }
        public static readonly PubSub Get = new PubSub();
    }
}
