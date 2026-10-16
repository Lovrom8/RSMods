using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Net.WebSockets;
using System.Threading;
using System.Threading.Tasks;

namespace RSMods.Twitch
{
    public sealed class TwitchEventSubStateChangedEventArgs(TwitchEventSubState state, string detail) : EventArgs
    {
        public TwitchEventSubState State { get; } = state;
        public string Detail { get; } = detail ?? string.Empty;
    }

    public sealed class TwitchEventSubCredentials(string accessToken, string broadcasterUserId)
    {
        public string AccessToken { get; } = accessToken;
        public string BroadcasterUserId { get; } = broadcasterUserId;
    }

    /// <summary>
    /// Supplies the credentials for each new EventSub session. <paramref name="rejectedAccessToken"/> is the token Twitch
    /// just refused (null otherwise), which the provider should refresh. Null means authorization is gone.
    /// </summary>
    public delegate Task<TwitchEventSubCredentials> TwitchEventSubCredentialProvider(
        string rejectedAccessToken,
        CancellationToken cancellationToken);

    public interface ITwitchEventSubClient : IAsyncDisposable
    {
        event EventHandler<TwitchEventSubStateChangedEventArgs> StateChanged;
        event EventHandler<TwitchTriggerEventArgs> TriggerReceived;
        event EventHandler<TwitchRevocationEventArgs> Revoked;

        // Asked on every new session: a token captured once at start goes stale after a refresh.
        Task StartAsync(TwitchEventSubCredentialProvider credentials, CancellationToken cancellationToken = default(CancellationToken));
        Task StopAsync();
    }

    public sealed class TwitchEventSubClient(
        TwitchApiClient apiClient,
        TwitchOptions options,
        ITwitchWebSocketFactory socketFactory = null,
        ITwitchClock clock = null) : ITwitchEventSubClient, IDisposable
    {
        private sealed class SubscriptionRevokedException : Exception
        {
        }

        private sealed class Connection : IDisposable
        {
            public ITwitchWebSocket Socket { get; set; }
            public TimeSpan KeepaliveTimeout { get; set; }
            public IReadOnlyList<TwitchApiException> RejectedSubscriptions { get; set; } = [];

            public void Dispose() => Socket?.Dispose();
        }

        private readonly TwitchApiClient _apiClient = apiClient ?? throw new ArgumentNullException(nameof(apiClient));
        private readonly TwitchOptions _options = options ?? throw new ArgumentNullException(nameof(options));
        private readonly ITwitchWebSocketFactory _socketFactory = socketFactory ?? new TwitchWebSocketFactory();
        private readonly ITwitchClock _clock = clock ?? SystemTwitchClock.Instance;
        private readonly BoundedEventIdCache _eventIds = new BoundedEventIdCache(options.NotificationDeduplicationCapacity, options.NotificationDeduplicationTtl);
        private readonly SemaphoreSlim _gate = new SemaphoreSlim(1, 1);
        private CancellationTokenSource _runCancellation;
        private Task _runTask;
        private bool _disposed;

        public event EventHandler<TwitchEventSubStateChangedEventArgs> StateChanged;
        public event EventHandler<TwitchTriggerEventArgs> TriggerReceived;
        public event EventHandler<TwitchRevocationEventArgs> Revoked;

        /// <summary>Listens with fixed credentials; a rejected token ends the session.</summary>
        public Task StartAsync(
            string accessToken,
            string broadcasterUserId,
            CancellationToken cancellationToken = default(CancellationToken))
        {
            if (string.IsNullOrWhiteSpace(accessToken))
                throw new ArgumentException("An access token is required.", nameof(accessToken));
            if (string.IsNullOrWhiteSpace(broadcasterUserId))
                throw new ArgumentException("A broadcaster user ID is required.", nameof(broadcasterUserId));

            var credentials = new TwitchEventSubCredentials(accessToken, broadcasterUserId);
            return StartAsync((rejected, _) => Task.FromResult(rejected == null ? credentials : null), cancellationToken);
        }

        public async Task StartAsync(
            TwitchEventSubCredentialProvider credentials,
            CancellationToken cancellationToken = default(CancellationToken))
        {
            if (credentials == null)
                throw new ArgumentNullException(nameof(credentials));

            await _gate.WaitAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                ThrowIfDisposed();
                if (_runTask != null && !_runTask.IsCompleted)
                    return;

                _runCancellation?.Dispose();
                _runCancellation = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
                _runTask = RunAsync(credentials, _runCancellation.Token);
            }
            finally
            {
                _gate.Release();
            }
        }

        public async Task StopAsync()
        {
            Task runTask;
            await _gate.WaitAsync().ConfigureAwait(false);
            try
            {
                runTask = _runTask;
                _runCancellation?.Cancel();
            }
            finally
            {
                _gate.Release();
            }

            if (runTask != null)
            {
                try
                {
                    await runTask.ConfigureAwait(false);
                }
                catch (OperationCanceledException)
                {
                }
            }

            SetState(TwitchEventSubState.Stopped, string.Empty);
        }

        private async Task RunAsync(TwitchEventSubCredentialProvider credentialProvider, CancellationToken cancellationToken)
        {
            int reconnectAttempt = 0;
            Uri endpoint = _options.EventSubWebSocketEndpoint;
            bool plannedReconnect = false;
            Connection connection = null;
            TwitchEventSubCredentials credentials = null;
            string rejectedAccessToken = null;

            try
            {
                while (!cancellationToken.IsCancellationRequested)
                {
                    try
                    {
                        SetState(reconnectAttempt == 0 && !plannedReconnect
                            ? TwitchEventSubState.Connecting
                            : TwitchEventSubState.Reconnecting, string.Empty);
                        // A planned reconnect keeps its subscriptions, so only a new session needs credentials.
                        if (!plannedReconnect)
                        {
                            credentials = await credentialProvider(rejectedAccessToken, cancellationToken).ConfigureAwait(false);
                            rejectedAccessToken = null;
                            if (credentials == null)
                            {
                                SetState(TwitchEventSubState.Error, "Twitch authorization is required before listening can resume.");
                                return;
                            }
                        }

                        Connection next = await ConnectAsync(
                            endpoint,
                            plannedReconnect ? null : credentials,
                            cancellationToken).ConfigureAwait(false);

                        if (connection != null)
                        {
                            await CloseQuietlyAsync(connection, cancellationToken).ConfigureAwait(false);
                            connection.Dispose();
                        }

                        connection = next;
                        reconnectAttempt = 0;
                        plannedReconnect = false;
                        endpoint = _options.EventSubWebSocketEndpoint;
                        SetState(TwitchEventSubState.Connected, DescribeRejections(connection.RejectedSubscriptions));

                        Uri reconnectUri = await ReceiveUntilReconnectAsync(connection, cancellationToken).ConfigureAwait(false);
                        if (reconnectUri != null)
                        {
                            endpoint = reconnectUri;
                            plannedReconnect = true;
                            SetState(TwitchEventSubState.Reconnecting, "Twitch requested a planned reconnect.");
                            continue;
                        }

                        throw new IOException("The Twitch EventSub connection closed.");
                    }
                    catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
                    {
                        break;
                    }
                    catch (TwitchApiException ex) when (ex.StatusCode == HttpStatusCode.Unauthorized)
                    {
                        // Usually an expired token. The provider refreshes it for the next attempt and only gives up
                        // (returns null) when the refresh itself is refused.
                        rejectedAccessToken = credentials?.AccessToken;
                        SetState(TwitchEventSubState.Reconnecting, "Twitch rejected the access token; refreshing authorization.");
                    }
                    catch (SubscriptionRevokedException)
                    {
                        return;
                    }
                    catch (JsonException)
                    {
                        SetState(TwitchEventSubState.Error, "Twitch sent an invalid EventSub message.");
                    }
                    catch (TwitchApiException ex)
                    {
                        // Every subscription refused (400/403) or a Twitch outage: worth another try, never a sign-out.
                        SetState(TwitchEventSubState.Reconnecting, ex.Message);
                    }
                    // An OperationCanceledException here is an HttpClient timeout, not a stop.
                    catch (Exception ex) when (ex is IOException || ex is InvalidDataException || ex is WebSocketException ||
                                               ex is TimeoutException || ex is HttpRequestException ||
                                               ex is OperationCanceledException)
                    {
                        SetState(TwitchEventSubState.Reconnecting, "The Twitch connection was interrupted.");
                    }
                    catch (Exception ex)
                    {
                        // Don't end silently while the state still says Connected; maintenance restarts an Error session.
                        SetState(TwitchEventSubState.Error, $"Twitch listening stopped after an unexpected error: {ex.Message}");
                        return;
                    }

                    connection?.Dispose();
                    connection = null;
                    endpoint = _options.EventSubWebSocketEndpoint;
                    plannedReconnect = false;
                    reconnectAttempt++;
                    double seconds = Math.Min(30, Math.Pow(2, Math.Min(reconnectAttempt - 1, 5)));
                    await _clock.DelayAsync(TimeSpan.FromSeconds(seconds), cancellationToken).ConfigureAwait(false);
                }
            }
            finally
            {
                if (connection != null)
                {
                    await CloseQuietlyAsync(connection, CancellationToken.None).ConfigureAwait(false);
                    connection.Dispose();
                }
            }
        }

        // Null credentials keep the session's existing subscriptions (planned reconnect).
        private async Task<Connection> ConnectAsync(
            Uri endpoint,
            TwitchEventSubCredentials credentials,
            CancellationToken cancellationToken)
        {
            ITwitchWebSocket socket = _socketFactory.Create();
            try
            {
                await socket.ConnectAsync(endpoint, cancellationToken).ConfigureAwait(false);
                string welcomeJson = await socket.ReceiveTextAsync(TimeSpan.FromSeconds(10), cancellationToken).ConfigureAwait(false);
                if (welcomeJson == null)
                    throw new IOException("Twitch closed EventSub before sending a welcome message.");

                TwitchEventSubEnvelope welcome = TwitchEventSubMessageParser.ParseEnvelope(welcomeJson, _clock.UtcNow);
                if (!TwitchEventSubMessageParser.TryGetWelcome(welcome, out string sessionId, out TimeSpan keepaliveTimeout))
                    throw new InvalidDataException("Twitch did not send an EventSub welcome message.");

                IReadOnlyList<TwitchApiException> rejected = [];
                if (credentials != null)
                {
                    rejected = await _apiClient.CreateEventSubSubscriptionsAsync(
                        credentials.AccessToken,
                        credentials.BroadcasterUserId,
                        sessionId,
                        cancellationToken).ConfigureAwait(false);
                }

                return new Connection { Socket = socket, KeepaliveTimeout = keepaliveTimeout, RejectedSubscriptions = rejected };
            }
            catch
            {
                socket.Dispose();
                throw;
            }
        }

        private async Task<Uri> ReceiveUntilReconnectAsync(Connection connection, CancellationToken cancellationToken)
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                string json = await connection.Socket.ReceiveTextAsync(connection.KeepaliveTimeout, cancellationToken).ConfigureAwait(false);
                if (json == null)
                    return null;

                TwitchEventSubEnvelope envelope = TwitchEventSubMessageParser.ParseEnvelope(json, _clock.UtcNow);
                if (TwitchEventSubMessageParser.TryGetReconnectUrl(envelope, out Uri reconnectUri))
                    return reconnectUri;

                if (TwitchEventSubMessageParser.TryGetRevocation(envelope, out string type, out string status))
                {
                    if (!IsAuthorizationRevocation(status))
                    {
                        // e.g. version_removed: the login is still good, so maintenance resubscribes later.
                        SetState(TwitchEventSubState.Error, $"Twitch removed the {type} subscription ({status}).");
                        throw new SubscriptionRevokedException();
                    }

                    SetState(TwitchEventSubState.Revoked, $"Twitch revoked the {type} subscription ({status}).");
                    Revoked?.Invoke(this, new TwitchRevocationEventArgs(type, status));
                    throw new SubscriptionRevokedException();
                }

                if (TwitchEventSubMessageParser.TryGetTrigger(envelope, out TwitchTriggerEvent trigger) &&
                    _eventIds.TryAdd(envelope.MessageId, _clock.UtcNow))
                {
                    TriggerReceived?.Invoke(this, new TwitchTriggerEventArgs(trigger));
                }
            }

            return null;
        }

        private static bool IsAuthorizationRevocation(string status) =>
            string.Equals(status, "authorization_revoked", StringComparison.Ordinal) ||
            string.Equals(status, "user_removed", StringComparison.Ordinal) ||
            string.Equals(status, "revoked", StringComparison.Ordinal); // The parser's stand-in for a missing status.

        private static string DescribeRejections(IReadOnlyList<TwitchApiException> rejected) =>
            rejected.Count == 0
                ? string.Empty
                : string.Join(" ", rejected.Select(exception => exception.Message)) + " Other Twitch events still arrive.";

        private static async Task CloseQuietlyAsync(Connection connection, CancellationToken cancellationToken)
        {
            using var closeCancellation = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);

            try
            {
                closeCancellation.CancelAfter(TimeSpan.FromSeconds(2));
                await connection.Socket.CloseAsync(closeCancellation.Token).ConfigureAwait(false);
            }
            catch
            {
            }
        }

        private void SetState(TwitchEventSubState state, string detail) =>
            StateChanged?.Invoke(this, new TwitchEventSubStateChangedEventArgs(state, detail));

        private void ThrowIfDisposed()
        {
            if (_disposed)
            {
                throw new ObjectDisposedException(nameof(TwitchEventSubClient));
            }
        }

        public async ValueTask DisposeAsync()
        {
            if (_disposed)
                return;
            _disposed = true;
            await StopAsync().ConfigureAwait(false);
            _runCancellation?.Dispose();
            _gate.Dispose();
        }

        public void Dispose()
        {
            DisposeAsync().AsTask().GetAwaiter().GetResult();
        }
    }
}
