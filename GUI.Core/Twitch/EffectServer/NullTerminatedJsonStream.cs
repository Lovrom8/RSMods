using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace RSMods.Twitch.EffectServer
{
    internal sealed class NullTerminatedJsonStream(Stream stream)
    {
        private readonly Stream _stream = stream ?? throw new ArgumentNullException(nameof(stream));
        private readonly List<byte> _pending = new();

        public async Task WriteFrameAsync(string json, CancellationToken cancellationToken)
        {
            byte[] jsonBytes = Encoding.UTF8.GetBytes(json ?? string.Empty);
            byte[] frame = new byte[jsonBytes.Length + 1];

            Buffer.BlockCopy(jsonBytes, 0, frame, 0, jsonBytes.Length);

            await _stream.WriteAsync(frame, 0, frame.Length, cancellationToken).ConfigureAwait(false);
            await _stream.FlushAsync(cancellationToken).ConfigureAwait(false);
        }

        public async Task<string> ReadFrameAsync(CancellationToken cancellationToken)
        {
            while (true)
            {
                int terminator = _pending.IndexOf(0);
                if (terminator >= 0)
                {
                    byte[] frame = _pending.GetRange(0, terminator).ToArray();
                    _pending.RemoveRange(0, terminator + 1);
                    return Encoding.UTF8.GetString(frame);
                }

                byte[] buffer = new byte[1024];
                int bytesRead = await _stream.ReadAsync(buffer, 0, buffer.Length, cancellationToken).ConfigureAwait(false);
                if (bytesRead == 0)
                    throw new EndOfStreamException("The Rocksmith effect connection closed before a complete response was received.");

                for (int i = 0; i < bytesRead; i++)
                    _pending.Add(buffer[i]);

                if (_pending.Count > 1024 * 1024)
                    throw new InvalidDataException("The Rocksmith effect response exceeded the 1 MB framing limit.");
            }
        }
    }
}
