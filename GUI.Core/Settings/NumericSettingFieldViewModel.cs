#nullable enable
using System;
using System.Globalization;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.Core.Settings;

/// <summary>
/// View model for numeric / integer settings with optional min/max and scaling.
/// </summary>
public sealed partial class NumericSettingFieldViewModel(SettingDescriptor descriptor) : SettingFieldViewModel(descriptor)
{
    [ObservableProperty]
    private decimal _value;

    private decimal _initialValue;
    private readonly int _defaultInt = int.TryParse(descriptor.Default, NumberStyles.Integer, CultureInfo.InvariantCulture, out int d) ? d : 0;

    public bool HasScale => Descriptor.Scale.HasValue && Math.Abs(Descriptor.Scale.Value - 1.0) > 1e-6;

    /// <summary>
    /// Multiplier converting stored value to UI value (e.g. 0.001 converts ms to seconds).
    /// Supports both standard multiplier convention (&lt; 1.0) and divisor convention (&gt; 1.0).
    /// </summary>
    private double ScaleFactor => (Descriptor.Scale.HasValue && Descriptor.Scale.Value > 1.0)
        ? (1.0 / Descriptor.Scale.Value)
        : (Descriptor.Scale ?? 1.0);

    // Unbounded settings get the full int range the DLL stores; a narrower fallback would clamp real values
    // (a secondary monitor at X=1920 or X=-1920) and write the clamped value back.
    public decimal Minimum => (decimal)((Descriptor.Min ?? int.MinValue) * ScaleFactor);
    public decimal Maximum => (decimal)((Descriptor.Max ?? int.MaxValue) * ScaleFactor);
    public double Scale => Descriptor.Scale ?? 1.0;
    public decimal Increment => HasScale ? 0.25m : 1m;
    public string FormatString => HasScale ? "0.###" : "0";

    partial void OnValueChanged(decimal value)
    {
        IsDirty = value != _initialValue;
        NotifyValueChanged();
    }

    public override void Load(IniManager ini)
    {
        int stored = ini.GetInt(IniSection, IniKey, _defaultInt);
        Value = (decimal)(stored * ScaleFactor);
        _initialValue = Value;
        IsDirty = false;
    }

    public override void Save(IniManager ini)
    {
        int toStore = (int)Math.Round((double)Value / ScaleFactor);
        ini.SetInt(IniSection, IniKey, toStore);
        _initialValue = Value;

        IsDirty = false;
    }

    public override string GetCurrentStringValue() => ((int)Math.Round((double)Value / ScaleFactor)).ToString(CultureInfo.InvariantCulture);
}
