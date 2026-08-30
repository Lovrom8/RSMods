using CommunityToolkit.Mvvm.ComponentModel;
using RSMods;
using RSMods.Util;

namespace RSMods.ViewModels;

/// <summary>
/// One editable keybinding row (a mod or audio bind). Holds the stored virtual-key string as an
/// in-memory snapshot; the parent screen writes it back to the shared store on Save. The display
/// value strips the "VK_" prefix, matching the WinForms keybinding labels.
/// </summary>
internal sealed partial class KeybindRowViewModel(KeybindItem item) : ObservableObject
{
    private readonly KeybindItem _item = item;

    public string DisplayName => _item.DisplayName;

    [ObservableProperty]
    [NotifyPropertyChangedFor(nameof(KeyDisplay))]
    private string _vkey = item.GetKey();

    /// <summary>The stored key without its "VK_" prefix; an unset bind shows as a dash.</summary>
    public string KeyDisplay
    {
        get
        {
            string ui = KeyConversion.VKeyToUI(Vkey);
            return string.IsNullOrEmpty(ui) ? "—" : ui;
        }
    }

    /// <summary>Persists this row's current value back to the shared store.</summary>
    public void WriteBack() => _item.SetKey(Vkey);
}
