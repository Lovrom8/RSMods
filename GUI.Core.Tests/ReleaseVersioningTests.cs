using System;
using RSMods.Core.Update;

namespace GUI.Core.Tests;

public class ReleaseVersioningTests
{
    [Theory]
    [InlineData("RSModsInstaller-v1.2.8.4_OnCommit", "1.2.8.4")]
    [InlineData("v1.2.8", "1.2.8")]
    [InlineData("1.2", "1.2")]
    [InlineData("release-2.0.0.0-final", "2.0.0.0")]
    public void TryExtractVersion_PullsDottedVersionFromTag(string tag, string expected)
    {
        Assert.True(ReleaseVersioning.TryExtractVersion(tag, out Version version));
        Assert.Equal(Version.Parse(expected), version);
    }

    [Theory]
    [InlineData("")]
    [InlineData("   ")]
    [InlineData(null)]
    [InlineData("no-version-here")]
    [InlineData("v1")] // a bare major with no dot is not a comparable version
    public void TryExtractVersion_ReturnsFalseWhenNoVersionPresent(string? tag)
    {
        Assert.False(ReleaseVersioning.TryExtractVersion(tag, out Version version));
        Assert.Null(version);
    }

    [Fact]
    public void ExtractedVersions_CompareSoNewerReleaseWins()
    {
        Assert.True(ReleaseVersioning.TryExtractVersion("RSModsInstaller-v1.2.8.5", out Version newer));
        Assert.True(ReleaseVersioning.TryExtractVersion("RSModsInstaller-v1.2.8.4", out Version current));
        Assert.True(newer > current);
    }
}
