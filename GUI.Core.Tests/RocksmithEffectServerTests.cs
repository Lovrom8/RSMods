using Newtonsoft.Json;
using RSMods.Twitch;
using RSMods.Twitch.EffectServer;
using System.Net.Sockets;
using System.Text;

namespace RSMods.Core.Tests;

public sealed class RocksmithEffectServerTests
{
    [Fact]
    public void RequestFactory_UsesNativeDurationAndRgbContract()
    {
        var ordinary = new TwitchReward
        {
            InternalMsgEnable = "enable RainbowStrings",
            Length = 12
        };
        var solid = new TwitchReward
        {
            InternalMsgEnable = "solidnotes",
            AdditionalMsg = "A0B1C2",
            Length = 7
        };

        RocksmithEffectRequest ordinaryRequest = RocksmithEffectRequestFactory.Create(ordinary, 10, "viewer");
        RocksmithEffectRequest solidRequest = RocksmithEffectRequestFactory.Create(solid, 11, "viewer");

        Assert.Equal("rainbowstrings", ordinaryRequest.Code);
        Assert.Equal(12_000, ordinaryRequest.DurationMilliseconds);
        Assert.Empty(ordinaryRequest.Parameters);
        Assert.Equal("solidcustomrgb", solidRequest.Code);
        Assert.Equal([160, 177, 194], solidRequest.Parameters.Cast<int>());
        Assert.Equal(7_000, solidRequest.DurationMilliseconds);
    }

    [Fact]
    public async Task Server_FramesRequestsAndRetriesNativeStatusThree()
    {
        var options = new RocksmithEffectServerOptions
        {
            Port = 0,
            MaximumRetryCount = 2,
            FirstRetryDelay = TimeSpan.FromMilliseconds(10)
        };
        await using var server = new RocksmithEffectServer(options);
        var enabled = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);
        var log = new List<string>();
        server.LogMessage += message =>
        {
            log.Add(message);
            if (message.StartsWith("Enabled:", StringComparison.Ordinal))
                enabled.TrySetResult(true);
        };
        await server.StartAsync();

        using var client = new TcpClient();
        await client.ConnectAsync("127.0.0.1", server.ListeningPort);
        NetworkStream stream = client.GetStream();
        Assert.True(server.TryQueueEffect(new TwitchReward
        {
            Name = "Rainbow Strings",
            InternalMsgEnable = "rainbowstrings",
            Length = 3
        }, "test-viewer"));

        string firstJson = await ReadFrameAsync(stream);
        RocksmithEffectRequest? first = JsonConvert.DeserializeObject<RocksmithEffectRequest>(firstJson);
        Assert.True(first is not null, "Request JSON: " + firstJson + " Logs: " + string.Join(" | ", log));
        await WriteFrameFragmentedAsync(stream, JsonConvert.SerializeObject(new RocksmithEffectResponse
        {
            Id = first.Id,
            Code = first.Code,
            Status = 3,
            Type = 0
        }));

        string secondJson = await ReadFrameAsync(stream);
        RocksmithEffectRequest? second = JsonConvert.DeserializeObject<RocksmithEffectRequest>(secondJson);
        Assert.True(second is not null, "Request JSON: " + secondJson);
        Assert.NotEqual(first.Id, second.Id);
        Assert.Equal("test-viewer", second.Viewer);
        await WriteFrameFragmentedAsync(stream, JsonConvert.SerializeObject(new RocksmithEffectResponse
        {
            Id = second.Id,
            Code = second.Code,
            Status = 0,
            Type = 0
        }));

        await enabled.Task.WaitAsync(TimeSpan.FromSeconds(5));
        await server.StopAsync();
        Assert.False(server.IsRunning);
        Assert.False(server.IsConnected);
    }

    [Fact]
    public void Retries_BackOffFromHalfASecondDoublingEachTime()
    {
        var options = new RocksmithEffectServerOptions();

        Assert.Equal(5, options.MaximumRetryCount);
        Assert.Equal(
            [0.5, 1, 2, 4, 8],
            Enumerable.Range(1, options.MaximumRetryCount).Select(attempt => RocksmithEffectServer.RetryDelay(options.FirstRetryDelay, attempt).TotalSeconds));
    }

    [Fact]
    public async Task Server_DropsAnEffectAfterItsLastRetry()
    {
        var options = new RocksmithEffectServerOptions { Port = 0, FirstRetryDelay = TimeSpan.FromMilliseconds(1) };
        await using var server = new RocksmithEffectServer(options);
        var dropped = new TaskCompletionSource<string>(TaskCreationOptions.RunContinuationsAsynchronously);
        server.LogMessage += message =>
        {
            if (message.StartsWith("Dropped", StringComparison.Ordinal))
                dropped.TrySetResult(message);
        };
        await server.StartAsync();

        using var client = new TcpClient();
        await client.ConnectAsync("127.0.0.1", server.ListeningPort);
        NetworkStream stream = client.GetStream();
        Assert.True(server.TryQueueEffect(new TwitchReward { Name = "Rainbow Strings", InternalMsgEnable = "rainbowstrings", Length = 3 }));

        // The first try and five retries, each answered "retry".
        for (int request = 0; request < 1 + options.MaximumRetryCount; request++)
            await AnswerRetryAsync(stream);

        Assert.Contains("after 5 retries", await dropped.Task.WaitAsync(TimeSpan.FromSeconds(5)));
        await server.StopAsync();
    }

    [Fact]
    public async Task Server_SendsOtherEffectsWhileOneWaitsToRetry()
    {
        // A long wait: if it blocked the queue, the second effect couldn't arrive before the retry.
        var options = new RocksmithEffectServerOptions { Port = 0, FirstRetryDelay = TimeSpan.FromSeconds(30) };
        await using var server = new RocksmithEffectServer(options);
        await server.StartAsync();

        using var client = new TcpClient();
        await client.ConnectAsync("127.0.0.1", server.ListeningPort);
        NetworkStream stream = client.GetStream();
        Assert.True(server.TryQueueEffect(new TwitchReward { Name = "Rainbow Strings", InternalMsgEnable = "rainbowstrings", Length = 3 }));
        Assert.True(server.TryQueueEffect(new TwitchReward { Name = "Rainbow Notes", InternalMsgEnable = "rainbownotes", Length = 3 }));

        Assert.Equal("rainbowstrings", await AnswerRetryAsync(stream));
        string next = (await ReadFrameAsync(stream).WaitAsync(TimeSpan.FromSeconds(5)));
        Assert.Equal("rainbownotes", JsonConvert.DeserializeObject<RocksmithEffectRequest>(next)!.Code);

        await server.StopAsync().WaitAsync(TimeSpan.FromSeconds(5));
    }

    // Reads the next request and answers it with status 3 (retry); returns the request's effect code.
    private static async Task<string> AnswerRetryAsync(Stream stream)
    {
        RocksmithEffectRequest request = JsonConvert.DeserializeObject<RocksmithEffectRequest>(
            await ReadFrameAsync(stream).WaitAsync(TimeSpan.FromSeconds(5)))!;
        await WriteFrameFragmentedAsync(stream, JsonConvert.SerializeObject(new RocksmithEffectResponse
        {
            Id = request.Id,
            Code = request.Code,
            Status = 3,
            Type = 0
        }));
        return request.Code;
    }

    [Fact]
    public async Task Server_StopsCleanlyWithoutAConnectedGame()
    {
        await using var server = new RocksmithEffectServer(new RocksmithEffectServerOptions { Port = 0 });
        await server.StartAsync();

        await server.StopAsync().WaitAsync(TimeSpan.FromSeconds(5));

        Assert.False(server.IsRunning);
    }

    private static async Task<string> ReadFrameAsync(Stream stream)
    {
        using var bytes = new System.IO.MemoryStream();
        var buffer = new byte[1];
        while (true)
        {
            int read = await stream.ReadAsync(buffer);
            if (read == 0)
                throw new EndOfStreamException();
            if (buffer[0] == 0)
                return Encoding.UTF8.GetString(bytes.ToArray());
            bytes.WriteByte(buffer[0]);
        }
    }

    private static async Task WriteFrameFragmentedAsync(Stream stream, string json)
    {
        byte[] bytes = Encoding.UTF8.GetBytes(json + "\0");
        int midpoint = bytes.Length / 2;
        await stream.WriteAsync(bytes.AsMemory(0, midpoint));
        await stream.FlushAsync();
        await stream.WriteAsync(bytes.AsMemory(midpoint));
        await stream.FlushAsync();
    }
}
