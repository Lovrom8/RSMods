#nullable enable
using RSMods;

namespace RSMods.Core.Tests;

// MacLocations reads RS_NATIVE_HOME, which off Windows GenUtil's lookups use too, so no other test may run beside these.
[CollectionDefinition(nameof(MacLocationsCollection), DisableParallelization = true)]
public sealed class MacLocationsCollection;

[Collection(nameof(MacLocationsCollection))]
public sealed class MacLocationsTests : IDisposable
{
    private const string HomeVariable = "RS_NATIVE_HOME";

    private readonly string? _originalHome = Environment.GetEnvironmentVariable(HomeVariable);
    private readonly TemporaryDirectory _home = new();

    public MacLocationsTests() => Environment.SetEnvironmentVariable(HomeVariable, _home.Path);

    public void Dispose()
    {
        Environment.SetEnvironmentVariable(HomeVariable, _originalHome);
        _home.Dispose();
    }

    private string AddAccount(string account, bool withProfiles)
    {
        string remote = Path.Combine(_home.Path, "steam", account, "221680", "remote");
        Directory.CreateDirectory(remote);
        if (withProfiles)
            File.WriteAllText(Path.Combine(remote, "LocalProfiles.json"), "{}");
        return remote;
    }

    [Fact]
    public void TheEnvironmentOverridesTheHomeFolder()
    {
        Assert.Equal(_home.Path, MacLocations.Home);
        Assert.Equal(Path.Combine(_home.Path, "game"), MacLocations.GameFolder);
    }

    [Theory]
    [InlineData(null)]
    [InlineData("")]
    public void WithoutItTheHomeFolderIsInApplicationSupport(string? value)
    {
        Environment.SetEnvironmentVariable(HomeVariable, value);

        Assert.EndsWith(Path.Combine("Library", "Application Support", "RS Re-ARMed"), MacLocations.Home);
    }

    [Fact]
    public void NoSteamFolderMeansNoSaves()
    {
        Assert.Equal(string.Empty, MacLocations.SaveFolder());
    }

    [Fact]
    public void AnAccountWithoutProfilesIsSkipped()
    {
        AddAccount("111", withProfiles: false);

        Assert.Equal(string.Empty, MacLocations.SaveFolder());

        string remote = AddAccount("222", withProfiles: true);

        Assert.Equal(remote, MacLocations.SaveFolder());
    }
}
