#nullable enable
using System;
using System.Globalization;
using Avalonia.Data.Converters;

namespace RSMods.Converters;

/// <summary>
/// Returns true when the bound string equals the supplied ConverterParameter (ordinal, case-insensitive).
/// Used to light up the active sidebar item by comparing the current section key against each nav button's key.
/// </summary>
public sealed class StringEqualsConverter : IValueConverter
{
    public static readonly StringEqualsConverter Instance = new();

    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        => string.Equals(value as string, parameter as string, StringComparison.OrdinalIgnoreCase);

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();
}
