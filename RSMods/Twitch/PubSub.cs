using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using TwitchLib.PubSub;

namespace RSMods.Twitch
{
    public class PubSub
    {
        private TwitchPubSub pubSub;

        public void SetUp()
        {
            pubSub = new TwitchPubSub();
            pubSub.OnPubSubServiceConnected += OnPubSubServiceConnected;

            pubSub.ListenToVideoPlayback(TwitchSettings.Username);
            pubSub.Connect();
        }

        private void OnPubSubServiceConnected(object sender, System.EventArgs e)
        {
            Debug.WriteLine("PubSubServiceConnected!");

            pubSub.ListenToBitsEvents(TwitchSettings.ChannelID);
            pubSub.OnBitsReceived += OnBitsReceived;

            pubSub.SendTopics(TwitchSettings.AccessToken);
        }

        private void OnBitsReceived(object sender, TwitchLib.PubSub.Events.OnBitsReceivedArgs e)
        {
            Debug.WriteLine($"{e.Username} cheered {e.BitsUsed} bits");
        }
    }
}
