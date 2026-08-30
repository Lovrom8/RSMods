using RSMods.Twitch;
using RSMods.Twitch.EffectServer;
using System.Net;
using System.Net.Http;

namespace RSMods.Core.Tests;

public sealed class TwitchServiceTests
{
    [Fact]
    public async Task Start_ValidatesStoredToken_ObtainsIdentity_AndStartsOneEventSubSession()
    {
        if (!OperatingSystem.IsWindows())
            return;

        using var temporary = new TemporaryDirectory();
        string tokenPath = temporary.File("TwitchAuth.dat");
        string rewardsPath = temporary.File("TwitchEnabledEffects.xml");
        var tokenStore = new TwitchTokenStore(tokenPath);
        tokenStore.Save(new TwitchTokenSet
        {
            AccessToken = "stored-access",
            RefreshToken = "stored-refresh",
            ClientId = "public-client",
            ExpiresAtUtc = DateTimeOffset.UtcNow.AddHours(2),
            Scopes = TwitchOptions.RequiredScopes
        });
        var handler = new QueueHttpMessageHandler();
        handler.Enqueue(HttpStatusCode.OK, """
            {"client_id":"public-client","login":"streamer","user_id":"1234","expires_in":7200,"scopes":["bits:read","channel:read:redemptions","channel:read:subscriptions"]}
            """);
        handler.Enqueue(HttpStatusCode.OK, """
            {"data":[{"id":"1234","login":"streamer","display_name":"Streamer"}]}
            """);
        var options = new TwitchOptions("public-client");
        var api = new TwitchApiClient(new HttpClient(handler), options);
        var eventSub = new FakeEventSubClient();
        await using var service = new TwitchService(
            options,
            tokenStore,
            new TwitchRewardRepository(rewardsPath),
            new TwitchAuthService(new HttpClient(handler), options),
            api,
            eventSub,
            new RocksmithEffectServer(new RocksmithEffectServerOptions { Port = 0 }));

        await service.StartAsync();

        Assert.Equal(TwitchAuthenticationState.Authorized, service.AuthenticationState);
        Assert.Equal(TwitchEventSubState.Connected, service.EventSubState);
        Assert.Equal("Streamer", service.Identity.DisplayName);
        Assert.Equal(1, eventSub.StartCount);
        Assert.Equal("stored-access", eventSub.AccessToken);
        Assert.Equal("1234", eventSub.BroadcasterUserId);
        Assert.DoesNotContain(service.GetLogSnapshot(), entry =>
            entry.Message.Contains("stored-access", StringComparison.Ordinal) ||
            entry.Message.Contains("stored-refresh", StringComparison.Ordinal));

        await service.ForgetAuthorizationAsync();

        Assert.Null(tokenStore.Load());
        Assert.Equal(TwitchAuthenticationState.NotAuthorized, service.AuthenticationState);
        Assert.Equal(TwitchEventSubState.Stopped, service.EventSubState);
    }

    [Fact]
    public async Task Start_MissingRequiredScope_ClearsTokenAndRequiresReauthorization()
    {
        if (!OperatingSystem.IsWindows())
            return;

        using var temporary = new TemporaryDirectory();
        string tokenPath = temporary.File("TwitchAuth.dat");
        var tokenStore = new TwitchTokenStore(tokenPath);
        tokenStore.Save(new TwitchTokenSet
        {
            AccessToken = "old-access",
            RefreshToken = "old-refresh",
            ClientId = "public-client",
            ExpiresAtUtc = DateTimeOffset.UtcNow.AddHours(2)
        });
        var handler = new QueueHttpMessageHandler();
        handler.Enqueue(HttpStatusCode.OK, """
            {"client_id":"public-client","login":"streamer","user_id":"1234","expires_in":7200,"scopes":["bits:read"]}
            """);
        var options = new TwitchOptions("public-client");
        var api = new TwitchApiClient(new HttpClient(handler), options);
        var eventSub = new FakeEventSubClient();
        await using var service = new TwitchService(
            options,
            tokenStore,
            new TwitchRewardRepository(temporary.File("TwitchEnabledEffects.xml")),
            new TwitchAuthService(new HttpClient(handler), options),
            api,
            eventSub,
            new RocksmithEffectServer(new RocksmithEffectServerOptions { Port = 0 }));

        await service.StartAsync();

        Assert.Equal(TwitchAuthenticationState.ReauthorizationRequired, service.AuthenticationState);
        Assert.Null(tokenStore.Load());
        Assert.Equal(0, eventSub.StartCount);
    }
}
