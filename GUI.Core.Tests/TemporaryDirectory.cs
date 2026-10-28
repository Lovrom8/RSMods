namespace RSMods.Core.Tests;

internal sealed class TemporaryDirectory : IDisposable
{
    public TemporaryDirectory()
    {
        Path = System.IO.Path.Combine(
            System.IO.Path.GetTempPath(),
            "RSMods.Core.Tests",
            Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(Path);
    }

    public string Path { get; }

    public string File(string name) => System.IO.Path.Combine(Path, name);

    /// <summary>Adds the game as GenUtil.IsRSFolder looks for it: the .exe, or on macOS RS Re-ARMed's app.</summary>
    public void AddGame()
    {
        if (OperatingSystem.IsWindows())
            System.IO.File.WriteAllText(File("Rocksmith2014.exe"), string.Empty);
        else
            Directory.CreateDirectory(File("Rocksmith2014.app"));
    }

    public void Dispose()
    {
        if (Directory.Exists(Path))
            Directory.Delete(Path, recursive: true);
    }
}
