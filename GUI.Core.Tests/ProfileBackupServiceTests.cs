namespace RSMods.Core.Tests;

public sealed class ProfileBackupServiceTests
{
    private const string Account = "12345";

    // Mirrors "<Steam>\userdata\<account>\221680\remote" next to a fake Rocksmith folder.
    private static (string RsFolder, string SaveFolder, string AccountFolder) Layout(TemporaryDirectory temporary)
    {
        string rsFolder = Path.Combine(temporary.Path, "Rocksmith2014");
        string saveFolder = Path.Combine(temporary.Path, "Steam", "userdata", Account, "221680", "remote");
        Directory.CreateDirectory(rsFolder);
        Directory.CreateDirectory(saveFolder);
        File.WriteAllText(Path.Combine(saveFolder, "profile_PRFLDB"), "save");
        return (rsFolder, saveFolder, Path.Combine(rsFolder, "Profile_AutoBackups", Account));
    }

    private static string MakeBackup(string accountFolder, string source, string name)
    {
        string folder = Path.Combine(accountFolder, source, name);
        Directory.CreateDirectory(folder);
        return folder;
    }

    [Fact]
    public void AccountBackupsFolder_UsesTheSteamAccountFromTheSavePath()
    {
        using var temporary = new TemporaryDirectory();
        var (rsFolder, saveFolder, accountFolder) = Layout(temporary);

        Assert.Equal(accountFolder, ProfileBackupService.AccountBackupsFolder(rsFolder, saveFolder));
        Assert.Equal(string.Empty, ProfileBackupService.AccountBackupsFolder(rsFolder, Path.Combine(temporary.Path, "not", "a", "save")));
    }

    [Fact]
    public void CreateBackup_CopiesTheSaveIntoBeforeGui()
    {
        using var temporary = new TemporaryDirectory();
        var (rsFolder, saveFolder, accountFolder) = Layout(temporary);

        ProfileBackupService.CreateBackup(saveFolder, rsFolder);

        string[] made = Directory.GetDirectories(Path.Combine(accountFolder, ProfileBackupService.BeforeGuiBackups));
        Assert.Single(made);
        Assert.Equal("save", File.ReadAllText(Path.Combine(made[0], "profile_PRFLDB")));
        Assert.DoesNotContain(".partial_", Path.GetFileName(made[0]));
    }

    [Fact]
    public void ListBackups_MergesLinkedTiersAndKeepsOldGuiBackups()
    {
        using var temporary = new TemporaryDirectory();
        var (rsFolder, saveFolder, accountFolder) = Layout(temporary);

        // One DLL backup linked into two tiers, a newer GUI backup with the same name, and an old-format backup.
        MakeBackup(accountFolder, "Every 10 Minutes", "2026-09-20_10-00-00");
        MakeBackup(accountFolder, "Hourly", "2026-09-20_10-00-00");
        MakeBackup(accountFolder, ProfileBackupService.BeforeGuiBackups, "2026-09-20_10-00-00");
        MakeBackup(accountFolder, "Daily", "2026-09-21_08-00-00");
        MakeBackup(accountFolder, "Daily", "not-a-backup");
        Directory.CreateDirectory(Path.Combine(rsFolder, "Profile_Backups", "01-02-2020_03-04-05"));

        List<ProfileBackup> backups = ProfileBackupService.ListBackups(rsFolder, saveFolder);

        Assert.Equal(4, backups.Count);
        Assert.Equal(["Daily"], backups[0].Sources); // Newest first.
        Assert.Contains(backups, b => b.Sources.SequenceEqual(["Every 10 Minutes", "Hourly"]));
        Assert.Contains(backups, b => b.Sources.SequenceEqual([ProfileBackupService.BeforeGuiBackups]));
        Assert.Equal(["Old GUI Backup"], backups[^1].Sources);
        Assert.Equal(new DateTime(2026, 9, 21, 8, 0, 0, DateTimeKind.Utc), backups[0].Utc);
    }

    [Fact]
    public void DeleteOldBackups_PrunesOnlyBeforeGuiAndClearsPartialCopies()
    {
        using var temporary = new TemporaryDirectory();
        var (rsFolder, saveFolder, accountFolder) = Layout(temporary);

        string oldest = MakeBackup(accountFolder, ProfileBackupService.BeforeGuiBackups, "2026-09-20_10-00-00");
        string middle = MakeBackup(accountFolder, ProfileBackupService.BeforeGuiBackups, "2026-09-21_10-00-00");
        string newest = MakeBackup(accountFolder, ProfileBackupService.BeforeGuiBackups, "2026-09-22_10-00-00");
        string partial = MakeBackup(accountFolder, ProfileBackupService.BeforeGuiBackups, ".partial_2026-09-23_10-00-00");
        string dllTier = MakeBackup(accountFolder, "Daily", "2026-09-19_10-00-00");

        ProfileBackupService.DeleteOldBackups(rsFolder, saveFolder, 2);

        Assert.False(Directory.Exists(oldest));
        Assert.True(Directory.Exists(middle));
        Assert.True(Directory.Exists(newest));
        Assert.False(Directory.Exists(partial));
        Assert.True(Directory.Exists(dllTier)); // The DLL prunes its own tiers.
    }

    [Fact]
    public void DeleteOldBackups_ZeroKeepsEveryBackup()
    {
        using var temporary = new TemporaryDirectory();
        var (rsFolder, saveFolder, accountFolder) = Layout(temporary);
        string backup = MakeBackup(accountFolder, ProfileBackupService.BeforeGuiBackups, "2026-09-20_10-00-00");

        ProfileBackupService.DeleteOldBackups(rsFolder, saveFolder, 0);

        Assert.True(Directory.Exists(backup));
    }
}
