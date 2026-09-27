#nullable enable
using System;
using System.Threading;
using System.Threading.Tasks;

namespace RSMods.Util
{
    /// <summary>
    /// Saves a settings screen shortly after its last change, the way the WinForms configurator saved on
    /// every change, without writing the file (and pinging the game) on every step of a held spinner.
    /// <para>
    /// Saves never overlap: a change made while one is running is saved by one more run afterwards, so the
    /// last value always reaches disk. A failed save is reported and doesn't stop later ones.
    /// </para>
    /// </summary>
    public sealed class DebouncedSaver(Func<Task> save, TimeSpan delay, Action<Exception>? onError = null)
    {
        private readonly object _gate = new();
        private CancellationTokenSource? _delay;
        private Task _running = Task.CompletedTask;
        private bool _pending;

        /// <summary>True while a change is waiting to be saved or a save is still running.</summary>
        public bool HasUnsavedChanges
        {
            get
            {
                lock (_gate)
                    return _pending || !_running.IsCompleted;
            }
        }

        /// <summary>Records a change and (re)starts the quiet period before it's saved.</summary>
        public void Request()
        {
            CancellationTokenSource delayToken;
            lock (_gate)
            {
                _pending = true;
                _delay?.Cancel();
                delayToken = _delay = new CancellationTokenSource();
            }

            _ = SaveAfterDelayAsync(delayToken.Token);
        }

        /// <summary>Saves any pending change now, and waits for a running save. Used on exit.</summary>
        public async Task FlushAsync()
        {
            while (true)
            {
                Task toAwait;
                lock (_gate)
                {
                    if (_running.IsCompleted)
                    {
                        if (!_pending)
                            return;

                        _delay?.Cancel();
                        _pending = false;
                        _running = RunSaveAsync();
                    }

                    toAwait = _running;
                }

                await toAwait;
            }
        }

        private async Task SaveAfterDelayAsync(CancellationToken token)
        {
            try
            {
                await Task.Delay(delay, token);
            }
            catch (OperationCanceledException)
            {
                return; // A newer change restarted the quiet period.
            }

            await FlushAsync();
        }

        private async Task RunSaveAsync()
        {
            try
            {
                await save();
            }
            catch (Exception ex)
            {
                onError?.Invoke(ex);
            }
        }
    }
}
