using System.Collections.ObjectModel;
using System.Linq;
using RSMods.Core.Settings;

namespace RSMods.ViewModels;

/// <summary>
/// A mod in the Mod Settings list, with the key bind rows its detail pane edits. The flags pick the bespoke
/// panels (font preview, monitor capture, Guitar Speak mappings) that only some mods show.
/// </summary>
internal sealed class ModItemViewModel(ModEntryViewModel entry, ObservableCollection<KeybindRowViewModel> keybinds)
{
    public ModEntryViewModel Entry { get; } = entry;
    public ObservableCollection<KeybindRowViewModel> Keybinds { get; } = keybinds;
    public bool HasKeybinds => Entry.HasKeybinds;

    public bool IsOnScreenText => Entry.Key == "OnScreenFont";
    public bool IsSecondaryMonitor => Entry.Key == "SecondaryMonitor";
    public bool IsGuitarSpeak => Entry.Key == "GuitarSpeak";

    public bool Matches(string? filter) =>
        Entry.Matches(filter) ||
        (!string.IsNullOrWhiteSpace(filter) && Keybinds.Any(k => k.DisplayName.Contains(filter.Trim(), System.StringComparison.OrdinalIgnoreCase)));
}
