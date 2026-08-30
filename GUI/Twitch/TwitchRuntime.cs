using RSMods.Twitch.EffectServer;
using RSMods.Data;
using System;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace RSMods.Twitch
{
    /// <summary>WinForms adapter for the application-scoped Core Twitch runtime.</summary>
    internal static class TwitchRuntime
    {
        private static readonly object Sync = new object();
        private static readonly HttpClient HttpClient = new HttpClient();
        private static Control _dispatcher;
        private static TwitchService _service;

        public static TwitchService Service
        {
            get
            {
                lock (Sync)
                    return _service ?? throw new InvalidOperationException("The Twitch runtime has not been initialized.");
            }
        }

        public static void Initialize(Control dispatcher)
        {
            if (dispatcher == null)
                throw new ArgumentNullException(nameof(dispatcher));

            lock (Sync)
            {
                if (_service != null)
                    return;

                _dispatcher = dispatcher;
                var options = new TwitchOptions();
                var apiClient = new TwitchApiClient(HttpClient, options);
                var authService = new TwitchAuthService(HttpClient, options);
                var eventSub = new TwitchEventSubClient(apiClient, options);
                var effectServer = new RocksmithEffectServer();
                _service = new TwitchService(
                    options,
                    new TwitchTokenStore(Constants.TwitchTokenPath),
                    new TwitchRewardRepository(Constants.TwitchRewardsPath),
                    authService,
                    apiClient,
                    eventSub,
                    effectServer);
                _service.StateChanged += OnStateChanged;
                _service.LogReceived += OnLogReceived;
                _service.SetRewards(TwitchSettings.Get.Rewards);
            }
        }

        public static Task StartAsync(CancellationToken cancellationToken = default(CancellationToken)) =>
            Service.StartAsync(cancellationToken);

        public static bool TryQueueReward(TwitchReward reward, string viewer = "rsmods") =>
            Service.TryQueueReward(reward, viewer);

        public static void RewardsChanged() => Service.SetRewards(TwitchSettings.Get.Rewards);

        public static async Task StopAsync()
        {
            TwitchService service;
            lock (Sync)
                service = _service;
            if (service == null)
                return;

            await service.DisposeAsync();
            lock (Sync)
            {
                if (ReferenceEquals(_service, service))
                    _service = null;
            }
        }

        private static void OnStateChanged(object sender, TwitchStateChangedEventArgs e)
        {
            Dispatch(() =>
            {
                TwitchSettings settings = TwitchSettings.Get;
                settings.Authorized = e.AuthenticationState == TwitchAuthenticationState.Authorized;
                settings.AuthorizationStored = sender is TwitchService service && service.HasStoredAuthorization;
                settings.Username = e.Identity?.DisplayName ?? e.Identity?.Login ?? string.Empty;
                settings.ChannelID = e.Identity?.UserId ?? string.Empty;
                settings.ListeningStatus = DescribeStatus(e);
                if (e.Identity != null)
                    settings.SaveSettings();
            });
        }

        private static void OnLogReceived(object sender, TwitchLogEventArgs e) =>
            Dispatch(() => TwitchSettings.Get.AddToLog(e.Entry.ToString()));

        private static string DescribeStatus(TwitchStateChangedEventArgs state)
        {
            if (state.AuthenticationState == TwitchAuthenticationState.Authorizing)
                return "Waiting for Twitch authorization";
            if (state.AuthenticationState == TwitchAuthenticationState.ReauthorizationRequired)
                return "Twitch reauthorization required";
            if (state.AuthenticationState == TwitchAuthenticationState.Error)
                return "Twitch unavailable; retrying";
            if (state.AuthenticationState != TwitchAuthenticationState.Authorized)
                return "Not authorized with Twitch";

            switch (state.EventSubState)
            {
                case TwitchEventSubState.Connected:
                    return "Listening to Twitch events";
                case TwitchEventSubState.Connecting:
                    return "Connecting to Twitch events";
                case TwitchEventSubState.Reconnecting:
                    return "Reconnecting to Twitch events";
                case TwitchEventSubState.Revoked:
                    return "Twitch reauthorization required";
                case TwitchEventSubState.Error:
                    return "Twitch event connection failed";
                default:
                    return "Authorized; Twitch listening is stopped";
            }
        }

        private static void Dispatch(Action action)
        {
            Control dispatcher = _dispatcher;
            if (dispatcher == null || dispatcher.IsDisposed)
                return;

            if (dispatcher.InvokeRequired)
                dispatcher.BeginInvoke(action);
            else
                action();
        }
    }
}
