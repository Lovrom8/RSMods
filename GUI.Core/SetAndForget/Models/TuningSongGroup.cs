using System.Collections.Generic;

namespace RSMods.SetAndForget.Models
{
    /// <summary>
    /// One tuning found in the scanned songs and its songs, one entry per song naming its parts in the tuning.
    /// <see cref="Label"/> names the tuning, e.g. "D A D G A D (-2 0 0 0 -2 -2)".
    /// </summary>
    public sealed class TuningSongGroup(string label, TuningStrings strings, bool bassOnly, IReadOnlyList<string> songs)
    {
        public string Label { get; } = label;
        public TuningStrings Strings { get; } = strings;

        /// <summary>Only bass parts use the tuning, so <see cref="Label"/> names just the four bass strings.</summary>
        public bool BassOnly { get; } = bassOnly;

        public IReadOnlyList<string> Songs { get; } = songs;
    }
}
