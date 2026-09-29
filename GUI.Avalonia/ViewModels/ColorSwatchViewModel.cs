using System;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.ViewModels;

/// <summary>
/// One editable color cell on the Custom Colors screen: a label plus its 6-digit hex value. Tracks the
/// value it was loaded with so the screen can persist only the cells the user actually changed.
/// </summary>
internal sealed partial class ColorSwatchViewModel(string label) : ObservableObject
{
    public string Label { get; } = label;

    private string _originalHex = string.Empty;

    [ObservableProperty]
    private string _hex = string.Empty;

    /// <summary>Loads a value from the store and treats it as the new saved baseline.</summary>
    public void Set(string? hex)
    {
        Hex = hex ?? string.Empty;
        _originalHex = Hex;
    }

    /// <summary>Re-baselines after a successful save so later saves only write further edits.</summary>
    public void Commit() => _originalHex = Hex;

    /// <summary>True for a complete color, so a half-typed value is held back until it's finished.</summary>
    public bool IsValid => Canonical(Hex) is { Length: 6 } hex && hex.All(Uri.IsHexDigit);

    /// <summary>True when the current value differs from the loaded/last-saved value (ignoring case and a leading #).</summary>
    public bool Changed => !string.Equals(Canonical(Hex), Canonical(_originalHex), StringComparison.OrdinalIgnoreCase);

    private static string Canonical(string? hex) => (hex ?? string.Empty).Trim().TrimStart('#');
}
