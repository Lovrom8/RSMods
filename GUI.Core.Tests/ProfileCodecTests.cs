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
}
