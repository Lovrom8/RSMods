#nullable enable
using System;
using System.Threading.Tasks;

namespace RSMods.Services;

public enum NavigationTarget
{
    Status,
    ModSettings,
    Colors,
    Rocksmith,
    Asio,
    Themes,
    Profiles,
    SoundPacks,
    SetAndForget,
    Twitch
}

public interface INavigationService
{
    Task NavigateToAsync(NavigationTarget target, object? parameter = null);
}

public sealed class NavigationService : INavigationService
{
    private Func<NavigationTarget, object?, Task>? _handler;

    public void RegisterHandler(Func<NavigationTarget, object?, Task> handler)
    {
        _handler = handler;
    }

    public Task NavigateToAsync(NavigationTarget target, object? parameter = null)
    {
        if (_handler is not null)
            return _handler(target, parameter);

        return Task.CompletedTask;
    }
}
