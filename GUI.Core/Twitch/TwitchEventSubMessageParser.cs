using Newtonsoft.Json.Linq;
using System;
using System.Globalization;

namespace RSMods.Twitch
{
    internal sealed class TwitchEventSubEnvelope
    {
        public string MessageId { get; set; }
        public string MessageType { get; set; }
        public string SubscriptionType { get; set; }
        public DateTimeOffset Timestamp { get; set; }
        public JObject Payload { get; set; }
    }

    internal static class TwitchEventSubMessageParser
    {
        public static TwitchEventSubEnvelope ParseEnvelope(string json, DateTimeOffset fallbackTimestamp)
        {
            JObject root = JObject.Parse(json);
            JObject metadata = root["metadata"] as JObject ?? [];
            string rawTimestamp = (string)metadata["message_timestamp"];
            if (!DateTimeOffset.TryParse(rawTimestamp, CultureInfo.InvariantCulture, DateTimeStyles.AssumeUniversal, out DateTimeOffset timestamp))
            {
                timestamp = fallbackTimestamp;
            }

            return new TwitchEventSubEnvelope
            {
                MessageId = (string)metadata["message_id"] ?? string.Empty,
                MessageType = (string)metadata["message_type"] ?? string.Empty,
                SubscriptionType = (string)metadata["subscription_type"] ?? string.Empty,
                Timestamp = timestamp,
                Payload = root["payload"] as JObject ?? new JObject()
            };
        }

        public static bool TryGetWelcome(
            TwitchEventSubEnvelope envelope,
            out string sessionId,
            out TimeSpan keepaliveTimeout)
        {
            sessionId = string.Empty;
            keepaliveTimeout = TimeSpan.Zero;
            if (!string.Equals(envelope.MessageType, "session_welcome", StringComparison.Ordinal))
            {
                return false;
            }

            JObject session = envelope.Payload["session"] as JObject;
            sessionId = (string)session?["id"] ?? string.Empty;
            int seconds = (int?)session?["keepalive_timeout_seconds"] ?? 10;
            keepaliveTimeout = TimeSpan.FromSeconds(Math.Max(10, seconds) + 5);

            return !string.IsNullOrWhiteSpace(sessionId);
        }

        public static bool TryGetReconnectUrl(TwitchEventSubEnvelope envelope, out Uri reconnectUri)
        {
            reconnectUri = null;
            if (!string.Equals(envelope.MessageType, "session_reconnect", StringComparison.Ordinal))
                return false;

            string value = (string)envelope.Payload["session"]?["reconnect_url"];
            return Uri.TryCreate(value, UriKind.Absolute, out reconnectUri);
        }

        public static bool TryGetRevocation(
            TwitchEventSubEnvelope envelope,
            out string subscriptionType,
            out string status)
        {
            subscriptionType = string.Empty;
            status = string.Empty;
            if (!string.Equals(envelope.MessageType, "revocation", StringComparison.Ordinal))
                return false;

            JObject subscription = envelope.Payload["subscription"] as JObject;
            subscriptionType = (string)subscription?["type"] ?? envelope.SubscriptionType;
            status = (string)subscription?["status"] ?? "revoked";
            return true;
        }

        public static bool TryGetTrigger(TwitchEventSubEnvelope envelope, out TwitchTriggerEvent trigger)
        {
            trigger = null;
            if (!string.Equals(envelope.MessageType, "notification", StringComparison.Ordinal))
                return false;

            JObject eventBody = envelope.Payload["event"] as JObject;
            if (eventBody == null)
                return false;

            switch (envelope.SubscriptionType)
            {
                case "channel.cheer":
                    bool anonymous = (bool?)eventBody["is_anonymous"] ?? false;
                    trigger = CreateTrigger(
                        envelope,
                        TwitchTriggerKind.Bits,
                        anonymous ? "Anonymous" : ReadViewer(eventBody),
                        (int?)eventBody["bits"] ?? 0);
                    return true;

                case "channel.channel_points_custom_reward_redemption.add":
                    trigger = CreateTrigger(
                        envelope,
                        TwitchTriggerKind.ChannelPoints,
                        ReadViewer(eventBody),
                        (int?)eventBody["reward"]?["cost"] ?? 0);
                    return true;

                case "channel.subscribe":
                case "channel.subscription.message":
                    trigger = CreateTrigger(
                        envelope,
                        TwitchTriggerKind.Subscription,
                        ReadViewer(eventBody),
                        1);
                    return true;

                default:
                    return false;
            }
        }

        private static TwitchTriggerEvent CreateTrigger(
            TwitchEventSubEnvelope envelope,
            TwitchTriggerKind kind,
            string viewer,
            int amount)
        {
            return new TwitchTriggerEvent
            {
                EventId = envelope.MessageId,
                Kind = kind,
                Viewer = string.IsNullOrWhiteSpace(viewer) ? "Unknown viewer" : viewer,
                Amount = amount,
                Timestamp = envelope.Timestamp
            };
        }

        private static string ReadViewer(JObject eventBody) =>
            (string)eventBody["user_name"] ?? (string)eventBody["user_login"] ?? string.Empty;
    }
}
