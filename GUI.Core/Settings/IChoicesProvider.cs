#nullable enable
using System;
using System.Collections.Generic;

namespace RSMods.Core.Settings;

/// <summary>
/// Provides dynamically enumerated choices for settings with a ChoicesSource (e.g. microphones, MIDI devices).
/// </summary>
public interface IChoicesProvider
{
    IReadOnlyList<string> GetChoices(string choicesSource);
}

/// <summary>
/// Configurable choices provider allowing registration of string list resolvers by source name.
/// </summary>
public sealed class DefaultChoicesProvider : IChoicesProvider
{
    private readonly Dictionary<string, Func<IReadOnlyList<string>>> _sources = new(StringComparer.OrdinalIgnoreCase);

    public void Register(string sourceName, Func<IReadOnlyList<string>> provider)
    {
        _sources[sourceName] = provider;
    }

    public IReadOnlyList<string> GetChoices(string choicesSource)
    {
        if (_sources.TryGetValue(choicesSource, out var provider))
        {
            try
            {
                return provider();
            }
            catch
            {
            }
        }

        return [];
    }
}
