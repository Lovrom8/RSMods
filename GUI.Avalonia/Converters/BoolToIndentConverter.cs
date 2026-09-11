#nullable enable
using System;
using System.Globalization;
using Avalonia;
using Avalonia.Data.Converters;

namespace RSMods.Converters;

/// <summary>
/// Converts a boolean (e.g. IsNested) into an indented Thickness (24, 0, 0, 0) or zero.
/// </summary>
public sealed class BoolToIndentConverter : IValueConverter
{
    public static readonly BoolToIndentConverter Instance = new();

    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        if (value is true)
            return new Thickness(24, 0, 0, 0);
        return new Thickness(0);
    }

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();
}
