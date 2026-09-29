using System;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.VisualTree;
using RSMods.ViewModels;

namespace RSMods.Views;

/// <summary>
/// Shows a key bind and rebinds it in place: click it, then press a key (or the middle / a side mouse button).
/// Clicking again or moving focus away cancels. The accepted keys and the reserved-key prompt come from
/// <see cref="ModSettingsViewModel.CaptureKeybindAsync"/>.
/// </summary>
internal sealed class KeyCaptureButton : Button
{
    protected override Type StyleKeyOverride => typeof(Button);

    public KeyCaptureButton()
    {
        AddHandler(KeyDownEvent, (_, e) => OnCaptureKey(e, KeyCapturePhase.KeyDown), RoutingStrategies.Tunnel);
        AddHandler(KeyUpEvent, (_, e) => OnCaptureKey(e, KeyCapturePhase.KeyUp), RoutingStrategies.Tunnel);
        AddHandler(PointerPressedEvent, OnCapturePointer, RoutingStrategies.Tunnel);
    }

    private KeybindRowViewModel? Row => DataContext as KeybindRowViewModel;

    private ModSettingsViewModel? Owner => this.FindAncestorOfType<ModSettingsView>()?.DataContext as ModSettingsViewModel;

    protected override void OnClick()
    {
        base.OnClick();

        if (Row is { } row)
        {
            row.IsCapturing = !row.IsCapturing;
            Focus();
        }
    }

    protected override void OnLostFocus(RoutedEventArgs e)
    {
        base.OnLostFocus(e);

        if (Row is { } row)
            row.IsCapturing = false;
    }

    private void OnCaptureKey(KeyEventArgs e, KeyCapturePhase phase)
    {
        if (Row is not { IsCapturing: true } row || Owner is not { } owner)
            return;

        // Swallow the press so it neither clicks the button nor moves focus.
        e.Handled = true;
        _ = owner.CaptureKeybindAsync(row, e.Key.ToString(), phase);
    }

    private void OnCapturePointer(object? sender, PointerPressedEventArgs e)
    {
        if (Row is not { IsCapturing: true } row || Owner is not { } owner)
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
        _ = owner.CaptureKeybindAsync(row, buttonName, KeyCapturePhase.Mouse);
    }
}
