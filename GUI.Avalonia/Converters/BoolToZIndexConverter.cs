#nullable enable
using System;
using System.Globalization;
using Avalonia.Data.Converters;

namespace RSMods.Converters;

/// <summary>
/// Converts a boolean (e.g. IsNested) into a ZIndex of -1 or 0, so a nested row's guide line passes under the
/// row above it instead of across its checkbox.
/// </summary>
public sealed class BoolToZIndexConverter : IValueConverter
{
    public static readonly BoolToZIndexConverter Instance = new();

    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        => value is true ? -1 : 0;

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();
}
