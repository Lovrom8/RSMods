using Newtonsoft.Json.Linq;

namespace RSMods.Core.Tests;

public sealed class ProfileServiceTests
{
    [Fact]
    public void SaveActiveProfile_UsesPathResolvedDuringSelection()
    {
        using var temporary = new TemporaryDirectory();
        string localProfilesPath = temporary.File("LocalProfiles.json");
        string profilePath = temporary.File("profile-id_PRFLDB");
        byte[] userId = [0x12, 0x34, 0x56, 0x78];
        string localProfilesJson = "{\"Profiles\":[{\"PlayerName\":\"Player\",\"UniqueID\":\"profile-id\"}]}";
        string profileJson = "{\"SongListsRoot\":{\"SongLists\":[[],[],[],[],[],[]]},\"Marker\":\"before\"}";
        ProfileCodec.Encode(localProfilesJson, localProfilesPath, userId);
        ProfileCodec.Encode(profileJson, profilePath, userId);
        var service = new ProfileService(() => temporary.Path);

        service.SelectProfile("Player");
        File.Delete(localProfilesPath);
        service.ActiveProfile["Marker"] = "after";
        service.SaveActiveProfile();

        JObject saved = JObject.Parse(ProfileCodec.Decode(profilePath).Json);
        Assert.Equal("after", saved["Marker"]?.ToString());
    }
}
