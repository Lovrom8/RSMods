using RSMods.SetAndForget.Models;

namespace RSMods.ViewModels;

/// <summary>A row in the "Shows as Custom Tuning" list: a tuning heading or a song in that tuning.</summary>
internal abstract record CustomTuningRow(string Text)
{
    /// <summary>Headings can't be selected; the list's item style binds IsEnabled to this.</summary>
    public abstract bool IsSelectable { get; }
}

/// <summary>The heading over a tuning's songs, e.g. "D A D G A D (-2 0 0 0 -2 -2)".</summary>
internal sealed record CustomTuningHeaderRow(string Text) : CustomTuningRow(Text)
{
    public override bool IsSelectable => false;
}

/// <summary>A song under its tuning, e.g. "Artist - Title (Lead &amp; Alt Bass)"; picking it picks the tuning.</summary>
internal sealed record CustomTuningSongRow(string Text, TuningSongGroup Group) : CustomTuningRow(Text)
{
    public override bool IsSelectable => true;
}
