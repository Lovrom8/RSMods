using System;
using System.ComponentModel;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods.Core;
using RSMods.Services;

namespace RSMods.ViewModels;

/// <summary>
/// A single accent choice: the persisted 6-digit hex (empty for FluentTheme's built-in accent) plus the
/// hex the preview swatch shows (never empty, so "Default" still renders a colour).
/// </summary>
internal sealed record AccentPreset(string Name, string Hex, string SwatchHex);

/// <summary>
/// The Appearance screen. Replaces the WinForms tri-colour configurator recolouring with the idiomatic
/// FluentTheme model: a light/dark/system variant plus an optional accent colour. Changes preview live via
/// <see cref="ThemeService"/> and are only persisted to <see cref="RsModsSettings.GUISettings"/> on Save;
/// </summary>
internal sealed partial class ThemesViewModel(SettingsService settings, ThemeService theme) : ObservableObject
{
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
    [NotifyCanExecuteChangedFor(nameof(SaveCommand), nameof(RevertCommand))]
    private bool _isDirty;

    [ObservableProperty]
    private string _statusMessage = string.Empty;

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
            IsDirty = false;
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

        // Preview live so the whole app reflects the choice, even while unsaved.
        theme.Apply(SelectedThemeVariant, SelectedAccent.Hex);

        if (_loading)
            return;

        IsDirty = true;
        StatusMessage = string.Empty;
    }

    private bool CanSaveOrRevert => IsDirty;

    [RelayCommand(CanExecute = nameof(CanSaveOrRevert))]
    private async Task SaveAsync()
    {
        RsModsSettings.GUISettings.AppThemeVariant = SelectedThemeVariant;
        RsModsSettings.GUISettings.AppAccentColor = SelectedAccent.Hex;

        await settings.SaveAsync();

        IsDirty = false;
        StatusMessage = "Appearance saved.";
    }

    [RelayCommand(CanExecute = nameof(CanSaveOrRevert))]
    private void Revert()
    {
        Load();
        // Re-apply the restored appearance so the live preview matches the reverted values.
        theme.Apply(SelectedThemeVariant, SelectedAccent.Hex);
        StatusMessage = "Reverted to the last saved values.";
    }
}
