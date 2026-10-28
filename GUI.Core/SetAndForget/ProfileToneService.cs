using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using RSMods.Data;
using RSMods.Util;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace RSMods.SetAndForget
{
    /// <summary>
    /// Owns tones read from Steam profiles: enumerating the profiles, caching each tone's gear list as the
    /// profile stores it (the UI shows a name now and applies the matching gear later), and
    /// writing a chosen tone into the cache's tone manager. The tone cache belongs to this instance rather
    /// than a static dictionary, so it lives and dies with the service.
    /// </summary>
    /// <summary>The tones found in the profiles, and the profiles that couldn't be read (their file names).</summary>
    public sealed record ProfileToneScan(List<string> ToneNames, List<string> UnreadableProfiles);

    /// <summary>A profile tone's name and its GearList exactly as the profile saved it (null when absent).</summary>
    internal sealed record ProfileTone(string Name, JToken GearList);

    public sealed class ProfileToneService(CachePsarcService cache)
    {
        private const int GuitarcadeToneOffset = 8;

        private readonly CachePsarcService _cache = cache ?? throw new ArgumentNullException(nameof(cache));
        private readonly Dictionary<string, ProfileTone> _tonesByName = [];

        /// <summary>
        /// Rescans the available Steam profiles, refreshing the tone cache. Returns the distinct tone names in the order
        /// first seen, and any profile that couldn't be read, which is skipped rather than failing the rest.
        /// </summary>
        // Same folder as the Profiles tab: the configured save path or the Steam user in the registry. The userdata
        // probe alone takes whichever account it meets first, which may not be the one playing.
        public ProfileToneScan LoadProfileTones() => LoadProfileTones(GenUtil.GetSaveDirectory());

        public ProfileToneScan LoadProfileTones(string userProfileFolder)
        {
            var toneNames = new List<string>();
            var unreadable = new List<string>();

            _tonesByName.Clear();

            if (!Directory.Exists(userProfileFolder))
                return new ProfileToneScan(toneNames, unreadable);

            foreach (string profile in Directory.EnumerateFiles(userProfileFolder, "*_PRFLDB", SearchOption.AllDirectories))
            {
                List<ProfileTone> tones;
                try
                {
                    tones = ReadCustomTones(profile);
                }
                catch (Exception ex) when (ex is IOException or InvalidDataException or JsonException or UnauthorizedAccessException or
                                           System.Security.Cryptography.CryptographicException or zlib.ZStreamException)
                {
                    unreadable.Add(Path.GetFileName(profile));
                    continue;
                }

                foreach (ProfileTone tone in tones)
                {
                    if (_tonesByName.ContainsKey(tone.Name))
                        continue;

                    _tonesByName.Add(tone.Name, tone);
                    toneNames.Add(tone.Name);
                }
            }

            return new ProfileToneScan(toneNames, unreadable);
        }

        /// <summary>
        /// The tones a profile saves (its CustomTones), read with RSMods' own profile codec. The toolkit's reader only
        /// accepts the compression header the game writes, not the one other tools write. Only the tone list is
        /// parsed, so a large profile isn't held as a whole JSON tree.
        /// </summary>
        private static List<ProfileTone> ReadCustomTones(string profile)
        {
            string json = ProfileCodec.Decode(profile).Json;
            using var reader = new JsonTextReader(new StringReader(json));
            while (reader.Read())
            {
                if (reader.TokenType != JsonToken.PropertyName || reader.Depth != 1 || (string)reader.Value != "CustomTones")
                    continue;

                if (!reader.Read() || reader.TokenType != JsonToken.StartArray)
                    return [];

                return JArray.Load(reader)
                    .OfType<JObject>()
                    .Select(tone => new ProfileTone((string)tone["Name"], tone["GearList"]))
                    .Where(tone => !string.IsNullOrEmpty(tone.Name))
                    .ToList();
            }

            return [];
        }

        public (bool IsSuccess, string Message) SetDefaultTone(string selectedToneName, int selectedToneType)
        {
            if (selectedToneType < 0 || selectedToneType > 2)
                return (false, "The selected default tone target is invalid.");

            return ApplyTone(selectedToneName, selectedToneType, "Successfully changed default tones!");
        }

        public (bool IsSuccess, string Message) SetGuitarcadeTone(string selectedToneName, int selectedToneType)
        {
            if (selectedToneType < 0 || selectedToneType > 9)
                return (false, "The selected Guitarcade tone target is invalid.");

            return ApplyTone(selectedToneName, selectedToneType + GuitarcadeToneOffset, "Successfully changed Guitarcade tones!");
        }

        private (bool IsSuccess, string Message) ApplyTone(string selectedToneName, int targetToneIndex, string successMessage)
        {
            if (!_tonesByName.TryGetValue(selectedToneName, out ProfileTone selectedTone))
                return (false, $"The tone '{selectedToneName}' could not be found in the loaded profiles.");
            if (selectedTone.GearList is not JObject gearList)
                return (false, $"The tone '{selectedToneName}' has no gear list.");

            try
            {
                _cache.Modify(cache =>
                {
                    if (!cache.ExtractFile(Constants.Cache7_7zPath, Constants.ToneManager_InternalPath, Constants.CustomModsFolder) ||
                        !File.Exists(Constants.ToneManager_CustomPath))
                    {
                        throw new IOException("Could not read the tones from the game's files. Please check your existing settings.");
                    }

                    JObject tonesJson = JObject.Parse(File.ReadAllText(Constants.ToneManager_CustomPath));
                    tonesJson["Static"]["ToneManager"]["Tones"][targetToneIndex]["GearList"] = gearList.DeepClone();
                    File.WriteAllText(Constants.ToneManager_CustomPath, tonesJson.ToString());

                    cache.Inject(Constants.ToneManager_CustomPath, Constants.Cache7_7zPath, Constants.ToneManager_InternalPath);
                });
            }
            catch (Exception ex)
            {
                return (false, ex.Message);
            }

            return (true, successMessage);
        }
    }
}
