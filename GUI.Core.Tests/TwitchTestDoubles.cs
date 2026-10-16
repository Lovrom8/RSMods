using RSMods.Twitch;
using System.Collections.Concurrent;
using System.Net;
using System.Net.Http;

namespace RSMods.Core.Tests;

internal sealed class QueueHttpMessageHandler : HttpMessageHandler
{
    private readonly ConcurrentQueue<Func<HttpRequestMessage, Task<HttpResponseMessage>>> _responses = new();

    public List<RecordedHttpRequest> Requests { get; } = [];

    public void Enqueue(HttpStatusCode statusCode, string json) =>
        Enqueue(_ => new HttpResponseMessage(statusCode)
        {
            Content = new StringContent(json)
        });

    public void Enqueue(Func<HttpRequestMessage, HttpResponseMessage> response) =>
        _responses.Enqueue(request => Task.FromResult(response(request)));

    /// <summary>A response that arrives when the test says so, for racing other calls against a request in flight.</summary>
    public void EnqueueAsync(Func<HttpRequestMessage, Task<HttpResponseMessage>> response) => _responses.Enqueue(response);

    protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
    {
        string body = request.Content == null ? string.Empty : await request.Content.ReadAsStringAsync(cancellationToken);
        lock (Requests)
            Requests.Add(new RecordedHttpRequest(request.Method, request.RequestUri!, body, request.Headers.Authorization?.Parameter));
        if (!_responses.TryDequeue(out Func<HttpRequestMessage, Task<HttpResponseMessage>>? response))
            throw new InvalidOperationException($"No response configured for {request.Method} {request.RequestUri}.");
        return await response(request);
    }
}

internal sealed record RecordedHttpRequest(HttpMethod Method, Uri Uri, string Body, string? AccessToken = null);

/// <summary>A clock whose delays finish only when the test advances it.</summary>
internal sealed class ManualTwitchClock(DateTimeOffset now) : ITwitchClock
{
    private sealed record Waiter(DateTimeOffset Due, TimeSpan Delay, TaskCompletionSource Completion);

    private readonly object _sync = new();
    private readonly List<Waiter> _waiters = [];
    private DateTimeOffset _now = now;

    public DateTimeOffset UtcNow
    {
        get { lock (_sync) return _now; }
    }

    public IReadOnlyList<TimeSpan> PendingDelays
    {
        get { lock (_sync) return _waiters.Select(waiter => waiter.Delay).ToList(); }
    }

    public Task DelayAsync(TimeSpan delay, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (delay <= TimeSpan.Zero)
            return Task.CompletedTask;

        var waiter = new Waiter(UtcNow + delay, delay, new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously));
        lock (_sync)
            _waiters.Add(waiter);
        cancellationToken.Register(() =>
        {
            lock (_sync)
                _waiters.Remove(waiter);
            waiter.Completion.TrySetCanceled(cancellationToken);
        });
        return waiter.Completion.Task;
    }

    public void Advance(TimeSpan by)
    {
        List<Waiter> due;
        lock (_sync)
        {
            _now += by;
            due = _waiters.Where(waiter => waiter.Due <= _now).ToList();
            foreach (Waiter waiter in due)
                _waiters.Remove(waiter);
        }
        foreach (Waiter waiter in due)
            waiter.Completion.TrySetResult();
    }
}

internal static class TwitchTestWait
{
    public static async Task UntilAsync(Func<bool> condition, string description, int timeoutMilliseconds = 5000)
    {
        var stopwatch = System.Diagnostics.Stopwatch.StartNew();
        while (!condition())
        {
            if (stopwatch.ElapsedMilliseconds > timeoutMilliseconds)
                throw new TimeoutException($"Timed out waiting for {description}.");
            await Task.Delay(10);
        }
    }
}

internal sealed class AdvancingTwitchClock(DateTimeOffset now) : ITwitchClock
{
    public DateTimeOffset UtcNow { get; private set; } = now;
    public List<TimeSpan> Delays { get; } = [];

    public Task DelayAsync(TimeSpan delay, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        Delays.Add(delay);
        UtcNow += delay;
        return Task.CompletedTask;
    }
}

internal sealed class FakeTwitchWebSocketFactory(params FakeTwitchWebSocket[] sockets) : ITwitchWebSocketFactory
{
    private readonly Queue<FakeTwitchWebSocket> _sockets = new(sockets);
    public List<Uri> ConnectedUris { get; } = [];

    public ITwitchWebSocket Create()
    {
        FakeTwitchWebSocket socket = _sockets.Dequeue();
        socket.Connected += uri => ConnectedUris.Add(uri);
        return socket;
    }
}

internal sealed class FakeTwitchWebSocket(params string[] messages) : ITwitchWebSocket
{
    private readonly Queue<string> _messages = new(messages);
    public event Action<Uri>? Connected;

    public Task ConnectAsync(Uri uri, CancellationToken cancellationToken)
    {
        Connected?.Invoke(uri);
        return Task.CompletedTask;
    }

    public async Task<string> ReceiveTextAsync(TimeSpan timeout, CancellationToken cancellationToken)
    {
        if (_messages.Count > 0)
            return _messages.Dequeue();
        await Task.Delay(Timeout.InfiniteTimeSpan, cancellationToken);
        throw new InvalidOperationException();
    }

    public Task CloseAsync(CancellationToken cancellationToken) => Task.CompletedTask;
    public void Dispose() { }
}

internal sealed class FakeEventSubClient : ITwitchEventSubClient
{
    public event EventHandler<TwitchEventSubStateChangedEventArgs>? StateChanged;
    public event EventHandler<TwitchTriggerEventArgs>? TriggerReceived;
    public event EventHandler<TwitchRevocationEventArgs>? Revoked;

    public int StartCount { get; private set; }
    public int StopCount { get; private set; }
    public TwitchEventSubCredentialProvider? Credentials { get; private set; }

    public Task StartAsync(TwitchEventSubCredentialProvider credentials, CancellationToken cancellationToken = default)
    {
        StartCount++;
        Credentials = credentials;
        StateChanged?.Invoke(this, new TwitchEventSubStateChangedEventArgs(TwitchEventSubState.Connected, string.Empty));
        return Task.CompletedTask;
    }

    /// <summary>Asks for credentials the way the real client does before a new session.</summary>
    public Task<TwitchEventSubCredentials> GetCredentialsAsync(string? rejectedAccessToken = null) =>
        Credentials!(rejectedAccessToken!, CancellationToken.None);

    public void SetState(TwitchEventSubState state, string detail = "") =>
        StateChanged?.Invoke(this, new TwitchEventSubStateChangedEventArgs(state, detail));

    public Task StopAsync()
    {
        StopCount++;
        StateChanged?.Invoke(this, new TwitchEventSubStateChangedEventArgs(TwitchEventSubState.Stopped, string.Empty));
        return Task.CompletedTask;
    }

    public ValueTask DisposeAsync() => ValueTask.CompletedTask;

    public void Emit(TwitchTriggerEvent trigger) => TriggerReceived?.Invoke(this, new TwitchTriggerEventArgs(trigger));
    public void Revoke(string type, string status) => Revoked?.Invoke(this, new TwitchRevocationEventArgs(type, status));
}
