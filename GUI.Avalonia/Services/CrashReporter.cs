using System.IO;
using System.Runtime.InteropServices;
using Avalonia.Threading;

namespace RSMods.Services;

/// <summary>
/// The last line of defence the old WinForms Program had (a try/catch with a message box): an error nothing else
/// handled is written to a crash log and shown, instead of the app vanishing without a word.
/// </summary>
internal static class CrashReporter
{
    private const string LogFileName = "RSMods_crash.log";
    private const uint MB_ICONERROR = 0x10;

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern int MessageBoxW(IntPtr hWnd, string text, string caption, uint type);

    /// <summary>Hooks background threads and unobserved tasks. Call once, before the app starts.</summary>
    public static void Install()
    {
        AppDomain.CurrentDomain.UnhandledException += (_, e) =>
        {
            if (e.ExceptionObject is Exception ex)
                Report(ex, fatal: e.IsTerminating);
        };

        // A failed fire-and-forget task only gets logged; showing it could pop up long after whatever caused it.
        TaskScheduler.UnobservedTaskException += (_, e) =>
        {
            WriteLog(e.Exception);
            e.SetObserved();
        };
    }

    /// <summary>
    /// Hooks the UI thread. Only once Avalonia is set up: touching Dispatcher.UIThread earlier makes it settle on a
    /// placeholder dispatcher that can't run the main loop, and the app fails to start.
    /// </summary>
    public static void InstallUiThread()
    {
        // Errors on the UI thread (button handlers, async void) are shown and the app carries on, as WinForms did.
        Dispatcher.UIThread.UnhandledException += (_, e) =>
        {
            Report(e.Exception, fatal: false);
            e.Handled = true;
        };
    }

    public static void Report(Exception exception, bool fatal)
    {
        string? log = WriteLog(exception);
        string text = (fatal ? "RSMods ran into an error and has to close:" : "RSMods ran into an error:")
            + $"\n\n{exception.Message}"
            + (log is null ? string.Empty : $"\n\nThe details were saved to:\n{log}");

        // A native box needs no running Avalonia, so it also works when startup itself failed.
        MessageBoxW(IntPtr.Zero, text, "RSMods error", MB_ICONERROR);
    }

    private static string? WriteLog(Exception exception)
    {
        string entry = $"[{DateTime.Now:yyyy-MM-dd HH:mm:ss}] {exception}{Environment.NewLine}{Environment.NewLine}";
        // Next to RSMods first; the temp folder if that one isn't writable.
        foreach (string folder in new[] { AppContext.BaseDirectory, Path.GetTempPath() })
        {
            try
            {
                string path = Path.Combine(folder, LogFileName);
                File.AppendAllText(path, entry);
                return path;
            }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
            {
            }
        }

        return null;
    }
}
