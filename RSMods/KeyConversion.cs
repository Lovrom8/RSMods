using System;
using static System.StringComparison;
using System.Windows.Forms;

namespace RSMods
{
    class KeyConversion
    {
        public static string VirtualKey(string Key)
        {
            if (Key.StartsWith("VK_"))
            {
                return Key;
            }

            /* Sorry this code is a long and junky conversion from Console.ReadKey().Key to VKey for the DLL || Vkey to Int conversion here: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
              Originally written by: Ffio
                
            if (String.Equals(Key, "VKeyName", CurrentCultureIgnoreCase) ^ String.Equals(Key, "CommonName", CurrentCultureIgnoreCase)) { || If the key set is equal to the VKey Name or the Common Name
                return #; || Send the VKey name back to whoever sent the function
            }

            Template:
            if (String.Equals(Key, "", CurrentCultureIgnoreCase) ^ String.Equals(Key, "", CurrentCultureIgnoreCase)) // 
            {
                return "";
            }
            
            */

        // Mouse
            if (String.Equals(Key, "LButton", CurrentCultureIgnoreCase) ^ String.Equals(Key, "LeftMouseButton", CurrentCultureIgnoreCase)) // Left Mouse Button
            {
                return "VK_LBUTTON";
            }
            if (String.Equals(Key, "RButton", CurrentCultureIgnoreCase) ^ String.Equals(Key, "RightMouseButton", CurrentCultureIgnoreCase)) // Right Mouse Button
            { 
                return "VK_RBUTTON";
            }
            if (String.Equals(Key, "MBUTTON", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Middle", CurrentCultureIgnoreCase)) // Middle Mouse Button
            { 
                return "VK_MBUTTON";
            }
            if (String.Equals(Key, "XButton1", CurrentCultureIgnoreCase) ^ String.Equals(Key, "MouseXButton1", CurrentCultureIgnoreCase)) // X1 Mouse Button
            {
                return "VK_XBUTTON1";
            }
            if (String.Equals(Key, "XButton2", CurrentCultureIgnoreCase) ^ String.Equals(Key, "MouseXButton2", CurrentCultureIgnoreCase)) // X2 Mouse Button
            {
                return "VK_XBUTTON2";
            }
        // Utility Keys
            if (String.Equals(Key, "Back", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Backspace", CurrentCultureIgnoreCase)) // Backspace
            {
                return "VK_BACK";
            }
            if (String.Equals(Key, "Cancel", CurrentCultureIgnoreCase) ^ String.Equals(Key, "CancelBreak", CurrentCultureIgnoreCase)) // Control + Break
            {
                return "VKEY_CANCEL";
            }
            if (String.Equals(Key, "Tab", CurrentCultureIgnoreCase)) // Tab Key
            {
                return "VK_TAB";
            }
            if (String.Equals(Key, "Clear", CurrentCultureIgnoreCase)) // Clear Key
            {
                return "VK_CLEAR";
            }
            if (String.Equals(Key, "Return", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Enter", CurrentCultureIgnoreCase)) // Enter Key
            {
                return "VK_RETURN";
            }
            if (String.Equals(Key, "Shift", CurrentCultureIgnoreCase) ^ String.Equals(Key, "LShift", CurrentCultureIgnoreCase)) // Left-Shift Key
            {
                return "VK_LSHIFT";
            }
            if (String.Equals(Key, "RShift", CurrentCultureIgnoreCase)) // Right-Shift Key
            {
                return "VK_RSHIFT";
            }
            if (String.Equals(Key, "Control", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Ctrl", CurrentCultureIgnoreCase) ^ String.Equals(Key, "LCtrl", CurrentCultureIgnoreCase) ^ String.Equals(Key, "LControl", CurrentCultureIgnoreCase)) // Control Key
            {
                return "VK_LCONTROL";
            }
            if (String.Equals(Key, "RControl", CurrentCultureIgnoreCase) ^ String.Equals(Key, "RCtrl", CurrentCultureIgnoreCase)) // Control Key
            {
                return "VK_RCONTROL";
            }
            if (String.Equals(Key, "Menu", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Alt", CurrentCultureIgnoreCase) ^ String.Equals(Key, "LMenu", CurrentCultureIgnoreCase) ^ String.Equals(Key, "LAlt", CurrentCultureIgnoreCase)) // Alt Key
            {
                return "VK_LMENU";
            }
            if (String.Equals(Key, "RMenu", CurrentCultureIgnoreCase) ^ String.Equals(Key, "RAlt", CurrentCultureIgnoreCase)) // Alt Key
            {
                return "VK_RMENU";
            }
            if (String.Equals(Key, "Pause", CurrentCultureIgnoreCase)) // Pause Key
            {
                return "VK_PAUSE";
            }
            if (String.Equals(Key, "Capital", CurrentCultureIgnoreCase) ^ String.Equals(Key, "CapsLock", CurrentCultureIgnoreCase)) // Caps Lock Key
            {
                return "VK_CAPITAL";
            }
            if (String.Equals(Key, "Escape", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Esc", CurrentCultureIgnoreCase)) // Escape Key
            {
                return "VK_ESCAPE";
            }
            if (String.Equals(Key, "Space", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Spacebar", CurrentCultureIgnoreCase)) // Spacebar
            {
                return "VK_SPACE";
            }
            if (String.Equals(Key, "Prior", CurrentCultureIgnoreCase) ^ String.Equals(Key, "PageUp", CurrentCultureIgnoreCase)) // Page Up Key
            {
                return "VK_PRIOR";
            }
            if (String.Equals(Key, "Next", CurrentCultureIgnoreCase) ^ String.Equals(Key, "PageDown", CurrentCultureIgnoreCase)) // Page Down Key
            {
                return "VK_NEXT";
            }
            if (String.Equals(Key, "End", CurrentCultureIgnoreCase)) // End Key
            {
                return "VK_END";
            }
            if (String.Equals(Key, "Home", CurrentCultureIgnoreCase)) // Home Key
            {
                return "VK_HOME";
            }
            if (String.Equals(Key, "Select", CurrentCultureIgnoreCase)) // Select Key
            {
                return "VK_SELECT";
            }
            if (String.Equals(Key, "Print", CurrentCultureIgnoreCase)) // Print Key
            {
                return "VK_PRINT";
            }
            if (String.Equals(Key, "Execute", CurrentCultureIgnoreCase)) // Execute Key
            {
                return "VK_EXECUTE";
            }
            if (String.Equals(Key, "Snapshot", CurrentCultureIgnoreCase) ^ String.Equals(Key, "PrintScreen", CurrentCultureIgnoreCase)) // Print Screen Key
            {
                return "VK_SNAPSHOT";
            }
            if (String.Equals(Key, "Scroll", CurrentCultureIgnoreCase) ^ String.Equals(Key, "ScrollLock", CurrentCultureIgnoreCase)) // Scroll Lock Key
            {
                return "VK_SCROLL";
            }
            if (String.Equals(Key, "Insert", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Ins", CurrentCultureIgnoreCase)) // Insert Key
            {
                return "VK_INSERT";
            }
            if (String.Equals(Key, "Delete", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Del", CurrentCultureIgnoreCase)) // Delete Key (Not Backspace)
            {
                return "VK_DELETE";
            }
            if (String.Equals(Key, "Help", CurrentCultureIgnoreCase)) // Help Key
            {
                return "VK_HELP";
            }
            if (String.Equals(Key, "Win", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Windows", CurrentCultureIgnoreCase) ^ String.Equals(Key, "LWin", CurrentCultureIgnoreCase) ^ String.Equals(Key, "LWindows", CurrentCultureIgnoreCase)) // Left Windows Key
            {
                return "VK_LWIN";
            }
            if (String.Equals(Key, "RWin", CurrentCultureIgnoreCase) ^ String.Equals(Key, "RWindows", CurrentCultureIgnoreCase)) // Right Windows Key
            {
                return "VK_RWIN";
            }
            if (String.Equals(Key, "Apps", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Applications", CurrentCultureIgnoreCase)) // Applications Button
            {
                return "VK_APPS";
            }
            if (String.Equals(Key, "Sleep", CurrentCultureIgnoreCase)) // Sleep/ Moon Key
            {
                return "VK_SLEEP";
            }
            if (String.Equals(Key, "Attn", CurrentCultureIgnoreCase)) // Attn Key
            {
                return "VK_ATTN";
            }
            if (String.Equals(Key, "CrSel", CurrentCultureIgnoreCase)) // CrSel Key
            {
                return "VK_CRSEL";
            }
            if (String.Equals(Key, "ExSel", CurrentCultureIgnoreCase)) // ExSel Key
            {
                return "VK_EXSEL";
            }
            if (String.Equals(Key, "ErEOF", CurrentCultureIgnoreCase)) // Erase EOF Key
            {
                return "VK_EREOF";
            }
            if (String.Equals(Key, "Play", CurrentCultureIgnoreCase)) // Play Key
            {
                return "VK_PLAY";
            }
            if (String.Equals(Key, "Zoom", CurrentCultureIgnoreCase)) // Zoom Key
            {
                return "VK_ZOOM";
            }
            if (String.Equals(Key, "PA1", CurrentCultureIgnoreCase)) // PA1 Key
            {
                return "VK_PA1";
            }

            // Arrow Keys
            if (String.Equals(Key, "Left", CurrentCultureIgnoreCase) ^ String.Equals(Key, "LeftArrow", CurrentCultureIgnoreCase)) // Left Arrow Key
            {
                return "VK_LEFT";
            }
            if (String.Equals(Key, "Up", CurrentCultureIgnoreCase) ^ String.Equals(Key, "UpArrow", CurrentCultureIgnoreCase)) // Up Arrow Key
            {
                return "VK_UP";
            }
            if (String.Equals(Key, "Down", CurrentCultureIgnoreCase) ^ String.Equals(Key, "DownArrow", CurrentCultureIgnoreCase)) // Down Arrow Key
            {
                return "VK_DOWN";
            }
            if (String.Equals(Key, "Right", CurrentCultureIgnoreCase) ^ String.Equals(Key, "RightArrow", CurrentCultureIgnoreCase)) // Right Arrow Key
            {
                return "VK_RIGHT";
            }

        //IME Keys
            if (String.Equals(Key, "Kana", CurrentCultureIgnoreCase) ^ String.Equals(Key, "IMEKana", CurrentCultureIgnoreCase)) // IME Kana Mode
            {
                return "VK_KANA";
            }
            if (String.Equals(Key, "Hanguel", CurrentCultureIgnoreCase) ^ String.Equals(Key, "IMEHanguel", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Hangul", CurrentCultureIgnoreCase) ^ String.Equals(Key, "IMEHangul", CurrentCultureIgnoreCase)) // Hanguel & Compatability Mode Hangul (Share same key)
            {
                return "VK_HANGUL";
            }
            if (String.Equals(Key, "IMEOn", CurrentCultureIgnoreCase)) // IME Enable Key
            {
                return "VK_IME_ON";
            }
            if (String.Equals(Key, "Junja", CurrentCultureIgnoreCase) ^ String.Equals(Key, "IMEJunja", CurrentCultureIgnoreCase)) // IME Junja Mode 
            {
                return "VK_JUNJA";
            }
            if (String.Equals(Key, "Final", CurrentCultureIgnoreCase) ^ String.Equals(Key, "IMEFinal", CurrentCultureIgnoreCase)) // IME Final Mode
            {
                return "VK_FINAL";
            }
            if (String.Equals(Key, "Hanja", CurrentCultureIgnoreCase) ^ String.Equals(Key, "IMEHanja", CurrentCultureIgnoreCase)) // IME Hanja Mode
            {
                return "VK_HANJA";
            }
            if (String.Equals(Key, "Kanji", CurrentCultureIgnoreCase) ^ String.Equals(Key, "IMEKanji", CurrentCultureIgnoreCase)) // IME Kanji Mode
            {
                return "VK_KANJI";
            }
            if (String.Equals(Key, "IMEOff", CurrentCultureIgnoreCase)) // IME Off
            {
                return "VK_IME_OFF";
            }
            if (String.Equals(Key, "Convert", CurrentCultureIgnoreCase) ^ String.Equals(Key, "IMEConvert", CurrentCultureIgnoreCase)) // IME Convert Key
            {
                return "VK_CONVERT";
            }
            if (String.Equals(Key, "NonConvert", CurrentCultureIgnoreCase) ^ String.Equals(Key, "IMENonConvert", CurrentCultureIgnoreCase)) // IME Non-Convert Key
            {
                return "VK_NONCONVERT";
            }
            if (String.Equals(Key, "Accept", CurrentCultureIgnoreCase) ^ String.Equals(Key, "IMEAccept", CurrentCultureIgnoreCase)) // IME Accept Key
            {
                return "VK_ACCEPT";
            }
            if (String.Equals(Key, "ModeChange", CurrentCultureIgnoreCase) ^ String.Equals(Key, "IMEModeChange", CurrentCultureIgnoreCase)) // IME Mode Change Request
            {
                return "VK_MODECHANGE";
            }
            if (String.Equals(Key, "Process", CurrentCultureIgnoreCase) ^ String.Equals(Key, "IMEProcess", CurrentCultureIgnoreCase)) // IME Process Key
            {
                return "VK_PROCESSKEY";
            }

            // Number Row
            if (String.Equals(Key, "0", CurrentCultureIgnoreCase) ^ String.Equals(Key, "D0", CurrentCultureIgnoreCase)) // Number Row 0
            {
                return "0";
            }
            if (String.Equals(Key, "1", CurrentCultureIgnoreCase) ^ String.Equals(Key, "D1", CurrentCultureIgnoreCase)) // Number Row 1
            {
                return "1";
            }
            if (String.Equals(Key, "2", CurrentCultureIgnoreCase) ^ String.Equals(Key, "D2", CurrentCultureIgnoreCase)) // Number Row 2
            {
                return "2";
            }
            if (String.Equals(Key, "3", CurrentCultureIgnoreCase) ^ String.Equals(Key, "D3", CurrentCultureIgnoreCase)) // Number Row 3
            {
                return "3";
            }
            if (String.Equals(Key, "4", CurrentCultureIgnoreCase) ^ String.Equals(Key, "D4", CurrentCultureIgnoreCase)) // Number Row 4
            {
                return "4";
            }
            if (String.Equals(Key, "5", CurrentCultureIgnoreCase) ^ String.Equals(Key, "D5", CurrentCultureIgnoreCase)) // Number Row 5
            {
                return "5";
            }
            if (String.Equals(Key, "6", CurrentCultureIgnoreCase) ^ String.Equals(Key, "D6", CurrentCultureIgnoreCase)) // Number Row 6
            {
                return "6";
            }
            if (String.Equals(Key, "7", CurrentCultureIgnoreCase) ^ String.Equals(Key, "D7", CurrentCultureIgnoreCase)) // Number Row 7
            {
                return "7";
            }
            if (String.Equals(Key, "8", CurrentCultureIgnoreCase) ^ String.Equals(Key, "D8", CurrentCultureIgnoreCase)) // Number Row 8
            {
                return "8";
            }
            if (String.Equals(Key, "9", CurrentCultureIgnoreCase) ^ String.Equals(Key, "D9", CurrentCultureIgnoreCase)) // Number Row 9
            {
                return "9";
            }

        // English Alphabet
            if (String.Equals(Key, "A", CurrentCultureIgnoreCase)) // Letter A
            {
                return "A";
            }
            if (String.Equals(Key, "B", CurrentCultureIgnoreCase)) // Letter B
            {
                return "B";
            }
            if (String.Equals(Key, "C", CurrentCultureIgnoreCase)) // Letter C
            {
                return "C";
            }
            if (String.Equals(Key, "D", CurrentCultureIgnoreCase)) // Letter D
            {
                return "D";
            }
            if (String.Equals(Key, "E", CurrentCultureIgnoreCase)) // Letter E
            {
                return "E";
            }
            if (String.Equals(Key, "F", CurrentCultureIgnoreCase)) // Letter F
            {
                return "F";
            }
            if (String.Equals(Key, "G", CurrentCultureIgnoreCase)) // Letter G
            {
                return "G";
            }
            if (String.Equals(Key, "H", CurrentCultureIgnoreCase)) // Letter H
            {
                return "H";
            }
            if (String.Equals(Key, "I", CurrentCultureIgnoreCase)) // Letter I
            {
                return "I";
            }
            if (String.Equals(Key, "J", CurrentCultureIgnoreCase)) // Letter J
            {
                return "J";
            }
            if (String.Equals(Key, "K", CurrentCultureIgnoreCase)) // Letter K
            {
                return "K";
            }
            if (String.Equals(Key, "L", CurrentCultureIgnoreCase)) // Letter L
            {
                return "L";
            }
            if (String.Equals(Key, "M", CurrentCultureIgnoreCase)) // Letter M
            {
                return "M";
            }
            if (String.Equals(Key, "N", CurrentCultureIgnoreCase)) // Letter N
            {
                return "N";
            }
            if (String.Equals(Key, "O", CurrentCultureIgnoreCase)) // Letter O
            {
                return "O";
            }
            if (String.Equals(Key, "P", CurrentCultureIgnoreCase)) // Letter P
            {
                return "P";
            }
            if (String.Equals(Key, "Q", CurrentCultureIgnoreCase)) // Letter Q
            {
                return "Q";
            }
            if (String.Equals(Key, "R", CurrentCultureIgnoreCase)) // Letter R
            {
                return "R";
            }
            if (String.Equals(Key, "S", CurrentCultureIgnoreCase)) // Letter S
            {
                return "S";
            }
            if (String.Equals(Key, "T", CurrentCultureIgnoreCase)) // Letter T
            {
                return "T";
            }
            if (String.Equals(Key, "U", CurrentCultureIgnoreCase)) // Letter U
            {
                return "U";
            }
            if (String.Equals(Key, "V", CurrentCultureIgnoreCase)) // Letter V
            {
                return "V";
            }
            if (String.Equals(Key, "W", CurrentCultureIgnoreCase)) // Letter W
            {
                return "W";
            }
            if (String.Equals(Key, "X", CurrentCultureIgnoreCase)) // Letter X
            {
                return "X";
            }
            if (String.Equals(Key, "Y", CurrentCultureIgnoreCase)) // Letter Y
            {
                return "Y";
            }
            if (String.Equals(Key, "Z", CurrentCultureIgnoreCase)) // Letter Z
            {
                return "Z";
            }

        // Numpad Keys
            if (String.Equals(Key, "NumLock", CurrentCultureIgnoreCase)) // NumLock
            {
                return "VK_NUMLOCK";
            }
            if (String.Equals(Key, "Numpad0", CurrentCultureIgnoreCase)) // Numpad Number 0
            {
                return "VK_NUMPAD0";
            }
            if (String.Equals(Key, "Numpad1", CurrentCultureIgnoreCase)) // Numpad Number 1
            {
                return "VK_NUMPAD1";
            }
            if (String.Equals(Key, "Numpad2", CurrentCultureIgnoreCase)) // Numpad Number 2
            {
                return "VK_NUMPAD2";
            }
            if (String.Equals(Key, "Numpad3", CurrentCultureIgnoreCase)) // Numpad Number 3
            {
                return "VK_NUMPAD3";
            }
            if (String.Equals(Key, "Numpad4", CurrentCultureIgnoreCase)) // Numpad Number 4
            {
                return "VK_NUMPAD4";
            }
            if (String.Equals(Key, "Numpad5", CurrentCultureIgnoreCase)) // Numpad Number 5
            {
                return "VK_NUMPAD5";
            }
            if (String.Equals(Key, "Numpad6", CurrentCultureIgnoreCase)) // Numpad Number 6
            {
                return "VK_NUMPAD6";
            }
            if (String.Equals(Key, "Numpad7", CurrentCultureIgnoreCase)) // Numpad Number 7
            {
                return "VK_NUMPAD7";
            }
            if (String.Equals(Key, "Numpad8", CurrentCultureIgnoreCase)) // Numpad Number 8
            {
                return "VK_NUMPAD8";
            }
            if (String.Equals(Key, "Numpad9", CurrentCultureIgnoreCase)) // Numpad Number 9
            {
                return "VK_NUMPAD9";
            }
            if (String.Equals(Key, "Decimal", CurrentCultureIgnoreCase)) // Numpad Decimal
            {
                return "VK_DECIMAL";
            }
            if (String.Equals(Key, "NumpadAdd", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Numpad+", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Add", CurrentCultureIgnoreCase)) // Numpad Plus / Add Key
            {
                return "VK_ADD";
            }
            if (String.Equals(Key, "NumpadMultiply", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Numpad*", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Multiply", CurrentCultureIgnoreCase)) // Numpad Times / Multiply Key
            {
                return "VK_MULTIPLY";
            }
            if (String.Equals(Key, "NumpadSubtract", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Numpad-", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Subtract", CurrentCultureIgnoreCase)) // Numpad Minus / Subtract Key
            {
                return "VK_SUBTRACT";
            }
            if (String.Equals(Key, "NumpadDivide", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Numpad/", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Divide", CurrentCultureIgnoreCase)) // Numpad Divide Key
            {
                return "VK_DIVIDE";
            }

            // F-Keys
            if (String.Equals(Key, "F1", CurrentCultureIgnoreCase)) // F1
            {
                return "VK_F1";
            }
            if (String.Equals(Key, "F2", CurrentCultureIgnoreCase)) // F2
            {
                return "VK_F2";
            }
            if (String.Equals(Key, "F3", CurrentCultureIgnoreCase)) // F3
            {
                return "VK_F3";
            }
            if (String.Equals(Key, "F4", CurrentCultureIgnoreCase)) // F4
            {
                return "VK_F4";
            }
            if (String.Equals(Key, "F5", CurrentCultureIgnoreCase)) // F5
            {
                return "VK_F5";
            }
            if (String.Equals(Key, "F6", CurrentCultureIgnoreCase)) // F6
            {
                return "VK_F6";
            }
            if (String.Equals(Key, "F7", CurrentCultureIgnoreCase)) // F7
            {
                return "VK_F7";
            }
            if (String.Equals(Key, "F8", CurrentCultureIgnoreCase)) // F8
            {
                return "VK_F8";
            }
            if (String.Equals(Key, "F9", CurrentCultureIgnoreCase)) // F9
            {
                return "VK_F9";
            }
            if (String.Equals(Key, "F10", CurrentCultureIgnoreCase)) // F10
            {
                return "VK_F10";
            }
            if (String.Equals(Key, "F11", CurrentCultureIgnoreCase)) // F11
            {
                return "VK_F11";
            }
            if (String.Equals(Key, "F12", CurrentCultureIgnoreCase)) // F12
            {
                return "VK_F12";
            }
            if (String.Equals(Key, "F13", CurrentCultureIgnoreCase)) // F13
            {
                return "VK_F13";
            }
            if (String.Equals(Key, "F14", CurrentCultureIgnoreCase)) // F14
            {
                return "VK_F14";
            }
            if (String.Equals(Key, "F15", CurrentCultureIgnoreCase)) // F15
            {
                return "VK_F15";
            }
            if (String.Equals(Key, "F16", CurrentCultureIgnoreCase)) // F16
            {
                return "VK_F16";
            }
            if (String.Equals(Key, "F17", CurrentCultureIgnoreCase)) // F17
            {
                return "VK_F17";
            }
            if (String.Equals(Key, "F18", CurrentCultureIgnoreCase)) // F18
            {
                return "VK_F18";
            }
            if (String.Equals(Key, "F19", CurrentCultureIgnoreCase)) // F19
            {
                return "VK_F19";
            }
            if (String.Equals(Key, "F20", CurrentCultureIgnoreCase)) // F20
            {
                return "VK_F20";
            }
            if (String.Equals(Key, "F21", CurrentCultureIgnoreCase)) // F21
            {
                return "VK_F21";
            }
            if (String.Equals(Key, "F22", CurrentCultureIgnoreCase)) // F22
            {
                return "VK_F22";
            }
            if (String.Equals(Key, "F23", CurrentCultureIgnoreCase)) // F23
            {
                return "VK_F23";
            }
            if (String.Equals(Key, "F24", CurrentCultureIgnoreCase)) // F24
            {
                return "VK_F24";
            }

        // Browser Keys
            if (String.Equals(Key, "BrowserBack", CurrentCultureIgnoreCase)) // Browser Back Key
            {
                return "VK_BROWSER_BACK";
            }
            if (String.Equals(Key, "BrowserForward", CurrentCultureIgnoreCase)) // Browser Forward Key
            {
                return "VK_BROWSER_FORWARD";
            }
            if (String.Equals(Key, "BrowserRefresh", CurrentCultureIgnoreCase)) // Browser Refresh Key
            {
                return "VK_BROWSER_REFRESH";
            }
            if (String.Equals(Key, "BrowserStop", CurrentCultureIgnoreCase)) // Browser Stop Key
            {
                return "VK_BROWSER_STOP";
            }
            if (String.Equals(Key, "BrowserSearch", CurrentCultureIgnoreCase)) // Browser Search Key
            {
                return "VK_BROWSER_SEARCH";
            }
            if (String.Equals(Key, "BrowserFavorites", CurrentCultureIgnoreCase)) // Browser Favorites Key
            {
                return "VK_BROWSER_FAVORITES";
            }
            if (String.Equals(Key, "BrowserHome", CurrentCultureIgnoreCase)) // Browser Home Key
            {
                return "VK_BROWSER_HOME";
            }
        // Volume Keys
            if (String.Equals(Key, "VolumeMute", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Mute", CurrentCultureIgnoreCase)) // Volume Mute Key 
            {
                return "VK_VOLUME_MUTE";
            }
            if (String.Equals(Key, "VolumeDown", CurrentCultureIgnoreCase)) // Volume Down Key
            {
                return "VK_VOLUME_DOWN";
            }
            if (String.Equals(Key, "Volume Up", CurrentCultureIgnoreCase)) // Volume Up Key
            {
                return "VK_VOLUME_UP";
            }
        // Media Keys
            if (String.Equals(Key, "MediaNextTrack", CurrentCultureIgnoreCase) ^ String.Equals(Key, "NextTrack", CurrentCultureIgnoreCase)) // Media Next Track Key 
            {
                return "VK_MEDIA_NEXT_TRACK";
            }
            if (String.Equals(Key, "MediaPrevTrack", CurrentCultureIgnoreCase) ^ String.Equals(Key, "MediaPreviousTrack", CurrentCultureIgnoreCase)) // Media Previous Track Key
            {
                return "VK_MEDIA_PREV_TRACK";
            }
            if (String.Equals(Key, "MediaStop", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Stop", CurrentCultureIgnoreCase)) // Stop Media Key 
            {
                return "VK_MEDIA_STOP";
            }
            if (String.Equals(Key, "MediaPlayPause", CurrentCultureIgnoreCase) ^ String.Equals(Key, "PlayPause", CurrentCultureIgnoreCase)) // Media Play Pause Key
            {
                return "VK_MEDIA_PLAY_PAUSE";
            }
        // Launch Application Keys
            if (String.Equals(Key, "LaunchMail", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Mail", CurrentCultureIgnoreCase)) // Launch Mail Key
            {
                return "VK_LAUNCH_MAIL";
            }
            if (String.Equals(Key, "LaunchMediaSelect", CurrentCultureIgnoreCase) ^ String.Equals(Key, "MediaSelect", CurrentCultureIgnoreCase)) // Launch Media Key
            {
                return "VK_LAUNCH_MEDIA_SELECT";
            }
            if (String.Equals(Key, "LaunchApp1", CurrentCultureIgnoreCase) ^ String.Equals(Key, "App1", CurrentCultureIgnoreCase)) // Launch App1 Key
            {
                return "VK_LAUNCH_APP1";
            }
            if (String.Equals(Key, "LaunchApp2", CurrentCultureIgnoreCase) ^ String.Equals(Key, "App2", CurrentCultureIgnoreCase)) // Launch App2 Key
            {
                return "VK_LAUNCH_APP2";
            }

        //OEM (Symbol) Keys
            if (String.Equals(Key, "ColonSemiColon", CurrentCultureIgnoreCase) ^ String.Equals(Key, "SemiColonColon", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Oem1", CurrentCultureIgnoreCase)) // SemiColon & Colon Key
            {
                return "VK_OEM_1";
            }
            if (String.Equals(Key, "OEMPlus", CurrentCultureIgnoreCase) ^ String.Equals(Key, "NumRowPlus", CurrentCultureIgnoreCase)) // Num-Row Add Key
            {
                return "VK_OEM_PLUS";
            }
            if (String.Equals(Key, "Comma", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Oemcomma", CurrentCultureIgnoreCase)) // Comma / < Key
            {
                return "VK_OEM_COMMA";
            }
            if (String.Equals(Key, "OEMMinus", CurrentCultureIgnoreCase) ^ String.Equals(Key, "NumRowMinus", CurrentCultureIgnoreCase)) // Num-Row Subtract Key
            {
                return "VK_OEM_MINUS";
            }
            if (String.Equals(Key, "Period", CurrentCultureIgnoreCase) ^ String.Equals(Key, "OemPeriod", CurrentCultureIgnoreCase)) // Period/ > Key
            {
                return "VK_OEM_PERIOD";
            }
            if (String.Equals(Key, "SlashQuestionMark", CurrentCultureIgnoreCase) ^ String.Equals(Key, "QuestionMarkSlash", CurrentCultureIgnoreCase) ^ String.Equals(Key, "OemQuestion", CurrentCultureIgnoreCase) ^ String.Equals(Key, "/?", CurrentCultureIgnoreCase)) // Question Mark & Forward Slash
            {
                return "VK_OEM_2";
            }
            if (String.Equals(Key, "Tilda", CurrentCultureIgnoreCase) ^ String.Equals(Key, "BackTick", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Oemtilde", CurrentCultureIgnoreCase)) // Back Tick / Tilda Key
            {
                return "VK_OEM_3";
            }
            if (String.Equals(Key, "OpenBrackets", CurrentCultureIgnoreCase) ^ String.Equals(Key, "[{", CurrentCultureIgnoreCase) ^ String.Equals(Key, "{[", CurrentCultureIgnoreCase)) // Open Curly ({) & Block ([) Brackets Key
            {
                return "VK_OEM_4";
            }
            if (String.Equals(Key, "BackSlashPipe", CurrentCultureIgnoreCase) ^ String.Equals(Key, "PipeBackSlash", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Oem5", CurrentCultureIgnoreCase) ^ String.Equals(Key, "|\\", CurrentCultureIgnoreCase)) // Backslash & Pipe key
            {
                return "VK_OEM_5";
            }
            if (String.Equals(Key, "CloseBrackets", CurrentCultureIgnoreCase) ^ String.Equals(Key, "}]", CurrentCultureIgnoreCase) ^ String.Equals(Key, "]}", CurrentCultureIgnoreCase)) // Close Curly (}) & Block (]) Brackets Key
            {
                return "VK_OEM_6";
            }
            if (String.Equals(Key, "Quotes", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Oem7", CurrentCultureIgnoreCase) ^ String.Equals(Key, "\"'", CurrentCultureIgnoreCase)) // Single & Double Quotes Key
            {
                return "VK_OEM_7";
            }
            if (String.Equals(Key, "OEMClear", CurrentCultureIgnoreCase) ^ String.Equals(Key, "Clear", CurrentCultureIgnoreCase)) // Clear Key
            {
                return "VK_OEM_CLEAR";
            }

        // X-Input Gamepad
            // Main Buttons
            if (String.Equals(Key, "XInputGamepadA", CurrentCultureIgnoreCase)) // Press "A" Button
            {
                return "VK_PAD_A";
            }
            if (String.Equals(Key, "XInputGamepadB", CurrentCultureIgnoreCase)) // Press "B" Button
            {
                return "VK_PAD_B";
            }
            if (String.Equals(Key, "XInputGamepadX", CurrentCultureIgnoreCase)) // Press "X" Button
            {
                return "VK_PAD_X";
            }
            if (String.Equals(Key, "XInputGamepadY", CurrentCultureIgnoreCase)) // Press "Y" Button
            {
                return "VK_PAD_Y";
            }
            if (String.Equals(Key, "XInputGamepadStart", CurrentCultureIgnoreCase)) // Press "Start" Button
            {
                return "VK_PAD_START";
            }
            if (String.Equals(Key, "XInputGamepadBack", CurrentCultureIgnoreCase)) // Press "Back" Button
            {
                return "VK_PAD_BACK";
            }
            if (String.Equals(Key, "XInputGamepadLeftShoulder", CurrentCultureIgnoreCase)) // Press "Left Sholder" Button
            {
                return "VK_PAD_LSHOULDER";
            }
            if (String.Equals(Key, "XInputGamepadRightShoulder", CurrentCultureIgnoreCase)) // Press "Right Shoulder" Button
            {
                return "VK_PAD_RSHOULDER";
            }
            if (String.Equals(Key, "XInputGamepadLeftTrigger", CurrentCultureIgnoreCase)) // Press Left Trigger
            {
                return "VK_PAD_LTRIGGER";
            }
            if (String.Equals(Key, "XInputGamepadRightTrigger", CurrentCultureIgnoreCase)) // Press Right Trigger
            {
                return "VK_PAD_RTRIGGER";
            }
            // DPad
            if (String.Equals(Key, "XInputGamepadDPadUp", CurrentCultureIgnoreCase)) // DPad-Up
            {
                return "VK_PAD_DPAD_UP";
            }
            if (String.Equals(Key, "XInputGamepadDPadDown", CurrentCultureIgnoreCase)) // DPad-Down
            {
                return "VK_PAD_DPAD_DOWN";
            }
            if (String.Equals(Key, "XInputGamepadDPadLeft", CurrentCultureIgnoreCase)) // DPad-Left
            {
                return "VK_PAD_DPAD_LEFT";
            }
            if (String.Equals(Key, "XInputGamepadDPadRight", CurrentCultureIgnoreCase)) // DPad-Right
            {
                return "VK_PAD_DPAD_RIGHT";
            }
            // Left Thumbstick
            if (String.Equals(Key, "XInputGamepadLeftThumbPress", CurrentCultureIgnoreCase)) // Press-in Left-Thumbstick
            {
                return "VK_PAD_LTHUMB_PRESS";
            }
            if (String.Equals(Key, "XInputGamepadLeftThumbUp", CurrentCultureIgnoreCase)) // Thumbstick Up
            {
                return "VK_PAD_LTHUMB_UP";
            }
            if (String.Equals(Key, "XInputGamepadLeftThumbDown", CurrentCultureIgnoreCase)) // Thumbstick Down
            {
                return "VK_PAD_LTHUMB_DOWN";
            }
            if (String.Equals(Key, "XInputGamepadLeftThumbLeft", CurrentCultureIgnoreCase)) // Thumbstick Left
            {
                return "VK_PAD_LTHUMB_RIGHT";
            }
            if (String.Equals(Key, "XInputGamepadLeftThumbRight", CurrentCultureIgnoreCase)) // Thumbstick Right
            {
                return "VK_PAD_LTHUMB_LEFT";
            }
            if (String.Equals(Key, "XInputGamepadLeftThumbUpLeft", CurrentCultureIgnoreCase)) // Thumbstick Up&Left
            {
                return "VK_PAD_LTHUMB_UPLEFT";
            }
            if (String.Equals(Key, "XInputGamepadLeftThumbUpRight", CurrentCultureIgnoreCase)) // Thumstick Up&Right
            {
                return "VK_PAD_LTHUMB_UPRIGHT";
            }
            if (String.Equals(Key, "XInputGamepadLeftThumbDownLeft", CurrentCultureIgnoreCase)) // Thumbstick Down&Left
            {
                return "VK_PAD_LTHUMB_DOWNLEFT";
            }
            if (String.Equals(Key, "XInputGamepadLeftThumbDownRight", CurrentCultureIgnoreCase)) // Thumbstick Down&Right
            {
                return "VK_PAD_LTHUMB_DOWNRIGHT";
            }
            // Right Thumbstick
            if (String.Equals(Key, "XInputGamepadRightThumbPress", CurrentCultureIgnoreCase)) // Press-in Right-Thumbstick
            {
                return "VK_PAD_RTHUMB_PRESS";
            }
            if (String.Equals(Key, "XInputGamepadRightThumbUp", CurrentCultureIgnoreCase)) // Thumbstick Up
            {
                return "VK_PAD_RTHUMB_UP";
            }
            if (String.Equals(Key, "XInputGamepadRightThumbDown", CurrentCultureIgnoreCase)) // Thumbstick Down
            {
                return "VK_PAD_RTHUMB_DOWN";
            }
            if (String.Equals(Key, "XInputGamepadRightThumbLeft", CurrentCultureIgnoreCase)) // Thumbstick Left
            {
                return "VK_PAD_RTHUMB_LEFT";
            }
            if (String.Equals(Key, "XInputGamepadRightThumbRight", CurrentCultureIgnoreCase)) // Thumbstick Right
            {
                return "VK_PAD_RTHUMB_RIGHT";
            }
            if (String.Equals(Key, "XInputGamepadRightThumbUpLeft", CurrentCultureIgnoreCase)) // Thumbstick Up&Left
            {
                return "VK_PAD_RTHUMB_UPLEFT";
            }
            if (String.Equals(Key, "XInputGamepadRightThumbUpRight", CurrentCultureIgnoreCase)) // Thumstick Up&Right
            {
                return "VK_PAD_RTHUMB_UPRIGHT";
            }
            if (String.Equals(Key, "XInputGamepadRightThumbDownLeft", CurrentCultureIgnoreCase)) // Thumbstick Down&Left
            {
                return "VK_PAD_RTHUMB_DOWNRIGHT";
            }
            if (String.Equals(Key, "XInputGamepadRightThumbDownRight", CurrentCultureIgnoreCase)) // Thumbstick Down&Right
            {
                return "VK_PAD_RTHUMB_DOWNLEFT";
            }
            return "";
        }


        public static string VKeyToUI(string VKey)
        {
            string ReplacementValue = VKey;
            if (VKey.Contains("VK_"))
                ReplacementValue = ReplacementValue.Substring(3, ReplacementValue.Length - 3);
            

            return ReplacementValue;
        }

        public static System.Windows.Forms.Keys[] KeyDownDictionary = new System.Windows.Forms.Keys[38] // As to why not every key is in here, refer to this picture as to what keys are used my Rocksmith by default. https://pastebin.com/raw/7f1CvkUt | https://i.imgur.com/lpNv3yG.png
        {
            // Standard Function Keys
             Keys.F1, Keys.F2, Keys.F3, Keys.F4, Keys.F5, Keys.F6, Keys.F7, Keys.F9, Keys.F10,

            // Elgato Keys / Extended Function Keys
            Keys.F13, Keys.F14, Keys.F15, Keys.F16, Keys.F17, Keys.F18, Keys.F19, Keys.F20, Keys.F21, Keys.F22,  Keys.F23, Keys.F24,

            // Special Keys
            Keys.Pause, Keys.Scroll, Keys.Insert,

            // Symbol Keys
            Keys.OemSemicolon, Keys.Oemtilde, Keys.Oemcomma, Keys.OemMinus, Keys.OemPeriod, Keys.OemQuotes, Keys.OemQuestion,

            // Modifier Keys
            Keys.CapsLock, Keys.Apps,

            // Numlock Keys
            Keys.Subtract, Keys.NumLock, Keys.Multiply, Keys.Divide, Keys.Decimal
        };
        public static System.Windows.Forms.Keys[] KeyUpDictionary = new System.Windows.Forms.Keys[6] // Same with KeyDownDictionary, but these are all the keys that can only be read on KeyUp
        {
            // Print Screen
            Keys.PrintScreen,

            // Media Keys
            Keys.Play, Keys.MediaNextTrack, Keys.MediaPlayPause, Keys.MediaPreviousTrack, Keys.MediaStop
        };
        public static System.Windows.Forms.MouseButtons[] MouseButtonDictionary = new System.Windows.Forms.MouseButtons[3] // Same with KeyUpDictonary, and KeyDownDictionary but with the mouse's extra buttons.
        {
             // Middle Mouse Button
             MouseButtons.Middle,

             // Side Buttons
             MouseButtons.XButton1, MouseButtons.XButton2
        };
    }
}