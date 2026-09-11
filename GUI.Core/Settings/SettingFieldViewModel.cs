#nullable enable
using System;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.Core.Settings;

/// <summary>
/// Base class for all data-driven mod setting field view models.
/// </summary>
public abstract partial class SettingFieldViewModel(SettingDescriptor descriptor) : ObservableObject
{
    public SettingDescriptor Descriptor { get; } = descriptor;

    public string Key => Descriptor.Key;
    public string Label => Descriptor.Label;
    public string? Hint => Descriptor.Hint;
    public bool HasHint => !string.IsNullOrWhiteSpace(Hint);
    public bool IsNested => Descriptor.VisibleWhen != null;

    public string IniSection { get; } = NormalizeSection(descriptor.Ini.Section);
    public string IniKey => Descriptor.Ini.Name;

    [ObservableProperty]
    private bool _isVisible = true;

    [ObservableProperty]
    private bool _isDirty;

    public event EventHandler? ValueChanged;

    public abstract void Load(IniManager ini);
    public abstract void Save(IniManager ini);
    public abstract string GetCurrentStringValue();

    public virtual bool MatchesCondition(string expectedValue) =>
        string.Equals(GetCurrentStringValue(), expectedValue, StringComparison.OrdinalIgnoreCase);

    protected void NotifyValueChanged() => ValueChanged?.Invoke(this, EventArgs.Empty);

    protected static string NormalizeSection(string section) => section.StartsWith('[') && section.EndsWith(']') ? section : $"[{section}]";
}
