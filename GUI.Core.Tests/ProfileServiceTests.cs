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

    [Fact]
    public void AFailedSelectLeavesNoProfileActive()
    {
        using var temporary = new TemporaryDirectory();
        string profilePath = WriteProfiles(temporary);
        var service = new ProfileService(() => temporary.Path);
        service.SelectProfile("Player");
        File.WriteAllBytes(profilePath, [1, 2, 3]); // The next load of it fails

        Assert.ThrowsAny<Exception>(() => service.SelectProfile("Player"));

        // Edits must not reach the previously loaded copy and be written back over the file.
        Assert.Null(service.ActiveProfile);
        Assert.Throws<InvalidOperationException>(service.SaveActiveProfile);
    }

    [Fact]
    public void ClearActiveProfileStopsEditsAndSaves()
    {
        using var temporary = new TemporaryDirectory();
        WriteProfiles(temporary);
        var service = new ProfileService(() => temporary.Path);
        Assert.True(service.SelectProfile("Player"));

        service.ClearActiveProfile();

        Assert.Null(service.ActiveProfile);
        Assert.Equal(6, service.SongListCount);
        Assert.Throws<InvalidOperationException>(() => service.ChangeRewardStatus(unlock: true));
        Assert.Throws<InvalidOperationException>(service.SaveActiveProfile);
    }

    private static string WriteProfiles(TemporaryDirectory temporary)
    {
        string localProfilesPath = temporary.File("LocalProfiles.json");
        string profilePath = temporary.File("profile-id_PRFLDB");
        byte[] userId = [0x12, 0x34, 0x56, 0x78];
        ProfileCodec.Encode("{\"Profiles\":[{\"PlayerName\":\"Player\",\"UniqueID\":\"profile-id\"}]}", localProfilesPath, userId);
        ProfileCodec.Encode("{\"SongListsRoot\":{\"SongLists\":[[],[],[],[],[],[]]},\"Prizes\":{}}", profilePath, userId);
        return profilePath;
    }
}
