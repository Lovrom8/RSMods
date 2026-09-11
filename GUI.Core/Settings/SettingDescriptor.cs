#nullable enable
using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace RSMods.Core.Settings;

public enum SettingType
{
    Bool,
    Int,
    Enum,
    String
}

public sealed record IniLocation(
    [property: JsonPropertyName("section")] string Section,
    [property: JsonPropertyName("name")] string Name
);

public sealed record SettingVisibilityCondition(
    [property: JsonPropertyName("key")] string Key,
    [property: JsonPropertyName("equals")] string ExpectedValue
);

public sealed record SettingDescriptor(
    [property: JsonPropertyName("key")] string Key,
    [property: JsonPropertyName("ini")] IniLocation Ini,
    [property: JsonPropertyName("type")] SettingType Type,
    [property: JsonPropertyName("default")] string Default,
    [property: JsonPropertyName("label")] string Label,
    [property: JsonPropertyName("hint")] string? Hint,
    [property: JsonPropertyName("category")] string Category,
    [property: JsonPropertyName("min")] int? Min,
    [property: JsonPropertyName("max")] int? Max,
    [property: JsonPropertyName("scale")] double? Scale,
    [property: JsonPropertyName("choices")] IReadOnlyList<string>? Choices,
    [property: JsonPropertyName("choicesSource")] string? ChoicesSource,
    [property: JsonPropertyName("visibleWhen")] SettingVisibilityCondition? VisibleWhen,
    [property: JsonPropertyName("editor")] string? Editor
);
