#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.Core.Settings;

/// <summary>
/// Coordinates manifest-driven setting field view models, category grouping,
/// reactive visibility conditions, and INI load/save orchestration.
/// </summary>
public sealed partial class SettingsCoordinator : ObservableObject
{
    private readonly List<SettingFieldViewModel> _allFields = [];
    private readonly Dictionary<string, SettingFieldViewModel> _fieldsByKey = new(StringComparer.OrdinalIgnoreCase);
    private readonly List<SettingGroupViewModel> _groups = [];

    public IReadOnlyList<SettingFieldViewModel> AllFields => _allFields;
    public IReadOnlyList<SettingGroupViewModel> Groups => _groups;

    public bool IsDirty => _allFields.Any(f => f.IsDirty);

    public event EventHandler? StateChanged;

    public SettingsCoordinator(IManifestService manifestService, IChoicesProvider? choicesProvider = null)
    {
        InitializeFields(manifestService, choicesProvider);
        WireVisibilityConditions();
        GroupByCategory();
    }

    public SettingFieldViewModel? Find(string key) => _fieldsByKey.TryGetValue(key, out var field) ? field : null;

    public T? Find<T>(string key) where T : SettingFieldViewModel => Find(key) as T;

    public void Load(IniManager ini)
    {
        foreach (var field in _allFields)
            field.Load(ini);

        ReevaluateAllVisibilities();

        foreach (var group in _groups)
            group.UpdateVisibility();

        OnPropertyChanged(nameof(IsDirty));
        StateChanged?.Invoke(this, EventArgs.Empty);
    }

    public void Save(IniManager ini)
    {
        using (ini.SuspendSave())
        {
            foreach (var field in _allFields)
            {
                if (field.IsDirty)
                    field.Save(ini);
            }
        }

        OnPropertyChanged(nameof(IsDirty));
        StateChanged?.Invoke(this, EventArgs.Empty);
    }

    private void InitializeFields(IManifestService manifestService, IChoicesProvider? choicesProvider)
    {
        foreach (var desc in manifestService.AllSettings)
        {
            var field = CreateField(desc, choicesProvider);
            _allFields.Add(field);
            _fieldsByKey[desc.Key] = field;
        }
    }

    private void WireVisibilityConditions()
    {
        foreach (var field in _allFields)
        {
            if (field.Descriptor.VisibleWhen is { } cond &&
                _fieldsByKey.TryGetValue(cond.Key, out var parentField))
            {
                parentField.ValueChanged += (_, _) => UpdateVisibility(field, parentField, cond);
                parentField.PropertyChanged += (_, e) =>
                {
                    if (e.PropertyName == nameof(SettingFieldViewModel.IsVisible))
                        UpdateVisibility(field, parentField, cond);
                };
            }
        }

        ReevaluateAllVisibilities();
    }

    private void GroupByCategory()
    {
        var categoryGroups = _allFields
            .GroupBy(f => string.IsNullOrWhiteSpace(f.Descriptor.Category) ? "General" : f.Descriptor.Category);

        foreach (var group in categoryGroups)
        {
            string categoryName = group.Key;
            string displayTitle = FormatCategoryTitle(categoryName);
            var groupVm = new SettingGroupViewModel(categoryName, displayTitle, group);

            foreach (var field in group)
            {
                field.PropertyChanged += (_, e) =>
                {
                    if (e.PropertyName == nameof(SettingFieldViewModel.IsVisible))
                        groupVm.UpdateVisibility();

                    if (e.PropertyName == nameof(SettingFieldViewModel.IsDirty))
                    {
                        OnPropertyChanged(nameof(IsDirty));
                        StateChanged?.Invoke(this, EventArgs.Empty);
                    }
                };
            }

            _groups.Add(groupVm);
        }
    }

    private void ReevaluateAllVisibilities()
    {
        foreach (var field in _allFields)
        {
            if (field.Descriptor.VisibleWhen is { } cond &&
                _fieldsByKey.TryGetValue(cond.Key, out var parent))
            {
                UpdateVisibility(field, parent, cond);
            }
        }
    }

    private static void UpdateVisibility(SettingFieldViewModel field, SettingFieldViewModel parent, SettingVisibilityCondition condition)
    {
        field.IsVisible = parent.IsVisible && parent.MatchesCondition(condition.ExpectedValue);
    }

    private static SettingFieldViewModel CreateField(SettingDescriptor desc, IChoicesProvider? choicesProvider)
    {
        if (!string.IsNullOrEmpty(desc.Editor))
            return new CustomEditorFieldViewModel(desc);

        return desc.Type switch
        {
            SettingType.Bool => new BoolSettingFieldViewModel(desc),
            SettingType.Int => new NumericSettingFieldViewModel(desc),
            SettingType.Enum => new EnumSettingFieldViewModel(desc, choicesProvider),
            SettingType.String => (desc.Choices is { Count: > 0 } || !string.IsNullOrEmpty(desc.ChoicesSource))
                ? new EnumSettingFieldViewModel(desc, choicesProvider)
                : new StringSettingFieldViewModel(desc),
            _ => new StringSettingFieldViewModel(desc)
        };
    }

    private static string FormatCategoryTitle(string category) => category switch
    {
        "Toggle Switches" => "Toggles",
        "Mod Settings" => "Mod Settings",
        _ => category
    };
}
