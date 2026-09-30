using RSMods.SetAndForget;

namespace RSMods.Core.Tests;

public sealed class ProfileToneServiceTests
{
    [Fact]
    public void LoadProfileTones_ReadsEveryProfileAndSkipsOnesThatCantBeRead()
    {
        using var folder = new TemporaryDirectory();
        // Saved by another tool, at zlib's default level: the toolkit's reader refused these.
        ProfileCodec.Encode(
            """{"CustomTones":[{"Name":"Clean","Key":"Clean"},{"Name":"Thrashy","Key":"Thrashy"}],"Tones":[null]}""",
            folder.File("A_PRFLDB"), [1, 2, 3, 4], compressionLevel: 6);
        ProfileCodec.Encode(
            """{"CustomTones":[{"Name":"Clean","Key":"Clean"},{"Name":"Solo","Key":"Solo"}]}""",
            folder.File("B_PRFLDB"), [1, 2, 3, 4]);
        File.WriteAllText(folder.File("C_PRFLDB"), "not a profile");

        ProfileToneScan scan = new ProfileToneService(new CachePsarcService()).LoadProfileTones(folder.Path);

        Assert.Equal(["Clean", "Thrashy", "Solo"], scan.ToneNames);
        Assert.Equal(["C_PRFLDB"], scan.UnreadableProfiles);
    }
}
