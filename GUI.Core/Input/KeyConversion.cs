using System;
using System.Collections.Generic;

namespace RSMods {
    // Portable string <-> virtual-key conversion used by both UI frontends.
    public static class KeyConversion {
        #region Convert VKey To UI
        public static string VKeyToUI(string VKey)
        {
            string ReplacementValue = VKey;
            if (VKey.Contains("VK_"))
                ReplacementValue = ReplacementValue.Substring(3, ReplacementValue.Length - 3);
            return ReplacementValue;
        }
        #endregion
        #region C# Keys -> VKey
        private static readonly Dictionary<string, string> KeyMap = new(StringComparer.OrdinalIgnoreCase);

        static KeyConversion()
        {
            // Helper method to assign multiple aliases to one target Virtual Key
            static void MapAliasesToVK(string targetVk, params string[] aliases)
            {
                foreach (var alias in aliases)
                {
                    if (!KeyMap.ContainsKey(alias))
                    {
                        KeyMap.Add(alias, targetVk);
                    }
                }
            }

            // Mouse
            MapAliasesToVK("VK_LBUTTON", "LButton", "LeftMouseButton");
            MapAliasesToVK("VK_RBUTTON", "RButton", "RightMouseButton");
            MapAliasesToVK("VK_MBUTTON", "MBUTTON", "Middle");
            MapAliasesToVK("VK_XBUTTON1", "XButton1", "MouseXButton1");
            MapAliasesToVK("VK_XBUTTON2", "XButton2", "MouseXButton2");

            // Utility Keys
            MapAliasesToVK("VK_BACK", "Back", "Backspace");
            MapAliasesToVK("VK_CANCEL", "Cancel", "CancelBreak");
            MapAliasesToVK("VK_TAB", "Tab");
            MapAliasesToVK("VK_CLEAR", "Clear");
            MapAliasesToVK("VK_RETURN", "Return", "Enter");
            MapAliasesToVK("VK_LSHIFT", "Shift", "LShift");
            MapAliasesToVK("VK_RSHIFT", "RShift");
            MapAliasesToVK("VK_LCONTROL", "Control", "Ctrl", "LCtrl", "LControl");
            MapAliasesToVK("VK_RCONTROL", "RControl", "RCtrl");
            MapAliasesToVK("VK_LMENU", "Menu", "Alt", "LMenu", "LAlt");
            MapAliasesToVK("VK_RMENU", "RMenu", "RAlt");
            MapAliasesToVK("VK_PAUSE", "Pause");
            MapAliasesToVK("VK_CAPITAL", "Capital", "CapsLock");
            MapAliasesToVK("VK_ESCAPE", "Escape", "Esc");
            MapAliasesToVK("VK_SPACE", "Space", "Spacebar");
            MapAliasesToVK("VK_PRIOR", "Prior", "PageUp");
            MapAliasesToVK("VK_NEXT", "Next", "PageDown");
            MapAliasesToVK("VK_END", "End");
            MapAliasesToVK("VK_HOME", "Home");
            MapAliasesToVK("VK_SELECT", "Select");
            MapAliasesToVK("VK_PRINT", "Print");
            MapAliasesToVK("VK_EXECUTE", "Execute");
            MapAliasesToVK("VK_SNAPSHOT", "Snapshot", "PrintScreen");
            MapAliasesToVK("VK_SCROLL", "Scroll", "ScrollLock");
            MapAliasesToVK("VK_INSERT", "Insert", "Ins");
            MapAliasesToVK("VK_DELETE", "Delete", "Del");
            MapAliasesToVK("VK_HELP", "Help");
            MapAliasesToVK("VK_LWIN", "Win", "Windows", "LWin", "LWindows");
            MapAliasesToVK("VK_RWIN", "RWin", "RWindows");
            MapAliasesToVK("VK_APPS", "Apps", "Applications");
            MapAliasesToVK("VK_SLEEP", "Sleep");
            MapAliasesToVK("VK_ATTN", "Attn");
            MapAliasesToVK("VK_CRSEL", "CrSel");
            MapAliasesToVK("VK_EXSEL", "ExSel");
            MapAliasesToVK("VK_EREOF", "ErEOF");
            MapAliasesToVK("VK_PLAY", "Play");
            MapAliasesToVK("VK_ZOOM", "Zoom");
            MapAliasesToVK("VK_PA1", "PA1");

            // Arrow Keys
            MapAliasesToVK("VK_LEFT", "Left", "LeftArrow");
            MapAliasesToVK("VK_UP", "Up", "UpArrow");
            MapAliasesToVK("VK_DOWN", "Down", "DownArrow");
            MapAliasesToVK("VK_RIGHT", "Right", "RightArrow");

            // IME Keys
            MapAliasesToVK("VK_KANA", "Kana", "IMEKana");
            MapAliasesToVK("VK_HANGUL", "Hanguel", "IMEHanguel", "Hangul", "IMEHangul");
            MapAliasesToVK("VK_IME_ON", "IMEOn");
            MapAliasesToVK("VK_JUNJA", "Junja", "IMEJunja");
            MapAliasesToVK("VK_FINAL", "Final", "IMEFinal");
            MapAliasesToVK("VK_HANJA", "Hanja", "IMEHanja");
            MapAliasesToVK("VK_KANJI", "Kanji", "IMEKanji");
            MapAliasesToVK("VK_IME_OFF", "IMEOff");
            MapAliasesToVK("VK_CONVERT", "Convert", "IMEConvert");
            MapAliasesToVK("VK_NONCONVERT", "NonConvert", "IMENonConvert");
            MapAliasesToVK("VK_ACCEPT", "Accept", "IMEAccept");
            MapAliasesToVK("VK_MODECHANGE", "ModeChange", "IMEModeChange");
            MapAliasesToVK("VK_PROCESSKEY", "Process", "IMEProcess");

            // Number Row (0-9)
            for (int i = 0; i <= 9; i++) MapAliasesToVK(i.ToString(), i.ToString(), $"D{i}");

            // English Alphabet (A-Z)
            for (char c = 'A'; c <= 'Z'; c++) MapAliasesToVK(c.ToString(), c.ToString());

            // Numpad Keys
            MapAliasesToVK("VK_NUMLOCK", "NumLock");
            for (int i = 0; i <= 9; i++) MapAliasesToVK($"VK_NUMPAD{i}", $"Numpad{i}");
            MapAliasesToVK("VK_DECIMAL", "Decimal");
            MapAliasesToVK("VK_ADD", "NumpadAdd", "Numpad+", "Add");
            MapAliasesToVK("VK_MULTIPLY", "NumpadMultiply", "Numpad*", "Multiply");
            MapAliasesToVK("VK_SUBTRACT", "NumpadSubtract", "Numpad-", "Subtract");
            MapAliasesToVK("VK_DIVIDE", "NumpadDivide", "Numpad/", "Divide");

            // F-Keys (F1-F24)
            for (int i = 1; i <= 24; i++) MapAliasesToVK($"VK_F{i}", $"F{i}");

            // Browser Keys
            MapAliasesToVK("VK_BROWSER_BACK", "BrowserBack");
            MapAliasesToVK("VK_BROWSER_FORWARD", "BrowserForward");
            MapAliasesToVK("VK_BROWSER_REFRESH", "BrowserRefresh");
            MapAliasesToVK("VK_BROWSER_STOP", "BrowserStop");
            MapAliasesToVK("VK_BROWSER_SEARCH", "BrowserSearch");
            MapAliasesToVK("VK_BROWSER_FAVORITES", "BrowserFavorites");
            MapAliasesToVK("VK_BROWSER_HOME", "BrowserHome");

            // Volume Keys
            MapAliasesToVK("VK_VOLUME_MUTE", "VolumeMute", "Mute");
            MapAliasesToVK("VK_VOLUME_DOWN", "VolumeDown");
            MapAliasesToVK("VK_VOLUME_UP", "Volume Up");

            // Media Keys
            MapAliasesToVK("VK_MEDIA_NEXT_TRACK", "MediaNextTrack", "NextTrack");
            MapAliasesToVK("VK_MEDIA_PREV_TRACK", "MediaPrevTrack", "MediaPreviousTrack");
            MapAliasesToVK("VK_MEDIA_STOP", "MediaStop", "Stop");
            MapAliasesToVK("VK_MEDIA_PLAY_PAUSE", "MediaPlayPause", "PlayPause");

            // Launch Application Keys
            MapAliasesToVK("VK_LAUNCH_MAIL", "LaunchMail", "Mail");
            MapAliasesToVK("VK_LAUNCH_MEDIA_SELECT", "LaunchMediaSelect", "MediaSelect");
            MapAliasesToVK("VK_LAUNCH_APP1", "LaunchApp1", "App1");
            MapAliasesToVK("VK_LAUNCH_APP2", "LaunchApp2", "App2");

            // OEM (Symbol) Keys
            MapAliasesToVK("VK_OEM_1", "ColonSemiColon", "SemiColonColon", "Oem1", "OemSemicolon");
            MapAliasesToVK("VK_OEM_PLUS", "OEMPlus", "NumRowPlus");
            MapAliasesToVK("VK_OEM_COMMA", "Comma", "Oemcomma");
            MapAliasesToVK("VK_OEM_MINUS", "OEMMinus", "NumRowMinus");
            MapAliasesToVK("VK_OEM_PERIOD", "Period", "OemPeriod");
            MapAliasesToVK("VK_OEM_2", "SlashQuestionMark", "QuestionMarkSlash", "OemQuestion", "/?");
            MapAliasesToVK("VK_OEM_3", "Tilda", "BackTick", "Oemtilde");
            MapAliasesToVK("VK_OEM_4", "OemOpenBrackets", "[", "Oem4");
            MapAliasesToVK("VK_OEM_5", "BackSlashPipe", "PipeBackSlash", "Oem5", "|\\");
            MapAliasesToVK("VK_OEM_6", "CloseBrackets", "]", "Oem6", "OemCloseBrackets");
            MapAliasesToVK("VK_OEM_7", "Quotes", "Oem7", "OemQuotes", "\"'");
            MapAliasesToVK("VK_OEM_CLEAR", "OEMClear");

            // X-Input Gamepad
            MapAliasesToVK("VK_PAD_A", "XInputGamepadA");
            MapAliasesToVK("VK_PAD_B", "XInputGamepadB");
            MapAliasesToVK("VK_PAD_X", "XInputGamepadX");
            MapAliasesToVK("VK_PAD_Y", "XInputGamepadY");
            MapAliasesToVK("VK_PAD_START", "XInputGamepadStart");
            MapAliasesToVK("VK_PAD_BACK", "XInputGamepadBack");
            MapAliasesToVK("VK_PAD_LSHOULDER", "XInputGamepadLeftShoulder");
            MapAliasesToVK("VK_PAD_RSHOULDER", "XInputGamepadRightShoulder");
            MapAliasesToVK("VK_PAD_LTRIGGER", "XInputGamepadLeftTrigger");
            MapAliasesToVK("VK_PAD_RTRIGGER", "XInputGamepadRightTrigger");

            MapAliasesToVK("VK_PAD_DPAD_UP", "XInputGamepadDPadUp");
            MapAliasesToVK("VK_PAD_DPAD_DOWN", "XInputGamepadDPadDown");
            MapAliasesToVK("VK_PAD_DPAD_LEFT", "XInputGamepadDPadLeft");
            MapAliasesToVK("VK_PAD_DPAD_RIGHT", "XInputGamepadDPadRight");

            MapAliasesToVK("VK_PAD_LTHUMB_PRESS", "XInputGamepadLeftThumbPress");
            MapAliasesToVK("VK_PAD_LTHUMB_UP", "XInputGamepadLeftThumbUp");
            MapAliasesToVK("VK_PAD_LTHUMB_DOWN", "XInputGamepadLeftThumbDown");
            MapAliasesToVK("VK_PAD_LTHUMB_RIGHT", "XInputGamepadLeftThumbLeft");   // Left / Right swapped
            MapAliasesToVK("VK_PAD_LTHUMB_LEFT", "XInputGamepadLeftThumbRight");
            MapAliasesToVK("VK_PAD_LTHUMB_UPLEFT", "XInputGamepadLeftThumbUpLeft");
            MapAliasesToVK("VK_PAD_LTHUMB_UPRIGHT", "XInputGamepadLeftThumbUpRight");
            MapAliasesToVK("VK_PAD_LTHUMB_DOWNLEFT", "XInputGamepadLeftThumbDownLeft");
            MapAliasesToVK("VK_PAD_LTHUMB_DOWNRIGHT", "XInputGamepadLeftThumbDownRight");

            MapAliasesToVK("VK_PAD_RTHUMB_PRESS", "XInputGamepadRightThumbPress");
            MapAliasesToVK("VK_PAD_RTHUMB_UP", "XInputGamepadRightThumbUp");
            MapAliasesToVK("VK_PAD_RTHUMB_DOWN", "XInputGamepadRightThumbDown");
            MapAliasesToVK("VK_PAD_RTHUMB_LEFT", "XInputGamepadRightThumbLeft");
            MapAliasesToVK("VK_PAD_RTHUMB_RIGHT", "XInputGamepadRightThumbRight");
            MapAliasesToVK("VK_PAD_RTHUMB_UPLEFT", "XInputGamepadRightThumbUpLeft");
            MapAliasesToVK("VK_PAD_RTHUMB_UPRIGHT", "XInputGamepadRightThumbUpRight");
            MapAliasesToVK("VK_PAD_RTHUMB_DOWNRIGHT", "XInputGamepadRightThumbDownLeft"); // Left / Right swapped
            MapAliasesToVK("VK_PAD_RTHUMB_DOWNLEFT", "XInputGamepadRightThumbDownRight");
        }

        public static string VirtualKey(string key)
        {
            if (string.IsNullOrWhiteSpace(key))
                return "";

            if (key.StartsWith("VK_", StringComparison.OrdinalIgnoreCase))
                return key;

            return KeyMap.TryGetValue(key, out string targetVk) ? targetVk : "";
        }
        #endregion
    }
}
