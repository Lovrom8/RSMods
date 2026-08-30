namespace RSMods.Services;

/// <summary>
/// Frontend-owned boundary over the static <see cref="RsModsSettings"/> store. View-models bind to
/// their own snapshots and write back through here so persistence stays off the UI thread and in one
/// place, rather than each screen touching the static store directly.
/// </summary>
internal sealed class SettingsService
{
    public Task SaveAsync() => Task.Run(RsModsSettings.Save);
}
