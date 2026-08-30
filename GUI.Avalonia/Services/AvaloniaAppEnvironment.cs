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
}
