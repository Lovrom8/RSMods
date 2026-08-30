using System;
using System.Collections.Generic;
using System.Linq;

namespace RSMods.Twitch
{
    public sealed class TwitchOptions
    {
        // This public identifier must belong to an RSMods Twitch application registered as a public client.
        public const string RSModsClientId = "8uq8vhme1zmy4gsjnzuq5qaca9i804";

        public static readonly string[] RequiredScopes =
        {
            "bits:read",
            "channel:read:redemptions",
            "channel:read:subscriptions"
        };

        public TwitchOptions(string clientId = RSModsClientId)
        {
            if (string.IsNullOrWhiteSpace(clientId))
                throw new ArgumentException("A Twitch client ID is required.", nameof(clientId));

            ClientId = clientId;
        }

        public string ClientId { get; }
        public IReadOnlyList<string> Scopes => RequiredScopes;
        public Uri DeviceAuthorizationEndpoint { get; set; } = new Uri("https://id.twitch.tv/oauth2/device");
        public Uri TokenEndpoint { get; set; } = new Uri("https://id.twitch.tv/oauth2/token");
        public Uri TokenValidationEndpoint { get; set; } = new Uri("https://id.twitch.tv/oauth2/validate");
        public Uri UsersEndpoint { get; set; } = new Uri("https://api.twitch.tv/helix/users");
        public Uri EventSubSubscriptionsEndpoint { get; set; } = new Uri("https://api.twitch.tv/helix/eventsub/subscriptions");
        public Uri EventSubWebSocketEndpoint { get; set; } = new Uri("wss://eventsub.wss.twitch.tv/ws?keepalive_timeout_seconds=30");
        public TimeSpan ValidationInterval { get; set; } = TimeSpan.FromHours(1);
        public TimeSpan RefreshBeforeExpiry { get; set; } = TimeSpan.FromMinutes(5);
        public TimeSpan NotificationDeduplicationTtl { get; set; } = TimeSpan.FromHours(1);
        public int NotificationDeduplicationCapacity { get; set; } = 4096;
        public int LogCapacity { get; set; } = 500;

        public bool HasRequiredScopes(IEnumerable<string> scopes)
        {
            var granted = new HashSet<string>(scopes ?? [], StringComparer.OrdinalIgnoreCase);
            return RequiredScopes.All(granted.Contains);
        }
    }
}
