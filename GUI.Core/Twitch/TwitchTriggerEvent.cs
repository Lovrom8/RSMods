using System;

namespace RSMods.Twitch
{
    public enum TwitchTriggerKind
    {
        Bits,
        ChannelPoints,
        Subscription
    }

    public sealed class TwitchTriggerEvent
    {
        public string EventId { get; set; }
        public TwitchTriggerKind Kind { get; set; }
        public string Viewer { get; set; }
        public int Amount { get; set; }
        public DateTimeOffset Timestamp { get; set; }
    }
}
