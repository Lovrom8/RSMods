using Avalonia;

namespace RSMods;

internal static class Program
{
    [STAThread]
    public static int Main(string[] args)
    {
        // Headless helper commands the DLL runs; they never open a window.
        if (ExtractBeatsCommand.TryRun(args, out int exitCode))
            return exitCode;

        return BuildAvaloniaApp().StartWithClassicDesktopLifetime(args);
    }

    // Avalonia configuration used by both the application and the XAML previewer.
    public static AppBuilder BuildAvaloniaApp() => AppBuilder
        .Configure<App>()
        .UsePlatformDetect()
        .WithInterFont()
        .LogToTrace();
}
