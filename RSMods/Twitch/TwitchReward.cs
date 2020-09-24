using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Serialization;

namespace RSMods.Twitch
{
    [XmlInclude(typeof(BitsReward))]
    [XmlInclude(typeof(ChannelPointsReward))]
    public class TwitchReward
    {
        public string InternalMsgEnable { get; set; }
        public string InternalMsgDisable { get; set; }
        public string AdditionalMsg { get; set; }
        public string Description { get; set; }
        public string Name { get; set; }
        public int Length { get; set; }
        public bool Enabled { get; set; }

        public TwitchReward() { }

        public TwitchReward(string name, string description, string disableMsg, string enableMsg)
        {
            InternalMsgDisable = disableMsg;
            InternalMsgEnable = enableMsg;
            Description = description;
            Name = name;
        }
    }

    public class BitsReward : TwitchReward
    {
        public int BitsID { get; set; }
        public int BitsAmount { get; set; }
    }

    public class ChannelPointsReward : TwitchReward
    {
        public int PointsID { get; set; }
        public int PointsAmount { get; set; }
    }
}
