#nullable enable
using System;
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
    public IReadOnlyList<SettingBlockViewModel> Blocks { get; }

    [ObservableProperty]
    private bool _hasVisibleFields = true;

    public SettingGroupViewModel(string category, string title, IEnumerable<SettingFieldViewModel> fields)
    {
        Category = category;
        Title = title;
        Fields = new ObservableCollection<SettingFieldViewModel>(fields);
        Blocks = BuildBlocks(Fields);

        UpdateVisibility();
    }

    public void UpdateVisibility() => HasVisibleFields = Fields.Any(f => f.IsVisible);

    /// <summary>
    /// Gathers each field's dependents under it. A field whose parent is in another group (or missing) starts
    /// its own block, and only fields placed under a parent here are marked nested.
    /// </summary>
    private static IReadOnlyList<SettingBlockViewModel> BuildBlocks(IReadOnlyList<SettingFieldViewModel> fields)
    {
        var inGroup = new HashSet<string>(fields.Select(f => f.Descriptor.Key), StringComparer.OrdinalIgnoreCase);
        var children = fields
            .Where(f => f.Descriptor.VisibleWhen is { } cond && inGroup.Contains(cond.Key))
            .ToLookup(f => f.Descriptor.VisibleWhen!.Key, StringComparer.OrdinalIgnoreCase);

        var placed = new HashSet<SettingFieldViewModel>();
        var blocks = new List<SettingBlockViewModel>();

        void Collect(SettingFieldViewModel field, List<SettingFieldViewModel> into, bool nested)
        {
            if (!placed.Add(field))
                return;

            field.IsNested = nested;
            into.Add(field);
            foreach (var child in children[field.Descriptor.Key])
                Collect(child, into, nested: true);
        }

        // Roots first; any field left over sits in a dependency cycle and gets a block of its own.
        var roots = fields.Where(f => f.Descriptor.VisibleWhen is not { } cond || !inGroup.Contains(cond.Key));
        foreach (var field in roots.Concat(fields))
        {
            if (placed.Contains(field))
                continue;

            var blockFields = new List<SettingFieldViewModel>();
            Collect(field, blockFields, nested: false);
            blocks.Add(new SettingBlockViewModel(field, blockFields));
        }

        // Keep the manifest order of the roots.
        var order = fields.Select((f, i) => (f, i)).ToDictionary(x => x.f, x => x.i);
        return blocks.OrderBy(b => order[b.Root]).ToList();
    }
}
