using Newtonsoft.Json;
using System;
using System.IO;
using System.Net;
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

    public interface ITwitchEventSubClient : IAsyncDisposable
    {
        event EventHandler<TwitchEventSubStateChangedEventArgs> StateChanged;
        event EventHandler<TwitchTriggerEventArgs> TriggerReceived;
        event EventHandler<TwitchRevocationEventArgs> Revoked;

        Task StartAsync(string accessToken, string broadcasterUserId, CancellationToken cancellationToken = default(CancellationToken));
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

        public async Task StartAsync(
            string accessToken,
            string broadcasterUserId,
            CancellationToken cancellationToken = default(CancellationToken))
        {
            if (string.IsNullOrWhiteSpace(accessToken))
                throw new ArgumentException("An access token is required.", nameof(accessToken));
            if (string.IsNullOrWhiteSpace(broadcasterUserId))
                throw new ArgumentException("A broadcaster user ID is required.", nameof(broadcasterUserId));

            await _gate.WaitAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                ThrowIfDisposed();
                if (_runTask != null && !_runTask.IsCompleted)
                    return;

                _runCancellation?.Dispose();
                _runCancellation = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
                _runTask = RunAsync(accessToken, broadcasterUserId, _runCancellation.Token);
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

        private async Task RunAsync(string accessToken, string broadcasterUserId, CancellationToken cancellationToken)
        {
            int reconnectAttempt = 0;
            Uri endpoint = _options.EventSubWebSocketEndpoint;
            bool plannedReconnect = false;
            Connection connection = null;

            try
            {
                while (!cancellationToken.IsCancellationRequested)
                {
                    try
                    {
                        SetState(reconnectAttempt == 0 && !plannedReconnect
                            ? TwitchEventSubState.Connecting
                            : TwitchEventSubState.Reconnecting, string.Empty);
                        Connection next = await ConnectAsync(
                            endpoint,
                            !plannedReconnect,
                            accessToken,
                            broadcasterUserId,
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
                        SetState(TwitchEventSubState.Connected, string.Empty);

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
                    catch (TwitchApiException ex) when (
                        ex.StatusCode == HttpStatusCode.BadRequest ||
                        ex.StatusCode == HttpStatusCode.Unauthorized ||
                        ex.StatusCode == HttpStatusCode.Forbidden)
                    {
                        SetState(TwitchEventSubState.Error, ex.Message);
                        Revoked?.Invoke(this, new TwitchRevocationEventArgs(string.Empty, "authorization_failed"));
                        return;
                    }
                    catch (SubscriptionRevokedException)
                    {
                        return;
                    }
                    catch (JsonException)
                    {
                        SetState(TwitchEventSubState.Error, "Twitch sent an invalid EventSub message.");
                    }
                    catch (Exception ex) when (ex is IOException || ex is WebSocketException || ex is TimeoutException || ex is TwitchApiException)
                    {
                        SetState(TwitchEventSubState.Reconnecting, "The Twitch connection was interrupted.");
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

        private async Task<Connection> ConnectAsync(
            Uri endpoint,
            bool createSubscriptions,
            string accessToken,
            string broadcasterUserId,
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

                if (createSubscriptions)
                {
                    await _apiClient.CreateEventSubSubscriptionsAsync(
                        accessToken,
                        broadcasterUserId,
                        sessionId,
                        cancellationToken).ConfigureAwait(false);
                }

                return new Connection { Socket = socket, KeepaliveTimeout = keepaliveTimeout };
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
