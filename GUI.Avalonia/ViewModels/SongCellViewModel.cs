using System;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.ViewModels;

/// <summary>
/// One checkbox cell in the song-list grid: a song's membership in a single list (Favorites or a numbered
/// song list). Toggling writes straight through to the in-memory profile via the supplied callback. 
/// Persisting to disk happens when the user saves the song lists.
/// </summary>
internal sealed partial class SongCellViewModel : ObservableObject
{
    private readonly Action<bool> _write;
    private readonly bool _suppress;

    public SongCellViewModel(bool initial, Action<bool> write)
    {
        _write = write;
        // Set the initial membership without writing it back (it already reflects the loaded profile).
        _suppress = true;
        IsChecked = initial;
        _suppress = false;
    }

    [ObservableProperty]
    private bool _isChecked;

    partial void OnIsCheckedChanged(bool value)
    {
        if (_suppress)
            return;

        _write(value);
    }
}
