using Avalonia;
using RSMods.Services;

namespace RSMods;

internal static class Program
{
    [STAThread]
    public static void Main(string[] args)
    {
        CrashReporter.Install();
        try
        {
            BuildAvaloniaApp()
                .AfterSetup(_ => CrashReporter.InstallUiThread())
                .StartWithClassicDesktopLifetime(args);
        }
        catch (Exception ex)
        {
            CrashReporter.Report(ex, fatal: true);
            return;
        }

        // Only now that this instance has shut down and released everything (the Twitch effect port among them).
        AvaloniaAppEnvironment.StartRequestedRestart();
    }

    // Avalonia configuration used by both the application and the XAML previewer.
    public static AppBuilder BuildAvaloniaApp() => AppBuilder
        .Configure<App>()
        .UsePlatformDetect()
        .WithInterFont()
        .LogToTrace();
}
