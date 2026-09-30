#nullable enable
using System.Collections.Generic;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.Core.Settings;

/// <summary>
/// One mod in the Mod Settings list: the setting that turns it on (or its main setting when it has no on/off
/// switch), the settings under it, and the key binds that drive it. The list shows the name and whether it's on;
/// the detail pane shows everything else.
/// </summary>
public sealed partial class ModEntryViewModel : ObservableObject
{
    public ModEntryViewModel(string key, string title, string? description, string category, SettingFieldViewModel root,
        IReadOnlyList<SettingFieldViewModel> fields, IReadOnlyList<SettingDescriptor> keybinds)
    {
        Key = key;
        Title = title;
        Description = description;
        Category = category;
        Root = root;
        Toggle = root as BoolSettingFieldViewModel;
        Fields = fields;
        Keybinds = keybinds;

        if (Toggle is not null)
        {
            Toggle.PropertyChanged += (_, e) =>
            {
                if (e.PropertyName == nameof(BoolSettingFieldViewModel.Value))
                {
                    OnPropertyChanged(nameof(IsOn));
                    OnPropertyChanged(nameof(Status));
                    OnPropertyChanged(nameof(EnableHint));
                }
            };
        }
    }

    /// <summary>The manifest key of the root setting; bespoke detail content is picked by it.</summary>
    public string Key { get; }
    public string Title { get; }
    public string? Description { get; }
    public bool HasDescription => !string.IsNullOrWhiteSpace(Description);

    /// <summary>The list heading the mod is grouped under.</summary>
    public string Category { get; }

    public SettingFieldViewModel Root { get; }

    /// <summary>The mod's on/off switch, or null when its main setting is not a toggle.</summary>
    public BoolSettingFieldViewModel? Toggle { get; }
    public bool HasToggle => Toggle is not null;

    /// <summary>The mod has an on/off switch and it's on; the list marks these with a check.</summary>
    public bool IsOn => Toggle is { Value: true };

    /// <summary>What a screen reader says for the list's check mark.</summary>
    public string Status => Toggle is null ? string.Empty : IsOn ? "On" : "Off";

    /// <summary>What the detail pane lists under the header: every setting but the on/off switch, in manifest order.</summary>
    public IReadOnlyList<SettingFieldViewModel> Fields { get; }

    /// <summary>
    /// Some settings grey out while the mod is off, so the pane has a line saying why. The line keeps its place
    /// when the mod is on (see <see cref="EnableHint"/>), so flipping the switch doesn't move the settings.
    /// </summary>
    public bool HasEnableHint => Toggle is not null && Fields.Any(f => f.Descriptor.VisibleWhen is not null);

    public string EnableHint => HasEnableHint && !IsOn ? "Turn this mod on to change its greyed-out settings." : string.Empty;

    public IReadOnlyList<SettingDescriptor> Keybinds { get; }
    public bool HasKeybinds => Keybinds.Count > 0;

    public bool Matches(string? filter) =>
        string.IsNullOrWhiteSpace(filter) ||
        Title.Contains(filter.Trim(), System.StringComparison.OrdinalIgnoreCase) ||
        (Description?.Contains(filter.Trim(), System.StringComparison.OrdinalIgnoreCase) ?? false);
}
