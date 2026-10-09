#nullable enable
using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using Xunit;

namespace RSMods.Core.Tests;

public sealed class IniManagerTests
{
    // Screens save on the thread pool while the UI thread reads (seeding defaults) and writes values.
    [Fact]
    public async Task SaveWhileAnotherThreadEditsNeverThrows()
    {
        using var dir = new TemporaryDirectory();
        var ini = new IniManager(dir.File("RSMods.ini"));
        using var stop = new CancellationTokenSource(TimeSpan.FromMilliseconds(500));

        Task writer = Task.Run(() =>
        {
            for (int i = 0; !stop.IsCancellationRequested; i++)
            {
                ini.SetString($"[Section{i % 7}]", $"Key{i}", "value");
                ini.GetString("[Seeded]", $"Default{i}", "x");
            }
        });
        Task saver = Task.Run(() =>
        {
            while (!stop.IsCancellationRequested)
                ini.Save();
        });

        await Task.WhenAll(writer, saver);

        var reloaded = new IniManager(dir.File("RSMods.ini"));
        ini.Save();
        reloaded.Load();
        Assert.Equal("value", reloaded.GetString("[Section0]", "Key0"));
    }

    [Fact]
    public void SaveReportsAFileItCannotWrite()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File("RSMods.ini");
        var ini = new IniManager(path);
        ini.SetString("[Toggle Switches]", "Key", "on");

        using (new FileStream(path, FileMode.Create, FileAccess.ReadWrite, FileShare.None))
            Assert.ThrowsAny<IOException>(() => ini.Save());
    }

    [Fact]
    public void SaveReportsWhetherAnyValueChanged()
    {
        using var dir = new TemporaryDirectory();
        var ini = new IniManager(dir.File("RSMods.ini"));

        ini.GetString("[Toggle Switches]", "Seeded", "off");
        Assert.False(ini.Save()); // Seeding a default is written but isn't a change.

        ini.SetString("[Toggle Switches]", "Seeded", "off");
        Assert.False(ini.Save()); // Same value.

        ini.SetString("[Toggle Switches]", "Seeded", "on");
        Assert.True(ini.Save());
        Assert.False(ini.Save()); // Reported once.

        ini.SetCommentedString("[Asio.Input.1]", "Driver", "X", commented: true);
        Assert.True(ini.Save());
        ini.SetCommentedString("[Asio.Input.1]", "Driver", "X", commented: false);
        Assert.True(ini.Save()); // Uncommenting is a change even with the same value.
    }

    [Fact]
    public void ChangeWrittenBySuspendedScopeIsReportedByTheNextSave()
    {
        using var dir = new TemporaryDirectory();
        var ini = new IniManager(dir.File("RSMods.ini"));

        using (ini.SuspendSave())
        {
            ini.SetString("[Mod Settings]", "Key", "1");
            Assert.False(ini.Save()); // Deferred to the end of the scope.
        }

        Assert.True(ini.Save());
    }

    [Fact]
    public void FailedSaveKeepsTheChangeForTheNextSave()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File("RSMods.ini");
        var ini = new IniManager(path);
        ini.SetString("[Toggle Switches]", "Key", "on");

        using (new FileStream(path, FileMode.Create, FileAccess.ReadWrite, FileShare.None))
            Assert.ThrowsAny<IOException>(() => ini.Save());

        Assert.True(ini.Save());
    }

    [Fact]
    public void SaveLeavesNoTemporaryFileBehind()
    {
        using var dir = new TemporaryDirectory();
        var ini = new IniManager(dir.File("RSMods.ini"));
        ini.SetString("[Toggle Switches]", "Key", "on");

        ini.Save();
        ini.Save(); // The second save replaces an existing file.

        Assert.Equal(["RSMods.ini"], Directory.GetFiles(dir.Path).Select(Path.GetFileName));
    }

    // The game reads the INI with a handle that allows writes but not a replace; the save waits for it.
    [Fact]
    public async Task SaveWaitsOutABriefReader()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File("RSMods.ini");
        var ini = new IniManager(path);
        ini.SetString("[Toggle Switches]", "Key", "old");
        ini.Save();

        var reader = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
        ini.SetString("[Toggle Switches]", "Key", "new");
        Task save = Task.Run(() => ini.Save());
        await Task.Delay(80);
        reader.Dispose();
        await save;

        Assert.Contains("Key=new", File.ReadAllLines(path));
    }

    [Fact]
    public void FailedSaveLeavesTheOldFileWhole()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File("RSMods.ini");
        var ini = new IniManager(path);
        ini.SetString("[Toggle Switches]", "Key", "old");
        ini.Save();

        ini.SetString("[Toggle Switches]", "Key", "new");
        using (new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.None))
            Assert.ThrowsAny<IOException>(() => ini.Save());

        Assert.Contains("Key=old", File.ReadAllLines(path));
        Assert.Equal(["RSMods.ini"], Directory.GetFiles(dir.Path).Select(Path.GetFileName));
    }

    [Fact]
    public void FileOutOfDateTracksSeededDefaultsButNotReads()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File("RSMods.ini");
        File.WriteAllText(path, "[Toggle Switches]" + Environment.NewLine + "Key=on" + Environment.NewLine);
        var ini = new IniManager(path);
        ini.Load();

        ini.GetString("[Toggle Switches]", "Key", "off");
        Assert.False(ini.FileOutOfDate); // Read an existing value.

        ini.GetString("[Toggle Switches]", "Missing", "off");
        Assert.True(ini.FileOutOfDate); // Seeded a default the file lacks.

        ini.Save();
        Assert.False(ini.FileOutOfDate);

        ini.SetString("[Toggle Switches]", "Key", "on");
        Assert.False(ini.FileOutOfDate); // Same value.
        ini.SetString("[Toggle Switches]", "Key", "off");
        Assert.True(ini.FileOutOfDate);
    }

    [Fact]
    public void SaveWritesTheFileBackAsItWas()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File("RSMods.ini");
        const string contents = "; top comment\nStray=1\n[Section]\n; note\nKey = spaced ; inline\nDup=1\nDup=2\n;Old=x\njunk line\n\n[Other]\nA=1";
        File.WriteAllText(path, contents);
        var ini = new IniManager(path);
        ini.Load();

        Assert.Equal("spaced ; inline", ini.GetString("[Section]", "Key"));
        Assert.Equal("2", ini.GetString("[Section]", "Dup"));
        ini.Save();

        Assert.Equal(contents + "\n", File.ReadAllText(path)); // LF endings kept too.
    }

    [Fact]
    public void SettingAValueRewritesOnlyItsLine()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File("RSMods.ini");
        File.WriteAllLines(path, ["[Section]", "; note", "Key = old", "Other=1", "", "; leads into Next", "[Next]", "B=2"]);
        var ini = new IniManager(path);
        ini.Load();

        ini.SetString("[Section]", "Key", "new");
        ini.SetString("[Section]", "Added", "1");
        ini.SetString("[New]", "C", "3");
        ini.Save();

        Assert.Equal<string[]>(
            ["[Section]", "; note", "Key=new", "Other=1", "Added=1", "", "; leads into Next", "[Next]", "B=2", "", "[New]", "C=3", ""],
            File.ReadAllLines(path));
    }

    // RS_ASIO users keep alternatives as comments, e.g. an old interface's driver under the current one.
    [Fact]
    public void AnActiveLineWinsOverACommentedCopyOfTheKey()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File("RS_ASIO.ini");
        File.WriteAllLines(path, ["[Asio.Output]", "Driver=Current", ";Driver=Old"]);
        var ini = new IniManager(path);
        ini.Load();

        Assert.False(ini.IsCommented("[Asio.Output]", "Driver"));
        Assert.Equal("Current", ini.GetString("[Asio.Output]", "Driver"));
        ini.SetString("[Asio.Output]", "Driver", "Current");
        ini.Save();

        Assert.Equal<string[]>(["[Asio.Output]", "Driver=Current", ";Driver=Old"], File.ReadAllLines(path));
    }

    [Fact]
    public void CommentingAKeyOutAndBackInChangesItsLineInPlace()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File("RS_ASIO.ini");
        File.WriteAllLines(path, ["[Asio.Input.1]", "Driver=Current", "Channel=1", ";Driver=Old"]);
        var ini = new IniManager(path);
        ini.Load();

        ini.SetCommentedString("[Asio.Input.1]", "Driver", "Current", commented: true);
        ini.Save();
        Assert.Equal<string[]>(["[Asio.Input.1]", ";Driver=Current", "Channel=1", ";Driver=Old"], File.ReadAllLines(path));
        Assert.Equal("Current", ini.GetCommentedString("[Asio.Input.1]", "Driver"));

        ini.SetCommentedString("[Asio.Input.1]", "Driver", "Current", commented: false);
        ini.Save();
        Assert.Equal<string[]>(["[Asio.Input.1]", "Driver=Current", "Channel=1", ";Driver=Old"], File.ReadAllLines(path));
    }

    [Fact]
    public void WithoutFillDefaultsShownDefaultsAreNotWritten()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File("RS_ASIO.ini");
        File.WriteAllLines(path, ["[Asio]", "CustomBufferSize=64 ; low latency"]);
        var ini = new IniManager(path, fillDefaults: false);
        ini.Load();
        IniValidationWarning? warning = null;
        ini.ValidationWarning += w => warning = w;

        Assert.Equal(48, ini.GetInt("[Asio]", "CustomBufferSize", 48));
        Assert.True(warning!.ValueKept);
        Assert.Equal(0, ini.GetInt("[Asio]", "Missing", 0));
        Assert.False(ini.FileOutOfDate);

        ini.SetInt("[Asio]", "CustomBufferSize", 48);
        ini.SetInt("[Asio]", "Missing", 0);
        Assert.False(ini.Save());
        Assert.Equal<string[]>(["[Asio]", "CustomBufferSize=64 ; low latency"], File.ReadAllLines(path));

        ini.SetInt("[Asio]", "CustomBufferSize", 128);
        Assert.True(ini.Save());
        Assert.Equal<string[]>(["[Asio]", "CustomBufferSize=128"], File.ReadAllLines(path));
    }
}
