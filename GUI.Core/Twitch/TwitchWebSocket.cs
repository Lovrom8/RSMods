using System;
using System.IO;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace RSMods.Twitch
{
    public interface ITwitchWebSocket : IDisposable
    {
        Task ConnectAsync(Uri uri, CancellationToken cancellationToken);
        Task<string> ReceiveTextAsync(TimeSpan timeout, CancellationToken cancellationToken);
        Task CloseAsync(CancellationToken cancellationToken);
    }

    public interface ITwitchWebSocketFactory
    {
        ITwitchWebSocket Create();
    }

    public sealed class TwitchWebSocketFactory : ITwitchWebSocketFactory
    {
        public ITwitchWebSocket Create() => new ClientTwitchWebSocket();
    }

    internal sealed class ClientTwitchWebSocket : ITwitchWebSocket
    {
        private const int MaximumMessageBytes = 1024 * 1024;
        private readonly ClientWebSocket _socket = new ClientWebSocket();

        public ClientTwitchWebSocket()
        {
            _socket.Options.KeepAliveInterval = TimeSpan.FromSeconds(20);
        }

        public Task ConnectAsync(Uri uri, CancellationToken cancellationToken) =>
            _socket.ConnectAsync(uri, cancellationToken);

        public async Task<string> ReceiveTextAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            using var timeoutSource = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            timeoutSource.CancelAfter(timeout);

            try
            {
                using System.IO.MemoryStream message = new System.IO.MemoryStream();
                var buffer = new byte[8192];

                WebSocketReceiveResult result;
                do
                {
                    result = await _socket.ReceiveAsync(new ArraySegment<byte>(buffer), timeoutSource.Token).ConfigureAwait(false);
                    if (result.MessageType == WebSocketMessageType.Close)
                        return null;
                    if (result.MessageType != WebSocketMessageType.Text)
                        throw new InvalidDataException("Twitch EventSub sent a non-text message.");

                    message.Write(buffer, 0, result.Count);
                    if (message.Length > MaximumMessageBytes)
                        throw new InvalidDataException("Twitch EventSub message exceeded the size limit.");
                }
                while (!result.EndOfMessage);

                return Encoding.UTF8.GetString(message.ToArray());
            }
            catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
            {
                throw new TimeoutException("Twitch EventSub keepalive timed out.");
            }
        }

        public async Task CloseAsync(CancellationToken cancellationToken)
        {
            if (_socket.State == WebSocketState.Open || _socket.State == WebSocketState.CloseReceived)
            {
                await _socket.CloseAsync(WebSocketCloseStatus.NormalClosure, "RSMods stopping", cancellationToken)
                    .ConfigureAwait(false);
            }
        }

        public void Dispose() => _socket.Dispose();
    }
}
