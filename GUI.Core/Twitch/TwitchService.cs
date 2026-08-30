using RSMods.Twitch.EffectServer;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;
using RSMods.Util;

namespace RSMods.Twitch
{
    /// <summary>
    /// Application-scoped owner for Twitch authorization, EventSub, reward matching, and the
    /// Rocksmith effect bridge. Frontends subscribe to state/log events and never own network loops.
    /// </summary>
    public sealed class TwitchService : IAsyncDisposable, IDisposable
    {
        private readonly TwitchOptions _options;
        private readonly TwitchTokenStore _tokenStore;
        private readonly TwitchRewardRepository _rewardRepository;
        private readonly TwitchAuthService _authService;
        private readonly TwitchApiClient _apiClient;
        private readonly ITwitchEventSubClient _eventSub;
        private readonly RocksmithEffectServer _effectServer;
        private readonly ITwitchClock _clock;
        private readonly SemaphoreSlim _lifecycleGate = new SemaphoreSlim(1, 1);
        private readonly SemaphoreSlim _sessionGate = new SemaphoreSlim(1, 1);
        private readonly object _stateSync = new object();
        private readonly object _rewardSync = new object();
        private readonly Queue<TwitchLogEntry> _logs = new Queue<TwitchLogEntry>();
        private List<TwitchReward> _rewards = new List<TwitchReward>();
        private CancellationTokenSource _lifetime;
        private Task _maintenanceTask;
        private TwitchTokenSet _tokens;
        private TwitchIdentity _identity;
        private TwitchAuthenticationState _authenticationState = TwitchAuthenticationState.NotAuthorized;
        private TwitchEventSubState _eventSubState = TwitchEventSubState.Stopped;
        private string _stateDetail = string.Empty;
        private bool _started;
        private bool _disposed;
        private bool? _turboSpeedEnabled;

        public TwitchService(
            TwitchOptions options,
            TwitchTokenStore tokenStore,
            TwitchRewardRepository rewardRepository,
            TwitchAuthService authService,
            TwitchApiClient apiClient,
            ITwitchEventSubClient eventSub,
            RocksmithEffectServer effectServer,
            ITwitchClock clock = null)
        {
            _options = options ?? throw new ArgumentNullException(nameof(options));
            _tokenStore = tokenStore ?? throw new ArgumentNullException(nameof(tokenStore));
            _rewardRepository = rewardRepository ?? throw new ArgumentNullException(nameof(rewardRepository));
            _authService = authService ?? throw new ArgumentNullException(nameof(authService));
            _apiClient = apiClient ?? throw new ArgumentNullException(nameof(apiClient));
            _eventSub = eventSub ?? throw new ArgumentNullException(nameof(eventSub));
            _effectServer = effectServer ?? throw new ArgumentNullException(nameof(effectServer));
            _clock = clock ?? SystemTwitchClock.Instance;

            _eventSub.StateChanged += OnEventSubStateChanged;
            _eventSub.TriggerReceived += OnTriggerReceived;
            _eventSub.Revoked += OnEventSubRevoked;
            _effectServer.LogMessage += OnEffectServerLog;
            _effectServer.ConnectionStateChanged += OnEffectServerConnectionChanged;
        }

        public event EventHandler<TwitchStateChangedEventArgs> StateChanged;
        public event EventHandler<TwitchLogEventArgs> LogReceived;
        public event EventHandler<TwitchTriggerEventArgs> TriggerReceived;

        public TwitchAuthenticationState AuthenticationState
        {
            get { lock (_stateSync) return _authenticationState; }
        }

        public TwitchEventSubState EventSubState
        {
            get { lock (_stateSync) return _eventSubState; }
        }

        public TwitchIdentity Identity
        {
            get { lock (_stateSync) return _identity; }
        }

        public bool IsRocksmithConnected => _effectServer.IsConnected;
        public bool HasStoredAuthorization => _tokenStore.Load() != null;

        public IReadOnlyList<TwitchLogEntry> GetLogSnapshot()
        {
            lock (_stateSync)
                return [.. _logs];
        }

        public void ClearLog()
        {
            lock (_stateSync)
                _logs.Clear();
        }

        public async Task StartAsync(CancellationToken cancellationToken = default(CancellationToken))
        {
            await _lifecycleGate.WaitAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                ThrowIfDisposed();
                if (_started)
                    return;

                _started = true;
                _lifetime = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
                try
                {
                    ReloadRewards();
                }
                catch (Exception ex) when (ex is System.IO.IOException || ex is InvalidOperationException)
                {
                    lock (_rewardSync)
                        _rewards = new List<TwitchReward>();
                    Log(TwitchLogLevel.Error, "Unable to load Twitch reward rules; no Twitch rewards will run.");
                }
                try
                {
                    await _effectServer.StartAsync(_lifetime.Token).ConfigureAwait(false);
                }
                catch (Exception ex) when (ex is System.IO.IOException || ex is System.Net.Sockets.SocketException)
                {
                    Log(TwitchLogLevel.Error, "Unable to start the Rocksmith effect bridge.");
                }
                _tokens = _tokenStore.Load();

                if (_tokens == null)
                {
                    SetAuthenticationState(TwitchAuthenticationState.NotAuthorized, null, "Authorize Twitch to start listening.");
                }
                else
                {
                    try
                    {
                        await ActivateStoredAuthorizationAsync(_lifetime.Token).ConfigureAwait(false);
                    }
                    catch (Exception ex) when (IsTransient(ex))
                    {
                        SetAuthenticationState(TwitchAuthenticationState.Error, null, "Twitch is currently unreachable; RSMods will retry.");
                        Log(TwitchLogLevel.Warning, "Twitch is currently unreachable; authorization validation will retry.");
                    }
                }

                _maintenanceTask = MaintainAuthorizationAsync(_lifetime.Token);
            }
            finally
            {
                _lifecycleGate.Release();
            }
        }

        public async Task<TwitchDeviceAuthorization> BeginAuthorizationAsync(CancellationToken cancellationToken)
        {
            ThrowIfDisposed();
            SetAuthenticationState(TwitchAuthenticationState.Authorizing, _identity, "Waiting for Twitch authorization.");
            await _eventSub.StopAsync().ConfigureAwait(false);
            try
            {
                TwitchDeviceAuthorization authorization = await _authService
                    .StartDeviceAuthorizationAsync(cancellationToken)
                    .ConfigureAwait(false);
                Log(TwitchLogLevel.Information, "Twitch device authorization started.");
                return authorization;
            }
            catch
            {
                await RestoreAuthorizationAfterFlowAsync().ConfigureAwait(false);
                throw;
            }
        }

        public async Task CompleteAuthorizationAsync(
            TwitchDeviceAuthorization authorization,
            CancellationToken cancellationToken)
        {
            ThrowIfDisposed();
            try
            {
                TwitchTokenSet tokens = await _authService
                    .PollForAuthorizationAsync(authorization, cancellationToken)
                    .ConfigureAwait(false);
                if (!_options.HasRequiredScopes(tokens.Scopes))
                    throw new TwitchAuthorizationException("Twitch did not grant all permissions required by RSMods.");

                tokens.ClientId = _options.ClientId;
                _tokenStore.Save(tokens);
                _tokens = tokens;
                await ActivateStoredAuthorizationAsync(cancellationToken).ConfigureAwait(false);
                Log(TwitchLogLevel.Information, "Twitch authorization completed.");
            }
            catch (OperationCanceledException)
            {
                await RestoreAuthorizationAfterFlowAsync().ConfigureAwait(false);
                throw;
            }
            catch (Exception ex) when (IsTransient(ex))
            {
                SetAuthenticationState(TwitchAuthenticationState.Error, null, "Twitch is currently unreachable; RSMods will retry.");
                Log(TwitchLogLevel.Warning, "Authorization succeeded, but Twitch identity validation is temporarily unavailable.");
                throw;
            }
            catch
            {
                SetAuthenticationState(TwitchAuthenticationState.ReauthorizationRequired, null, "Twitch authorization was not completed.");
                throw;
            }
        }

        public async Task StartListeningAsync(CancellationToken cancellationToken = default(CancellationToken))
        {
            ThrowIfDisposed();
            TwitchTokenSet tokens;
            TwitchIdentity identity;
            lock (_stateSync)
            {
                tokens = _tokens;
                identity = _identity;
            }

            if (tokens == null || identity == null || AuthenticationState != TwitchAuthenticationState.Authorized)
                throw new InvalidOperationException("Twitch authorization is required before listening can start.");

            await _eventSub.StartAsync(tokens.AccessToken, identity.UserId, cancellationToken).ConfigureAwait(false);
        }

        public Task StopListeningAsync() => _eventSub.StopAsync();

        public async Task ValidateNowAsync(CancellationToken cancellationToken = default(CancellationToken))
        {
            ThrowIfDisposed();
            await _sessionGate.WaitAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                if (_tokens != null)
                    await ValidateAndRefreshAsync(cancellationToken).ConfigureAwait(false);
            }
            finally
            {
                _sessionGate.Release();
            }
        }

        public async Task ForgetAuthorizationAsync()
        {
            ThrowIfDisposed();
            await _eventSub.StopAsync().ConfigureAwait(false);
            _tokenStore.Clear();
            lock (_stateSync)
            {
                _tokens = null;
                _identity = null;
            }
            SetAuthenticationState(TwitchAuthenticationState.NotAuthorized, null, "Twitch authorization was removed.");
            Log(TwitchLogLevel.Information, "Forgot Twitch authorization.");
        }

        public bool TryQueueReward(TwitchReward reward, string viewer = "rsmods")
        {
            bool queued = _effectServer.TryQueueEffect(reward, viewer);
            Log(queued ? TwitchLogLevel.Information : TwitchLogLevel.Warning,
                queued ? $"Queued: {reward?.Name}" : $"Unable to queue: {reward?.Name}");
            return queued;
        }

        public void ReloadRewards()
        {
            List<TwitchReward> loaded = _rewardRepository.Load();
            SetRewards(loaded);
        }

        public void SetRewards(IEnumerable<TwitchReward> rewards)
        {
            if (rewards == null)
                throw new ArgumentNullException(nameof(rewards));
            List<TwitchReward> snapshot = rewards.Where(reward => reward != null).ToList();
            bool turboEnabled = snapshot.Any(reward =>
                reward.Enabled && string.Equals(reward.InternalMsgEnable, "turbospeed", StringComparison.OrdinalIgnoreCase));
            bool updateTurbo;
            lock (_rewardSync)
            {
                _rewards = snapshot;
                updateTurbo = !_turboSpeedEnabled.HasValue || _turboSpeedEnabled.Value != turboEnabled;
                _turboSpeedEnabled = turboEnabled;
            }

            if (updateTurbo)
                WinMsgUtil.SendMsgToRS(turboEnabled ? "enable TurboSpeed" : "disable TurboSpeed");
        }

        private async Task ActivateStoredAuthorizationAsync(CancellationToken cancellationToken)
        {
            await _sessionGate.WaitAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                if (!await ValidateAndRefreshAsync(cancellationToken).ConfigureAwait(false))
                    return;

                await _eventSub.StartAsync(_tokens.AccessToken, _identity.UserId, _lifetime?.Token ?? cancellationToken)
                    .ConfigureAwait(false);
            }
            finally
            {
                _sessionGate.Release();
            }
        }

        private async Task<bool> ValidateAndRefreshAsync(CancellationToken cancellationToken)
        {
            TwitchTokenSet tokens = _tokens;
            if (tokens == null)
                return false;

            bool refreshed = false;
            if (tokens.ExpiresAtUtc <= _clock.UtcNow.Add(_options.RefreshBeforeExpiry))
            {
                if (string.IsNullOrWhiteSpace(tokens.RefreshToken))
                {
                    await RequireReauthorizationAsync("Twitch authorization expired and cannot be refreshed.")
                        .ConfigureAwait(false);
                    return false;
                }

                try
                {
                    tokens = await RefreshAsync(tokens, cancellationToken).ConfigureAwait(false);
                    refreshed = true;
                }
                catch (TwitchApiException ex) when (IsAuthorizationFailure(ex))
                {
                    await RequireReauthorizationAsync("Twitch authorization expired and could not be refreshed.")
                        .ConfigureAwait(false);
                    return false;
                }
            }

            TwitchTokenValidation validation = await _apiClient.ValidateTokenAsync(tokens.AccessToken, cancellationToken)
                .ConfigureAwait(false);
            if (validation == null && !refreshed && !string.IsNullOrWhiteSpace(tokens.RefreshToken))
            {
                try
                {
                    tokens = await RefreshAsync(tokens, cancellationToken).ConfigureAwait(false);
                    validation = await _apiClient.ValidateTokenAsync(tokens.AccessToken, cancellationToken).ConfigureAwait(false);
                }
                catch (TwitchApiException ex) when (IsAuthorizationFailure(ex))
                {
                    validation = null;
                }
            }

            if (validation == null ||
                !string.Equals(validation.ClientId, _options.ClientId, StringComparison.Ordinal) ||
                !_options.HasRequiredScopes(validation.Scopes) ||
                string.IsNullOrWhiteSpace(validation.UserId))
            {
                await RequireReauthorizationAsync("Twitch authorization is invalid or missing required permissions.")
                    .ConfigureAwait(false);
                return false;
            }

            tokens.ClientId = validation.ClientId;
            tokens.Scopes = validation.Scopes;
            if (validation.ExpiresInSeconds > 0)
                tokens.ExpiresAtUtc = _clock.UtcNow.AddSeconds(validation.ExpiresInSeconds);
            _tokenStore.Save(tokens);
            _tokens = tokens;

            TwitchIdentity identity;
            try
            {
                identity = await _apiClient.GetCurrentUserAsync(tokens.AccessToken, cancellationToken)
                    .ConfigureAwait(false);
            }
            catch (TwitchApiException ex) when (IsAuthorizationFailure(ex))
            {
                await RequireReauthorizationAsync("Twitch authorization became invalid while loading the account.")
                    .ConfigureAwait(false);
                return false;
            }
            _identity = identity;
            SetAuthenticationState(TwitchAuthenticationState.Authorized, identity, string.Empty);
            return true;
        }

        private async Task<TwitchTokenSet> RefreshAsync(TwitchTokenSet current, CancellationToken cancellationToken)
        {
            TwitchTokenSet refreshed = await _apiClient.RefreshTokenAsync(current.RefreshToken, cancellationToken)
                .ConfigureAwait(false);
            refreshed.ClientId = _options.ClientId;
            _tokenStore.Save(refreshed);
            _tokens = refreshed;
            Log(TwitchLogLevel.Information, "Refreshed Twitch authorization.");
            return refreshed;
        }

        private async Task MaintainAuthorizationAsync(CancellationToken cancellationToken)
        {
            int retryAttempt = 0;
            while (!cancellationToken.IsCancellationRequested)
            {
                bool authorized = AuthenticationState == TwitchAuthenticationState.Authorized;
                TimeSpan delay = _tokens == null || authorized
                    ? _options.ValidationInterval
                    : TimeSpan.FromSeconds(Math.Min(300, Math.Pow(2, Math.Min(retryAttempt + 2, 8))));

                try
                {
                    await _clock.DelayAsync(delay, cancellationToken).ConfigureAwait(false);
                    if (AuthenticationState == TwitchAuthenticationState.Authorizing)
                        continue;
                    if (_tokens == null)
                    {
                        retryAttempt = 0;
                        continue;
                    }

                    await ValidateNowAsync(cancellationToken).ConfigureAwait(false);
                    if (AuthenticationState == TwitchAuthenticationState.Authorized)
                    {
                        retryAttempt = 0;
                        if (EventSubState == TwitchEventSubState.Stopped || EventSubState == TwitchEventSubState.Error)
                            await StartListeningAsync(cancellationToken).ConfigureAwait(false);
                    }
                    else
                    {
                        retryAttempt++;
                    }
                }
                catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
                {
                    break;
                }
                catch (Exception ex) when (IsTransient(ex))
                {
                    retryAttempt++;
                    SetAuthenticationState(TwitchAuthenticationState.Error, _identity, "Twitch is currently unreachable; RSMods will retry.");
                    Log(TwitchLogLevel.Warning, "Twitch validation failed because the service is unreachable; retrying later.");
                }
            }
        }

        private async Task RequireReauthorizationAsync(string detail)
        {
            await _eventSub.StopAsync().ConfigureAwait(false);
            _tokenStore.Clear();
            lock (_stateSync)
            {
                _tokens = null;
                _identity = null;
            }
            SetAuthenticationState(TwitchAuthenticationState.ReauthorizationRequired, null, detail);
            Log(TwitchLogLevel.Warning, detail);
        }

        private void OnEventSubStateChanged(object sender, TwitchEventSubStateChangedEventArgs e)
        {
            lock (_stateSync)
            {
                _eventSubState = e.State;
                _stateDetail = e.Detail;
            }
            RaiseStateChanged();
            if (!string.IsNullOrWhiteSpace(e.Detail))
                Log(e.State == TwitchEventSubState.Error || e.State == TwitchEventSubState.Revoked
                    ? TwitchLogLevel.Error
                    : TwitchLogLevel.Information, e.Detail);
        }

        private void OnTriggerReceived(object sender, TwitchTriggerEventArgs e)
        {
            List<TwitchReward> rewards;
            lock (_rewardSync)
                rewards = new List<TwitchReward>(_rewards);

            List<TwitchReward> matches = TwitchRewardMatcher.FindMatches(rewards, e.Trigger);
            Log(TwitchLogLevel.Information, DescribeTrigger(e.Trigger));
            foreach (TwitchReward reward in matches)
                TryQueueReward(reward, e.Trigger.Viewer);
            TriggerReceived?.Invoke(this, e);
        }

        private void OnEventSubRevoked(object sender, TwitchRevocationEventArgs e)
        {
            _tokenStore.Clear();
            lock (_stateSync)
            {
                _tokens = null;
                _identity = null;
                _eventSubState = TwitchEventSubState.Revoked;
            }
            SetAuthenticationState(TwitchAuthenticationState.ReauthorizationRequired, null,
                "Twitch revoked authorization or a required EventSub subscription.");
        }

        private void OnEffectServerLog(string message) => Log(TwitchLogLevel.Information, message);

        private void OnEffectServerConnectionChanged(bool connected)
        {
            Log(TwitchLogLevel.Information, connected ? "Rocksmith connected." : "Rocksmith disconnected.");
            RaiseStateChanged();
        }

        private async Task RestoreAuthorizationAfterFlowAsync()
        {
            SetAuthenticationState(
                _tokens != null && _identity != null ? TwitchAuthenticationState.Authorized : TwitchAuthenticationState.NotAuthorized,
                _identity,
                string.Empty);
            if (AuthenticationState == TwitchAuthenticationState.Authorized)
                await StartListeningAsync(_lifetime?.Token ?? CancellationToken.None).ConfigureAwait(false);
        }

        private void SetAuthenticationState(TwitchAuthenticationState state, TwitchIdentity identity, string detail)
        {
            lock (_stateSync)
            {
                _authenticationState = state;
                _identity = identity;
                _stateDetail = detail ?? string.Empty;
            }
            RaiseStateChanged();
        }

        private void RaiseStateChanged()
        {
            TwitchStateChangedEventArgs args;
            lock (_stateSync)
                args = new TwitchStateChangedEventArgs(_authenticationState, _eventSubState, _identity, _stateDetail);
            StateChanged?.Invoke(this, args);
        }

        private void Log(TwitchLogLevel level, string message)
        {
            var entry = new TwitchLogEntry(_clock.UtcNow, level, message);
            lock (_stateSync)
            {
                _logs.Enqueue(entry);
                while (_logs.Count > _options.LogCapacity)
                    _logs.Dequeue();
            }
            LogReceived?.Invoke(this, new TwitchLogEventArgs(entry));
        }

        private static bool IsTransient(Exception ex)
        {
            if (ex is HttpRequestException || ex is TimeoutException || ex is Newtonsoft.Json.JsonException)
                return true;
            if (ex is TwitchApiException apiException)
                return (int)apiException.StatusCode >= 500 || apiException.StatusCode == (HttpStatusCode)429;
            return false;
        }

        private static bool IsAuthorizationFailure(TwitchApiException exception) =>
            exception.StatusCode == HttpStatusCode.BadRequest ||
            exception.StatusCode == HttpStatusCode.Unauthorized ||
            exception.StatusCode == HttpStatusCode.Forbidden;

        private static string DescribeTrigger(TwitchTriggerEvent trigger)
        {
            switch (trigger.Kind)
            {
                case TwitchTriggerKind.Bits:
                    return $"{trigger.Viewer} cheered {trigger.Amount} bits.";
                case TwitchTriggerKind.ChannelPoints:
                    return $"{trigger.Viewer} redeemed a {trigger.Amount}-point reward.";
                default:
                    return $"{trigger.Viewer} subscribed to the channel.";
            }
        }

        public async Task StopAsync()
        {
            await _lifecycleGate.WaitAsync().ConfigureAwait(false);
            try
            {
                if (!_started)
                    return;

                _started = false;
                _lifetime.Cancel();
                await _eventSub.StopAsync().ConfigureAwait(false);
                await _effectServer.StopAsync().ConfigureAwait(false);
                if (_maintenanceTask != null)
                {
                    try
                    {
                        await _maintenanceTask.ConfigureAwait(false);
                    }
                    catch (OperationCanceledException)
                    {
                    }
                }
                _lifetime.Dispose();
                _lifetime = null;
                _maintenanceTask = null;
            }
            finally
            {
                _lifecycleGate.Release();
            }
        }

        private void ThrowIfDisposed()
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(TwitchService));
        }

        public async ValueTask DisposeAsync()
        {
            if (_disposed)
                return;

            await StopAsync().ConfigureAwait(false);
            _disposed = true;
            _eventSub.StateChanged -= OnEventSubStateChanged;
            _eventSub.TriggerReceived -= OnTriggerReceived;
            _eventSub.Revoked -= OnEventSubRevoked;
            _effectServer.LogMessage -= OnEffectServerLog;
            _effectServer.ConnectionStateChanged -= OnEffectServerConnectionChanged;
            await _eventSub.DisposeAsync().ConfigureAwait(false);
            await _effectServer.DisposeAsync().ConfigureAwait(false);
            _lifecycleGate.Dispose();
            _sessionGate.Dispose();
        }

        public void Dispose() => DisposeAsync().AsTask().GetAwaiter().GetResult();
    }
}
