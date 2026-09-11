#nullable enable
using System;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.Core.Settings;

/// <summary>
/// View model for freeform text / string settings.
/// </summary>
public sealed partial class StringSettingFieldViewModel(SettingDescriptor descriptor) : SettingFieldViewModel(descriptor)
{
    [ObservableProperty]
    private string _value = "";

    private string _initialValue = "";

    partial void OnValueChanged(string value)
    {
        IsDirty = !string.Equals(value, _initialValue, StringComparison.Ordinal);
        NotifyValueChanged();
    }

    public override void Load(IniManager ini)
    {
        Value = ini.GetString(IniSection, IniKey, Descriptor.Default);
        _initialValue = Value;
        IsDirty = false;
    }

    public override void Save(IniManager ini)
    {
        ini.SetString(IniSection, IniKey, Value);
        _initialValue = Value;
        IsDirty = false;
    }

    public override string GetCurrentStringValue() => Value;
}
