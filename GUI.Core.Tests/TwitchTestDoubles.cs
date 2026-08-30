using RSMods.Twitch;
using System.Collections.Concurrent;
using System.Net;
using System.Net.Http;

namespace RSMods.Core.Tests;

internal sealed class QueueHttpMessageHandler : HttpMessageHandler
{
    private readonly ConcurrentQueue<Func<HttpRequestMessage, HttpResponseMessage>> _responses = new();

    public List<RecordedHttpRequest> Requests { get; } = [];

    public void Enqueue(HttpStatusCode statusCode, string json) =>
        _responses.Enqueue(_ => new HttpResponseMessage(statusCode)
        {
            Content = new StringContent(json)
        });

    public void Enqueue(Func<HttpRequestMessage, HttpResponseMessage> response) => _responses.Enqueue(response);

    protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
    {
        string body = request.Content == null ? string.Empty : await request.Content.ReadAsStringAsync(cancellationToken);
        lock (Requests)
            Requests.Add(new RecordedHttpRequest(request.Method, request.RequestUri!, body));
        if (!_responses.TryDequeue(out Func<HttpRequestMessage, HttpResponseMessage>? response))
            throw new InvalidOperationException($"No response configured for {request.Method} {request.RequestUri}.");
        return response(request);
    }
}

internal sealed record RecordedHttpRequest(HttpMethod Method, Uri Uri, string Body);

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
    public string? AccessToken { get; private set; }
    public string? BroadcasterUserId { get; private set; }

    public Task StartAsync(string accessToken, string broadcasterUserId, CancellationToken cancellationToken = default)
    {
        StartCount++;
        AccessToken = accessToken;
        BroadcasterUserId = broadcasterUserId;
        StateChanged?.Invoke(this, new TwitchEventSubStateChangedEventArgs(TwitchEventSubState.Connected, string.Empty));
        return Task.CompletedTask;
    }

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
