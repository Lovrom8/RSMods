using RSMods.Twitch;
using RSMods.Twitch.EffectServer;
using System.Net;
using System.Net.Http;

namespace RSMods.Core.Tests;

/// <summary>Token upkeep, listening and effect-bridge recovery over a session, driven by a manual clock.</summary>
public sealed class TwitchServiceSessionTests
{
    private static readonly DateTimeOffset Start = new(2026, 8, 11, 10, 0, 0, TimeSpan.Zero);

    private sealed class Harness : IAsyncDisposable
    {
        public Harness(int effectPort = 0, bool storeToken = true)
        {
            TokenStore = new TwitchTokenStore(Temporary.File("TwitchAuth.dat"));
            if (storeToken)
            {
                TokenStore.Save(new TwitchTokenSet
                {
                    AccessToken = "stored-access",
                    RefreshToken = "stored-refresh",
                    ClientId = "public-client",
                    ExpiresAtUtc = Start.AddHours(2),
                    Scopes = TwitchOptions.RequiredScopes
                });
            }
            var options = new TwitchOptions("public-client");
            Service = new TwitchService(
                options,
                TokenStore,
                new TwitchRewardRepository(Temporary.File("TwitchEnabledEffects.xml")),
                new TwitchAuthService(new HttpClient(Http), options, Clock),
                new TwitchApiClient(new HttpClient(Http), options, Clock),
                EventSub,
                new RocksmithEffectServer(new RocksmithEffectServerOptions { Port = effectPort }),
                Clock);
        }

        public TemporaryDirectory Temporary { get; } = new();
        public QueueHttpMessageHandler Http { get; } = new();
        public FakeEventSubClient EventSub { get; } = new();
        public ManualTwitchClock Clock { get; } = new(Start);
        public TwitchTokenStore TokenStore { get; }
        public TwitchService Service { get; }

        public void EnqueueValidation(int expiresIn = 7200) => Http.Enqueue(HttpStatusCode.OK, Validation(expiresIn));
        public void EnqueueUser() => Http.Enqueue(HttpStatusCode.OK, """{"data":[{"id":"1234","login":"streamer","display_name":"Streamer"}]}""");
        public void EnqueueRefresh(string access, string refresh) => Http.Enqueue(HttpStatusCode.OK,
            $$"""{"access_token":"{{access}}","refresh_token":"{{refresh}}","expires_in":14400,"scope":["bits:read","channel:read:redemptions","channel:read:subscriptions"]}""");

        public async Task StartAuthorizedAsync(int expiresIn = 7200)
        {
            EnqueueValidation(expiresIn);
            EnqueueUser();
            await Service.StartAsync();
            Assert.Equal(TwitchAuthenticationState.Authorized, Service.AuthenticationState);
        }

        /// <summary>Waits for maintenance to sleep again, then wakes it.</summary>
        public async Task RunMaintenanceAsync()
        {
            await TwitchTestWait.UntilAsync(() => Clock.PendingDelays.Count > 0, "maintenance to wait");
            Clock.Advance(Clock.PendingDelays.Max());
        }

        public async ValueTask DisposeAsync()
        {
            await Service.DisposeAsync();
            Temporary.Dispose();
        }
    }

    private static string Validation(int expiresIn) =>
        $$"""{"client_id":"public-client","login":"streamer","user_id":"1234","expires_in":{{expiresIn}},"scopes":["bits:read","channel:read:redemptions","channel:read:subscriptions"]}""";

    [Fact]
    public async Task Maintenance_RefreshesBeforeTheTokenExpires_NotAtTheNextHourlyCheck()
    {
        if (!OperatingSystem.IsWindows())
            return;

        await using var harness = new Harness();
        await harness.StartAuthorizedAsync(expiresIn: 20 * 60);

        // Twenty minutes left: wake at five before expiry, not after the hour, when it would long have expired.
        await TwitchTestWait.UntilAsync(() => harness.Clock.PendingDelays.Count > 0, "maintenance to wait");
        Assert.Equal(TimeSpan.FromMinutes(15), Assert.Single(harness.Clock.PendingDelays));

        harness.EnqueueRefresh("fresh-access", "fresh-refresh");
        harness.EnqueueValidation();
        harness.EnqueueUser();
        harness.Clock.Advance(TimeSpan.FromMinutes(15));

        // Polling the file itself would race the service writing it.
        await TwitchTestWait.UntilAsync(() => harness.Http.Requests.Count == 5, "the refresh and validation");
        await TwitchTestWait.UntilAsync(() => harness.Clock.PendingDelays.Count > 0, "maintenance to wait again");
        Assert.Equal("fresh-access", harness.TokenStore.Load()?.AccessToken);
        Assert.Equal("fresh-refresh", harness.TokenStore.Load()!.RefreshToken);
        Assert.Equal(TwitchAuthenticationState.Authorized, harness.Service.AuthenticationState);
    }

    [Fact]
    public async Task EventSubCredentials_RefreshARejectedToken_AndKeepTheLogin()
    {
        if (!OperatingSystem.IsWindows())
            return;

        await using var harness = new Harness();
        await harness.StartAuthorizedAsync();
        TwitchEventSubCredentials first = await harness.EventSub.GetCredentialsAsync();
        Assert.Equal("stored-access", first.AccessToken);

        harness.EnqueueRefresh("fresh-access", "fresh-refresh");
        TwitchEventSubCredentials second = await harness.EventSub.GetCredentialsAsync(first.AccessToken);

        Assert.Equal("fresh-access", second.AccessToken);
        Assert.Equal("1234", second.BroadcasterUserId);
        Assert.Equal("fresh-access", harness.TokenStore.Load()?.AccessToken);
        Assert.Equal(TwitchAuthenticationState.Authorized, harness.Service.AuthenticationState);
    }

    [Fact]
    public async Task EventSubCredentials_RefreshRefused_RequiresReauthorization()
    {
        if (!OperatingSystem.IsWindows())
            return;

        await using var harness = new Harness();
        await harness.StartAuthorizedAsync();

        harness.Http.Enqueue(HttpStatusCode.BadRequest, """{"message":"Invalid refresh token"}""");
        TwitchEventSubCredentials? credentials = await harness.EventSub.GetCredentialsAsync("stored-access");

        Assert.Null(credentials);
        Assert.Null(harness.TokenStore.Load());
        Assert.Equal(TwitchAuthenticationState.ReauthorizationRequired, harness.Service.AuthenticationState);
    }

    [Fact]
    public async Task Maintenance_SurvivesAnHttpClientTimeout_AndRecovers()
    {
        if (!OperatingSystem.IsWindows())
            return;

        await using var harness = new Harness();
        await harness.StartAuthorizedAsync();

        // HttpClient reports its timeout as a TaskCanceledException though nothing was cancelled.
        harness.Http.Enqueue(_ => throw new TaskCanceledException("The request was canceled due to the configured HttpClient.Timeout."));
        await harness.RunMaintenanceAsync();
        await TwitchTestWait.UntilAsync(() => harness.Service.AuthenticationState == TwitchAuthenticationState.Error, "the error state");

        harness.EnqueueValidation();
        harness.EnqueueUser();
        await harness.RunMaintenanceAsync();
        await TwitchTestWait.UntilAsync(() => harness.Service.AuthenticationState == TwitchAuthenticationState.Authorized, "recovery");
    }

    [Fact]
    public async Task Start_UnexpectedValidationFailure_LeavesMaintenanceToRetry()
    {
        if (!OperatingSystem.IsWindows())
            return;

        await using var harness = new Harness();
        harness.Http.Enqueue(_ => throw new InvalidOperationException("Something nobody planned for."));

        await harness.Service.StartAsync();
        Assert.Equal(TwitchAuthenticationState.Error, harness.Service.AuthenticationState);

        harness.EnqueueValidation();
        harness.EnqueueUser();
        await harness.RunMaintenanceAsync();
        await TwitchTestWait.UntilAsync(() => harness.Service.AuthenticationState == TwitchAuthenticationState.Authorized, "recovery");
        await TwitchTestWait.UntilAsync(() => harness.EventSub.StartCount == 1, "listening to start");
    }

    [Fact]
    public async Task Maintenance_RestartsAFailedSession_ButNotOneTheUserStopped()
    {
        if (!OperatingSystem.IsWindows())
            return;

        await using var harness = new Harness();
        await harness.StartAuthorizedAsync();
        Assert.Equal(1, harness.EventSub.StartCount);

        harness.EventSub.SetState(TwitchEventSubState.Error, "Lost.");
        harness.EnqueueValidation();
        harness.EnqueueUser();
        await harness.RunMaintenanceAsync();
        await TwitchTestWait.UntilAsync(() => harness.EventSub.StartCount == 2, "the failed session to restart");

        await harness.Service.StopListeningAsync();
        harness.EnqueueValidation();
        harness.EnqueueUser();
        await harness.RunMaintenanceAsync();
        await TwitchTestWait.UntilAsync(() => harness.Http.Requests.Count == 6, "the second validation");
        await TwitchTestWait.UntilAsync(() => harness.Clock.PendingDelays.Count > 0, "maintenance to wait again");

        Assert.Equal(2, harness.EventSub.StartCount);
        Assert.Equal(TwitchEventSubState.Stopped, harness.Service.EventSubState);
    }

    [Fact]
    public async Task Forget_DuringValidation_IsNotUndoneWhenTheValidationFinishes()
    {
        if (!OperatingSystem.IsWindows())
            return;

        await using var harness = new Harness();
        await harness.StartAuthorizedAsync();
        var validationArrived = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var releaseValidation = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        harness.Http.EnqueueAsync(async _ =>
        {
            validationArrived.TrySetResult();
            await releaseValidation.Task;
            return new HttpResponseMessage(HttpStatusCode.OK) { Content = new StringContent(Validation(7200)) };
        });
        harness.EnqueueUser();

        await harness.RunMaintenanceAsync();
        await validationArrived.Task.WaitAsync(TimeSpan.FromSeconds(5));
        Task forget = harness.Service.ForgetAuthorizationAsync();
        releaseValidation.SetResult();
        await forget.WaitAsync(TimeSpan.FromSeconds(5));

        Assert.Null(harness.TokenStore.Load());
        Assert.Equal(TwitchAuthenticationState.NotAuthorized, harness.Service.AuthenticationState);
    }

    [Fact]
    public async Task Revocation_DuringValidation_IsNotUndoneWhenTheValidationFinishes()
    {
        if (!OperatingSystem.IsWindows())
            return;

        await using var harness = new Harness();
        await harness.StartAuthorizedAsync();
        var validationArrived = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        var releaseValidation = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        harness.Http.EnqueueAsync(async _ =>
        {
            validationArrived.TrySetResult();
            await releaseValidation.Task;
            return new HttpResponseMessage(HttpStatusCode.OK) { Content = new StringContent(Validation(7200)) };
        });
        harness.EnqueueUser();

        await harness.RunMaintenanceAsync();
        await validationArrived.Task.WaitAsync(TimeSpan.FromSeconds(5));
        harness.EventSub.Revoke("channel.cheer", "authorization_revoked");
        releaseValidation.SetResult();
        await TwitchTestWait.UntilAsync(() => harness.Clock.PendingDelays.Count > 0, "maintenance to wait again");

        Assert.Null(harness.TokenStore.Load());
        Assert.Equal(TwitchAuthenticationState.ReauthorizationRequired, harness.Service.AuthenticationState);
    }

    [Fact]
    public async Task EffectBridge_RetriesTheListenerUntilThePortIsFree()
    {
        var blocker = new System.Net.Sockets.TcpListener(IPAddress.Loopback, 0);
        blocker.Start();
        int takenPort = ((IPEndPoint)blocker.LocalEndpoint).Port;
        try
        {
            await using var harness = new Harness(takenPort, storeToken: false);
            await harness.Service.StartAsync();
            Assert.True(harness.Service.IsEffectBridgeUnavailable);

            // Like a restart: the old instance lets go of the port a moment later.
            blocker.Stop();
            harness.Clock.Advance(TimeSpan.FromSeconds(1));

            await TwitchTestWait.UntilAsync(() => !harness.Service.IsEffectBridgeUnavailable, "the bridge to listen");
        }
        finally
        {
            blocker.Stop();
        }
    }
}
