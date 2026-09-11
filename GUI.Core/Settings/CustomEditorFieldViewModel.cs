#nullable enable
using System;
using CommunityToolkit.Mvvm.Input;

namespace RSMods.Core.Settings;

/// <summary>
/// View model for Tier 3 sub-UI / custom editor launchers.
/// Supports generic dirty tracking and save dispatch so bespoke sub-editors
/// (Guitar Speak, Colors, Twitch, MIDI) plug cleanly into SettingsCoordinator.
/// </summary>
public sealed partial class CustomEditorFieldViewModel(SettingDescriptor descriptor) : SettingFieldViewModel(descriptor)
{
    public string EditorType => Descriptor.Editor ?? "";
    public string ButtonText => $"Open {Descriptor.Label}";

    public event Action<string>? EditorRequested;

    /// <summary>Optional save handler dispatched during generic coordinator Save.</summary>
    public Action<IniManager>? SaveHandler { get; set; }

    /// <summary>Optional load handler dispatched during generic coordinator Load.</summary>
    public Action<IniManager>? LoadHandler { get; set; }

    [RelayCommand]
    private void OpenEditor() => EditorRequested?.Invoke(EditorType);

    /// <summary>
    /// Explicitly marks this custom editor dirty, propagating to Coordinator.IsDirty.
    /// </summary>
    public void SetDirty(bool dirty = true)
    {
        if (IsDirty != dirty)
        {
            IsDirty = dirty;
            NotifyValueChanged();
        }
    }

    public override void Load(IniManager ini)
    {
        LoadHandler?.Invoke(ini);
        IsDirty = false;
    }

    public override void Save(IniManager ini)
    {
        SaveHandler?.Invoke(ini);
        IsDirty = false;
    }

    public override string GetCurrentStringValue() => "";
}
