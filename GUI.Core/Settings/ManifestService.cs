#nullable enable
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace RSMods.Core.Settings;

public interface IManifestService
{
    IReadOnlyList<SettingDescriptor> AllSettings { get; }
    SettingDescriptor? Find(string key);
    IEnumerable<IGrouping<string, SettingDescriptor>> GetCategories();
}

public sealed class ManifestService : IManifestService
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNameCaseInsensitive = true,
        Converters = { new JsonStringEnumConverter() }
    };

    private readonly List<SettingDescriptor> _descriptors;
    private readonly Dictionary<string, SettingDescriptor> _byKey;

    public IReadOnlyList<SettingDescriptor> AllSettings => _descriptors;

    public ManifestService(string? customJson = null)
    {
        if (customJson != null)
        {
            _descriptors = JsonSerializer.Deserialize<List<SettingDescriptor>>(customJson, JsonOptions) ?? [];
        }
        else
        {
            _descriptors = LoadDescriptors(AppDomain.CurrentDomain.BaseDirectory, embeddedFitsThisBuild: OperatingSystem.IsWindows());
        }

        _byKey = _descriptors
            .GroupBy(d => d.Key, StringComparer.OrdinalIgnoreCase)
            .ToDictionary(g => g.Key, g => g.First(), StringComparer.OrdinalIgnoreCase);
    }

    public SettingDescriptor? Find(string key) => _byKey.TryGetValue(key, out var desc) ? desc : null;

    public IEnumerable<IGrouping<string, SettingDescriptor>> GetCategories() => _descriptors.GroupBy(d => string.IsNullOrWhiteSpace(d.Category) ? "General" : d.Category);

    // The embedded manifest is the Windows DLL's. Off Windows it would show another platform's mods without saying so, so
    // a build has to ship its own, and one that's missing or unreadable stops the app rather than falling back.
    internal static List<SettingDescriptor> LoadDescriptors(string appDir, bool embeddedFitsThisBuild)
    {
        // 1. Try loose file next to application
        string loosePath = Path.Combine(appDir, "mods.manifest.json");
        if (File.Exists(loosePath))
        {
            try
            {
                using var stream = File.OpenRead(loosePath);
                var list = JsonSerializer.Deserialize<List<SettingDescriptor>>(stream, JsonOptions);
                if (list != null && list.Count > 0)
                    return list;
            }
            catch (Exception ex)
            {
                if (!embeddedFitsThisBuild)
                    throw new InvalidOperationException($"Can't read {loosePath}: {ex.Message}", ex);

                // Fall through to embedded resource
            }
        }

        if (!embeddedFitsThisBuild)
            throw new InvalidOperationException($"This build's mod list is missing or empty: {loosePath}");

        // 2. Try embedded resource
        var assembly = typeof(ManifestService).Assembly;
        const string resourceName = "RSMods.Core.Resources.mods.manifest.json";
        using (var resourceStream = assembly.GetManifestResourceStream(resourceName))
        {
            if (resourceStream != null)
            {
                var list = JsonSerializer.Deserialize<List<SettingDescriptor>>(resourceStream, JsonOptions);
                if (list != null && list.Count > 0)
                    return list;
            }
        }

        return [];
    }
}
