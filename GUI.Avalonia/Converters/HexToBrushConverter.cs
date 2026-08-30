using System;
using System.Globalization;
using Avalonia.Data.Converters;
using Avalonia.Media;

namespace RSMods.Converters;

/// <summary>
/// One-way converter from a stored 6-digit hex colour (e.g. <c>FF4F5A</c>, with or without a leading
/// <c>#</c>) to a <see cref="SolidColorBrush"/> for a preview swatch. Empty or unparseable values render
/// transparent so the swatch reads as "unset" rather than throwing.
/// </summary>
public sealed class HexToBrushConverter : IValueConverter
{
    public static readonly HexToBrushConverter Instance = new();

    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        if (value is string hex && !string.IsNullOrWhiteSpace(hex))
        {
            string trimmed = hex.Trim();
            if (!trimmed.StartsWith('#'))
                trimmed = "#" + trimmed;

            if (Color.TryParse(trimmed, out Color color))
                return new SolidColorBrush(color);
        }

        return Brushes.Transparent;
    }

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();
}
