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

    public void RequestRestart()
    {
        // The new instance only reads settings files, which this one has finished writing by the time it's asked
        // to restart, so the two can overlap while this one closes.
        if (Environment.ProcessPath is { } exe)
            Process.Start(new ProcessStartInfo(exe) { WorkingDirectory = AppContext.BaseDirectory });

        RequestShutdown();
    }
}
