#nullable enable
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
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
            _descriptors = LoadDescriptors();
        }

        _byKey = _descriptors
            .GroupBy(d => d.Key, StringComparer.OrdinalIgnoreCase)
            .ToDictionary(g => g.Key, g => g.First(), StringComparer.OrdinalIgnoreCase);
    }

    public SettingDescriptor? Find(string key) =>
        _byKey.TryGetValue(key, out var desc) ? desc : null;

    public IEnumerable<IGrouping<string, SettingDescriptor>> GetCategories() =>
        _descriptors.GroupBy(d => string.IsNullOrWhiteSpace(d.Category) ? "General" : d.Category);

    private static List<SettingDescriptor> LoadDescriptors()
    {
        // 1. Try loose file next to application
        string appDir = AppDomain.CurrentDomain.BaseDirectory;
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
            catch
            {
                // Fall through to embedded resource
            }
        }

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
