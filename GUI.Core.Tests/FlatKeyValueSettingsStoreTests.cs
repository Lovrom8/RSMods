using RSMods;

namespace RSMods.Core.Tests;

public sealed class FlatKeyValueSettingsStoreTests
{
    [Fact]
    public void UpdateFile_PreservesUnknownContentAndValuesContainingEquals()
    {
        using var temporary = new TemporaryDirectory();
        string path = temporary.File("GUI_Settings.ini");
        File.WriteAllLines(path,
        [
            "; user comment",
            "RSPath = old",
            string.Empty,
            "UnknownKey = keep=this=value",
            "# another comment"
        ]);

        FlatKeyValueSettingsStore.UpdateFile(path, settings =>
        {
            settings.SetString("RSPath", "new path");
            settings.SetString("SavePath", "save path");
        });

        string[] lines = File.ReadAllLines(path);
        Assert.Equal("; user comment", lines[0]);
        Assert.Equal("RSPath = new path", lines[1]);
        Assert.Equal(string.Empty, lines[2]);
        Assert.Equal("UnknownKey = keep=this=value", lines[3]);
        Assert.Equal("# another comment", lines[4]);
        Assert.Equal("SavePath = save path", lines[5]);

        var reloaded = new FlatKeyValueSettingsStore(path);
        Assert.Equal("keep=this=value", reloaded.GetString("UnknownKey"));
    }

    [Fact]
    public void Remove_DeletesOnlyTheNamedActiveSetting()
    {
        using var temporary = new TemporaryDirectory();
        string path = temporary.File("GUI_Settings.ini");
        File.WriteAllLines(path,
        [
            "AccessToken = secret",
            ";AccessToken = commented-value",
            "Other = value"
        ]);

        FlatKeyValueSettingsStore.UpdateFile(path, settings => settings.Remove("AccessToken"));

        string contents = File.ReadAllText(path);
        Assert.DoesNotContain("AccessToken = secret", contents);
        Assert.Contains(";AccessToken = commented-value", contents);
        Assert.Contains("Other = value", contents);
    }
}
