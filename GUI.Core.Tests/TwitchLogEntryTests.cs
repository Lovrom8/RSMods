using RSMods.Twitch;

namespace RSMods.Core.Tests;

public sealed class TwitchLogEntryTests
{
    [Fact]
    public void ShowsTheTimeInLocalTimeNotUtc()
    {
        // The service stamps entries from a UTC clock; the log reads in the user's own time zone.
        var utc = new DateTimeOffset(2026, 8, 11, 10, 0, 0, TimeSpan.Zero);
        var entry = new TwitchLogEntry(utc, TwitchLogLevel.Information, "Connected");

        Assert.Equal(TimeZoneInfo.Local.GetUtcOffset(utc), entry.LocalTimestamp.Offset);
        Assert.Equal(utc, entry.LocalTimestamp);
        Assert.Equal($"[{utc.ToLocalTime():HH:mm:ss}] Connected", entry.ToString());
    }
}
