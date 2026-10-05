#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;

namespace RSMods.Core.Settings;

/// <summary>
/// Turns the manifest's flat setting list into one entry per mod for the Mod Settings list. A setting that
/// starts a mod is one with no <c>visibleWhen</c>; the settings gated on it (at any depth) go under it, and each
/// key bind goes to the mod declared just before it, which is how the DLL emits a mod's settings. Mods are grouped
/// under a heading by what they change, so a player can find one without knowing its name.
/// </summary>
public static class ModCatalog
{
    // Values another screen owns (Twitch, Custom Colors, the Guitar Speak card), or a button that only said where
    // to go. Listing them here as well would give one value two editors.
    private static readonly HashSet<string> Hidden = new(StringComparer.OrdinalIgnoreCase)
    {
        "TwitchSettings",
        "MidiCustomEditor",
        "GuitarSpeakCustomEditor",
        "CustomStringColors",
        "SeparateNoteColors",
        "SeparateNoteColorsMode",
        "CustomHighwayColors",
    };

    // Settings and key binds that belong to a mod the manifest order or visibleWhen doesn't place them under.
    private static readonly Dictionary<string, string> Owner = new(StringComparer.OrdinalIgnoreCase)
    {
        ["OnScreenFontSize"] = "OnScreenFont",
        ["TuningPedal"] = "AutoTuneForSong",
        ["TuningOffset"] = "AutoTuneForSong",
        ["TuningOffsetKey"] = "AutoTuneForSong",
        ["LoopStartKey"] = "AllowLooping",
        ["LoopEndKey"] = "AllowLooping",
        ["RewindKey"] = "AllowRewind",
        ["RainbowStringsKey"] = "RainbowStringsEnabled",
        ["ToggleExtendedRangeKey"] = "ExtendedRangeEnabled",
    };

    /// <summary>The list's headings, in the order they're shown. A mod not in <see cref="CategoryOf"/> goes under the last.</summary>
    public static readonly IReadOnlyList<string> Categories =
    [
        "Practice",
        "Tuning",
        "Highway & Scenery",
        "Colors",
        "On-Screen Info",
        "Audio",
        "Songs & Profiles",
        "Game & Window",
        "Other",
    ];

    private static readonly Dictionary<string, string> CategoryOf = new(StringComparer.OrdinalIgnoreCase)
    {
        ["AllowRewind"] = "Practice",
        ["AllowLooping"] = "Practice",
        ["RRSpeedAboveOneHundred"] = "Practice",
        ["LinearRiffRepeater"] = "Practice",
        ["UseCustomNSPTimer"] = "Practice",

        ["AutoTuneForSong"] = "Tuning",
        ["ExtendedRangeEnabled"] = "Tuning",

        ["RemoveSkylineEnabled"] = "Highway & Scenery",
        ["RemoveLyrics"] = "Highway & Scenery",
        ["RemoveLaneMarkersEnabled"] = "Highway & Scenery",
        ["RemoveInlaysEnabled"] = "Highway & Scenery",
        ["RemoveHeadstockEnabled"] = "Highway & Scenery",
        ["RemoveFingerprints"] = "Highway & Scenery",
        ["ToggleLoftEnabled"] = "Highway & Scenery",
        ["GreenScreenWallEnabled"] = "Highway & Scenery",
        ["FretlessModeEnabled"] = "Highway & Scenery",

        ["RainbowNotesEnabled"] = "Colors",
        ["RainbowStringsEnabled"] = "Colors",
        ["StringColorsCustomEditor"] = "Colors",

        ["DisplayCurrentAccuracy"] = "On-Screen Info",
        ["ShowSongTimerEnabled"] = "On-Screen Info",
        ["ShowCurrentNoteOnScreen"] = "On-Screen Info",
        ["OnScreenFont"] = "On-Screen Info",

        ["VolumeControlEnabled"] = "Audio",
        ["OverrideInputVolumeEnabled"] = "Audio",
        ["AltOutputSampleRate"] = "Audio",
        ["AllowAudioInBackground"] = "Audio",
        ["SongPreviews"] = "Audio",

        ["ForceReEnumerationEnabled"] = "Songs & Profiles",
        ["ScreenShotScores"] = "Songs & Profiles",
        ["ForceProfileEnabled"] = "Songs & Profiles",
        ["BackupProfile"] = "Songs & Profiles",

        ["BypassTwoRTCMessageBox"] = "Game & Window",
        ["SecondaryMonitor"] = "Game & Window",
        ["Ultrawide"] = "Game & Window",
        ["GuitarSpeak"] = "Game & Window",
        ["SettingsMenuEnabled"] = "Game & Window",
    };

    private static readonly Dictionary<string, string> Titles = new(StringComparer.OrdinalIgnoreCase)
    {
        ["OnScreenFont"] = "On-Screen Text",
        ["StringColorsCustomEditor"] = "Custom Colors",
    };

    private static readonly Dictionary<string, string> Descriptions = new(StringComparer.OrdinalIgnoreCase)
    {
        ["StringColorsCustomEditor"] = "String, note and highway colors have their own page.",
    };

    public static IReadOnlyList<ModEntryViewModel> Build(
        IEnumerable<SettingDescriptor> manifestOrder,
        IReadOnlyDictionary<string, SettingFieldViewModel> fieldsByKey)
    {
        var entries = new List<Builder>();
        var byRoot = new Dictionary<string, Builder>(StringComparer.OrdinalIgnoreCase);
        var rootOf = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        Builder? last = null;

        foreach (var desc in manifestOrder)
        {
            if (Hidden.Contains(desc.Key))
                continue;

            if (desc.Type == SettingType.Key)
            {
                Builder? owner = Owner.TryGetValue(desc.Key, out string? ownerKey) && rootOf.TryGetValue(ownerKey, out string? ownerRoot)
                    ? byRoot[ownerRoot]
                    : last;
                owner?.Keybinds.Add(desc);
                continue;
            }

            // Values a bespoke editor owns have no field of their own.
            if (!fieldsByKey.TryGetValue(desc.Key, out var field))
                continue;

            string? parentKey = Owner.TryGetValue(desc.Key, out string? explicitOwner) ? explicitOwner : desc.VisibleWhen?.Key;
            if (parentKey is not null && rootOf.TryGetValue(parentKey, out string? root))
            {
                Builder entry = byRoot[root];
                // Indented only when it hangs off another sub-setting rather than the mod itself.
                field.IsNested = desc.VisibleWhen is { } cond && !string.Equals(cond.Key, root, StringComparison.OrdinalIgnoreCase)
                                 && rootOf.ContainsKey(cond.Key);
                entry.Fields.Add(field);
                rootOf[desc.Key] = root;
                continue;
            }

            field.IsNested = false;
            var created = new Builder(field);
            entries.Add(created);
            byRoot[desc.Key] = created;
            rootOf[desc.Key] = desc.Key;
            last = created;
        }

        return entries
            .Select(b => b.Build())
            .OrderBy(e => IndexOfCategory(e.Category))
            .ThenBy(e => e.Title, StringComparer.CurrentCultureIgnoreCase)
            .ToList();
    }

    private static int IndexOfCategory(string category)
    {
        for (int i = 0; i < Categories.Count; i++)
        {
            if (Categories[i] == category)
                return i;
        }
        return Categories.Count;
    }

    private sealed class Builder(SettingFieldViewModel root)
    {
        public SettingFieldViewModel Root { get; } = root;
        public List<SettingFieldViewModel> Fields { get; } = [];
        public List<SettingDescriptor> Keybinds { get; } = [];

        public ModEntryViewModel Build()
        {
            string key = Root.Key;
            string title = Titles.TryGetValue(key, out string? t) ? t : Root.Label;
            bool isToggle = Root is BoolSettingFieldViewModel;

            // A toggle root is the entry's switch and its hint describes the mod; any other root is the entry's
            // first setting and shows its hint itself.
            string? description = Descriptions.TryGetValue(key, out string? d) ? d : isToggle ? Root.Hint : null;
            IReadOnlyList<SettingFieldViewModel> fields = isToggle ? Fields : [Root, .. Fields];

            string category = CategoryOf.TryGetValue(key, out string? c) ? c : Categories[^1];

            return new ModEntryViewModel(key, title, description, category, Root, fields, Keybinds);
        }
    }
}
