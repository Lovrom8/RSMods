using System;
using System.ComponentModel;
using System.Linq;
using Avalonia;
using Avalonia.Media;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Core;
using RSMods.Services;
using RSMods.Util;

namespace RSMods.ViewModels;

/// <summary>
/// A single accent choice: the persisted 6-digit hex (empty for FluentTheme's built-in accent) plus the
/// hex the preview swatch shows (never empty, so "Default" still renders a color).
/// </summary>
internal sealed record AccentPreset(string Name, string Hex, string SwatchHex);

/// <summary>
/// The Appearance screen. Replaces the WinForms tri-color configurator recoloring with the idiomatic
/// FluentTheme model: a light/dark/system variant plus an optional accent color. Changes apply live via
/// <see cref="ThemeService"/> and are saved to <see cref="RsModsSettings.GUISettings"/> a moment later.
/// </summary>
internal sealed partial class ThemesViewModel : ObservableObject
{
    // What Windows falls back to when it can't report its accent.
    private const string FallbackAccentHex = "0078D7";

    private readonly SettingsService settings;
    private readonly ThemeService theme;
    private readonly DebouncedSaver _saver;
    private bool _loading;
    private bool _initialized;

    public string[] ThemeVariants { get; } = ["System", "Light", "Dark"];

    // Curated accent choices; "Default" persists an empty hex so FluentTheme's own accent (the Windows accent
    // color) shows through, and its swatch previews that color.
    public AccentPreset[] AccentPresets { get; } =
    [
        new("Default", "", SystemAccentHex()),
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

    /// <summary>The chosen preset, or null while a custom accent is in use.</summary>
    [ObservableProperty]
    private AccentPreset? _selectedAccent;

    /// <summary>The color on the Custom swatch; picking one there makes it the accent.</summary>
    [ObservableProperty]
    private string _customAccentHex = SystemAccentHex();

    [ObservableProperty]
    private bool _isCustomAccent;

    [ObservableProperty]
    private string _statusMessage = string.Empty;

    public ThemesViewModel(SettingsService settings, ThemeService theme, AutoSaveService autoSave)
    {
        this.settings = settings;
        this.theme = theme;
        _saver = autoSave.Create(SaveAsync, ex => StatusMessage = $"Couldn't save: {ex.Message}");
        _selectedAccent = AccentPresets[0];
    }

    private string AccentHex => IsCustomAccent ? CustomAccentHex : SelectedAccent?.Hex ?? string.Empty;

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
            AccentPreset? preset = AccentPresets.FirstOrDefault(
                p => string.Equals(p.Hex, accentHex, StringComparison.OrdinalIgnoreCase));

            if (preset is null && IsValidHex(accentHex))
            {
                CustomAccentHex = accentHex.ToUpperInvariant();
                IsCustomAccent = true;
                SelectedAccent = null;
            }
            else
            {
                IsCustomAccent = false;
                SelectedAccent = preset ?? AccentPresets[0];
            }
        }
        finally
        {
            _loading = false;
            StatusMessage = string.Empty;
        }
    }

    partial void OnSelectedThemeVariantChanged(string value) => OnAppearanceChanged();

    partial void OnSelectedAccentChanged(AccentPreset? value)
    {
        // The ListBox can momentarily clear its selection while rebinding, and choosing a custom color clears it on
        // purpose; only a real pick moves off the custom color.
        if (value is null)
            return;

        IsCustomAccent = false;
        OnAppearanceChanged();
    }

    partial void OnCustomAccentHexChanged(string value)
    {
        // Half-typed or loaded values don't switch the accent; a color picked on the swatch does.
        if (_loading || !IsValidHex(value))
            return;

        IsCustomAccent = true;
        SelectedAccent = null;
        OnAppearanceChanged();
    }

    private void OnAppearanceChanged()
    {
        if (SelectedAccent is null && !IsCustomAccent)
            return;

        theme.Apply(SelectedThemeVariant, AccentHex);

        if (_loading)
            return;

        _saver.Request();
    }

    private async Task SaveAsync()
    {
        RsModsSettings.GUISettings.AppThemeVariant = SelectedThemeVariant;
        RsModsSettings.GUISettings.AppAccentColor = AccentHex;

        await settings.SaveAsync();
        StatusMessage = $"Saved at {DateTime.Now:HH:mm:ss}";
    }

    private static bool IsValidHex(string? hex) =>
        hex is { Length: 6 } && hex.All(Uri.IsHexDigit);

    /// <summary>The Windows accent color, which FluentTheme uses when no accent is chosen.</summary>
    private static string SystemAccentHex()
    {
        if (Application.Current?.PlatformSettings?.GetColorValues() is not { } values)
            return FallbackAccentHex;

        Color accent = values.AccentColor1;
        return accent.A == 0 ? FallbackAccentHex : $"{accent.R:X2}{accent.G:X2}{accent.B:X2}";
    }
}
