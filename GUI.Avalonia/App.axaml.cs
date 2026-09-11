using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using Microsoft.Extensions.DependencyInjection;
using RSMods.Core;
using RSMods.Core.Settings;
using RSMods.Services;
using RSMods.Util;
using RSMods.Data;
using RSMods.SetAndForget;
using RSMods.Twitch;
using RSMods.Twitch.EffectServer;
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
            services.AddSingleton<IManifestService, ManifestService>();
            services.AddSingleton<IChoicesProvider, AvaloniaChoicesProvider>();
            services.AddSingleton<SettingsCoordinator>();
            services.AddSingleton<SettingsService>();
            services.AddSingleton<RocksmithSettingsService>();
            services.AddSingleton<AsioSettingsService>();
            services.AddSingleton<SoundPackService>();
            services.AddSingleton<SongCatalogService>();
            services.AddSingleton<CachePsarcService>();
            services.AddSingleton<TuningService>();
            services.AddSingleton<ProfileToneService>();
            services.AddSingleton<IDriveInfoProvider, WmiDriveInfoProvider>();
            services.AddSingleton<FastLoadService>();
            services.AddSingleton<ProfileService>();
            services.AddSingleton<ProfileToneImportService>();
            services.AddSingleton<SettingsWarningPresenter>();
            services.AddSingleton<ThemeService>();
            services.AddSingleton(new HttpClient());
            services.AddSingleton<TwitchOptions>();
            services.AddSingleton<ITwitchClock>(SystemTwitchClock.Instance);
            services.AddSingleton(provider =>
            {
                var store = new TwitchTokenStore(Constants.TwitchTokenPath);
                store.LoadOrImportLegacy(
                    Constants.SettingsPath,
                    provider.GetRequiredService<TwitchOptions>().ClientId);
                return store;
            });
            services.AddSingleton(_ => new TwitchRewardRepository(Constants.TwitchRewardsPath));
            services.AddSingleton<TwitchAuthService>();
            services.AddSingleton<TwitchApiClient>();
            services.AddSingleton<ITwitchEventSubClient>(provider => new TwitchEventSubClient(
                provider.GetRequiredService<TwitchApiClient>(),
                provider.GetRequiredService<TwitchOptions>(),
                clock: provider.GetRequiredService<ITwitchClock>()));
            services.AddSingleton<RocksmithEffectServer>();
            services.AddSingleton<TwitchService>();
            services.AddTransient<StatusViewModel>();
            services.AddTransient<ModSettingsViewModel>();
            services.AddTransient<ColorsViewModel>();
            services.AddTransient<RocksmithSettingsViewModel>();
            services.AddTransient<AsioSettingsViewModel>();
            services.AddTransient<ThemesViewModel>();
            services.AddTransient<ProfilesViewModel>();
            services.AddTransient<SoundPacksViewModel>();
            services.AddTransient<SetAndForgetViewModel>();
            services.AddSingleton<TwitchViewModel>();
            services.AddTransient<MainWindowViewModel>();
            services.AddTransient<MainWindow>();

            _services = services.BuildServiceProvider();

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
