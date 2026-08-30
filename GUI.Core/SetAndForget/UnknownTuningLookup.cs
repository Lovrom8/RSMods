using RSMods.SetAndForget.Models;
using System.Collections.Generic;

namespace RSMods.SetAndForget
{
    /// <summary>
    /// An immutable snapshot of the tunings that "show up as Custom" for a scanned song library: their
    /// display keys (in the order the tuning service produced them) and the six string offsets behind each.
    /// Produced by <see cref="TuningService.GetUnknownTuningLookup"/>; held by the caller instead of the
    /// service, so there is no shared mutable state between the scan and its later use.
    /// </summary>
    public sealed class UnknownTuningLookup
    {
        private readonly IReadOnlyDictionary<string, TuningStrings> _stringsByKey;

        internal UnknownTuningLookup(IReadOnlyList<string> keys, IReadOnlyDictionary<string, TuningStrings> stringsByKey)
        {
            Keys = keys;
            _stringsByKey = stringsByKey;
        }

        public IReadOnlyList<string> Keys { get; }

        public TuningStrings GetStrings(string key) => _stringsByKey[key];
    }
}
