using RSMods.Twitch;
using System.Net;
using System.Net.Http;

namespace RSMods.Core.Tests;

public sealed class TwitchEventSubTests
{
    private static readonly DateTimeOffset Timestamp = new(2026, 8, 11, 10, 0, 0, TimeSpan.Zero);

    [Theory]
    [InlineData("channel.cheer", "{\"user_name\":\"Cheerer\",\"bits\":100}", TwitchTriggerKind.Bits, 100, "Cheerer")]
    [InlineData("channel.channel_points_custom_reward_redemption.add", "{\"user_name\":\"Redeemer\",\"reward\":{\"cost\":250}}", TwitchTriggerKind.ChannelPoints, 250, "Redeemer")]
    [InlineData("channel.subscribe", "{\"user_name\":\"NewSub\",\"is_gift\":true}", TwitchTriggerKind.Subscription, 1, "NewSub")]
    [InlineData("channel.subscription.message", "{\"user_name\":\"Resub\",\"cumulative_months\":4}", TwitchTriggerKind.Subscription, 1, "Resub")]
    public void Parser_NormalizesSupportedNotifications(
        string subscriptionType,
        string eventJson,
        TwitchTriggerKind expectedKind,
        int expectedAmount,
        string expectedViewer)
    {
        TwitchEventSubEnvelope envelope = TwitchEventSubMessageParser.ParseEnvelope(
            Notification("event-1", subscriptionType, eventJson), Timestamp);

        bool parsed = TwitchEventSubMessageParser.TryGetTrigger(envelope, out TwitchTriggerEvent? trigger);

        Assert.True(parsed);
        Assert.NotNull(trigger);
        Assert.Equal(expectedKind, trigger.Kind);
        Assert.Equal(expectedAmount, trigger.Amount);
        Assert.Equal(expectedViewer, trigger.Viewer);
        Assert.Equal("event-1", trigger.EventId);
    }

    [Fact]
    public void DeduplicationCache_IsBounded_AndAllowsIdsAfterTtl()
    {
        var cache = new BoundedEventIdCache(2, TimeSpan.FromMinutes(5));

        Assert.True(cache.TryAdd("one", Timestamp));
        Assert.False(cache.TryAdd("one", Timestamp.AddMinutes(1)));
        Assert.True(cache.TryAdd("two", Timestamp));
        Assert.True(cache.TryAdd("three", Timestamp));
        Assert.True(cache.TryAdd("one", Timestamp.AddMinutes(1)));
        Assert.True(cache.TryAdd("one", Timestamp.AddMinutes(10)));
    }

    [Fact]
    public async Task Client_DeduplicatesNotifications_AndDoesNotResubscribeOnPlannedReconnect()
    {
        const string reconnectUrl = "wss://eventsub.wss.twitch.tv/reconnect/session";
        string notification = Notification("same-id", "channel.cheer", "{\"user_name\":\"Viewer\",\"bits\":100}");
        var first = new FakeTwitchWebSocket(
            Welcome("first-session"),
            notification,
            notification,
            Reconnect(reconnectUrl));
        var second = new FakeTwitchWebSocket(Welcome("second-session"));
        var sockets = new FakeTwitchWebSocketFactory(first, second);
        var handler = new QueueHttpMessageHandler();
        for (int index = 0; index < 4; index++)
            handler.Enqueue(HttpStatusCode.Accepted, "{}" );
        var options = new TwitchOptions("public-client");
        var api = new TwitchApiClient(new HttpClient(handler), options);
        await using var client = new TwitchEventSubClient(api, options, sockets);
        var triggerReceived = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);
        var reconnected = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);
        int triggers = 0;
        int connectedStates = 0;
        client.TriggerReceived += (_, _) =>
        {
            Interlocked.Increment(ref triggers);
            triggerReceived.TrySetResult(true);
        };
        client.StateChanged += (_, args) =>
        {
            if (args.State == TwitchEventSubState.Connected && Interlocked.Increment(ref connectedStates) == 2)
                reconnected.TrySetResult(true);
        };

        await client.StartAsync("access", "broadcaster");
        await triggerReceived.Task.WaitAsync(TimeSpan.FromSeconds(5));
        await reconnected.Task.WaitAsync(TimeSpan.FromSeconds(5));
        await client.StopAsync();

        Assert.Equal(1, triggers);
        Assert.Equal(4, handler.Requests.Count);
        Assert.Equal(2, sockets.ConnectedUris.Count);
        Assert.Equal(reconnectUrl, sockets.ConnectedUris[1].AbsoluteUri);
    }

    private static string Welcome(string sessionId) =>
        "{\"metadata\":{\"message_id\":\"welcome-" + sessionId +
        "\",\"message_type\":\"session_welcome\",\"message_timestamp\":\"2026-08-11T10:00:00Z\"}," +
        "\"payload\":{\"session\":{\"id\":\"" + sessionId + "\",\"keepalive_timeout_seconds\":30}}}";

    private static string Reconnect(string uri) =>
        "{\"metadata\":{\"message_id\":\"reconnect\",\"message_type\":\"session_reconnect\"," +
        "\"message_timestamp\":\"2026-08-11T10:00:00Z\"},\"payload\":{\"session\":{\"reconnect_url\":\"" + uri + "\"}}}";

    private static string Notification(string id, string type, string eventJson) =>
        $"{{\"metadata\":{{\"message_id\":\"{id}\",\"message_type\":\"notification\",\"message_timestamp\":\"2026-08-11T10:00:00Z\",\"subscription_type\":\"{type}\"}},\"payload\":{{\"event\":{eventJson}}}}}";
}
