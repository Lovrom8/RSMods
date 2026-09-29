using System;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.ViewModels;

/// <summary>One editable string offset and its live note/color presentation.</summary>
internal sealed partial class TuningStringViewModel : ObservableObject
{
    public int Index { get; }
    public string Label { get; }
    public int BaseMidiNote { get; }

    [ObservableProperty]
    private decimal _offset;

    [ObservableProperty]
    private string _noteName = string.Empty;

    [ObservableProperty]
    private string _colorHex = string.Empty;

    // The palette the swatch shows: Rocksmith draws extended-range tunings with the colorblind colors.
    private bool _normalPalette = true;
    private bool _refreshing;

    /// <summary>Raised when a color is picked on the swatch: the string, which palette, and the new hex.</summary>
    public event Action<int, bool, string>? ColorPicked;

    public TuningStringViewModel(int index, string label, int baseMidiNote)
    {
        Index = index;
        Label = label;
        BaseMidiNote = baseMidiNote;
        RefreshPresentation();
    }

    partial void OnOffsetChanged(decimal value) => RefreshPresentation();

    partial void OnColorHexChanged(string value)
    {
        if (!_refreshing)
            ColorPicked?.Invoke(Index, _normalPalette, value);
    }

    /// <summary>Refreshes settings-backed color state when the user revisits the screen.</summary>
    public void RefreshPresentation()
    {
        int offset = (int)Offset;
        string noteName = GuitarSpeak.GuitarSpeakNoteOctaveMath((BaseMidiNote + offset).ToString());
        NoteName = Index == 5 ? noteName.ToLowerInvariant() : noteName;

        bool extendedRange = RsModsSettings.Toggles.ExtendedRange && RsModsSettings.ModSettings.ExtendedRangeModeAt >= offset;
        _refreshing = true;
        try
        {
            _normalPalette = !extendedRange;
            ColorHex = RsModsSettings.StringColors.GetStringColor(Index, normal: _normalPalette);
        }
        finally
        {
            _refreshing = false;
        }
    }
}
