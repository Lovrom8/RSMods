using System.IO;
using RSMods.Data;
using RSMods.Rocksmith;

namespace RSMods.Services;

/// <summary>
/// Owns the single <see cref="RocksmithSettings"/> instance, constructed lazily from the Rocksmith
/// install folder resolved at startup. Lazy construction keeps it out of the way until a screen
/// actually needs it, by which point <see cref="Constants.RSFolder"/> is set.
/// </summary>
internal sealed class RocksmithSettingsService
{
    private RocksmithSettings? _settings;

    public RocksmithSettings Get() => _settings ??= new RocksmithSettings(
        Path.Combine(Constants.RSFolder, RocksmithSettings.DefaultFileName));
}
