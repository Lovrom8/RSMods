#nullable enable
using System;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.Core.Settings;

/// <summary>
/// View model for boolean toggle settings.
/// </summary>
public sealed partial class BoolSettingFieldViewModel(SettingDescriptor descriptor) : SettingFieldViewModel(descriptor)
{
    [ObservableProperty]
    private bool _value;

    private bool _initialValue;
    private bool _isNumeric = descriptor.Default == "0" || descriptor.Default == "1";
    private readonly bool _defaultBool = descriptor.Default.Equals("on", StringComparison.OrdinalIgnoreCase) ||
                                         descriptor.Default.Equals("true", StringComparison.OrdinalIgnoreCase) ||
                                         descriptor.Default == "1";

    partial void OnValueChanged(bool value)
    {
        IsDirty = value != _initialValue;
        NotifyValueChanged();
    }

    public override void Load(IniManager ini)
    {
        string raw = ini.GetString(IniSection, IniKey, Descriptor.Default);
        _isNumeric = DetermineIsNumeric(raw);

        Value = ini.GetBool(IniSection, IniKey, _defaultBool, _isNumeric);
        _initialValue = Value;
        IsDirty = false;
    }

    private bool DetermineIsNumeric(string rawValue)
    {
        // 1. Mod toggles in [Toggle Switches] are strictly "on"/"off" per DLL Settings::IsOn convention
        string section = NormalizeSection(Descriptor.Ini.Section);
        if (section.Equals("Toggle Switches", StringComparison.OrdinalIgnoreCase))
            return false;

        // 2. Rocksmith.ini native sections are strictly numeric 1/0
        if (section.Equals("Audio", StringComparison.OrdinalIgnoreCase) ||
            section.Equals("Renderer.Win32", StringComparison.OrdinalIgnoreCase) ||
            section.Equals("Net", StringComparison.OrdinalIgnoreCase))
            return true;

        // 3. If present on disk, preserve the existing written format
        if (rawValue == "0" || rawValue == "1")
            return true;
        if (rawValue.Equals("on", StringComparison.OrdinalIgnoreCase) || rawValue.Equals("off", StringComparison.OrdinalIgnoreCase))
            return false;

        // 4. Default fallback: only numeric if explicitly "0" or "1"
        return Descriptor.Default == "0" || Descriptor.Default == "1";
    }

    public override void Save(IniManager ini)
    {
        ini.SetBool(IniSection, IniKey, Value, _isNumeric);
        _initialValue = Value;
        IsDirty = false;
    }

    public override string GetCurrentStringValue() => Value ? (_isNumeric ? "1" : "on") : (_isNumeric ? "0" : "off");

    public override bool MatchesCondition(string expectedValue)
    {
        bool expBool = expectedValue.Equals("on", StringComparison.OrdinalIgnoreCase) ||
                       expectedValue == "1" ||
                       expectedValue.Equals("true", StringComparison.OrdinalIgnoreCase);
        return Value == expBool;
    }
}
