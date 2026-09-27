#nullable enable
using System;
using CommunityToolkit.Mvvm.Input;

namespace RSMods.Core.Settings;

/// <summary>
/// A button that opens a bespoke editor (Guitar Speak, colours, Twitch, MIDI). It holds no value of its own:
/// the editor it opens reads and writes its settings.
/// </summary>
public sealed partial class CustomEditorFieldViewModel(SettingDescriptor descriptor) : SettingFieldViewModel(descriptor)
{
    public string EditorType => Descriptor.Editor ?? "";
    public string ButtonText => $"Open {Descriptor.Label}";

    public event Action<string>? EditorRequested;

    [RelayCommand]
    private void OpenEditor() => EditorRequested?.Invoke(EditorType);

    public override void Load(IniManager ini) { }

    public override void Save(IniManager ini) { }

    public override string GetCurrentStringValue() => "";
}
