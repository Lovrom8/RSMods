using System.Collections.ObjectModel;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;
using RSMods.Core.Settings;

namespace RSMods.ViewModels;

/// <summary>A row in the Mod Settings list: a category heading or a mod.</summary>
internal abstract class ModListRow : ObservableObject
{
    /// <summary>What a screen reader calls the row.</summary>
    public abstract string Title { get; }

    /// <summary>What a screen reader adds after the name, e.g. whether a mod is on.</summary>
    public virtual string Status => string.Empty;

    /// <summary>Headings can't be selected; the list's item style binds IsEnabled to this.</summary>
    public abstract bool IsSelectable { get; }
}

/// <summary>The heading over a category's mods.</summary>
internal sealed class ModCategoryHeader(string title) : ModListRow
{
    public override string Title { get; } = title;

    /// <summary>Shown in capitals, like the navigation's section headings.</summary>
    public string DisplayTitle { get; } = title.ToUpperInvariant();

    public override bool IsSelectable => false;
}

/// <summary>
/// A mod in the Mod Settings list, with the key bind rows its detail pane edits. The flags pick the bespoke
/// panels (font preview, monitor capture, Guitar Speak mappings) that only some mods show.
/// </summary>
internal sealed class ModItemViewModel : ModListRow
{
    public ModItemViewModel(ModEntryViewModel entry, ObservableCollection<KeybindRowViewModel> keybinds)
    {
        Entry = entry;
        Keybinds = keybinds;

        Entry.PropertyChanged += (_, e) =>
        {
            if (e.PropertyName == nameof(ModEntryViewModel.Status))
                OnPropertyChanged(nameof(Status));
        };
    }

    public ModEntryViewModel Entry { get; }
    public ObservableCollection<KeybindRowViewModel> Keybinds { get; }
    public bool HasKeybinds => Entry.HasKeybinds;

    public override string Title => Entry.Title;
    public override string Status => Entry.Status;
    public override bool IsSelectable => true;

    public bool IsOnScreenText => Entry.Key == "OnScreenFont";
    public bool IsSecondaryMonitor => Entry.Key == "SecondaryMonitor";
    public bool IsGuitarSpeak => Entry.Key == "GuitarSpeak";

    public bool Matches(string? filter) =>
        Entry.Matches(filter) ||
        (!string.IsNullOrWhiteSpace(filter) && Keybinds.Any(k => k.DisplayName.Contains(filter.Trim(), System.StringComparison.OrdinalIgnoreCase)));
}
