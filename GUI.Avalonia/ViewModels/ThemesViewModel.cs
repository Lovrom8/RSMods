using System;
using System.ComponentModel;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Core;
using RSMods.Services;
using RSMods.Util;

namespace RSMods.ViewModels;

/// <summary>
/// A single accent choice: the persisted 6-digit hex (empty for FluentTheme's built-in accent) plus the
/// hex the preview swatch shows (never empty, so "Default" still renders a colour).
/// </summary>
internal sealed record AccentPreset(string Name, string Hex, string SwatchHex);

/// <summary>
/// The Appearance screen. Replaces the WinForms tri-colour configurator recolouring with the idiomatic
/// FluentTheme model: a light/dark/system variant plus an optional accent colour. Changes apply live via
/// <see cref="ThemeService"/> and are saved to <see cref="RsModsSettings.GUISettings"/> a moment later.
/// </summary>
internal sealed partial class ThemesViewModel : ObservableObject
{
    private readonly SettingsService settings;
    private readonly ThemeService theme;
    private readonly DebouncedSaver _saver;
    private bool _loading;
    private bool _initialized;

    public string[] ThemeVariants { get; } = ["System", "Light", "Dark"];

    // Curated accent choices; "Default" persists an empty hex so FluentTheme's own accent shows through.
    // The swatch hex is the colour previewed in the picker (Default shows FluentTheme's default blue).
    public static AccentPreset[] AccentPresets { get; } =
    [
        new("Default", "", "0078D7"),
        new("Blue", "0078D7", "0078D7"),
        new("Teal", "00A3A3", "00A3A3"),
        new("Green", "2E9E4F", "2E9E4F"),
        new("Orange", "E8730C", "E8730C"),
        new("Red", "E0364B", "E0364B"),
        new("Purple", "8B5CF6", "8B5CF6"),
        new("Pink", "D6438A", "D6438A"),
    ];

    [ObservableProperty]
    private string _selectedThemeVariant = "System";

    [ObservableProperty]
    private AccentPreset _selectedAccent = AccentPresets[0];

    [ObservableProperty]
    private string _statusMessage = string.Empty;

    public ThemesViewModel(SettingsService settings, ThemeService theme, AutoSaveService autoSave)
    {
        this.settings = settings;
        this.theme = theme;
        _saver = autoSave.Create(SaveAsync, ex => StatusMessage = $"Couldn't save: {ex.Message}");
    }

    /// <summary>Builds the snapshot on first navigation; the store is already loaded by startup.</summary>
    public Task InitializeAsync()
    {
        if (_initialized)
            return Task.CompletedTask;
        _initialized = true;

        Load();
        return Task.CompletedTask;
    }

    private void Load()
    {
        _loading = true;
     
        try
        {
            string variant = RsModsSettings.GUISettings.AppThemeVariant;
            SelectedThemeVariant = ThemeVariants.Contains(variant) ? variant : "System";

            string accentHex = RsModsSettings.GUISettings.AppAccentColor;
            SelectedAccent = AccentPresets.FirstOrDefault(
                p => string.Equals(p.Hex, accentHex, StringComparison.OrdinalIgnoreCase))
                ?? AccentPresets[0];
        }
        finally
        {
            _loading = false;
            StatusMessage = string.Empty;
        }
    }

    partial void OnSelectedThemeVariantChanged(string value) => OnAppearanceChanged();

    partial void OnSelectedAccentChanged(AccentPreset value) => OnAppearanceChanged();

    private void OnAppearanceChanged()
    {
        // The ListBox can momentarily clear its selection while rebinding; ignore until it settles.
        if (SelectedAccent is null)
            return;

        theme.Apply(SelectedThemeVariant, SelectedAccent.Hex);

        if (_loading)
            return;

        _saver.Request();
    }

    private async Task SaveAsync()
    {
        RsModsSettings.GUISettings.AppThemeVariant = SelectedThemeVariant;
        RsModsSettings.GUISettings.AppAccentColor = SelectedAccent.Hex;

        await settings.SaveAsync();
        StatusMessage = $"Saved at {DateTime.Now:HH:mm:ss}";
    }
}
