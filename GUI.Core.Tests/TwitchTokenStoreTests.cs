using RSMods.Twitch;
using System.Text;

namespace RSMods.Core.Tests;

public sealed class TwitchTokenStoreTests
{
    [Fact]
    public void LoadOrImportLegacy_ProtectsTokenAndRemovesPlaintextSetting()
    {
        using var temporary = new TemporaryDirectory();
        string settingsPath = temporary.File("GUI_Settings.ini");
        string tokenPath = temporary.File("TwitchAuth.dat");
        const string secret = "legacy-super-secret-token";
        File.WriteAllLines(settingsPath, ["AccessToken = " + secret, "Unknown = preserved"]);
        var store = new TwitchTokenStore(tokenPath, new TestTokenProtector());

        TwitchTokenSet? imported = store.LoadOrImportLegacy(settingsPath, "client-id");

        Assert.NotNull(imported);
        Assert.Equal(secret, imported.AccessToken);
        Assert.Equal("client-id", imported.ClientId);
        Assert.DoesNotContain(secret, File.ReadAllText(settingsPath));
        Assert.Contains("Unknown = preserved", File.ReadAllText(settingsPath));
        Assert.DoesNotContain(secret, Encoding.UTF8.GetString(File.ReadAllBytes(tokenPath)));

        TwitchTokenSet? loaded = store.Load();
        Assert.NotNull(loaded);
        Assert.Equal(secret, loaded.AccessToken);
    }

    [Fact]
    public void LoadOrImportLegacy_RemovesLeftoverPlaintextWhenTokenIsAlreadyStored()
    {
        using var temporary = new TemporaryDirectory();
        string settingsPath = temporary.File("GUI_Settings.ini");
        var store = new TwitchTokenStore(temporary.File("TwitchAuth.dat"), new TestTokenProtector());
        store.Save(new TwitchTokenSet { AccessToken = "stored-access", RefreshToken = "stored-refresh" });
        // An earlier import saved the token, then failed to rewrite the settings file.
        File.WriteAllLines(settingsPath, ["AccessToken = leftover-plaintext", "Unknown = preserved"]);

        TwitchTokenSet? loaded = store.LoadOrImportLegacy(settingsPath, "client-id");

        Assert.NotNull(loaded);
        Assert.Equal("stored-access", loaded.AccessToken);
        Assert.DoesNotContain("leftover-plaintext", File.ReadAllText(settingsPath));
        Assert.Contains("Unknown = preserved", File.ReadAllText(settingsPath));
    }

    [Fact]
    public void Save_RoundTripsRefreshMetadata()
    {
        using var temporary = new TemporaryDirectory();
        var store = new TwitchTokenStore(temporary.File("TwitchAuth.dat"), new TestTokenProtector());
        var expected = new TwitchTokenSet
        {
            AccessToken = "access",
            RefreshToken = "refresh",
            ClientId = "client",
            ExpiresAtUtc = DateTimeOffset.UtcNow.AddHours(4),
            Scopes = ["bits:read", "channel:read:redemptions"]
        };

        store.Save(expected);
        TwitchTokenSet? actual = store.Load();

        Assert.NotNull(actual);
        Assert.Equal(expected.AccessToken, actual.AccessToken);
        Assert.Equal(expected.RefreshToken, actual.RefreshToken);
        Assert.Equal(expected.ClientId, actual.ClientId);
        Assert.Equal(expected.Scopes, actual.Scopes);
        Assert.Equal(expected.ExpiresAtUtc, actual.ExpiresAtUtc);
    }

    [Fact]
    public void Save_WithoutProtector_WritesNothing()
    {
        using var temporary = new TemporaryDirectory();
        string tokenPath = temporary.File("TwitchAuth.dat");
        var store = new TwitchTokenStore(tokenPath, null);

        store.Save(new TwitchTokenSet { AccessToken = "never-plaintext" });

        Assert.False(File.Exists(tokenPath));
        Assert.Null(store.Load());
    }

    [WindowsFact]
    public void DefaultStore_EncryptsWithDpapi()
    {
        using var temporary = new TemporaryDirectory();
        string tokenPath = temporary.File("TwitchAuth.dat");
        const string secret = "dpapi-secret-token";
        var store = new TwitchTokenStore(tokenPath);

        store.Save(new TwitchTokenSet { AccessToken = secret });

        Assert.DoesNotContain(secret, Encoding.UTF8.GetString(File.ReadAllBytes(tokenPath)));
        Assert.Equal(secret, store.Load()?.AccessToken);
    }
}
