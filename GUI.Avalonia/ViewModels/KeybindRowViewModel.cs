using CommunityToolkit.Mvvm.ComponentModel;
using RSMods;
using RSMods.Util;

namespace RSMods.ViewModels;

/// <summary>
/// One editable keybinding row (a mod or audio bind). Holds the stored virtual-key string as an
/// in-memory snapshot; the parent screen writes it back to the shared store when it saves. The display
/// value strips the "VK_" prefix.
/// </summary>
internal sealed partial class KeybindRowViewModel(KeybindItem item) : ObservableObject
{
    private readonly KeybindItem _item = item;
    private string _saved = item.GetKey();

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

    /// <summary>Persists this row's value if it changed since it was loaded or last written.</summary>
    public void WriteBack()
    {
        if (Vkey == _saved)
            return;

        _item.SetKey(Vkey);
        _saved = Vkey;
    }
}
