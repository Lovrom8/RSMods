#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;

namespace RSMods.Core.Settings;

/// <summary>
/// Turns the manifest's flat setting list into one entry per mod for the Mod Settings list. A setting that
/// starts a mod is one with no <c>visibleWhen</c>; the settings gated on it (at any depth) go under it, and each
/// key bind goes to the mod declared just before it, which is how the DLL emits a mod's settings.
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
            .OrderBy(e => e.Title, StringComparer.CurrentCultureIgnoreCase)
            .ToList();
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

            return new ModEntryViewModel(key, title, description, Root, fields, Keybinds);
        }
    }
}
