using System;
using System.Globalization;
using Avalonia.Data.Converters;

namespace RSMods.Converters;

/// <summary>
/// Turns a panel's available width into a WrapPanel <c>ItemWidth</c>: as many columns as fit at the
/// minimum width (the converter parameter, 300 by default), stretched to share the row evenly, so a
/// settings grid fills its card at any window size instead of leaving a ragged gap on the right.
/// </summary>
public sealed class ColumnWidthConverter : IValueConverter
{
    public static readonly ColumnWidthConverter Instance = new();

    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
    {
        double minimum = parameter is string text && double.TryParse(text, NumberStyles.Float, CultureInfo.InvariantCulture, out double parsed)
            ? parsed
            : 300;

        if (value is not double available || available <= 0 || double.IsInfinity(available))
            return minimum;

        int columns = Math.Max(1, (int)(available / minimum));
        // Floor so rounding never pushes the last column onto the next row.
        return Math.Floor(available / columns);
    }

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotSupportedException();
}
