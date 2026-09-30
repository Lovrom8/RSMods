using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Rocksmith2014PsarcLib.Psarc.Models.Json;
using RSMods.Data;
using RSMods.SetAndForget.Models;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using ArrangementTuning = Rocksmith2014PsarcLib.Psarc.Models.Json.SongArrangement.ArrangementAttributes.ArrangementTuning;

namespace RSMods.SetAndForget
{
    /// <summary>
    /// Owns custom tuning definitions, their JSON/CSV persistence, and tuning queries over scanned songs.
    /// </summary>
    public sealed class TuningService
    {
        public TuningDefinitionList Tunings { get; private set; } = new TuningDefinitionList();

        /// <summary>Loads the tuning database from the frontend's custom-mods folder.</summary>
        public void Load() => Load(Constants.TuningJSON_CustomPath);

        /// <summary>Saves the tuning database back to the frontend's custom-mods folder.</summary>
        public void Save() => Save(Constants.TuningJSON_CustomPath);

        public void Load(string tuningJsonPath)
        {
            string fileContent = File.ReadAllText(tuningJsonPath);
            JObject root = JObject.Parse(fileContent);
            JToken definitions = root["Static"]["TuningDefinitions"];
            Tunings = JsonConvert.DeserializeObject<TuningDefinitionList>(definitions.ToString())
                ?? [];
        }

        public void Save(string tuningJsonPath)
        {
            string fileContent = File.ReadAllText(tuningJsonPath);
            JObject root = JObject.Parse(fileContent);
            root["Static"]["TuningDefinitions"] = JObject.FromObject(Tunings);
            File.WriteAllText(tuningJsonPath, root.ToString());
        }

        public void AddLocalizationEntries(string localizationCsvPath, string tuningJsonPath)
        {
            HashSet<int> existingIndices = LoadExistingLocalizationIndices(localizationCsvPath);
            int nextAvailableIndex = 37500;

            using var writer = new StreamWriter(localizationCsvPath, append: true);
            foreach (KeyValuePair<string, TuningDefinitionInfo> definition in Tunings)
            {
                var (indexText, name) = SplitUiName(definition.Value.UIName);
                int.TryParse(indexText, out int currentIndex);

                if (currentIndex == 0)
                {
                    currentIndex = GetNextAvailableLocalizationIndex(existingIndices, nextAvailableIndex);
                    nextAvailableIndex = currentIndex + 1;
                    definition.Value.UIName = $"$[{currentIndex}]{name}";
                }

                if (existingIndices.Add(currentIndex))
                    AppendTuningToCsv(writer, currentIndex, name);
            }

            Save(tuningJsonPath);
        }

        public static (string Index, string Name) SplitUiName(string uiName)
        {
            if (string.IsNullOrEmpty(uiName))
                return ("0", uiName);

            int startBracket = uiName.IndexOf('[');
            int endBracket = uiName.IndexOf(']');
            if (startBracket >= 0 && endBracket > startBracket)
            {
                string indexText = uiName.Substring(startBracket + 1, endBracket - startBracket - 1);
                if (int.TryParse(indexText, out _))
                    return (indexText, uiName.Substring(endBracket + 1));
            }

            return ("0", uiName);
        }

        public static ArrangementTuning ToArrangementTuning(TuningDefinitionInfo tuning)
        {
            Dictionary<string, int> strings = tuning.Strings;
            return new ArrangementTuning
            {
                String0 = strings["string0"],
                String1 = strings["string1"],
                String2 = strings["string2"],
                String3 = strings["string3"],
                String4 = strings["string4"],
                String5 = strings["string5"]
            };
        }

        public static bool IsStandard(ArrangementTuning tuning, bool forceBass = false) =>
            tuning.String0 == tuning.String1 &&
            tuning.String1 == tuning.String2 &&
            tuning.String2 == tuning.String3 &&
            (forceBass || (tuning.String3 == tuning.String4 && tuning.String4 == tuning.String5));

        public static bool IsDrop(ArrangementTuning tuning, bool forceBass = false) =>
            tuning.String0 + 2 == tuning.String1 &&
            tuning.String1 == tuning.String2 &&
            tuning.String2 == tuning.String3 &&
            (forceBass || (tuning.String3 == tuning.String4 && tuning.String4 == tuning.String5));

        public IEnumerable<ArrangementTuning> GetDefinedTunings()
        {
            return Tunings.Values.Select(ToArrangementTuning);
        }

        public List<string> GetSongsWithTuning(IEnumerable<SongData> songs, ArrangementTuning tuning)
        {
            // One entry per song, naming every part in the tuning. The catalog is already in artist/title order.
            var result = new List<string>();
            foreach (SongData song in songs)
            {
                List<SongArrangement> parts = InstrumentArrangements(song).Where(a => a.Attributes.Tuning.Equals(tuning)).ToList();
                if (parts.Count > 0)
                    result.Add(FormatSongLabel(song, parts));
            }

            return result;
        }

        // --- Frontend-facing facade (POCO/string only; never leaks ArrangementTuning) ---

        /// <summary>Songs whose arrangements match a defined tuning, resolved by its internal name.</summary>
        public List<string> GetSongsWithSelectedTuning(string internalTuningName, IEnumerable<SongData> songs)
        {
            ArrangementTuning tuning = ToArrangementTuning(Tunings[internalTuningName]);
            return GetSongsWithTuning(songs, tuning);
        }

        /// <summary>
        /// The tunings in the scanned songs that Rocksmith would show as Custom Tuning (none of the defined tunings),
        /// sorted by tuning, each with its songs, one entry per song naming its parts
        /// in the tuning: "Artist - Title (Lead &amp; Alt Bass)". A bass only tunes four strings, so a bass part joins the
        /// tuning whose first four strings match it: the song's guitar parts' tuning, or failing that another song's.
        /// </summary>
        public IReadOnlyList<TuningSongGroup> GetCustomTuningGroups(IEnumerable<SongData> songs)
        {
            List<ArrangementTuning> defined = GetDefinedTunings().ToList();
            var groups = new List<(ArrangementTuning Tuning, bool BassOnly, List<(string Song, int Parts)> Songs)>();
            var bassOnly = new List<(ArrangementTuning Tuning, string Song, int Parts)>();

            foreach (SongData song in songs)
            {
                var byTuning = new List<(ArrangementTuning Tuning, List<SongArrangement> Parts)>();
                List<SongArrangement> custom = InstrumentArrangements(song)
                    .Where(a => !defined.Contains(a.Attributes.Tuning))
                    .ToList();

                foreach (SongArrangement guitar in custom.Where(a => !IsBass(a)))
                {
                    int at = byTuning.FindIndex(entry => entry.Tuning.Equals(guitar.Attributes.Tuning));
                    if (at < 0)
                        byTuning.Add((guitar.Attributes.Tuning, [guitar]));
                    else
                        byTuning[at].Parts.Add(guitar);
                }

                var basses = new List<(ArrangementTuning Tuning, List<SongArrangement> Parts)>();
                foreach (SongArrangement bass in custom.Where(IsBass))
                {
                    int at = FindBassTuning(byTuning.Select(entry => entry.Tuning).ToList(), bass.Attributes.Tuning);
                    if (at >= 0)
                    {
                        byTuning[at].Parts.Add(bass);
                        continue;
                    }

                    int own = basses.FindIndex(entry => SameBassStrings(entry.Tuning, bass.Attributes.Tuning));
                    if (own < 0)
                        basses.Add((bass.Attributes.Tuning, [bass]));
                    else
                        basses[own].Parts.Add(bass);
                }

                foreach ((ArrangementTuning tuning, List<SongArrangement> parts) in byTuning)
                {
                    int at = groups.FindIndex(group => group.Tuning.Equals(tuning));
                    if (at < 0)
                        groups.Add((tuning, false, [(FormatSongLabel(song, parts), parts.Count)]));
                    else
                        groups[at].Songs.Add((FormatSongLabel(song, parts), parts.Count));
                }

                foreach ((ArrangementTuning tuning, List<SongArrangement> parts) in basses)
                    bassOnly.Add((tuning, FormatSongLabel(song, parts), parts.Count));
            }

            // Placed once every guitar tuning is known, so a bass-only song lands with the guitar songs it matches.
            foreach ((ArrangementTuning tuning, string song, int parts) in bassOnly)
            {
                int at = FindBassTuning(groups.Select(group => group.Tuning).ToList(), tuning);
                if (at < 0)
                    groups.Add((tuning, true, [(song, parts)]));
                else
                    groups[at].Songs.Add((song, parts));
            }

            // Highest to lowest, string by string from the low E, so tunings a step apart sit next to each other.
            return groups
                .OrderByDescending(group => group.Tuning.String0)
                .ThenByDescending(group => group.Tuning.String1)
                .ThenByDescending(group => group.Tuning.String2)
                .ThenByDescending(group => group.Tuning.String3)
                .ThenByDescending(group => group.Tuning.String4)
                .ThenByDescending(group => group.Tuning.String5)
                .Select(group => new TuningSongGroup(
                    FormatTuning(group.Tuning, group.BassOnly),
                    ToTuningStrings(group.Tuning),
                    group.BassOnly,
                    group.Songs.Select(entry => entry.Song).OrderBy(song => song, StringComparer.OrdinalIgnoreCase).ToList()))
                .ToList();
        }

        // A bass part's tuning: the exact tuning when there is one, else the first whose four bass strings match.
        private static int FindBassTuning(List<ArrangementTuning> tunings, ArrangementTuning bass)
        {
            int exact = tunings.FindIndex(tuning => tuning.Equals(bass));
            return exact >= 0 ? exact : tunings.FindIndex(tuning => SameBassStrings(tuning, bass));
        }

        private static bool SameBassStrings(ArrangementTuning a, ArrangementTuning b) =>
            a.String0 == b.String0 && a.String1 == b.String1 && a.String2 == b.String2 && a.String3 == b.String3;

        private static bool IsBass(SongArrangement arrangement) =>
            arrangement.Attributes.ArrangementProperties?.PathBass == 1 ||
            (arrangement.Attributes.ArrangementName ?? string.Empty).IndexOf("bass", StringComparison.OrdinalIgnoreCase) >= 0;

        // Vocals and show lights have no tuning to speak of.
        private static IEnumerable<SongArrangement> InstrumentArrangements(SongData song) =>
            song.Arrangements.Where(a =>
                a.Attributes.Tuning is not null &&
                (a.Attributes.ArrangementName ?? string.Empty).IndexOf("vocal", StringComparison.OrdinalIgnoreCase) < 0 &&
                !string.Equals(a.Attributes.ArrangementName, "ShowLights", StringComparison.OrdinalIgnoreCase));

        // Standard tuning's open strings as MIDI notes, low E to high E.
        private static readonly int[] StandardMidi = [40, 45, 50, 55, 59, 64];

        // Flats throughout, so a tuning a half step down reads Eb Ab Db Gb Bb Eb rather than a mix.
        private static readonly string[] NoteNames = ["C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"];

        /// <summary>
        /// A tuning's open notes and offsets, e.g. "D A D G A D (-2 0 0 0 -2 -2)". A tuning only bass parts use names just
        /// the four strings a bass has, e.g. "Bass: D A D G (-2 0 0 0)".
        /// </summary>
        public static string FormatTuning(ArrangementTuning tuning, bool bassOnly = false)
        {
            int[] offsets = [tuning.String0, tuning.String1, tuning.String2, tuning.String3, tuning.String4, tuning.String5];
            if (bassOnly)
                offsets = offsets[..4];

            IEnumerable<string> notes = offsets.Select((offset, i) => NoteNames[((StandardMidi[i] + offset) % 12 + 12) % 12]);
            string label = $"{string.Join(" ", notes)} ({string.Join(" ", offsets)})";
            return bassOnly ? "Bass: " + label : label;
        }

        private static TuningStrings ToTuningStrings(ArrangementTuning tuning) => new()
        {
            String0 = tuning.String0,
            String1 = tuning.String1,
            String2 = tuning.String2,
            String3 = tuning.String3,
            String4 = tuning.String4,
            String5 = tuning.String5
        };

        public List<string> GetSongsWithBadBassTuning(IEnumerable<SongData> songs)
        {
            var result = new List<string>();
            foreach (SongData song in songs)
            {
                foreach (SongArrangement arrangement in song.Arrangements)
                {
                    if (arrangement.Attributes.ArrangementName.IndexOf("bass", StringComparison.OrdinalIgnoreCase) < 0)
                        continue;

                    ArrangementTuning tuning = arrangement.Attributes.Tuning;
                    if (IsStandard(tuning) || IsDrop(tuning))
                        continue;

                    string label = FormatArrangementLabel(song, arrangement);
                    if (result.Contains(label))
                        continue;

                    if (!song.ODLC &&
                        !(tuning.String0 == 0 || tuning.String1 == 0 || tuning.String2 == 0 || tuning.String3 == 0) &&
                        ((tuning.String4 == 0 && tuning.String5 == 0) ||
                         (tuning.String4 == 12 && tuning.String5 == 12)))
                    {
                        result.Add(label);
                    }
                }
            }

            return result;
        }

        private static HashSet<int> LoadExistingLocalizationIndices(string filePath)
        {
            var indices = new HashSet<int>();
            if (!File.Exists(filePath))
                return indices;

            foreach (string line in File.ReadLines(filePath))
            {
                string[] parts = line.Split(',');
                if (parts.Length > 0 && int.TryParse(parts[0], out int index))
                    indices.Add(index);
            }

            return indices;
        }

        private static int GetNextAvailableLocalizationIndex(HashSet<int> existingIndices, int startingIndex)
        {
            int currentIndex = startingIndex;
            while (existingIndices.Contains(currentIndex))
                currentIndex++;

            return currentIndex;
        }

        private static void AppendTuningToCsv(StreamWriter writer, int index, string tuningName)
        {
            string repeatedNames = string.Join(",", Enumerable.Repeat(tuningName, 7));
            writer.Write($"{Environment.NewLine}{index},{repeatedNames}");
        }

        private static string FormatArrangementLabel(SongData song, SongArrangement arrangement) =>
            FormatArrangementName(arrangement) + " for " + song.Artist + " - " + song.Title;

        private static string FormatArrangementName(SongArrangement arrangement)
        {
            string prefix = string.Empty;
            if (arrangement.Attributes.ArrangementProperties.Represent == 0)
                prefix = "Alt ";
            else if (arrangement.Attributes.ArrangementProperties.BonusArr == 1)
                prefix = "Bonus ";

            return prefix + arrangement.Attributes.ArrangementName;
        }

        /// <summary>
        /// A song and its parts, lead to bass with each main part before its alternates: "Artist - Title (Lead &amp; Alt
        /// Bass)". When the parts are all of the song's, and more than one, it's "Artist - Title (All)"; a single part keeps
        /// its name, so a bass-only song still says so.
        /// </summary>
        private static string FormatSongLabel(SongData song, IReadOnlyCollection<SongArrangement> parts)
        {
            if (parts.Count > 1 && parts.Count == InstrumentArrangements(song).Count())
                return $"{song.Artist} - {song.Title} (All)";

            IEnumerable<string> names = parts
                .OrderBy(PathOrder)
                .ThenBy(a => a.Attributes.ArrangementProperties.Represent == 0 ? 1 : a.Attributes.ArrangementProperties.BonusArr == 1 ? 2 : 0)
                .Select(FormatArrangementName)
                .Distinct(StringComparer.OrdinalIgnoreCase);

            return $"{song.Artist} - {song.Title} ({string.Join(" & ", names)})";
        }

        // Lead, Rhythm, Combo, Bass. By name first: some custom songs set the path flags wrongly, which put a Rhythm
        // before its Lead.
        private static int PathOrder(SongArrangement arrangement)
        {
            string name = arrangement.Attributes.ArrangementName ?? string.Empty;
            if (name.IndexOf("lead", StringComparison.OrdinalIgnoreCase) >= 0)
                return 0;
            if (name.IndexOf("rhythm", StringComparison.OrdinalIgnoreCase) >= 0)
                return 1;
            if (name.IndexOf("combo", StringComparison.OrdinalIgnoreCase) >= 0)
                return 2;
            if (IsBass(arrangement))
                return 3;

            var properties = arrangement.Attributes.ArrangementProperties;
            return properties.PathLead == 1 ? 0 : properties.PathRhythm == 1 ? 1 : 4;
        }
    }
}
