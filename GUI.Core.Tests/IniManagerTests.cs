#nullable enable
using System;
using System.Globalization;
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

        // The failed save didn't use up the change: the next one still reports it, so the game is told to reload.
        Assert.True(ini.FileOutOfDate);
        Assert.True(ini.Save());
        Assert.False(ini.FileOutOfDate);
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

    // A file that exists but can't be read must not load as empty and then get the defaults saved over it.
    [Fact]
    public void AFileThatCantBeReadIsNeverSavedOver()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File("RSMods.ini");
        File.WriteAllLines(path, ["[Toggle Switches]", "ExtendedRange=on"]);
        var ini = new IniManager(path);

        using (new FileStream(path, FileMode.Open, FileAccess.ReadWrite, FileShare.None))
            ini.Load();

        Assert.NotNull(ini.LoadError);
        Assert.Equal("off", ini.GetString("[Toggle Switches]", "ExtendedRange", "off"));
        Assert.True(ini.FileOutOfDate);
        Assert.ThrowsAny<IOException>(() => ini.Save());
        Assert.Equal<string[]>(["[Toggle Switches]", "ExtendedRange=on"], File.ReadAllLines(path));

        // Once it reads, it saves as usual.
        ini.Load();
        Assert.Null(ini.LoadError);
        Assert.Equal("on", ini.GetString("[Toggle Switches]", "ExtendedRange", "off"));
        ini.Save();
    }

    [Fact]
    public async Task LoadWaitsOutABriefWriter()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File("RSMods.ini");
        File.WriteAllLines(path, ["[Toggle Switches]", "ExtendedRange=on"]);
        var ini = new IniManager(path);

        var writer = new FileStream(path, FileMode.Open, FileAccess.ReadWrite, FileShare.None);
        Task load = Task.Run(ini.Load);
        await Task.Delay(80);
        writer.Dispose();
        await load;

        Assert.Null(ini.LoadError);
        Assert.Equal("on", ini.GetString("[Toggle Switches]", "ExtendedRange", "off"));
    }

    [Fact]
    public void AMissingFileLoadsEmpty()
    {
        using var dir = new TemporaryDirectory();
        var ini = new IniManager(dir.File("RSMods.ini"));
        ini.Load();

        Assert.Null(ini.LoadError);
        Assert.Equal("off", ini.GetString("[Toggle Switches]", "ExtendedRange", "off"));
        ini.Save();
        Assert.True(File.Exists(dir.File("RSMods.ini")));
    }

    // The DLL and RS_ASIO only read an ASCII minus; sv, nb and fi format negative numbers with U+2212.
    [Theory]
    [InlineData("sv-SE")]
    [InlineData("nb-NO")]
    [InlineData("fi-FI")]
    public void NumbersAreWrittenAndReadTheSameInEveryCulture(string culture)
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File("RSMods.ini");
        File.WriteAllLines(path, ["[Config]", "EnableWasapiOutputs=-1"]);
        CultureInfo previous = CultureInfo.CurrentCulture;
        CultureInfo.CurrentCulture = new CultureInfo(culture);
        try
        {
            var ini = new IniManager(path);
            ini.Load();

            Assert.Equal(-1, ini.GetInt("[Config]", "EnableWasapiOutputs"));
            Assert.Equal(ASIO.WasapiOutputMode.Prompt, ini.GetEnumInt("[Config]", "EnableWasapiOutputs", ASIO.WasapiOutputMode.Off));
            Assert.Equal(-5, ini.GetInt("[Mod Settings]", "ExtendedRangeModeAt", -5));
            ini.SetInt("[Mod Settings]", "Threshold", -12);
            ini.Save();
        }
        finally
        {
            CultureInfo.CurrentCulture = previous;
        }

        string[] lines = File.ReadAllLines(path);
        Assert.Contains("EnableWasapiOutputs=-1", lines);
        Assert.Contains("ExtendedRangeModeAt=-5", lines);
        Assert.Contains("Threshold=-12", lines);
    }

    [Fact]
    public void CommentingOutARepeatedKeyCommentsOutEveryActiveCopy()
    {
        using var dir = new TemporaryDirectory();
        string path = dir.File("RS_ASIO.ini");
        File.WriteAllLines(path, ["[Asio.Input.1]", "Driver=First", "Channel=1", "Driver=Second"]);
        var ini = new IniManager(path);
        ini.Load();

        ini.SetCommentedString("[Asio.Input.1]", "Driver", "Second", commented: true);
        ini.Save();

        Assert.Equal<string[]>(["[Asio.Input.1]", ";Driver=First", "Channel=1", ";Driver=Second"], File.ReadAllLines(path));
        var reloaded = new IniManager(path);
        reloaded.Load();
        Assert.True(reloaded.IsCommented("[Asio.Input.1]", "Driver"));
    }
}
