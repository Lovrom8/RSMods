#nullable enable
using System.Collections.Generic;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.Core.Settings;

/// <summary>
/// One mod in the Mod Settings list: the setting that turns it on (or its main setting when it has no on/off
/// switch), the settings under it, and the key binds that drive it. The list shows the name and switch; the
/// detail pane shows everything else.
/// </summary>
public sealed partial class ModEntryViewModel : ObservableObject
{
    public ModEntryViewModel(string key, string title, string? description, SettingFieldViewModel root,
        IReadOnlyList<SettingFieldViewModel> fields, IReadOnlyList<SettingDescriptor> keybinds)
    {
        Key = key;
        Title = title;
        Description = description;
        Root = root;
        Toggle = root as BoolSettingFieldViewModel;
        Fields = fields;
        Keybinds = keybinds;

        foreach (var field in Fields)
        {
            field.PropertyChanged += (_, e) =>
            {
                if (e.PropertyName == nameof(SettingFieldViewModel.IsVisible))
                {
                    OnPropertyChanged(nameof(HasVisibleFields));
                    OnPropertyChanged(nameof(ShowEnableHint));
                }
            };
        }

        if (Toggle is not null)
        {
            Toggle.PropertyChanged += (_, e) =>
            {
                if (e.PropertyName == nameof(BoolSettingFieldViewModel.Value))
                    OnPropertyChanged(nameof(ShowEnableHint));
            };
        }
    }

    /// <summary>The manifest key of the root setting; bespoke detail content is picked by it.</summary>
    public string Key { get; }
    public string Title { get; }
    public string? Description { get; }
    public bool HasDescription => !string.IsNullOrWhiteSpace(Description);

    public SettingFieldViewModel Root { get; }

    /// <summary>The mod's on/off switch, or null when its main setting is not a toggle.</summary>
    public BoolSettingFieldViewModel? Toggle { get; }
    public bool HasToggle => Toggle is not null;

    /// <summary>What the detail pane lists under the header: every setting but the on/off switch, in manifest order.</summary>
    public IReadOnlyList<SettingFieldViewModel> Fields { get; }
    public bool HasVisibleFields => Fields.Any(f => f.IsVisible);

    /// <summary>The mod is off and that hides all its settings, so the pane can say why it looks empty.</summary>
    public bool ShowEnableHint => Toggle is { Value: false } && Fields.Count > 0 && !HasVisibleFields;

    public IReadOnlyList<SettingDescriptor> Keybinds { get; }
    public bool HasKeybinds => Keybinds.Count > 0;

    public bool Matches(string? filter) =>
        string.IsNullOrWhiteSpace(filter) ||
        Title.Contains(filter.Trim(), System.StringComparison.OrdinalIgnoreCase) ||
        (Description?.Contains(filter.Trim(), System.StringComparison.OrdinalIgnoreCase) ?? false);
}
