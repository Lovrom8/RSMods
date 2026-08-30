using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using Microsoft.Extensions.DependencyInjection;
using RSMods.Core;
using RSMods.Services;
using RSMods.Util;
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
            services.AddSingleton<SettingsService>();
            services.AddSingleton<RocksmithSettingsService>();
            services.AddSingleton<AsioSettingsService>();
            services.AddSingleton<SoundPackService>();
            services.AddSingleton<SettingsWarningPresenter>();
            services.AddSingleton<ThemeService>();
            services.AddTransient<StatusViewModel>();
            services.AddTransient<ModSettingsViewModel>();
            services.AddTransient<ColorsViewModel>();
            services.AddTransient<RocksmithSettingsViewModel>();
            services.AddTransient<AsioSettingsViewModel>();
            services.AddTransient<ThemesViewModel>();
            services.AddTransient<ProfilesViewModel>();
            services.AddTransient<SoundPacksViewModel>();
            services.AddTransient<SetAndForgetViewModel>();
            services.AddTransient<MainWindowViewModel>();
            services.AddTransient<MainWindow>();

            _services = services.BuildServiceProvider();

            var dialogs = _services.GetRequiredService<IDialogService>();
            var environment = _services.GetRequiredService<IAppEnvironment>();
            AppServices.Initialize(dialogs, environment);

            TryApplySavedAppearanceEarly();

            desktop.MainWindow = _services.GetRequiredService<MainWindow>();
            desktop.Exit += (_, _) => _services.Dispose();
        }

        base.OnFrameworkInitializationCompleted();
    }

    /// <summary>
    /// When the Rocksmith folder is already known (returning users), loads settings
    /// and applies the saved appearance before the main window is shown, so there's no flash of the default theme.
    /// </summary>
    private void TryApplySavedAppearanceEarly()
    {
        if (string.IsNullOrEmpty(GenUtil.GetRSDirectory()))
            return;

        RsModsSettings.LoadSettingsFromINI();
        _services!.GetRequiredService<ThemeService>().ApplyFromSettings();
    }
}
