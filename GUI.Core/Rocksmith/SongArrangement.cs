using Newtonsoft.Json.Linq;
using System;
using System.Linq;

namespace RSMods
{
    /// <summary>An arrangement's string offsets from E Standard, low E first. Compares by value.</summary>
    public sealed record ArrangementTuning(int String0, int String1, int String2, int String3, int String4, int String5);

    /// <summary>One arrangement of a song: the parts of its manifest attributes the song lists use.</summary>
    public sealed class SongArrangement
    {
        public string Name { get; init; }

        /// <summary>Null for arrangements without one (vocals, show lights).</summary>
        public ArrangementTuning Tuning { get; init; }

        public bool IsAlternate { get; init; }
        public bool IsBonus { get; init; }
        public bool PathLead { get; init; }
        public bool PathRhythm { get; init; }
        public bool PathBass { get; init; }
    }

    /// <summary>An arrangement manifest (<c>manifests/…/*.json</c> in a psarc) with the song it belongs to.</summary>
    internal sealed record ArrangementManifest(string SongKey, string Artist, string Title, bool Shipping, string Sku, int Rs1AppId, SongArrangement Arrangement)
    {
        /// <summary>
        /// Reads the first entry's Attributes, or returns null when the JSON isn't an arrangement manifest. Missing fields
        /// read as empty: custom songs leave out all sorts.
        /// </summary>
        public static ArrangementManifest Parse(string json)
        {
            if (Get(JObject.Parse(json), "Entries") is not JObject entries ||
                Get(entries.Properties().FirstOrDefault()?.Value, "Attributes") is not JObject attributes)
            {
                return null;
            }

            JToken properties = Get(attributes, "ArrangementProperties");

            return new ArrangementManifest(
                (string)Get(attributes, "SongKey"),
                (string)Get(attributes, "ArtistName"),
                (string)Get(attributes, "SongName"),
                (bool?)Get(attributes, "Shipping") ?? false,
                (string)Get(attributes, "SKU"),
                (int?)Get(Get(attributes, "DLCRS1Key")?.FirstOrDefault(), "WIN32") ?? 0,
                new SongArrangement
                {
                    Name = (string)Get(attributes, "ArrangementName"),
                    Tuning = Get(attributes, "Tuning") is JObject strings ? ReadTuning(strings) : null,
                    IsAlternate = Flag(properties, "Represent") == false,
                    IsBonus = Flag(properties, "BonusArr") == true,
                    PathLead = Flag(properties, "PathLead") == true,
                    PathRhythm = Flag(properties, "PathRhythm") == true,
                    PathBass = Flag(properties, "PathBass") == true,
                });
        }

        private static ArrangementTuning ReadTuning(JObject strings)
        {
            int String(int index) => (int?)Get(strings, "string" + index) ?? 0;
            return new ArrangementTuning(String(0), String(1), String(2), String(3), String(4), String(5));
        }

        // The manifest's 0/1 flags; null when the property isn't there.
        private static bool? Flag(JToken properties, string name) =>
            Get(properties, name) is JToken value ? (int)value != 0 : null;

        // Case-insensitive like the JSON deserializer the catalog used before; a JSON null reads as missing.
        private static JToken Get(JToken token, string name) =>
            token is JObject value && value.GetValue(name, StringComparison.OrdinalIgnoreCase) is JToken found && found.Type != JTokenType.Null
                ? found
                : null;
    }
}
