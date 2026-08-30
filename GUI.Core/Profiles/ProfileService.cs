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
        private DecodedProfile _activeDocument;
        private JObject _activeProfile;
        private string _activeProfilePath = string.Empty;

        public ProfileService() : this(() => GenUtil.GetSaveDirectory())
        {
        }

        internal ProfileService(Func<string> getSaveDirectory)
        {
            _getSaveDirectory = getSaveDirectory ?? throw new ArgumentNullException(nameof(getSaveDirectory));
        }

        public JObject ActiveProfile => _activeProfile;
        public int SongListCount => _activeProfile != null ? GetProfileSongLists().Count : 6;

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

        public void SelectProfile(string profileName)
        {
            string profilePath = GetProfilePath(profileName);
            DecodedProfile document = ProfileCodec.Decode(profilePath);
            JObject profile = JObject.Parse(document.Json);

            _activeProfilePath = profilePath;
            _activeDocument = document;
            _activeProfile = profile;
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

        public void SaveActiveProfile()
        {
            EnsureActiveProfile();
            string json = _activeProfile.ToString(Formatting.None);
            ProfileCodec.Encode(json, _activeProfilePath, _activeDocument);
        }

        public void SaveTones(List<object> newGuitarTones, List<object> newBassTones)
        {
            EnsureActiveProfile();
            var existingGuitarTones = _activeProfile["CustomTones"]?.ToObject<List<object>>() ?? [];
            var existingBassTones = _activeProfile["BassTones"]?.ToObject<List<object>>() ?? [];

            existingGuitarTones.AddRange(newGuitarTones);
            existingBassTones.AddRange(newBassTones);
            _activeProfile["CustomTones"] = JToken.FromObject(existingGuitarTones);
            _activeProfile["BassTones"] = JToken.FromObject(existingBassTones);
            SaveActiveProfile();
        }

        public void ChangeRewardStatus(bool unlock)
        {
            EnsureActiveProfile();
            JToken prizes = _activeProfile["Prizes"];
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

            _activeProfile["Prizes"] = prizes;
        }

        public static bool ShouldIncludeSong(SongData song, HashSet<string> ownedDlc)
        {
            if (!song.Shipping || string.IsNullOrEmpty(song.Artist) || string.IsNullOrEmpty(song.Title))
                return false;

            return song.RS1AppID == 0 || ownedDlc.Contains(song.RS1AppID.ToString());
        }

        public HashSet<string> GetOwnedRs1Dlc()
        {
            EnsureActiveProfile();
            return new HashSet<string>(
                _activeProfile["Stats"]["DLCTag"]
                    .Children<JProperty>()
                    .Select(property => property.Name));
        }

        public List<List<string>> GetProfileSongLists()
        {
            EnsureActiveProfile();
            return _activeProfile["SongListsRoot"]["SongLists"].ToObject<List<List<string>>>();
        }

        public List<string> GetProfileFavoriteSongs()
        {
            EnsureActiveProfile();
            return _activeProfile["FavoritesListRoot"]["FavoritesList"].ToObject<List<string>>();
        }

        public List<List<string>> GetProfileSongListsWithFavorites()
        {
            List<List<string>> customLists = GetProfileSongLists();
            List<string> favorites = GetProfileFavoriteSongs();
            return [favorites, .. customLists];
        }

        public void SetSongInList(string dlcKey, int listIndex, bool add)
        {
            EnsureActiveProfile();
            List<string> list = _activeProfile["SongListsRoot"]["SongLists"][listIndex].ToObject<List<string>>();

            if (add && !list.Contains(dlcKey))
                list.Add(dlcKey);
            else if (!add && list.Contains(dlcKey))
                list.Remove(dlcKey);

            _activeProfile["SongListsRoot"]["SongLists"][listIndex] = JToken.FromObject(list);
        }

        public void SetSongInFavorites(string dlcKey, bool add)
        {
            EnsureActiveProfile();
            List<string> favorites = GetProfileFavoriteSongs();

            if (add && !favorites.Contains(dlcKey))
                favorites.Add(dlcKey);
            else if (!add && favorites.Contains(dlcKey))
                favorites.Remove(dlcKey);

            _activeProfile["FavoritesListRoot"]["FavoritesList"] = JToken.FromObject(favorites);
        }

        public bool AddSongList()
        {
            List<List<string>> lists = GetProfileSongLists();
            if (lists.Count >= 20)
                return false;

            lists.Add([]);
            _activeProfile["SongListsRoot"]["SongLists"] = JToken.FromObject(lists);
            SaveActiveProfile();
            return true;
        }

        public bool RemoveSongList()
        {
            List<List<string>> lists = GetProfileSongLists();
            if (lists.Count <= 6)
                return false;

            lists.RemoveAt(lists.Count - 1);
            _activeProfile["SongListsRoot"]["SongLists"] = JToken.FromObject(lists);
            SaveActiveProfile();
            return true;
        }

        private DecodedProfile LoadProfile(string profileName)
        {
            return ProfileCodec.Decode(GetProfilePath(profileName));
        }

        private void EnsureActiveProfile()
        {
            if (_activeProfile == null || _activeDocument == null || string.IsNullOrEmpty(_activeProfilePath))
                throw new InvalidOperationException("No Rocksmith profile has been selected.");
        }
    }
}
