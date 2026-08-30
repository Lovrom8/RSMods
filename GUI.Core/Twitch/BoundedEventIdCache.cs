using System;
using System.Collections.Generic;

namespace RSMods.Twitch
{
    internal sealed class BoundedEventIdCache
    {
        private sealed class Entry
        {
            public string Id { get; set; }
            public DateTimeOffset ExpiresAtUtc { get; set; }
        }

        private readonly int _capacity;
        private readonly TimeSpan _ttl;
        private readonly Dictionary<string, DateTimeOffset> _ids = new Dictionary<string, DateTimeOffset>(StringComparer.Ordinal);
        private readonly Queue<Entry> _order = new();
        private readonly object _sync = new();

        public BoundedEventIdCache(int capacity, TimeSpan ttl)
        {
            if (capacity <= 0)
                throw new ArgumentOutOfRangeException(nameof(capacity));
            if (ttl <= TimeSpan.Zero)
                throw new ArgumentOutOfRangeException(nameof(ttl));

            _capacity = capacity;
            _ttl = ttl;
        }

        public bool TryAdd(string id, DateTimeOffset now)
        {
            if (string.IsNullOrWhiteSpace(id))
                return false;

            lock (_sync)
            {
                Prune(now);
                if (_ids.ContainsKey(id))
                {
                    return false;
                }

                DateTimeOffset expiresAt = now.Add(_ttl);
                _ids[id] = expiresAt;
                _order.Enqueue(new Entry { Id = id, ExpiresAtUtc = expiresAt });

                while (_ids.Count > _capacity)
                {
                    RemoveOldest();
                }

                return true;
            }
        }

        private void Prune(DateTimeOffset now)
        {
            while (_order.Count > 0 && _order.Peek().ExpiresAtUtc <= now)
            {
                RemoveOldest();
            }
        }

        private void RemoveOldest()
        {
            if (_order.Count == 0)
                return;

            Entry entry = _order.Dequeue();
            if (_ids.TryGetValue(entry.Id, out DateTimeOffset currentExpiry) && currentExpiry == entry.ExpiresAtUtc)
            {
                _ids.Remove(entry.Id);
            }
        }
    }
}
