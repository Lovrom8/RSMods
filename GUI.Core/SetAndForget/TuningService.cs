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

        public SortedDictionary<string, ArrangementTuning> GetUnknownTunings(IEnumerable<SongData> songs)
        {
            List<ArrangementTuning> definedTunings = GetDefinedTunings().ToList();
            var result = new SortedDictionary<string, ArrangementTuning>();

            foreach (SongData song in songs)
            {
                foreach (SongArrangement arrangement in song.Arrangements)
                {
                    if (definedTunings.Contains(arrangement.Attributes.Tuning))
                        continue;

                    string label = FormatArrangementLabel(song, arrangement);
                    if (!result.ContainsKey(label))
                        result.Add(label, arrangement.Attributes.Tuning);
                }
            }

            return result;
        }

        public List<string> GetSongsWithTuning(IEnumerable<SongData> songs, ArrangementTuning tuning)
        {
            var result = new List<string>();
            foreach (SongData song in songs)
            {
                foreach (SongArrangement arrangement in song.Arrangements)
                {
                    if (arrangement.Attributes.Tuning.Equals(tuning))
                        result.Add(FormatArrangementLabel(song, arrangement));
                }
            }

            result.Sort();
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
        /// Builds an immutable lookup of the "shows up as Custom" tunings found in the scanned songs. Replaces
        /// the former shared mutable dictionary: the caller holds the result for as long as it needs it.
        /// </summary>
        public UnknownTuningLookup GetUnknownTuningLookup(IEnumerable<SongData> songs)
        {
            SortedDictionary<string, ArrangementTuning> unknown = GetUnknownTunings(songs);

            var keys = new List<string>(unknown.Count);
            var stringsByKey = new Dictionary<string, TuningStrings>(unknown.Count);
            foreach (KeyValuePair<string, ArrangementTuning> entry in unknown)
            {
                keys.Add(entry.Key);
                stringsByKey[entry.Key] = ToTuningStrings(entry.Value);
            }

            return new UnknownTuningLookup(keys, stringsByKey);
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

        private static string FormatArrangementLabel(SongData song, SongArrangement arrangement)
        {
            string prefix = string.Empty;
            if (arrangement.Attributes.ArrangementProperties.Represent == 0)
                prefix = "Alt ";
            else if (arrangement.Attributes.ArrangementProperties.BonusArr == 1)
                prefix = "Bonus ";

            return prefix + arrangement.Attributes.ArrangementName + " for " + song.Artist + " - " + song.Title;
        }
    }
}
