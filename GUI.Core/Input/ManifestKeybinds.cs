#nullable enable
using System.Collections.Generic;
using System.Linq;
using RSMods.Core.Settings;

namespace RSMods.Util
{
    /// <summary>Keybinding rows for the key binds the mods declare in the manifest.</summary>
    public static class ManifestKeybinds
    {
        public const string AudioCategory = "Audio Keybindings";

        public static IReadOnlyList<KeybindItem> Mod(IManifestService manifest, IniManager ini) => Build(manifest, ini, audio: false);

        public static IReadOnlyList<KeybindItem> Audio(IManifestService manifest, IniManager ini) => Build(manifest, ini, audio: true);

        private static List<KeybindItem> Build(IManifestService manifest, IniManager ini, bool audio) =>
            manifest.AllSettings
                .Where(d => d.Type == SettingType.Key && (d.Category == AudioCategory) == audio)
                .Select(d => Create(d, ini))
                .ToList();

        private static KeybindItem Create(SettingDescriptor descriptor, IniManager ini)
        {
            string section = SettingFieldViewModel.NormalizeSection(descriptor.Ini.Section);
            string name = descriptor.Ini.Name;

            // A missing entry shows the DLL's default rather than "unbound": that is the key the game uses.
            return new KeybindItem(
                descriptor.Label,
                key => ini.SetString(section, name, key),
                () =>
                {
                    string stored = ini.GetString(section, name, descriptor.Default);
                    string virtualKey = KeyConversion.VirtualKey(stored);
                    return string.IsNullOrEmpty(virtualKey) ? stored : virtualKey;
                });
        }
    }
}
