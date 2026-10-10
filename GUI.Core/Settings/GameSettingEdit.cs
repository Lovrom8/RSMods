#nullable enable
using System;
using System.IO;

namespace RSMods.Core.Settings;

/// <summary>
/// A setting changed in the game's in-game settings window. While the GUI runs it owns RSMods.ini (it rewrites the
/// whole file from memory), so the game sends its edits over WM_COPYDATA instead of writing the file (DLL SettingEdits).
/// </summary>
public sealed record GameSettingEdit(string IniPath, string Key, string Section, string Name, string Value)
{
    /// <summary>Parses <c>set\n&lt;ini path&gt;\n&lt;key&gt;\n&lt;section&gt;\n&lt;name&gt;\n&lt;value&gt;</c>; null for anything else.</summary>
    public static GameSettingEdit? Parse(string message)
    {
        string[] parts = message.Split('\n');
        return parts.Length == 6 && parts[0] == "set" ? new(parts[1], parts[2], parts[3], parts[4], parts[5]) : null;
    }

    /// <summary>
    /// Puts the value into <paramref name="ini"/> and refreshes the setting's field, unless the user has an unsaved
    /// edit there (the GUI's save then wins and the game reloads it). False when the game's INI isn't this one, i.e. the
    /// GUI is set up for another Rocksmith folder, so the game saves it itself. UI thread: it touches field view models.
    /// </summary>
    public bool ApplyTo(IniManager ini, SettingsCoordinator coordinator)
    {
        if (!SamePath(IniPath, ini.FilePath))
            return false;

        ini.SetStringFromGame(SettingFieldViewModel.NormalizeSection(Section), Name, Value);
        if (coordinator.Find(Key) is { IsDirty: false } field)
            field.Load(ini);
        return true;
    }

    private static bool SamePath(string a, string b)
    {
        try
        {
            return string.Equals(Path.GetFullPath(a), Path.GetFullPath(b), StringComparison.OrdinalIgnoreCase);
        }
        catch (Exception e) when (e is ArgumentException or NotSupportedException or PathTooLongException)
        {
            return false;
        }
    }
}
