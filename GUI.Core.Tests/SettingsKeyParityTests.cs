using System.Reflection;
using System.Text.RegularExpressions;
using RSMods;

namespace RSMods.Core.Tests;

/// <summary>
/// Guards the configurator/DLL contract: every setting the GUI writes to <c>RSMods.ini</c> must be a
/// setting the mod DLL actually reads, in the same section.
/// <para>
/// The two sides are organised on different axes - the DLL groups settings by owning mod, the GUI groups
/// them by INI section - so nothing but this test ties a key to its consumer. A key written to the wrong
/// section, or to no consumer at all, is silently inert at runtime: the GUI shows the user a value the
/// game never reads. That is exactly how the misplaced <c>OnScreenFontSize</c> shipped.
/// </para>
/// <para>
/// Scope: declarative keys only. Keys either side builds at runtime - the DLL's <c>string{n}_N</c> /
/// <c>note{n}_CB</c> colours and <c>SongListTitle_{i}</c> - are invisible to both halves of this test
/// (the GUI exposes them as methods rather than properties), so they drop out symmetrically rather than
/// producing false failures.
/// </para>
/// </summary>
public sealed class SettingsKeyParityTests
{
    private const string GuiOnlySection = "GUI Settings";

    /// <summary>
    /// Keys the DLL reads that no GUI has ever exposed. Each is dead on the DLL side too; this list is
    /// the debt, so it should only ever shrink. Removing an entry means either wiring the setting up in
    /// the GUI or deleting the DLL's read.
    /// </summary>
    private static readonly Dictionary<(string Section, string Key), string> KnownUnexposed = new()
    {
        [("Keybinds", "MenuToggleKey")] = "Read into the keybind map, but no framework command binds it.",
        [("Keybinds", "CustomSongListTitles")] = "Read into the keybind map, but no framework command binds it.",
        [("Toggle Switches", "DiscoMode")] = "Its D3DHooks implementation is entirely commented out.",
    };

    /// <summary>Maps each nested store class onto the INI section its <c>IniSection</c> writes to.</summary>
    private static readonly Dictionary<string, string> SectionForStoreClass = new()
    {
        ["Keybinds"] = "Keybinds",
        ["AudioKeybindings"] = "Audio Keybindings",
        ["Toggles"] = "Toggle Switches",
        ["ModSettings"] = "Mod Settings",
        ["GuitarSpeak"] = "Guitar Speak",
        ["HighwayColors"] = "Highway Colors",
        ["StringColors"] = "String Colors",
        ["GUISettings"] = GuiOnlySection,
    };

    [Fact]
    public void EverySettingTheGuiWritesIsReadByTheDll()
    {
        var dllKeys = ReadDllKeys();
        var guiKeys = ReadGuiKeys();

        var orphaned = guiKeys
            .Where(entry => entry.Section != GuiOnlySection && !dllKeys.Contains(entry))
            .OrderBy(entry => entry.Section).ThenBy(entry => entry.Key)
            .ToList();

        Assert.True(orphaned.Count == 0,
            "The GUI writes settings the DLL never reads, so they are inert at runtime:" +
            Environment.NewLine +
            string.Join(Environment.NewLine, orphaned.Select(entry => $"  [{entry.Section}] {entry.Key}")) +
            Environment.NewLine +
            "Check the section first - the same key under the wrong heading is the usual cause.");
    }

    [Fact]
    public void EverySettingTheDllReadsIsWrittenByTheGui()
    {
        var dllKeys = ReadDllKeys();
        var guiKeys = ReadGuiKeys();

        var unexposed = dllKeys
            .Where(entry => !guiKeys.Contains(entry) && !KnownUnexposed.ContainsKey(entry))
            .OrderBy(entry => entry.Section).ThenBy(entry => entry.Key)
            .ToList();

        Assert.True(unexposed.Count == 0,
            "The DLL reads settings no GUI screen writes, so users cannot reach them:" +
            Environment.NewLine +
            string.Join(Environment.NewLine, unexposed.Select(entry => $"  [{entry.Section}] {entry.Key}")) +
            Environment.NewLine +
            $"Expose them, or add them to {nameof(KnownUnexposed)} with the reason they are dead.");
    }

    [Fact]
    public void KnownUnexposedEntriesAreStillReadByTheDll()
    {
        // Stops the allowlist outliving the reads it excuses, which would quietly weaken the test above.
        var dllKeys = ReadDllKeys();

        var stale = KnownUnexposed.Keys.Where(entry => !dllKeys.Contains(entry)).ToList();

        Assert.True(stale.Count == 0,
            "The DLL no longer reads these allowlisted keys, so they should be deleted from " +
            $"{nameof(KnownUnexposed)}:" + Environment.NewLine +
            string.Join(Environment.NewLine, stale.Select(entry => $"  [{entry.Section}] {entry.Key}")));
    }

    [Fact]
    public void EveryStoreClassIsMappedToASection()
    {
        // Without this, adding a nested store class would silently opt its keys out of both checks.
        var unmapped = typeof(RsModsSettings)
            .GetNestedTypes(BindingFlags.Public | BindingFlags.Static)
            .Select(type => type.Name)
            .Where(name => !SectionForStoreClass.ContainsKey(name))
            .ToList();

        Assert.True(unmapped.Count == 0,
            $"New RsModsSettings store classes are not mapped in {nameof(SectionForStoreClass)}: " +
            string.Join(", ", unmapped));
    }

    /// <summary>
    /// Every <c>(section, key)</c> the GUI persists, taken from the store's own shape: each nested class
    /// is one INI section and, because the accessors use <c>[CallerMemberName]</c>, each static property
    /// name is literally its INI key. This is the same traversal <c>SeedDefaultsAndSave</c> performs.
    /// </summary>
    private static HashSet<(string Section, string Key)> ReadGuiKeys()
    {
        var keys = new HashSet<(string, string)>();

        foreach (Type storeClass in typeof(RsModsSettings).GetNestedTypes(BindingFlags.Public | BindingFlags.Static))
        {
            if (!SectionForStoreClass.TryGetValue(storeClass.Name, out string? section))
                continue; // Reported by EveryStoreClassIsMappedToASection.

            foreach (PropertyInfo property in storeClass.GetProperties(BindingFlags.Public | BindingFlags.Static))
                keys.Add((section, property.Name));
        }

        return keys;
    }

    /// <summary>
    /// Every <c>(section, key)</c> the DLL reads, scraped from its settings sources. Two forms appear
    /// there: a literal INI name, and a <c>Setting::Key::Name</c> constant that has to be resolved
    /// through the <c>Key</c> namespace in the header.
    /// </summary>
    private static HashSet<(string Section, string Key)> ReadDllKeys()
    {
        string repositoryRoot = FindRepositoryRoot();
        string source = File.ReadAllText(Path.Combine(repositoryRoot, "DLL", "Settings.cpp"));
        string header = File.ReadAllText(Path.Combine(repositoryRoot, "DLL", "Settings.hpp"));

        Dictionary<string, string> keyConstants = ReadKeyNamespaceConstants(header);
        var keys = new HashSet<(string, string)>();

        // reader.GetValue("Section", "Key"  /  GetLongValue / GetBoolValue / GetDoubleValue
        foreach (Match match in Regex.Matches(source,
            """reader\.Get(?:Long|Bool|Double)?Value\(\s*"(?<section>[^"]+)"\s*,\s*"(?<key>[^"]+)"\s*"""))
        {
            keys.Add((match.Groups["section"].Value, match.Groups["key"].Value));
        }

        // reader.GetValue("Section", Setting::Key::Name
        foreach (Match match in Regex.Matches(source,
            """reader\.Get(?:Long|Bool|Double)?Value\(\s*"(?<section>[^"]+)"\s*,\s*Setting::Key::(?<constant>\w+)"""))
        {
            if (keyConstants.TryGetValue(match.Groups["constant"].Value, out string? key))
                keys.Add((match.Groups["section"].Value, key));
        }

        // Declarative mod settings in mods.manifest.json
        string manifestPath = Path.Combine(repositoryRoot, "mods.manifest.json");
        if (File.Exists(manifestPath))
        {
            using var doc = System.Text.Json.JsonDocument.Parse(File.ReadAllText(manifestPath));
            foreach (System.Text.Json.JsonElement element in doc.RootElement.EnumerateArray())
            {
                // Tier 3 pure custom editor launchers (not INI values)
                if (element.TryGetProperty("editor", out var editor) &&
                    editor.ValueKind == System.Text.Json.JsonValueKind.String &&
                    !string.IsNullOrEmpty(editor.GetString()) &&
                    element.TryGetProperty("type", out var type) &&
                    type.GetString() == "String")
                {
                    continue;
                }

                if (element.TryGetProperty("ini", out System.Text.Json.JsonElement ini))
                {
                    string? section = ini.GetProperty("section").GetString();
                    string? name = ini.GetProperty("name").GetString();
                    if (!string.IsNullOrEmpty(section) && !string.IsNullOrEmpty(name))
                        keys.Add((section, name));
                }
            }
        }

        Assert.True(keys.Count > 50,
            $"Only found {keys.Count} DLL setting reads, so the scrape is probably broken rather than " +
            "the contract. Check whether DLL/Settings.cpp changed how it reads the INI.");

        return keys;
    }

    /// <summary>Resolves the <c>Setting::Key</c> constants to the INI names they hold.</summary>
    private static Dictionary<string, string> ReadKeyNamespaceConstants(string header)
    {
        var constants = new Dictionary<string, string>();

        Match keyNamespace = Regex.Match(header, @"namespace\s+Key\s*\{(?<body>.*?)\n\t*\}", RegexOptions.Singleline);
        Assert.True(keyNamespace.Success, "Could not find the Setting::Key namespace in DLL/Settings.hpp.");

        foreach (Match match in Regex.Matches(keyNamespace.Groups["body"].Value,
            "inline\\s+constexpr\\s+char\\s+(?<constant>\\w+)\\[\\]\\s*=\\s*\"(?<value>[^\"]+)\""))
        {
            constants[match.Groups["constant"].Value] = match.Groups["value"].Value;
        }

        return constants;
    }

    [Fact]
    public void CommittedManifestMatchesDllExportWhenDllIsPresent()
    {
        string root = FindRepositoryRoot();
        string committedManifestPath = Path.Combine(root, "mods.manifest.json");
        Assert.True(File.Exists(committedManifestPath), "mods.manifest.json must exist at repository root.");

        string[] candidateDllPaths =
        [
            Path.Combine(root, "DLL", "Installer", "Resources", "xinput1_3.dll"),
            Path.Combine(root, "Installer", "Resources", "xinput1_3.dll"),
            Path.Combine(root, "DLL", "Release", "xinput1_3.dll")
        ];

        string? dllPath = candidateDllPaths
            .Where(File.Exists)
            .OrderByDescending(File.GetLastWriteTimeUtc)
            .FirstOrDefault();
        if (dllPath is null)
        {
            // Native DLL not built in this test run environment; skip dump assertion
            return;
        }

        string tempManifestPath = Path.Combine(Path.GetTempPath(), $"mods.manifest.test.{Guid.NewGuid():N}.json");
        try
        {
            string systemX86 = Environment.GetFolderPath(Environment.SpecialFolder.SystemX86);
            string rundll32 = Path.Combine(string.IsNullOrEmpty(systemX86) ? "C:\\Windows\\System32" : systemX86, "rundll32.exe");

            var psi = new System.Diagnostics.ProcessStartInfo
            {
                FileName = rundll32,
                Arguments = $"\"{dllPath}\",DumpManifest \"{tempManifestPath}\"",
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var proc = System.Diagnostics.Process.Start(psi);
            Assert.NotNull(proc);
            bool finished = proc.WaitForExit(10000);
            Assert.True(finished, "rundll32 DumpManifest timed out.");
            Assert.True(File.Exists(tempManifestPath), "rundll32 DumpManifest failed to produce output file.");

            string committedJson = File.ReadAllText(committedManifestPath).Replace("\r\n", "\n").Trim();
            string dumpedJson = File.ReadAllText(tempManifestPath).Replace("\r\n", "\n").Trim();

            Assert.True(committedJson == dumpedJson,
                "mods.manifest.json is out of date with the DLL declarations! " +
                "Rebuild the DLL and run 'pwsh DLL/Framework/Tests/BuildAndRun.ps1 -DumpManifest' to sync the committed manifest.");
        }
        finally
        {
            if (File.Exists(tempManifestPath))
                File.Delete(tempManifestPath);
        }
    }

    private static string FindRepositoryRoot()
    {
        var directory = new DirectoryInfo(AppContext.BaseDirectory);

        while (directory is not null && !File.Exists(Path.Combine(directory.FullName, "RSMods.sln")))
            directory = directory.Parent;

        Assert.True(directory is not null,
            $"Could not find RSMods.sln walking up from {AppContext.BaseDirectory}; this test needs the " +
            "DLL sources to compare against.");

        return directory!.FullName;
    }
}
