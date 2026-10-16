using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using RSMods.Util;

namespace RSMods.Services;

/// <summary>
/// Hands each settings screen a <see cref="DebouncedSaver"/> and keeps them all, so the main window can
/// write any change still waiting out its quiet period before the app closes.
/// </summary>
internal sealed class AutoSaveService
{
    // Long enough to coalesce a held spinner or a typed value, short enough to feel immediate.
    private static readonly TimeSpan QuietPeriod = TimeSpan.FromMilliseconds(500);

    private readonly List<DebouncedSaver> _savers = [];

    /// <summary>The most recent save failure, for the message shown when closing can't save everything.</summary>
    public Exception? LastError { get; private set; }

    public DebouncedSaver Create(Func<Task> save, Action<Exception> onError)
    {
        var saver = new DebouncedSaver(save, QuietPeriod, ex =>
        {
            LastError = ex;
            onError(ex);
        });
        _savers.Add(saver);
        return saver;
    }

    public bool HasUnsavedChanges => _savers.Any(saver => saver.HasUnsavedChanges);

    /// <summary>Saves every screen's pending change; false when any of them couldn't be saved.</summary>
    public async Task<bool> FlushAllAsync()
    {
        bool[] saved = await Task.WhenAll(_savers.Select(saver => saver.FlushAsync()));
        return saved.All(ok => ok);
    }
}
