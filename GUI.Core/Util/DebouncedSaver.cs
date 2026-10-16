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
    /// last value always reaches disk. A failed save is reported and stays unsaved, so the next change or
    /// flush tries it again and closing the app can tell it never reached disk.
    /// </para>
    /// </summary>
    public sealed class DebouncedSaver(Func<Task> save, TimeSpan delay, Action<Exception>? onError = null)
    {
        private readonly object _gate = new();
        private CancellationTokenSource? _delay;
        private Task<bool> _running = Task.FromResult(true);
        private bool _pending;

        /// <summary>True while a change is waiting to be saved (or failed to save) or a save is still running.</summary>
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

        /// <summary>
        /// Saves any pending change now, and waits for a running save. Used on exit. False when a save failed,
        /// in which case the change is still unsaved.
        /// </summary>
        public async Task<bool> FlushAsync()
        {
            while (true)
            {
                Task<bool> toAwait;
                lock (_gate)
                {
                    if (_running.IsCompleted)
                    {
                        if (!_pending)
                            return true;

                        _delay?.Cancel();
                        _pending = false;
                        _running = RunSaveAsync();
                    }

                    toAwait = _running;
                }

                if (!await toAwait)
                    return false;
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

        private async Task<bool> RunSaveAsync()
        {
            try
            {
                await save();
                return true;
            }
            catch (Exception ex)
            {
                lock (_gate)
                    _pending = true; // Not on disk, so it's still a change to save
                onError?.Invoke(ex);
                return false;
            }
        }
    }
}
