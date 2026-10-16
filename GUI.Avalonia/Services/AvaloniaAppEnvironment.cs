using System.Diagnostics;
using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Threading;
using RSMods.Core;

namespace RSMods.Services;

internal sealed class AvaloniaAppEnvironment : IAppEnvironment
{
    public void RequestShutdown()
    {
        void Shutdown()
        {
            if (Application.Current?.ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
                desktop.Shutdown();
        }

        if (Dispatcher.UIThread.CheckAccess())
            Shutdown();
        else
            Dispatcher.UIThread.Post(Shutdown);
    }

    private static bool _restartRequested;

    public void RequestRestart()
    {
        // The new instance starts once this one has shut down (see Program.Main): until its services are disposed this
        // one still holds the Twitch effect port and the window the game sends in-game edits to.
        _restartRequested = true;
        RequestShutdown();
    }

    /// <summary>Starts the new instance a <see cref="RequestRestart"/> asked for. Called after shutdown.</summary>
    public static void StartRequestedRestart()
    {
        if (_restartRequested && Environment.ProcessPath is { } exe)
            Process.Start(new ProcessStartInfo(exe) { WorkingDirectory = AppContext.BaseDirectory });
    }
}
