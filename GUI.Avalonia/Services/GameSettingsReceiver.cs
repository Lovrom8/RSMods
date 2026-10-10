using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Avalonia.Controls;
using RSMods.Core.Settings;
using RSMods.Util;

namespace RSMods.Services;

/// <summary>
/// Receives settings changed in the game's in-game settings window, the reverse of the WM_COPYDATA messages the GUI
/// sends the game, so the GUI stays the only writer of RSMods.ini while it runs. The game finds this window by the
/// <see cref="WindowMarker"/> property; with no marked window it writes the file itself (DLL SettingEdits).
/// </summary>
internal sealed class GameSettingsReceiver(SettingsCoordinator coordinator, SettingsService settings)
{
    private const string WindowMarker = "RSModsGUI"; // Must match kGuiWindowMarker in DLL/SettingEdits.cpp
    private const uint WM_COPYDATA = 0x004A;
    private const int SettingEditData = 1;           // COPYDATASTRUCT.dwData, as for the GUI's own messages

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern bool SetProp(IntPtr hWnd, string lpString, IntPtr hData);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern IntPtr RemoveProp(IntPtr hWnd, string lpString);

    public void Attach(Window window)
    {
        window.Opened += (_, _) =>
        {
            if (window.TryGetPlatformHandle()?.Handle is not { } hwnd)
                return;

            Win32Properties.AddWndProcHookCallback(window, WndProc);
            SetProp(hwnd, WindowMarker, 1);
            window.Closed += (_, _) => RemoveProp(hwnd, WindowMarker);
        };
    }

    // Returns 1 once the edit is in the GUI's settings; 0 and the game writes the line itself.
    private IntPtr WndProc(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam, ref bool handled)
    {
        if (msg != WM_COPYDATA)
            return IntPtr.Zero;

        // A copy of the sender's struct: never Dispose it, the game owns that memory.
        var data = Marshal.PtrToStructure<WinMsgUtil.CopyData>(lParam);
        if (data.dwData != SettingEditData || data.lpData == IntPtr.Zero)
            return IntPtr.Zero;

        handled = true;
        string message = data.AsAnsiString.TrimEnd('\0');
        if (GameSettingEdit.Parse(message) is not { } edit || RsModsSettings.Ini is not { } ini || !edit.ApplyTo(ini, coordinator))
            return IntPtr.Zero;

        // Off the UI thread, like a screen's autosave. A failed write keeps the value in memory for the next save.
        _ = SaveAsync();
        return 1;
    }

    private async Task SaveAsync()
    {
        try
        {
            await settings.SaveAsync();
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException)
        {
            Debug.WriteLine($"Couldn't save a setting changed in game: {e.Message}");
        }
    }
}
