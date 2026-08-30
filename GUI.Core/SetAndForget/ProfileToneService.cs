using Newtonsoft.Json.Linq;
using RocksmithToolkitLib.DLCPackage.Manifest2014.Tone;
using RSMods.Data;
using RSMods.Util;
using System;
using System.Collections.Generic;
using System.IO;

namespace RSMods.SetAndForget
{
    /// <summary>
    /// Owns tones read from Steam profiles: enumerating the profiles, caching the imported
    /// <see cref="Tone2014"/> objects (the UI shows a name now and applies the matching gear later), and
    /// writing a chosen tone into the cache's tone manager. The tone cache belongs to this instance rather
    /// than a static dictionary, so it lives and dies with the service.
    /// </summary>
    public sealed class ProfileToneService(CachePsarcService cache)
    {
        private const int GuitarcadeToneOffset = 8;

        private readonly CachePsarcService _cache = cache ?? throw new ArgumentNullException(nameof(cache));
        private readonly Dictionary<string, Tone2014> _tonesByName = [];

        /// <summary>
        /// Rescans the available Steam profiles, refreshing the tone cache, and returns the distinct tone
        /// names in the order first seen.
        /// </summary>
        public List<string> LoadProfileTones()
        {
            var toneNames = new List<string>();
            string userProfileFolder = GenUtil.GetSteamProfilesFolderManual();

            _tonesByName.Clear();

            if (!Directory.Exists(userProfileFolder))
                return toneNames;

            foreach (string profile in Directory.EnumerateFiles(userProfileFolder, "*_PRFLDB", SearchOption.AllDirectories))
            {
                foreach (Tone2014 tone in Tone2014.Import(profile))
                {
                    if (_tonesByName.ContainsKey(tone.Name))
                        continue;

                    _tonesByName.Add(tone.Name, tone);
                    toneNames.Add(tone.Name);
                }
            }

            return toneNames;
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
            if (!_tonesByName.TryGetValue(selectedToneName, out Tone2014 selectedTone))
                return (false, $"The tone '{selectedToneName}' could not be found in the loaded profiles.");

            try
            {
                _cache.Modify(cache =>
                {
                    if (!ZipUtilities.ExtractSingleFile(Constants.CustomModsFolder, Constants.Cache7_7zPath, Constants.ToneManager_InternalPath) ||
                        !File.Exists(Constants.ToneManager_CustomPath))
                    {
                        throw new IOException("Could not extract tones from cache.psarc. Please check your existing settings.");
                    }

                    JObject tonesJson = JObject.Parse(File.ReadAllText(Constants.ToneManager_CustomPath));
                    tonesJson["Static"]["ToneManager"]["Tones"][targetToneIndex]["GearList"] = JObject.FromObject(selectedTone.GearList);
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
