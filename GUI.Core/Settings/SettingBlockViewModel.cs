#nullable enable
using System.Collections.Generic;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.Core.Settings;

/// <summary>
/// A setting followed by the settings in the same group that depend on it, laid out as one cell so a
/// sub-setting always sits under its parent however the group's cells wrap.
/// </summary>
public sealed partial class SettingBlockViewModel : ObservableObject
{
    public SettingFieldViewModel Root { get; }

    /// <summary>The root first, then its dependents depth-first in manifest order.</summary>
    public IReadOnlyList<SettingFieldViewModel> Fields { get; }

    [ObservableProperty]
    private bool _isVisible;

    public SettingBlockViewModel(SettingFieldViewModel root, IReadOnlyList<SettingFieldViewModel> fields)
    {
        Root = root;
        Fields = fields;
        IsVisible = root.IsVisible;

        root.PropertyChanged += (_, e) =>
        {
            if (e.PropertyName == nameof(SettingFieldViewModel.IsVisible))
                IsVisible = root.IsVisible;
        };
    }
}
