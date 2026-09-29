#nullable enable
using System;
using System.Globalization;
using Avalonia.Data.Converters;
using Avalonia.Media;

namespace RSMods.Converters;

/// <summary>Turns a font name into its <see cref="FontFamily"/>, so a font list can show each name in its own font.</summary>
public sealed class FontFamilyConverter : IValueConverter
{
    public static readonly FontFamilyConverter Instance = new();

    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        => value is string name && !string.IsNullOrWhiteSpace(name) ? new FontFamily(name) : FontFamily.Default;

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();
}
