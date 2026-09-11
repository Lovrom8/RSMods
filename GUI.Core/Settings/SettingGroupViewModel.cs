#nullable enable
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.Core.Settings;

/// <summary>
/// A category group of setting fields rendered together in a section card.
/// </summary>
public sealed partial class SettingGroupViewModel : ObservableObject
{
    public string Category { get; }
    public string Title { get; }
    public ObservableCollection<SettingFieldViewModel> Fields { get; }

    [ObservableProperty]
    private bool _hasVisibleFields = true;

    public SettingGroupViewModel(string category, string title, IEnumerable<SettingFieldViewModel> fields)
    {
        Category = category;
        Title = title;
        Fields = new ObservableCollection<SettingFieldViewModel>(fields);

        UpdateVisibility();
    }

    public void UpdateVisibility() => HasVisibleFields = Fields.Any(f => f.IsVisible);
}
