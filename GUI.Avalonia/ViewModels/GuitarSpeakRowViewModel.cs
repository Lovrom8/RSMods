using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using RSMods;
using RSMods.Util;

namespace RSMods.ViewModels;

/// <summary>
/// One Guitar Speak key-press mapping (for example "Delete" or "Space"). Holds the stored MIDI note number as an
/// in-memory snapshot; the parent screen writes it back when it saves. The row edits it as a note and an octave,
/// and stores it once both are picked.
/// </summary>
internal sealed partial class GuitarSpeakRowViewModel : ObservableObject
{
    public static string[] Notes { get; } = ["C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"];

    // MIDI note = note + 12 * (octave + 1), so octave -1 starts at note 0.
    public static string[] Octaves { get; } = ["-1", "0", "1", "2", "3", "4", "5", "6"];

    private readonly KeybindItem _item;
    private string _saved;
    private bool _syncing;

    public GuitarSpeakRowViewModel(KeybindItem item)
    {
        _item = item;
        _saved = item.GetKey();
        _value = _saved;
        SyncFromValue();
    }

    public string DisplayName => _item.DisplayName;

    [ObservableProperty]
    [NotifyCanExecuteChangedFor(nameof(ClearCommand))]
    private string _value;

    [ObservableProperty] private string? _note;
    [ObservableProperty] private string? _octave;

    public bool IsMapped => !string.IsNullOrEmpty(Value);

    partial void OnValueChanged(string value) => SyncFromValue();

    partial void OnNoteChanged(string? value) => SyncToValue();

    partial void OnOctaveChanged(string? value) => SyncToValue();

    private void SyncFromValue()
    {
        _syncing = true;
        try
        {
            if (int.TryParse(Value, out int midi) && midi >= 0 && midi < Notes.Length * Octaves.Length)
            {
                Note = Notes[midi % 12];
                Octave = Octaves[midi / 12];
            }
            else
            {
                Note = null;
                Octave = null;
            }
        }
        finally
        {
            _syncing = false;
        }
    }

    private void SyncToValue()
    {
        if (_syncing || Note is null || Octave is null)
            return;

        int note = System.Array.IndexOf(Notes, Note);
        int octave = System.Array.IndexOf(Octaves, Octave);
        if (note >= 0 && octave >= 0)
            Value = (note + (octave * 12)).ToString();
    }

    [RelayCommand(CanExecute = nameof(IsMapped))]
    private void Clear() => Value = string.Empty;

    /// <summary>Persists this row's value if it changed since it was loaded or last written.</summary>
    public void WriteBack()
    {
        if (Value == _saved)
            return;

        _item.SetKey(Value);
        _saved = Value;
    }
}
