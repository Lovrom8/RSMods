using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using RSMods.ViewModels;

namespace RSMods.Views;

internal sealed partial class ModSettingsView : UserControl
{
    public ModSettingsView()
    {
        InitializeComponent();

        // The capture boxes swallow the next key/mouse press and hand it to the view-model, which applies
        // the shared Rocksmith key policy. Tunnel handlers run before default text/navigation handling so
        // keys like Tab, Space, and the arrows are captured instead of moving focus or typing.
        WireCapture(ModCaptureBox, mods: true);
        WireCapture(AudioCaptureBox, mods: false);
    }

    private void WireCapture(TextBox box, bool mods)
    {
        box.AddHandler(KeyDownEvent, (_, e) => OnCaptureKey(e, mods, KeyCapturePhase.KeyDown), RoutingStrategies.Tunnel);
        box.AddHandler(KeyUpEvent, (_, e) => OnCaptureKey(e, mods, KeyCapturePhase.KeyUp), RoutingStrategies.Tunnel);
        box.AddHandler(PointerPressedEvent, (_, e) => OnCapturePointer(e, mods), RoutingStrategies.Tunnel);
    }

    private void OnCaptureKey(KeyEventArgs e, bool mods, KeyCapturePhase phase)
    {
        // Swallow the press so the read-only box neither types nor moves focus.
        e.Handled = true;

        if (DataContext is not ModSettingsViewModel vm)
            return;

        string keyName = e.Key.ToString();
        _ = mods
            ? vm.CaptureModKeybindAsync(keyName, phase)
            : vm.CaptureAudioKeybindAsync(keyName, phase);
    }

    private void OnCapturePointer(PointerPressedEventArgs e, bool mods)
    {
        if (DataContext is not ModSettingsViewModel vm)
            return;

        // Only the mouse buttons Rocksmith supports as binds; left/right stay for normal interaction.
        string? buttonName = e.GetCurrentPoint(this).Properties.PointerUpdateKind switch
        {
            PointerUpdateKind.MiddleButtonPressed => "Middle",
            PointerUpdateKind.XButton1Pressed => "XButton1",
            PointerUpdateKind.XButton2Pressed => "XButton2",
            _ => null,
        };

        if (buttonName is null)
            return;

        e.Handled = true;
        _ = mods
            ? vm.CaptureModKeybindAsync(buttonName, KeyCapturePhase.Mouse)
            : vm.CaptureAudioKeybindAsync(buttonName, KeyCapturePhase.Mouse);
    }
}
