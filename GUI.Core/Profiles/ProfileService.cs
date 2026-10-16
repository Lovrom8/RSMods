using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using RSMods.Util;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace RSMods
{
    public sealed class ProfileService
    {
        private readonly Func<string> _getSaveDirectory;
        private readonly object _gate = new();
        // Path, file and decoded JSON are swapped as one, so a save running beside a reselect can't write one
        // profile's JSON into another's file.
        private volatile Selected _active;
        private long _selection; // Bumped by every select/clear, so only the latest one is made active

        public ProfileService() : this(() => GenUtil.GetSaveDirectory())
        {
        }

        internal ProfileService(Func<string> getSaveDirectory)
        {
            _getSaveDirectory = getSaveDirectory ?? throw new ArgumentNullException(nameof(getSaveDirectory));
        }

        private sealed record Selected(string Path, DecodedProfile Document, JObject Profile);

        public JObject ActiveProfile => _active?.Profile;
        public int SongListCount => _active is { } active ? GetSongLists(active.Profile).Count : 6;

        public Dictionary<string, string> GetAvailableProfiles() => GetAvailableProfiles(_getSaveDirectory());

        private static Dictionary<string, string> GetAvailableProfiles(string saveDirectory)
        {
            var profiles = new Dictionary<string, string>();
            if (string.IsNullOrEmpty(saveDirectory)) return profiles;

            try
            {
                string localProfilesPath = Path.Combine(saveDirectory, "LocalProfiles.json");
                JObject localProfiles = JObject.Parse(ProfileCodec.Decode(localProfilesPath).Json);
                foreach (JToken profile in localProfiles.SelectToken("Profiles"))
                {
                    profiles.Add(
                        profile.SelectToken("PlayerName").ToString(),
                        profile.SelectToken("UniqueID").ToString());
                }
            }
            catch
            {
                // Preserve the legacy behavior: a corrupt/missing profile list is presented as empty.
            }

            return profiles;
        }

        public static string DecodeProfileJson(string path, bool dumpToFile = false, string dumpFile = "profileDump.json")
        {
            try
            {
                return ProfileCodec.Decode(path, dumpToFile, dumpFile).Json;
            }
            catch
            {
                return string.Empty;
            }
        }

        /// <summary>
        /// Decodes a profile and makes it the active one. The previous profile stops being active straight away, so a
        /// load that fails leaves no profile active rather than the old one. False when a newer select or
        /// <see cref="ClearActiveProfile"/> came in while this one was decoding; the newer one wins.
        /// </summary>
        public bool SelectProfile(string profileName)
        {
            long selection;
            lock (_gate)
            {
                selection = ++_selection;
                _active = null;
            }

            string profilePath = GetProfilePath(profileName);
            DecodedProfile document = ProfileCodec.Decode(profilePath);
            JObject profile = JObject.Parse(document.Json);

            lock (_gate)
            {
                if (selection != _selection)
                    return false;
                _active = new Selected(profilePath, document, profile);
                return true;
            }
        }

        /// <summary>Drops the active profile, e.g. before its files are replaced or the save folder changes.</summary>
        public void ClearActiveProfile()
        {
            lock (_gate)
            {
                _selection++;
                _active = null;
            }
        }

        public int GetSongListCount(string profileName)
        {
            try
            {
                JObject profile = JObject.Parse(LoadProfile(profileName).Json);
                return ((JArray)profile["SongListsRoot"]["SongLists"]).Count;
            }
            catch
            {
                return 0;
            }
        }

        public string GetProfilePath(string profileName)
        {
            string saveDirectory = _getSaveDirectory();
            return Path.Combine(saveDirectory, GetAvailableProfiles(saveDirectory)[profileName] + "_PRFLDB");
        }

        public void SaveActiveProfile() => Save(RequireActive());

        private static void Save(Selected active)
        {
            string json = active.Profile.ToString(Formatting.None);
            ProfileCodec.Encode(json, active.Path, active.Document);
        }

        public void SaveTones(List<object> newGuitarTones, List<object> newBassTones)
        {
            Selected active = RequireActive();
            JObject profile = active.Profile;
            var existingGuitarTones = profile["CustomTones"]?.ToObject<List<object>>() ?? [];
            var existingBassTones = profile["BassTones"]?.ToObject<List<object>>() ?? [];

            existingGuitarTones.AddRange(newGuitarTones);
            existingBassTones.AddRange(newBassTones);
            profile["CustomTones"] = JToken.FromObject(existingGuitarTones);
            profile["BassTones"] = JToken.FromObject(existingBassTones);
            Save(active);
        }

        public void ChangeRewardStatus(bool unlock)
        {
            JObject profile = RequireActive().Profile;
            JToken prizes = profile["Prizes"];
            prizes["NumPrizePoints"] = unlock ? 1300.0 : 0.0;
            prizes["UnawardedPrizePoints"] = 0.0;

            for (int index = 1; index <= 4; index++)
            {
                prizes[$"UplayAction{index}"] = true;
                prizes[$"UplayDlg{index}"] = true;
            }

            var skippedRewards = new HashSet<int>
            {
                7, 14, 22, 29, 36, 45, 60, 68, 77, 83, 93, 100, 109, 117, 125, 131,
                141, 148, 154, 155, 164, 169, 184
            };

            for (int index = 1; index < 187; index++)
            {
                if (skippedRewards.Contains(index))
                    continue;

                prizes[$"IsPrizeAwarded_{index}"] = unlock;
                prizes[$"HasPrizeDialogShown_{index}"] = unlock;
            }

            profile["Prizes"] = prizes;
        }

        public static bool ShouldIncludeSong(SongData song, HashSet<string> ownedDlc)
        {
            if (!song.Shipping || string.IsNullOrEmpty(song.Artist) || string.IsNullOrEmpty(song.Title))
                return false;

            return song.RS1AppID == 0 || ownedDlc.Contains(song.RS1AppID.ToString());
        }

        public HashSet<string> GetOwnedRs1Dlc()
        {
            return new HashSet<string>(
                RequireActive().Profile["Stats"]["DLCTag"]
                    .Children<JProperty>()
                    .Select(property => property.Name));
        }

        public List<List<string>> GetProfileSongLists() => GetSongLists(RequireActive().Profile);

        private static List<List<string>> GetSongLists(JObject profile) =>
            profile["SongListsRoot"]["SongLists"].ToObject<List<List<string>>>();

        public List<string> GetProfileFavoriteSongs() => GetFavorites(RequireActive().Profile);

        private static List<string> GetFavorites(JObject profile) =>
            profile["FavoritesListRoot"]["FavoritesList"].ToObject<List<string>>();

        public List<List<string>> GetProfileSongListsWithFavorites()
        {
            JObject profile = RequireActive().Profile;
            List<List<string>> customLists = GetSongLists(profile);
            List<string> favorites = GetFavorites(profile);
            return [favorites, .. customLists];
        }

        public void SetSongInList(string dlcKey, int listIndex, bool add)
        {
            JObject profile = RequireActive().Profile;
            List<string> list = profile["SongListsRoot"]["SongLists"][listIndex].ToObject<List<string>>();

            if (add && !list.Contains(dlcKey))
                list.Add(dlcKey);
            else if (!add && list.Contains(dlcKey))
                list.Remove(dlcKey);

            profile["SongListsRoot"]["SongLists"][listIndex] = JToken.FromObject(list);
        }

        public void SetSongInFavorites(string dlcKey, bool add)
        {
            JObject profile = RequireActive().Profile;
            List<string> favorites = GetFavorites(profile);

            if (add && !favorites.Contains(dlcKey))
                favorites.Add(dlcKey);
            else if (!add && favorites.Contains(dlcKey))
                favorites.Remove(dlcKey);

            profile["FavoritesListRoot"]["FavoritesList"] = JToken.FromObject(favorites);
        }

        public bool AddSongList()
        {
            Selected active = RequireActive();
            List<List<string>> lists = GetSongLists(active.Profile);
            if (lists.Count >= 20)
                return false;

            lists.Add([]);
            active.Profile["SongListsRoot"]["SongLists"] = JToken.FromObject(lists);
            Save(active);
            return true;
        }

        public bool RemoveSongList()
        {
            Selected active = RequireActive();
            List<List<string>> lists = GetSongLists(active.Profile);
            if (lists.Count <= 6)
                return false;

            lists.RemoveAt(lists.Count - 1);
            active.Profile["SongListsRoot"]["SongLists"] = JToken.FromObject(lists);
            Save(active);
            return true;
        }

        private DecodedProfile LoadProfile(string profileName)
        {
            return ProfileCodec.Decode(GetProfilePath(profileName));
        }

        // Read once per operation: everything an edit or save touches comes from the same selected profile.
        private Selected RequireActive() =>
            _active ?? throw new InvalidOperationException("No Rocksmith profile has been selected.");
    }
}
