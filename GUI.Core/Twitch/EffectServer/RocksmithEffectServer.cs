using Newtonsoft.Json;
using System;
using System.Collections.Concurrent;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

namespace RSMods.Twitch.EffectServer
{
    public sealed class RocksmithEffectServerOptions
    {
        public int Port { get; set; } = 45659;
        public int MaximumQueuedEffects { get; set; } = 256;

        /// <summary>How many times an effect Rocksmith can't take yet is retried before it's dropped.</summary>
        public int MaximumRetryCount { get; set; } = 5;

        /// <summary>The wait before the first retry; each later retry waits twice as long (0.5, 1, 2, 4, 8 seconds).</summary>
        public TimeSpan FirstRetryDelay { get; set; } = TimeSpan.FromMilliseconds(500);

        /// <summary>The wait before accepting connections again after the listener fails.</summary>
        public TimeSpan ListenerRetryDelay { get; set; } = TimeSpan.FromSeconds(1);

        /// <summary>
        /// Asks the game to connect. It dials in once when it loads and never again after a failed or dropped
        /// connection, so the server asks when it starts and while an effect is waiting for the game. Null asks nothing.
        /// </summary>
        public Action RequestGameConnection { get; set; }

        /// <summary>How long an effect waits for the game after asking it to connect before asking again.</summary>
        public TimeSpan ConnectionRequestInterval { get; set; } = TimeSpan.FromSeconds(5);

        /// <summary>
        /// How long the game gets to answer an effect request. Past it the connection is dropped (a late answer would be
        /// read as the next request's) and the effect retried once the game reconnects.
        /// </summary>
        public TimeSpan ResponseTimeout { get; set; } = TimeSpan.FromSeconds(10);
    }

    /// <summary>
    /// Owns the localhost server used by Rocksmith's Crowd Control client. Requests and responses are
    /// UTF-8 JSON frames terminated by a null byte.
    /// </summary>
    public sealed class RocksmithEffectServer : IDisposable, IAsyncDisposable
    {
        private sealed class QueuedEffect
        {
            public TwitchReward Reward { get; set; }
            public string Viewer { get; set; }
            public int Attempts { get; set; }
        }

        private sealed class ConnectionContext : IDisposable
        {
            public ConnectionContext(TcpClient client)
            {
                Client = client;
                Frames = new NullTerminatedJsonStream(client.GetStream());
            }

            public TcpClient Client { get; }
            public NullTerminatedJsonStream Frames { get; }
            public void Dispose() => Client.Dispose();
        }

        private readonly object _stateLock = new object();
        private readonly RocksmithEffectServerOptions _options;
        private readonly ConcurrentQueue<QueuedEffect> _queue = new ConcurrentQueue<QueuedEffect>();
        private readonly SemaphoreSlim _queueSignal = new SemaphoreSlim(0);
        private readonly SemaphoreSlim _connectionSignal = new SemaphoreSlim(0);

        private CancellationTokenSource _lifetime;
        private TcpListener _listener;
        private ConnectionContext _connection;
        private Task _acceptTask;
        private Task _workerTask;
        private int _queuedCount;
        private int _nextRequestId;
        private bool _disposed;

        public RocksmithEffectServer(RocksmithEffectServerOptions options = null)
        {
            _options = options ?? new RocksmithEffectServerOptions();
            if (_options.Port < 0 || _options.Port > 65535)
                throw new ArgumentOutOfRangeException(nameof(options), "Port must be between 0 and 65535.");
            if (_options.MaximumQueuedEffects <= 0)
                throw new ArgumentOutOfRangeException(nameof(options), "MaximumQueuedEffects must be positive.");
            if (_options.MaximumRetryCount < 0)
                throw new ArgumentOutOfRangeException(nameof(options), "MaximumRetryCount cannot be negative.");
            if (_options.FirstRetryDelay < TimeSpan.Zero)
                throw new ArgumentOutOfRangeException(nameof(options), "FirstRetryDelay cannot be negative.");
            if (_options.ConnectionRequestInterval <= TimeSpan.Zero)
                throw new ArgumentOutOfRangeException(nameof(options), "ConnectionRequestInterval must be positive.");
            if (_options.ResponseTimeout <= TimeSpan.Zero)
                throw new ArgumentOutOfRangeException(nameof(options), "ResponseTimeout must be positive.");
        }

        public event Action<string> LogMessage;
        public event Action<bool> ConnectionStateChanged;

        public bool IsRunning { get; private set; }
        public bool IsConnected { get; private set; }
        public int ListeningPort { get; private set; }

        public Task StartAsync(CancellationToken cancellationToken = default)
        {
            ThrowIfDisposed();

            lock (_stateLock)
            {
                if (IsRunning)
                    return Task.CompletedTask;

                var listener = new TcpListener(IPAddress.Loopback, _options.Port);
                // Throws when the port is taken; nothing is kept, so the owner can simply call this again later.
                listener.Start();
                _listener = listener;
                _lifetime = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
                ListeningPort = ((IPEndPoint)_listener.LocalEndpoint).Port;
                IsRunning = true;

                _acceptTask = AcceptConnectionsAsync(_lifetime.Token);
                _workerTask = ProcessQueueAsync(_lifetime.Token);
            }

            Log($"Started the Rocksmith effect server on 127.0.0.1:{ListeningPort}.");

            // A game that's already past loading tried once, before this was listening. Off the caller's thread: asking
            // waits on the game's window.
            _ = Task.Run(RequestGameConnection);
            return Task.CompletedTask;
        }

        public bool TryQueueEffect(TwitchReward reward, string viewer = "rsmods")
        {
            ThrowIfDisposed();
            if (reward == null || !IsRunning)
                return false;

            int queued = Interlocked.Increment(ref _queuedCount);
            if (queued > _options.MaximumQueuedEffects)
            {
                Interlocked.Decrement(ref _queuedCount);
                Log("The Rocksmith effect queue is full; the reward was not queued.");
                return false;
            }

            _queue.Enqueue(new QueuedEffect
            {
                Reward = reward,
                Viewer = string.IsNullOrWhiteSpace(viewer) ? "rsmods" : viewer
            });
            _queueSignal.Release();
            return true;
        }

        public async Task StopAsync()
        {
            Task acceptTask;
            Task workerTask;
            CancellationTokenSource lifetime;
            TcpListener listener;
            ConnectionContext connection;

            lock (_stateLock)
            {
                if (!IsRunning)
                    return;

                IsRunning = false;
                lifetime = _lifetime;
                listener = _listener;
                connection = _connection;
                acceptTask = _acceptTask;
                workerTask = _workerTask;
                _lifetime = null;
                _listener = null;
                _connection = null;
                _acceptTask = null;
                _workerTask = null;
            }

            lifetime.Cancel();
            listener.Stop();
            connection?.Dispose();
            SetConnected(false);

            try
            {
                await Task.WhenAll(acceptTask, workerTask).ConfigureAwait(false);
            }
            catch (OperationCanceledException)
            {
            }
            catch (ObjectDisposedException)
            {
            }
            finally
            {
                lifetime.Dispose();
                while (_queue.TryDequeue(out _))
                    Interlocked.Decrement(ref _queuedCount);
            }

            Log("Stopped the Rocksmith effect server.");
        }

        private async Task AcceptConnectionsAsync(CancellationToken cancellationToken)
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                TcpClient client;
                try
                {
                    client = await _listener.AcceptTcpClientAsync(cancellationToken).ConfigureAwait(false);
                }
                catch (ObjectDisposedException) when (cancellationToken.IsCancellationRequested)
                {
                    break;
                }
                catch (SocketException) when (cancellationToken.IsCancellationRequested)
                {
                    break;
                }
                catch (SocketException ex)
                {
                    Log($"The Rocksmith effect listener failed: {ex.Message}");
                    await Task.Delay(_options.ListenerRetryDelay, cancellationToken).ConfigureAwait(false);
                    continue;
                }

                client.NoDelay = true;
                var replacement = new ConnectionContext(client);
                ConnectionContext previous;
                lock (_stateLock)
                {
                    previous = _connection;
                    _connection = replacement;
                }

                previous?.Dispose();
                SetConnected(true);
                _connectionSignal.Release();
                Log("Connected to Rocksmith.");
            }
        }

        private async Task ProcessQueueAsync(CancellationToken cancellationToken)
        {
            while (!cancellationToken.IsCancellationRequested)
            {
                await _queueSignal.WaitAsync(cancellationToken).ConfigureAwait(false);
                if (!_queue.TryDequeue(out QueuedEffect queuedEffect))
                    continue;

                Interlocked.Decrement(ref _queuedCount);
                ConnectionContext connection = await WaitForConnectionAsync(cancellationToken).ConfigureAwait(false);

                try
                {
                    int requestId = Interlocked.Increment(ref _nextRequestId);
                    RocksmithEffectRequest request = RocksmithEffectRequestFactory.Create(
                        queuedEffect.Reward,
                        requestId,
                        queuedEffect.Viewer);
                    string requestJson = JsonConvert.SerializeObject(request);

                    Log($"Sending effect request {requestId} ({request.Code}, {requestJson.Length} JSON characters).");
                    await connection.Frames.WriteFrameAsync(requestJson, cancellationToken).ConfigureAwait(false);
                    string responseJson = await ReadResponseAsync(connection, cancellationToken).ConfigureAwait(false);
                    RocksmithEffectResponse response = JsonConvert.DeserializeObject<RocksmithEffectResponse>(responseJson);
                    if (response == null || response.Id != requestId)
                        throw new InvalidDataException("Rocksmith returned an invalid effect response.");

                    if (response.Status == 3)
                    {
                        ScheduleRetry(queuedEffect, "Rocksmith asked to retry the effect.", cancellationToken);
                    }
                    else if (response.Status == 0)
                    {
                        Log($"Enabled: {queuedEffect.Reward.Name}");
                    }
                    else
                    {
                        Log($"Rocksmith rejected '{queuedEffect.Reward.Name}' with status {response.Status}.");
                    }
                }
                catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
                {
                    break;
                }
                // InvalidDataException isn't an IOException: a wrong-id or oversized answer. Like a timeout, the stream
                // can't be trusted to line up with the next request any more, so it's dropped too.
                catch (Exception ex) when (ex is IOException ||
                                              ex is InvalidDataException ||
                                              ex is TimeoutException ||
                                              ex is SocketException ||
                                              ex is ObjectDisposedException ||
                                              ex is JsonException)
                {
                    RemoveConnection(connection);
                    ScheduleRetry(queuedEffect, $"The Rocksmith effect exchange failed: {ex.Message}", cancellationToken);
                }
                catch (Exception ex) when (ex is ArgumentException || ex is OverflowException)
                {
                    Log($"Dropped '{queuedEffect.Reward.Name}' because its effect request is invalid: {ex.Message}");
                }
                catch (Exception ex) when (!cancellationToken.IsCancellationRequested)
                {
                    // Anything else drops this one effect; letting it out would end this loop and every later effect with it.
                    Log($"Dropped '{queuedEffect.Reward.Name}' after an unexpected error: {ex.Message}");
                }
            }
        }

        private async Task<string> ReadResponseAsync(ConnectionContext connection, CancellationToken cancellationToken)
        {
            using var timeout = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            timeout.CancelAfter(_options.ResponseTimeout);

            try
            {
                return await connection.Frames.ReadFrameAsync(timeout.Token).ConfigureAwait(false);
            }
            catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
            {
                throw new TimeoutException($"Rocksmith didn't answer within {_options.ResponseTimeout.TotalSeconds:0.##} s.");
            }
        }

        private async Task<ConnectionContext> WaitForConnectionAsync(CancellationToken cancellationToken)
        {
            while (true)
            {
                lock (_stateLock)
                {
                    if (_connection != null)
                        return _connection;
                }

                RequestGameConnection();
                await _connectionSignal.WaitAsync(_options.ConnectionRequestInterval, cancellationToken).ConfigureAwait(false);
            }
        }

        private void RequestGameConnection()
        {
            Action request = _options.RequestGameConnection;
            if (request == null)
                return;

            try
            {
                request();
            }
            catch (Exception ex)
            {
                Log($"Couldn't ask Rocksmith to connect: {ex.Message}");
            }
        }

        /// <summary>
        /// Puts an effect Rocksmith couldn't take yet back in the queue after a wait that doubles with each retry, and
        /// drops it after <see cref="RocksmithEffectServerOptions.MaximumRetryCount"/> retries. The wait runs beside the
        /// queue rather than in it, so other viewers' effects aren't held up behind one that has to wait.
        /// </summary>
        private void ScheduleRetry(QueuedEffect queuedEffect, string reason, CancellationToken cancellationToken)
        {
            queuedEffect.Attempts++;
            if (queuedEffect.Attempts > _options.MaximumRetryCount)
            {
                Log($"Dropped '{queuedEffect.Reward.Name}' after {_options.MaximumRetryCount} retries. {reason}");
                return;
            }

            TimeSpan delay = RetryDelay(_options.FirstRetryDelay, queuedEffect.Attempts);
            Log($"Retrying '{queuedEffect.Reward.Name}' ({queuedEffect.Attempts}/{_options.MaximumRetryCount}) in {delay.TotalSeconds:0.##} s. {reason}");
            _ = RequeueAfterAsync(queuedEffect, delay, cancellationToken);
        }

        /// <summary>The wait before retry <paramref name="attempt"/> (from 1): <paramref name="firstDelay"/>, doubling each retry.</summary>
        internal static TimeSpan RetryDelay(TimeSpan firstDelay, int attempt) => firstDelay * Math.Pow(2, attempt - 1);

        private async Task RequeueAfterAsync(QueuedEffect queuedEffect, TimeSpan delay, CancellationToken cancellationToken)
        {
            try
            {
                await Task.Delay(delay, cancellationToken).ConfigureAwait(false);
            }
            catch (OperationCanceledException)
            {
                return; // The server stopped; the effect goes with the rest of the queue.
            }

            int queued = Interlocked.Increment(ref _queuedCount);
            if (queued > _options.MaximumQueuedEffects)
            {
                Interlocked.Decrement(ref _queuedCount);
                Log($"Dropped '{queuedEffect.Reward.Name}' because the retry queue is full.");
                return;
            }

            _queue.Enqueue(queuedEffect);
            try
            {
                _queueSignal.Release();
            }
            catch (ObjectDisposedException)
            {
                // Disposed while this retry waited; nothing is left to send it.
            }
        }

        private void RemoveConnection(ConnectionContext connection)
        {
            bool removed = false;
            lock (_stateLock)
            {
                if (ReferenceEquals(_connection, connection))
                {
                    _connection = null;
                    removed = true;
                }
            }

            if (removed)
            {
                connection.Dispose();
                SetConnected(false);
                Log("Disconnected from Rocksmith; waiting for it to reconnect.");
            }
        }

        private void SetConnected(bool connected)
        {
            if (IsConnected == connected)
                return;

            IsConnected = connected;
            ConnectionStateChanged?.Invoke(connected);
        }

        private void Log(string message) => LogMessage?.Invoke(message);

        private void ThrowIfDisposed()
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(RocksmithEffectServer));
        }

        public void Dispose()
        {
            if (_disposed)
                return;

            StopAsync().GetAwaiter().GetResult();
            _disposed = true;
            _queueSignal.Dispose();
            _connectionSignal.Dispose();
        }

        public async ValueTask DisposeAsync()
        {
            if (_disposed)
                return;

            await StopAsync().ConfigureAwait(false);
            _disposed = true;
            _queueSignal.Dispose();
            _connectionSignal.Dispose();
        }
    }
}
