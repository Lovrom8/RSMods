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

    public DebouncedSaver Create(Func<Task> save, Action<Exception> onError)
    {
        var saver = new DebouncedSaver(save, QuietPeriod, onError);
        _savers.Add(saver);
        return saver;
    }

    public bool HasUnsavedChanges => _savers.Any(saver => saver.HasUnsavedChanges);

    public Task FlushAllAsync() => Task.WhenAll(_savers.Select(saver => saver.FlushAsync()));
}
