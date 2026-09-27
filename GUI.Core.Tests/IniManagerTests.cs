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
}
