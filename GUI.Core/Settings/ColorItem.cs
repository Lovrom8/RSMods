using System;

namespace RSMods.Util
{
    /// <summary>UI-neutral metadata and accessors for a configurable color value.</summary>
    public class ColorItem(string displayName, Action<string> setColor, Func<string> getColor)
    {
        public string DisplayName { get; set; } = displayName;
        public Action<string> SetColor { get; set; } = setColor;
        public Func<string> GetColor { get; set; } = getColor;

        public override string ToString() => DisplayName;
    }
}
