using System;

namespace RSMods.Twitch
{
    public enum TwitchAuthenticationState
    {
        NotAuthorized,
        Authorizing,
        Authorized,
        ReauthorizationRequired,
        Error
    }

    public enum TwitchEventSubState
    {
        Stopped,
        Connecting,
        Connected,
        Reconnecting,
        Revoked,
        Error
    }

    public enum TwitchLogLevel
    {
        Information,
        Warning,
        Error
    }

    public sealed class TwitchIdentity
    {
        public string UserId { get; set; }
        public string Login { get; set; }
        public string DisplayName { get; set; }
    }

    public sealed class TwitchLogEntry(DateTimeOffset timestamp, TwitchLogLevel level, string message)
    {
        public DateTimeOffset Timestamp { get; } = timestamp;
        public TwitchLogLevel Level { get; } = level;
        public string Message { get; } = message ?? string.Empty;

        /// <summary>When it happened in the user's own time zone; <see cref="Timestamp"/> comes from a UTC clock.</summary>
        public DateTimeOffset LocalTimestamp => Timestamp.ToLocalTime();

        // The shown and saved log both read in local time.
        public override string ToString() => $"[{LocalTimestamp:HH:mm:ss}] {Message}";
    }

    public sealed class TwitchStateChangedEventArgs(
        TwitchAuthenticationState authenticationState,
        TwitchEventSubState eventSubState,
        TwitchIdentity identity,
        string detail) : EventArgs
    {
        public TwitchAuthenticationState AuthenticationState { get; } = authenticationState;
        public TwitchEventSubState EventSubState { get; } = eventSubState;
        public TwitchIdentity Identity { get; } = identity;
        public string Detail { get; } = detail ?? string.Empty;
    }

    public sealed class TwitchLogEventArgs(TwitchLogEntry entry) : EventArgs
    {
        public TwitchLogEntry Entry { get; } = entry ?? throw new ArgumentNullException(nameof(entry));
    }

    public sealed class TwitchTriggerEventArgs(TwitchTriggerEvent trigger) : EventArgs
    {
        public TwitchTriggerEvent Trigger { get; } = trigger ?? throw new ArgumentNullException(nameof(trigger));
    }

    public sealed class TwitchRevocationEventArgs(string subscriptionType, string status) : EventArgs
    {
        public string SubscriptionType { get; } = subscriptionType ?? string.Empty;
        public string Status { get; } = status ?? string.Empty;
    }
}
