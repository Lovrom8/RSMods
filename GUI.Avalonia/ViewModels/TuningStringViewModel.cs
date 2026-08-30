using System;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.ViewModels;

/// <summary>One editable string offset and its live note/colour presentation.</summary>
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

    public TuningStringViewModel(int index, string label, int baseMidiNote)
    {
        Index = index;
        Label = label;
        BaseMidiNote = baseMidiNote;
        RefreshPresentation();
    }

    partial void OnOffsetChanged(decimal value) => RefreshPresentation();

    /// <summary>Refreshes settings-backed colour state when the user revisits the screen.</summary>
    public void RefreshPresentation()
    {
        int offset = (int)Offset;
        string noteName = GuitarSpeak.GuitarSpeakNoteOctaveMath((BaseMidiNote + offset).ToString());
        NoteName = Index == 5 ? noteName.ToLowerInvariant() : noteName;

        bool extendedRange = RsModsSettings.Toggles.ExtendedRange &&
            RsModsSettings.ModSettings.ExtendedRangeModeAt >= offset;
        ColorHex = RsModsSettings.StringColors.GetStringColor(Index, normal: !extendedRange);
    }
}
