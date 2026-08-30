using System;

namespace RSMods.Util
{
    public class KeybindItem(string displayName, Action<string> setKey, Func<string> getKey)
    {
        public string DisplayName { get; set; } = displayName;
        public Action<string> SetKey { get; set; } = setKey;
        public Func<string> GetKey { get; set; } = getKey;

        public override string ToString() => DisplayName;
    }
}
