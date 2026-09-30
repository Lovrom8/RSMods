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
}
