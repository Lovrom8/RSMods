#nullable enable
using System;
using System.Collections.Generic;
using System.Linq;

namespace RSMods.Core.Settings;

/// <summary>
/// Turns the manifest's flat setting list into the Mod Settings list. The DLL decides the grouping
/// (SettingsSchemaRegistry::Entries) and writes it into the manifest, so the in-game settings window shows the same
/// entries: each setting names the <c>entry</c> it's listed under, and the setting that starts an entry carries its
/// <c>heading</c> and <c>entryTitle</c>. Entries are grouped under headings by what they change, so a player can find
/// one without knowing its name.
/// </summary>
public static class ModCatalog
{
    // Buttons that only said where to go; their screens have their own tabs.
    private static readonly HashSet<string> Hidden = new(StringComparer.OrdinalIgnoreCase)
    {
        "TwitchSettings",
        "MidiCustomEditor",
        "GuitarSpeakCustomEditor",
    };

    /// <summary>
    /// The list's headings, in the order they're shown; keep in step with SettingHeading in the DLL's
    /// SettingsSchema.hpp. An entry with no heading goes under the last.
    /// </summary>
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

    private static readonly Dictionary<string, string> Descriptions = new(StringComparer.OrdinalIgnoreCase)
    {
        ["StringColorsCustomEditor"] = "String, note and highway colors have their own page.",
    };

    public static IReadOnlyList<ModEntryViewModel> Build(
        IEnumerable<SettingDescriptor> manifestOrder,
        IReadOnlyDictionary<string, SettingFieldViewModel> fieldsByKey)
    {
        var settings = manifestOrder.ToList();
        var entryOf = new Dictionary<string, string?>(StringComparer.OrdinalIgnoreCase);
        foreach (var desc in settings)
            entryOf.TryAdd(desc.Key, EntryKeyOf(desc));

        var entries = new List<Builder>();
        var byRoot = new Dictionary<string, Builder>(StringComparer.OrdinalIgnoreCase);

        foreach (var desc in settings)
        {
            // Key binds have no field; values a bespoke editor owns have none either.
            if (Hidden.Contains(desc.Key) || desc.Type == SettingType.Key || !fieldsByKey.TryGetValue(desc.Key, out var field))
                continue;

            string? entryKey = entryOf[desc.Key];
            if (string.Equals(entryKey, desc.Key, StringComparison.OrdinalIgnoreCase))
            {
                field.IsNested = false;
                var created = new Builder(desc, field);
                entries.Add(created);
                byRoot[desc.Key] = created;
            }
            else if (entryKey is not null && byRoot.TryGetValue(entryKey, out var entry))
            {
                // Indented only when it hangs off another sub-setting rather than the entry's own.
                field.IsNested = desc.VisibleWhen is { } cond && !string.Equals(cond.Key, entryKey, StringComparison.OrdinalIgnoreCase)
                                 && entryOf.TryGetValue(cond.Key, out string? condEntry)
                                 && string.Equals(condEntry, entryKey, StringComparison.OrdinalIgnoreCase);
                entry.Fields.Add(field);
            }
        }

        foreach (var desc in settings.Where(d => d.Type == SettingType.Key))
        {
            if (desc.Entry is { } key && byRoot.TryGetValue(key, out var entry))
                entry.Keybinds.Add(desc);
        }

        return entries
            .Select(b => b.Build())
            .OrderBy(e => IndexOfCategory(e.Category))
            .ThenBy(e => e.Title, StringComparer.CurrentCultureIgnoreCase)
            .ToList();
    }

    // A hand-written manifest (as in tests) may leave "entry" out; each setting then stands alone.
    private static string? EntryKeyOf(SettingDescriptor desc) => desc.Entry ?? (desc.Type == SettingType.Key ? null : desc.Key);

    private static int IndexOfCategory(string category)
    {
        for (int i = 0; i < Categories.Count; i++)
        {
            if (Categories[i] == category)
                return i;
        }
        return Categories.Count;
    }

    private sealed class Builder(SettingDescriptor rootDescriptor, SettingFieldViewModel root)
    {
        public SettingFieldViewModel Root { get; } = root;
        public List<SettingFieldViewModel> Fields { get; } = [];
        public List<SettingDescriptor> Keybinds { get; } = [];

        public ModEntryViewModel Build()
        {
            string key = Root.Key;
            string title = rootDescriptor.EntryTitle ?? Root.Label;
            bool isToggle = Root is BoolSettingFieldViewModel;

            // A toggle root is the entry's switch and its hint describes the mod; any other root is the entry's
            // first setting and shows its hint itself.
            string? description = Descriptions.TryGetValue(key, out string? d) ? d : isToggle ? Root.Hint : null;
            IReadOnlyList<SettingFieldViewModel> fields = isToggle ? Fields : [Root, .. Fields];

            string category = string.IsNullOrEmpty(rootDescriptor.Heading) ? Categories[^1] : rootDescriptor.Heading;

            return new ModEntryViewModel(key, title, description, category, Root, fields, Keybinds);
        }
    }
}
