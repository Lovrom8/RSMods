using RSMods.Twitch;
using System.Net;
using System.Net.Http;

namespace RSMods.Core.Tests;

public sealed class TwitchAuthServiceTests
{
    [Fact]
    public async Task DeviceCodeFlow_PollsAtRequestedInterval_AndReturnsRefreshableTokens()
    {
        var handler = new QueueHttpMessageHandler();
        handler.Enqueue(HttpStatusCode.OK, """
            {"device_code":"private-device-code","user_code":"ABCD1234","verification_uri":"https://www.twitch.tv/activate","expires_in":60,"interval":3}
            """);
        handler.Enqueue(HttpStatusCode.BadRequest, """{"status":400,"message":"authorization_pending"}""");
        handler.Enqueue(HttpStatusCode.OK, """
            {"access_token":"new-access","refresh_token":"new-refresh","expires_in":14400,"scope":["bits:read","channel:read:redemptions","channel:read:subscriptions"],"token_type":"bearer"}
            """);
        var clock = new AdvancingTwitchClock(new DateTimeOffset(2026, 8, 11, 10, 0, 0, TimeSpan.Zero));
        var options = new TwitchOptions("public-client");
        var service = new TwitchAuthService(new HttpClient(handler), options, clock);

        TwitchDeviceAuthorization authorization = await service.StartDeviceAuthorizationAsync(CancellationToken.None);
        TwitchTokenSet tokens = await service.PollForAuthorizationAsync(authorization, CancellationToken.None);

        Assert.Equal("ABCD1234", authorization.UserCode);
        Assert.Equal(TimeSpan.FromSeconds(3), authorization.PollingInterval);
        Assert.Equal([TimeSpan.FromSeconds(3), TimeSpan.FromSeconds(3)], clock.Delays);
        Assert.Equal("new-access", tokens.AccessToken);
        Assert.Equal("new-refresh", tokens.RefreshToken);
        Assert.Equal("public-client", tokens.ClientId);
        Assert.Equal(TwitchOptions.RequiredScopes, tokens.Scopes);
        Assert.DoesNotContain(handler.Requests, request => request.Body.Contains("client_secret", StringComparison.Ordinal));
    }

    [Fact]
    public async Task ApiClient_RefreshesPublicToken_AndCreatesExactlyFourWebSocketSubscriptions()
    {
        var handler = new QueueHttpMessageHandler();
        handler.Enqueue(HttpStatusCode.OK, """
            {"access_token":"rotated-access","refresh_token":"rotated-refresh","expires_in":14400,"scope":["bits:read","channel:read:redemptions","channel:read:subscriptions"]}
            """);
        for (int index = 0; index < 4; index++)
            handler.Enqueue(HttpStatusCode.Accepted, """{"data":[]}""");
        var options = new TwitchOptions("public-client");
        var client = new TwitchApiClient(new HttpClient(handler), options);

        TwitchTokenSet refreshed = await client.RefreshTokenAsync("one-use-refresh", CancellationToken.None);
        await client.CreateEventSubSubscriptionsAsync("access", "broadcaster", "session", CancellationToken.None);

        Assert.Equal("rotated-refresh", refreshed.RefreshToken);
        Assert.Equal(5, handler.Requests.Count);
        string refreshForm = handler.Requests[0].Body;
        Assert.Contains("grant_type=refresh_token", refreshForm);
        Assert.DoesNotContain("client_secret", refreshForm);
        string[] bodies = handler.Requests.Skip(1).Select(request => request.Body).ToArray();
        Assert.Contains(bodies, body => body.Contains("channel.cheer", StringComparison.Ordinal));
        Assert.Contains(bodies, body => body.Contains("channel.channel_points_custom_reward_redemption.add", StringComparison.Ordinal));
        Assert.Contains(bodies, body => body.Contains("channel.subscribe", StringComparison.Ordinal));
        Assert.Contains(bodies, body => body.Contains("channel.subscription.message", StringComparison.Ordinal));
    }
}
