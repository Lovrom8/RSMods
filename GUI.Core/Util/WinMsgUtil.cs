using System;
using System.Runtime.InteropServices;

namespace RSMods.Util
{
    public struct Rect
    {
        public int Left { get; set; }
        public int Top { get; set; }
        public int Right { get; set; }
        public int Bottom { get; set; }
    }

    public static class WinMsgUtil // Use WM_COPYDATA message as a means of interprocess communication between the GUI and RS
    {
        [DllImport("user32.dll")]
        public static extern bool GetWindowRect(IntPtr hwnd, ref Rect rectangle);

        [DllImport("user32.dll", SetLastError = true)]
        static extern IntPtr FindWindow(string lpClassName, string lpWindowName);

        [DllImport("user32.dll")]
        private static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

        [StructLayout(LayoutKind.Sequential)]
        public struct CopyData : IDisposable
        {
            [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = false)]
            static extern IntPtr SendMessageTimeout(IntPtr hWnd, uint Msg, IntPtr wParam, ref CopyData target,
                                                    SendMessageTimeoutFlags fuFlags, uint uTimeout, out UIntPtr lpdwResult);

            [Flags]
            enum SendMessageTimeoutFlags : uint
            {
                SMTO_NORMAL = 0x0,
                SMTO_BLOCK = 0x1,
                SMTO_ABORTIFHUNG = 0x2,
                SMTO_NOTIMEOUTIFNOTHUNG = 0x8
            }
            const uint WM_COPYDATA = 0x4A;

            public IntPtr dwData;
            public int cbData;
            public IntPtr lpData;

            public void Dispose()
            {
                if (lpData != IntPtr.Zero)
                {
                    Marshal.FreeCoTaskMem(lpData);
                    lpData = IntPtr.Zero;
                    cbData = 0;
                }
            }

            public string AsAnsiString => Marshal.PtrToStringAnsi(lpData, cbData);
            public string AsUnicodeString => Marshal.PtrToStringUni(lpData);

            public static CopyData CreateForString(int dwData, string value, bool Unicode = false)
            {
                IntPtr data = Unicode ? Marshal.StringToCoTaskMemUni(value) : Marshal.StringToCoTaskMemAnsi(value);
                return new CopyData
                {
                    dwData = (IntPtr)dwData,
                    lpData = data,
                    // In bytes, with the terminator: a character is two in UTF-16, and can be two in an ANSI code page.
                    cbData = Unicode ? (value.Length + 1) * 2 : AnsiLength(data) + 1
                };
            }

            private static int AnsiLength(IntPtr data)
            {
                int length = 0;
                while (Marshal.ReadByte(data, length) != 0)
                    length++;
                return length;
            }

            public static UIntPtr Send(IntPtr targetHandle, int dwData, string value, uint timeoutMs = 1000, bool Unicode = false)
            {
                var cds = CopyData.CreateForString(dwData, value, Unicode);
                SendMessageTimeout(targetHandle, WM_COPYDATA, IntPtr.Zero, ref cds, SendMessageTimeoutFlags.SMTO_NORMAL, timeoutMs, out UIntPtr result);
                cds.Dispose();
                return result;
            }
        }

        public static void SendMsgToRS(string msg)
        {
            IntPtr rsHandle = FindWindow("Rocksmith 2014", null);
            if (rsHandle == IntPtr.Zero)
                return; // The game isn't running

            CopyData.Send(rsHandle, 1, msg);
        }
    }
}
