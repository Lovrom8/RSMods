using CommunityToolkit.Mvvm.ComponentModel;
using RSMods;
using RSMods.Util;

namespace RSMods.ViewModels;

/// <summary>
/// One Guitar Speak key-press mapping (for example "Delete" or "Space"). Holds the stored MIDI
/// note/octave value as an in-memory snapshot; the parent screen writes it back when it saves. The display
/// value is the human-readable note name, computed by the shared <see cref="GuitarSpeak"/> helper.
/// </summary>
internal sealed partial class GuitarSpeakRowViewModel(KeybindItem item) : ObservableObject
{
    private readonly KeybindItem _item = item;
    private string _saved = item.GetKey();

    public string DisplayName => _item.DisplayName;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(NoteDisplay))]
    private string _value = item.GetKey();

    /// <summary>The mapped note (name + octave); an unmapped key shows as a dash.</summary>
    public string NoteDisplay
    {
        get
        {
            string note = GuitarSpeak.GuitarSpeakNoteOctaveMath(Value);
            return string.IsNullOrEmpty(note) ? "—" : note;
        }
    }

    /// <summary>Persists this row's value if it changed since it was loaded or last written.</summary>
    public void WriteBack()
    {
        if (Value == _saved)
            return;

        _item.SetKey(Value);
        _saved = Value;
    }
}
