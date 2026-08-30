using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using Microsoft.Extensions.DependencyInjection;
using RSMods.Core;
using RSMods.Services;
using RSMods.ViewModels;
using RSMods.Views;

namespace RSMods;

public sealed class App : Application
{
    private ServiceProvider? _services;

    public override void Initialize() => AvaloniaXamlLoader.Load(this);

    public override void OnFrameworkInitializationCompleted()
    {
        if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
        {
            var services = new ServiceCollection();
            services.AddSingleton<IAppEnvironment, AvaloniaAppEnvironment>();
            services.AddSingleton<IDialogService, AvaloniaDialogService>();
            services.AddSingleton<StartupService>();
            services.AddTransient<MainWindowViewModel>();
            services.AddTransient<MainWindow>();

            _services = services.BuildServiceProvider();

            var dialogs = _services.GetRequiredService<IDialogService>();
            var environment = _services.GetRequiredService<IAppEnvironment>();
            AppServices.Initialize(dialogs, environment);

            desktop.MainWindow = _services.GetRequiredService<MainWindow>();
            desktop.Exit += (_, _) => _services.Dispose();
        }

        base.OnFrameworkInitializationCompleted();
    }
}
