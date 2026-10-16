namespace RSMods.Core.Tests;

public sealed class ProfileCodecTests
{
    [Fact]
    public void EncodeAndDecode_RoundTripsLargeUnicodeProfile()
    {
        using var temporary = new TemporaryDirectory();
        string profilePath = temporary.File("profile_PRFLDB");
        string json = "{\"PlayerName\":\"J\u00f6rg \U0001F3B8\",\"Data\":\"" + new string('x', 200_000) + "\"}";
        byte[] userId = [0x12, 0x34, 0x56, 0x78];

        ProfileCodec.Encode(json, profilePath, userId);
        DecodedProfile decoded = ProfileCodec.Decode(profilePath);

        Assert.Equal(json, decoded.Json);
        Assert.Equal(userId, decoded.UserId);
    }

    [Fact]
    public void Decode_ReadsDefaultCompressionProfiles()
    {
        // Other tools save profiles at zlib's default level (header 78 9C) rather than the game's best (78 DA).
        using var temporary = new TemporaryDirectory();
        string profilePath = temporary.File("profile_PRFLDB");
        const string json = "{\"CustomTones\":[]}";

        ProfileCodec.Encode(json, profilePath, [1, 2, 3, 4], compressionLevel: 6);

        Assert.Equal(json, ProfileCodec.Decode(profilePath).Json);
    }

    [Fact]
    public void Encode_ReplacesTheProfileAndLeavesNoTemporaryFile()
    {
        using var temporary = new TemporaryDirectory();
        string profilePath = temporary.File("profile_PRFLDB");

        ProfileCodec.Encode("{\"Old\":1}", profilePath, [1, 2, 3, 4]);
        ProfileCodec.Encode("{\"New\":2}", profilePath, [1, 2, 3, 4]);

        Assert.Equal("{\"New\":2}", ProfileCodec.Decode(profilePath).Json);
        Assert.Equal(["profile_PRFLDB"], Directory.GetFiles(temporary.Path).Select(Path.GetFileName));
    }

    // The game holds its profile open while it runs; a save that can't replace it mustn't have truncated it first.
    [Fact]
    public void Encode_ThatCantReplaceTheProfileLeavesItWhole()
    {
        using var temporary = new TemporaryDirectory();
        string profilePath = temporary.File("profile_PRFLDB");
        ProfileCodec.Encode("{\"Old\":1}", profilePath, [1, 2, 3, 4]);

        using (new FileStream(profilePath, FileMode.Open, FileAccess.Read, FileShare.Read))
            Assert.ThrowsAny<IOException>(() => ProfileCodec.Encode("{\"New\":2}", profilePath, [1, 2, 3, 4]));

        Assert.Equal("{\"Old\":1}", ProfileCodec.Decode(profilePath).Json);
        Assert.Equal(["profile_PRFLDB"], Directory.GetFiles(temporary.Path).Select(Path.GetFileName));
    }
}
