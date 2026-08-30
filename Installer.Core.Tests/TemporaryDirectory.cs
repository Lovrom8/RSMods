using System;
using System.IO;

namespace RS2014_Mod_Installer.Tests;

// A self-deleting temp directory for filesystem tests.
internal sealed class TemporaryDirectory : IDisposable
{
    public string Path { get; }

    public TemporaryDirectory()
    {
        Path = System.IO.Path.Combine(System.IO.Path.GetTempPath(), "RSModsInstallerTests_" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(Path);
    }

    public string Combine(params string[] parts)
    {
        string result = Path;
        foreach (string part in parts)
            result = System.IO.Path.Combine(result, part);
        return result;
    }

    public void Dispose()
    {
        try
        {
            if (Directory.Exists(Path))
                Directory.Delete(Path, recursive: true);
        }
        catch (IOException) { }
        catch (UnauthorizedAccessException) { }
    }
}
