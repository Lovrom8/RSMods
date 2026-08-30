using System;
using System.Collections.Generic;

namespace RSMods
{
    /// <summary>
    /// Describes how a keyboard or mouse input should be captured for a Rocksmith keybinding.
    /// </summary>
    public enum RocksmithInputClassification
    {
        Unsupported,
        KeyDown,
        KeyUp,
        MouseButton,
        Reserved
    }

    /// <summary>
    /// Shared Rocksmith keybinding policy. Callers pass the framework key name (for example
    /// WinForms Keys.F1.ToString() or Avalonia Key.F1.ToString()); KeyConversion normalizes it
    /// to the virtual-key representation stored in the RSMods configuration.
    /// </summary>
    public static class RocksmithKeys
    {
        // Rocksmith accepts these keys on the normal key-down event.
        private static readonly HashSet<string> KeyDownVirtualKeys = new(StringComparer.OrdinalIgnoreCase)
        {
            // Standard and extended function keys (F8, F11 and F12 are reserved by Rocksmith).
            "VK_F1", "VK_F2", "VK_F3", "VK_F4", "VK_F5", "VK_F6", "VK_F7", "VK_F9", "VK_F10",
            "VK_F13", "VK_F14", "VK_F15", "VK_F16", "VK_F17", "VK_F18", "VK_F19", "VK_F20",
            "VK_F21", "VK_F22", "VK_F23", "VK_F24",

            "VK_PAUSE", "VK_SCROLL", "VK_INSERT",
            "VK_OEM_1", "VK_OEM_3", "VK_OEM_COMMA", "VK_OEM_MINUS", "VK_OEM_PERIOD",
            "VK_OEM_7", "VK_OEM_2", "VK_OEM_4", "VK_OEM_6",
            "VK_CAPITAL", "VK_APPS",
            "VK_SUBTRACT", "VK_NUMLOCK", "VK_MULTIPLY", "VK_DIVIDE", "VK_DECIMAL"
        };

        // WinForms and some keyboard drivers only surface these reliably on key-up.
        private static readonly HashSet<string> KeyUpVirtualKeys = new(StringComparer.OrdinalIgnoreCase)
        {
            "VK_SNAPSHOT", "VK_PLAY", "VK_MEDIA_NEXT_TRACK", "VK_MEDIA_PLAY_PAUSE",
            "VK_MEDIA_PREV_TRACK", "VK_MEDIA_STOP"
        };

        private static readonly HashSet<string> MouseVirtualKeys = new(StringComparer.OrdinalIgnoreCase)
        {
            "VK_MBUTTON", "VK_XBUTTON1", "VK_XBUTTON2"
        };

        /// <summary>
        /// Converts a WinForms or Avalonia input name to the canonical value stored in the INI.
        /// Returns an empty string when the input is unknown.
        /// </summary>
        public static string Normalize(string inputName)
        {
            return KeyConversion.VirtualKey(inputName);
        }

        /// <summary>
        /// Classifies a framework input name without taking a dependency on that framework's key enum.
        /// </summary>
        public static RocksmithInputClassification Classify(string inputName)
        {
            string virtualKey = Normalize(inputName);

            if (KeyDownVirtualKeys.Contains(virtualKey))
                return RocksmithInputClassification.KeyDown;

            if (KeyUpVirtualKeys.Contains(virtualKey))
                return RocksmithInputClassification.KeyUp;

            if (MouseVirtualKeys.Contains(virtualKey))
                return RocksmithInputClassification.MouseButton;

            if (IsReservedVirtualKey(virtualKey))
                return RocksmithInputClassification.Reserved;

            return RocksmithInputClassification.Unsupported;
        }

        public static bool IsRocksmithReservedKey(string inputName)
        {
            return IsReservedVirtualKey(Normalize(inputName));
        }

        private static bool IsReservedVirtualKey(string virtualKey)
        {
            if (virtualKey.Length != 1)
                return false;

            char key = virtualKey[0];
            return (key >= '0' && key <= '9') ||
                   (key >= 'A' && key <= 'Z') ||
                   (key >= 'a' && key <= 'z');
        }
    }
}
