using Newtonsoft.Json.Linq;
using RocksmithToolkitLib.DLCPackage.Manifest2014.Tone;
using System;
using System.Collections.Generic;
using System.IO;

namespace RSMods
{
    /// <summary>Imports JSON manifest and toolkit XML tones into the active profile.</summary>
    public sealed class ProfileToneImportService(ProfileService profiles)
    {
        private readonly ProfileService _profiles = profiles ?? throw new ArgumentNullException(nameof(profiles));

        public (int ImportedCount, List<string> ErrorMessages) ImportManifests(string[] fileNames)
        {
            var guitarTones = new List<object>();
            var bassTones = new List<object>();
            var errors = new List<string>();

            foreach (string fileName in fileNames)
            {
                try
                {
                    var (fileGuitarTones, fileBassTones) = ParseManifest(fileName);
                    guitarTones.AddRange(fileGuitarTones);
                    bassTones.AddRange(fileBassTones);
                }
                catch (Exception exception)
                {
                    errors.Add(exception.Message);
                }
            }

            int importedCount = guitarTones.Count + bassTones.Count;
            if (importedCount > 0)
            {
                _profiles.SaveTones(guitarTones, bassTones);
            }

            return (importedCount, errors);
        }

        public (int ImportedCount, List<string> ErrorMessages) ImportXmlTones(string[] fileNames, Func<Tone2014, bool> isGuitarPrompt)
        {
            ArgumentNullException.ThrowIfNull(isGuitarPrompt);

            var guitarTones = new List<object>();
            var bassTones = new List<object>();
            var errors = new List<string>();

            foreach (string fileName in fileNames)
            {
                try
                {
                    Tone2014 tone = Tone2014.LoadFromXmlTemplateFile(fileName);
                    if (tone == null)
                    {
                        errors.Add($"Failed to load tone from {Path.GetFileName(fileName)}");
                        continue;
                    }

                    if (isGuitarPrompt(tone))
                        guitarTones.Add(tone);
                    else
                        bassTones.Add(tone);
                }
                catch (Exception exception)
                {
                    errors.Add($"Error reading {Path.GetFileName(fileName)}: {exception.Message}");
                }
            }

            int importedCount = guitarTones.Count + bassTones.Count;
            if (importedCount > 0)
            {
                _profiles.SaveTones(guitarTones, bassTones);
            }

            return (importedCount, errors);
        }

        /// <summary>Imports toolkit XML tones while exposing only their display names to the frontend.</summary>
        public (int ImportedCount, List<string> ErrorMessages) ImportXmlTonesByName(string[] fileNames, Func<string, bool> isGuitarPromptByName)
        {
            ArgumentNullException.ThrowIfNull(isGuitarPromptByName);

            return ImportXmlTones(fileNames, tone => isGuitarPromptByName(tone.Name));
        }

        private static (List<object> GuitarTones, List<object> BassTones) ParseManifest(string fileName)
        {
            string displayName = Path.GetFileName(fileName);
            JObject manifest = JObject.Parse(File.ReadAllText(fileName));

            if (manifest["Entries"] is not JObject entries)
                throw new InvalidDataException($"Input Tone Manifest missing valid Entries: {displayName}");

            if (!entries.HasValues)
                throw new InvalidDataException($"Input Tone Manifest Entries has no children: {displayName}");

            JProperty firstEntry = GetFirstProperty(entries);

            if (firstEntry == null || string.IsNullOrEmpty(firstEntry.Name))
                throw new InvalidDataException($"Input Tone Manifest has no ArrangementId: {displayName}");

            if (firstEntry.Value.Type == JTokenType.Null || firstEntry.Value.Type == JTokenType.Undefined)
                throw new InvalidDataException($"Input Tone Manifest has invalid ArrangementId: {displayName}");

            if (firstEntry.Value["Attributes"] is not JToken attributes)
                throw new InvalidDataException($"Input Tone Manifest has no arrangement Attributes: {displayName}");

            string arrangementName = attributes["ArrangementName"]?.ToString();
            if (string.IsNullOrEmpty(arrangementName))
                throw new InvalidDataException($"Input Tone Manifest missing Arrangement Name: {displayName}");

            if (attributes["Tones"] is not JArray tones)
                throw new InvalidDataException($"Input Tone Manifest missing Tones: {displayName}");

            bool isBass = arrangementName.IndexOf("Bass", StringComparison.OrdinalIgnoreCase) >= 0;
            List<object> toneList = tones.ToObject<List<object>>();
            return isBass ? ([], toneList) : (toneList, []);
        }

        private static JProperty GetFirstProperty(JObject value)
        {
            foreach (JProperty property in value.Properties())
                return property;

            return null;
        }
    }
}
