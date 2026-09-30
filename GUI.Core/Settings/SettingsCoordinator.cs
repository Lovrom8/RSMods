#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.Core.Settings;

/// <summary>
/// Coordinates manifest-driven setting field view models, their grouping into mods,
/// reactive enable conditions, and INI load/save orchestration.
/// </summary>
public sealed partial class SettingsCoordinator : ObservableObject
{
    private readonly List<SettingFieldViewModel> _allFields = [];
    private readonly Dictionary<string, SettingFieldViewModel> _fieldsByKey = new(StringComparer.OrdinalIgnoreCase);

    public IReadOnlyList<SettingFieldViewModel> AllFields => _allFields;

    /// <summary>One entry per mod, grouped by <see cref="ModEntryViewModel.Category"/> and sorted by name within each.</summary>
    public IReadOnlyList<ModEntryViewModel> Mods { get; }

    public bool IsDirty => _allFields.Any(f => f.IsDirty);

    public event EventHandler? StateChanged;

    public SettingsCoordinator(IManifestService manifestService, IChoicesProvider? choicesProvider = null)
    {
        InitializeFields(manifestService, choicesProvider);
        WireEnableConditions();
        WireDirtyTracking();
        Mods = ModCatalog.Build(manifestService.AllSettings, _fieldsByKey);
    }

    public SettingFieldViewModel? Find(string key) => _fieldsByKey.TryGetValue(key, out var field) ? field : null;

    public T? Find<T>(string key) where T : SettingFieldViewModel => Find(key) as T;

    public void Load(IniManager ini)
    {
        foreach (var field in _allFields)
            field.Load(ini);

        ReevaluateAllConditions();

        OnPropertyChanged(nameof(IsDirty));
        StateChanged?.Invoke(this, EventArgs.Empty);
    }

    public void Save(IniManager ini)
    {
        // Fields only set values in memory; the caller writes the file.
        foreach (var field in _allFields)
        {
            if (field.IsDirty)
                field.Save(ini);
        }

        OnPropertyChanged(nameof(IsDirty));
        StateChanged?.Invoke(this, EventArgs.Empty);
    }

    private void InitializeFields(IManifestService manifestService, IChoicesProvider? choicesProvider)
    {
        // Key binds live on the keybindings page, which captures key presses (see ManifestKeybinds), and values a
        // bespoke editor owns would otherwise get a second field that overwrites it with a stale value.
        foreach (var desc in manifestService.AllSettings.Where(d => d.Type != SettingType.Key && string.IsNullOrEmpty(d.EditedBy)))
        {
            var field = CreateField(desc, choicesProvider);
            _allFields.Add(field);
            _fieldsByKey[desc.Key] = field;
        }
    }

    private void WireEnableConditions()
    {
        foreach (var field in _allFields)
        {
            if (field.Descriptor.VisibleWhen is { } cond &&
                _fieldsByKey.TryGetValue(cond.Key, out var parentField))
            {
                parentField.ValueChanged += (_, _) => UpdateEnabled(field, parentField, cond);
                parentField.PropertyChanged += (_, e) =>
                {
                    if (e.PropertyName == nameof(SettingFieldViewModel.IsEnabled))
                        UpdateEnabled(field, parentField, cond);
                };
            }
        }

        ReevaluateAllConditions();
    }

    private void WireDirtyTracking()
    {
        foreach (var field in _allFields)
        {
            field.PropertyChanged += (_, e) =>
            {
                if (e.PropertyName == nameof(SettingFieldViewModel.IsDirty))
                {
                    OnPropertyChanged(nameof(IsDirty));
                    StateChanged?.Invoke(this, EventArgs.Empty);
                }
            };
        }
    }

    private void ReevaluateAllConditions()
    {
        foreach (var field in _allFields)
        {
            if (field.Descriptor.VisibleWhen is { } cond &&
                _fieldsByKey.TryGetValue(cond.Key, out var parent))
            {
                UpdateEnabled(field, parent, cond);
            }
        }
    }

    // The manifest calls the condition visibleWhen; the GUI shows the setting either way and only greys it out.
    private static void UpdateEnabled(SettingFieldViewModel field, SettingFieldViewModel parent, SettingVisibilityCondition condition)
    {
        field.IsEnabled = parent.IsEnabled && parent.MatchesCondition(condition.ExpectedValue);
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
}
