using RSMods.Util;

namespace RSMods.Services;

/// <summary>
/// Frontend-owned boundary over the static <see cref="RsModsSettings"/> store. View-models bind to
/// their own snapshots and write back through here so persistence stays off the UI thread and in one
/// place, rather than each screen touching the static store directly.
/// </summary>
internal sealed class SettingsService
{
    /// <summary>
    /// The mod DLL only reloads settings when it receives the WM_COPYDATA "update all" message; without
    /// it a save sits on disk until the next game launch and the mod framework's per-mod
    /// <c>OnSettingsChanged</c> hooks never fire. 
    /// </summary>
    public Task SaveAsync() => Task.Run(() =>
    {
        // Only when a value changed: the game rereads the whole INI on every "update all".
        if (RsModsSettings.Save())
            WinMsgUtil.SendMsgToRS("update all");
    });
}
