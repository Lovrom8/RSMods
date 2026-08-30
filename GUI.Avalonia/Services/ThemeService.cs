using Avalonia;
using Avalonia.Media;
using Avalonia.Styling;
using RSMods.Core;

namespace RSMods.Services;

/// <summary>
/// Applies the Avalonia frontend's appearance (FluentTheme light/dark variant and accent colour) to the
/// live <see cref="Application"/>. This deliberately replaces the WinForms tri-colour recolouring with the
/// idiomatic FluentTheme approach: a theme variant plus an optional accent, rather than recolouring every
/// control by hand. Persistence lives in <see cref="RsModsSettings.GUISettings"/>; this service only
/// applies values.
/// </summary>
internal sealed class ThemeService
{
    // The accent resource keys FluentTheme reads (as dynamic resources) for its accent shades.
    private static readonly string[] AccentKeys =
    [
        "SystemAccentColor",
        "SystemAccentColorLight1", "SystemAccentColorLight2", "SystemAccentColorLight3",
        "SystemAccentColorDark1", "SystemAccentColorDark2", "SystemAccentColorDark3",
    ];

    /// <summary>Applies the persisted appearance settings; used once at startup.</summary>
    public void ApplyFromSettings() => Apply(RsModsSettings.GUISettings.AppThemeVariant, RsModsSettings.GUISettings.AppAccentColor);

    /// <summary>Applies a theme variant ("System" | "Light" | "Dark") and a 6-digit hex accent ("" = default).</summary>
    public void Apply(string variant, string accentHex)
    {
        Application? app = Application.Current;
        if (app is null)
            return;

        app.RequestedThemeVariant = variant switch
        {
            "Light" => ThemeVariant.Light,
            "Dark" => ThemeVariant.Dark,
            _ => ThemeVariant.Default,
        };

        ApplyAccent(app, accentHex);
    }

    private static void ApplyAccent(Application app, string accentHex)
    {
        if (!TryParseHex(accentHex, out Color accent))
        {
            // Drop any overrides so FluentTheme's built-in accent shows through.
            foreach (string key in AccentKeys)
            {
                app.Resources.Remove(key);
            }

            return;
        }

        app.Resources["SystemAccentColor"] = accent;
        app.Resources["SystemAccentColorLight1"] = Blend(accent, Colors.White, 0.15);
        app.Resources["SystemAccentColorLight2"] = Blend(accent, Colors.White, 0.30);
        app.Resources["SystemAccentColorLight3"] = Blend(accent, Colors.White, 0.45);
        app.Resources["SystemAccentColorDark1"] = Blend(accent, Colors.Black, 0.15);
        app.Resources["SystemAccentColorDark2"] = Blend(accent, Colors.Black, 0.30);
        app.Resources["SystemAccentColorDark3"] = Blend(accent, Colors.Black, 0.45);
    }

    private static bool TryParseHex(string hex, out Color color)
    {
        color = default;
        if (string.IsNullOrWhiteSpace(hex))
            return false;

        return Color.TryParse(hex.StartsWith('#') ? hex : "#" + hex, out color);
    }

    /// <summary>Linearly blends <paramref name="from"/> toward <paramref name="to"/> by <paramref name="t"/> (0..1).</summary>
    private static Color Blend(Color from, Color to, double t) => Color.FromArgb(
        255,
        (byte)(from.R + ((to.R - from.R) * t)),
        (byte)(from.G + ((to.G - from.G) * t)),
        (byte)(from.B + ((to.B - from.B) * t)));
}
