using System;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.Primitives;
using Avalonia.Data;
using Avalonia.Input;
using Avalonia.Layout;
using Avalonia.Media;
using Avalonia.VisualTree;

namespace RSMods.Views;

/// <summary>
/// A color swatch that opens a color wheel when clicked. Two-way bound to a 6-digit hex string (no leading #,
/// upper case, as the settings store keeps colors), so it sits beside or inside a hex text box and both stay in step.
/// </summary>
internal sealed class ColorSwatchButton : Button
{
    public static readonly StyledProperty<string?> HexProperty =
        AvaloniaProperty.Register<ColorSwatchButton, string?>(nameof(Hex), defaultBindingMode: BindingMode.TwoWay);

    private readonly Border _swatch;
    private readonly ColorView _picker;
    private bool _syncing;

    protected override Type StyleKeyOverride => typeof(Button);

    public ColorSwatchButton()
    {
        Padding = new Thickness(0);
        MinWidth = 0;
        MinHeight = 0;
        VerticalAlignment = VerticalAlignment.Center;
        ToolTip.SetTip(this, "Pick a color");

        _swatch = new Border
        {
            CornerRadius = new CornerRadius(3),
            BorderThickness = new Thickness(1),
        };
        _swatch.Bind(Border.BorderBrushProperty, _swatch.GetResourceObservable("CardBorderBrush"));
        Content = _swatch;
        HorizontalContentAlignment = HorizontalAlignment.Stretch;
        VerticalContentAlignment = VerticalAlignment.Stretch;

        _picker = new ColorView
        {
            ColorSpectrumShape = ColorSpectrumShape.Ring,
            IsAlphaEnabled = false,
            IsAlphaVisible = false,
            IsColorPaletteVisible = false,
            // Just the preview: the lighter / darker shade buttons beside it are more clutter than shortcut.
            IsAccentColorsVisible = false,
        };
        _picker.TemplateApplied += (_, e) =>
        {
            if (e.NameScope.Find<ColorSpectrum>("ColorSpectrum") is { } spectrum)
                spectrum.TemplateApplied += (_, se) => UnclipSelectionRing(se.NameScope);
        };
        _picker.ColorChanged += (_, e) =>
        {
            if (!_syncing)
                Hex = ToHex(e.NewColor);
        };

        // The color view brings its own margins; the presenter's padding on top of them clips its right edge
        // (Avalonia's own ColorPicker drops it the same way).
        var flyout = new Flyout { Content = _picker, Placement = PlacementMode.BottomEdgeAlignedLeft };
        flyout.FlyoutPresenterClasses.Add("nopadding");
        flyout.Opened += (_, _) => MarkPreviewClickable();
        Flyout = flyout;

        // Clicking the preview takes its color and closes the picker. The color is already applied as it changes, so
        // this is the "done" click. handledEventsToo, in case the previewer marks its own taps handled.
        _picker.AddHandler(Gestures.TappedEvent, (_, e) =>
        {
            if (e.Source is not Visual source || source.FindAncestorOfType<ColorPreviewer>(includeSelf: true) is null)
                return;

            Hex = ToHex(_picker.Color);
            flyout.Hide();
        }, handledEventsToo: true);
        UpdateFromHex();
    }

    public string? Hex
    {
        get => GetValue(HexProperty);
        set => SetValue(HexProperty, value);
    }

    protected override void OnPropertyChanged(AvaloniaPropertyChangedEventArgs change)
    {
        base.OnPropertyChanged(change);

        if (change.Property == HexProperty)
            UpdateFromHex();
    }

    // The preview is part of the picker's template, so it's found once the flyout has built it.
    private void MarkPreviewClickable()
    {
        foreach (ColorPreviewer preview in _picker.GetVisualDescendants().OfType<ColorPreviewer>())
        {
            preview.Cursor = new Cursor(StandardCursorType.Hand);
            ToolTip.SetTip(preview, "Use this color");
        }
    }

    // The wheel clips to its own square, cutting the selection ring in half at the wheel's edge. The template sets the
    // clip, which a style can't override, so it's cleared once the template is in place.
    private static void UnclipSelectionRing(INameScope scope)
    {
        if (scope.Find<Panel>("PART_SizingPanel") is { } sizingPanel)
            sizingPanel.ClipToBounds = false;
    }

    private void UpdateFromHex()
    {
        bool valid = TryParse(Hex, out Color color);
        _swatch.Background = valid ? new SolidColorBrush(color) : Brushes.Transparent;

        // Only a complete color moves the wheel, so typing a hex digit by digit doesn't make it jump around.
        if (!valid)
            return;

        _syncing = true;
        try
        {
            _picker.Color = color;
        }
        finally
        {
            _syncing = false;
        }
    }

    private static bool TryParse(string? hex, out Color color)
    {
        color = default;
        string trimmed = (hex ?? string.Empty).Trim().TrimStart('#');
        return trimmed.Length == 6 && Color.TryParse("#" + trimmed, out color);
    }

    private static string ToHex(Color color) => $"{color.R:X2}{color.G:X2}{color.B:X2}";
}
