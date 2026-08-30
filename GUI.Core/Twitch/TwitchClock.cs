using System;
using System.Threading;
using System.Threading.Tasks;

namespace RSMods.Twitch
{
    public interface ITwitchClock
    {
        DateTimeOffset UtcNow { get; }
        Task DelayAsync(TimeSpan delay, CancellationToken cancellationToken);
    }

    public sealed class SystemTwitchClock : ITwitchClock
    {
        public static readonly SystemTwitchClock Instance = new SystemTwitchClock();

        private SystemTwitchClock()
        {
        }

        public DateTimeOffset UtcNow => DateTimeOffset.UtcNow;

        public Task DelayAsync(TimeSpan delay, CancellationToken cancellationToken) =>
            Task.Delay(delay, cancellationToken);
    }
}
