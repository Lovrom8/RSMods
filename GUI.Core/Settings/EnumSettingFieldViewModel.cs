#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;

namespace RSMods.Core.Settings;

/// <summary>
/// View model for enumeration settings or string settings with predefined/dynamic choices.
/// </summary>
public sealed partial class EnumSettingFieldViewModel : SettingFieldViewModel
{
    private readonly IChoicesProvider? _choicesProvider;

    [ObservableProperty]
    private IReadOnlyList<string> _choices = [];

    [ObservableProperty]
    private string? _selectedValue;

    private string? _initialValue;

    public EnumSettingFieldViewModel(SettingDescriptor descriptor, IChoicesProvider? choicesProvider = null) : base(descriptor)
    {
        _choicesProvider = choicesProvider;
        RefreshChoices();
    }

    public void RefreshChoices()
    {
        List<string> list = [];

        if (Descriptor.Choices != null && Descriptor.Choices.Count > 0)
        {
            list.AddRange(Descriptor.Choices);
        }
        else if (!string.IsNullOrEmpty(Descriptor.ChoicesSource) && _choicesProvider != null)
        {
            var dynamicList = _choicesProvider.GetChoices(Descriptor.ChoicesSource);
            if (dynamicList != null)
                list.AddRange(dynamicList);
        }

        if (SelectedValue != null && !list.Contains(SelectedValue, StringComparer.OrdinalIgnoreCase))
        {
            list.Add(SelectedValue);
        }

        Choices = list;
    }

    partial void OnSelectedValueChanged(string? value)
    {
        IsDirty = !string.Equals(value, _initialValue, StringComparison.OrdinalIgnoreCase);
        NotifyValueChanged();
    }

    public override void Load(IniManager ini)
    {
        string raw = ini.GetString(IniSection, IniKey, Descriptor.Default);

        RefreshChoices();

        var match = Choices.FirstOrDefault(c => string.Equals(c, raw, StringComparison.OrdinalIgnoreCase));
        if (match == null && !string.IsNullOrEmpty(raw))
        {
            var updated = new List<string>(Choices) { raw };
            Choices = updated;
            match = raw;
        }

        SelectedValue = match ?? raw;
        _initialValue = SelectedValue;
        
        IsDirty = false;
    }

    public override void Save(IniManager ini)
    {
        ini.SetString(IniSection, IniKey, SelectedValue ?? Descriptor.Default);
        _initialValue = SelectedValue;

        IsDirty = false;
    }

    public override string GetCurrentStringValue() => SelectedValue ?? "";
}
