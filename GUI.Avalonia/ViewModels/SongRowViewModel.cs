using System.Collections.Generic;

namespace RSMods.ViewModels;

/// <summary>
/// One row in the song-list grid: a song's artist/title plus its membership cells. Cell 0 is Favorites and
/// cells 1..N are the profile's numbered song lists, matching the columns the view builds at load time.
/// </summary>
internal sealed class SongRowViewModel
{
    public string Artist { get; }
    public string Title { get; }
    public IReadOnlyList<SongCellViewModel> Cells { get; }

    public SongRowViewModel(string artist, string title, IReadOnlyList<SongCellViewModel> cells)
    {
        Artist = artist;
        Title = title;
        Cells = cells;
    }
}
