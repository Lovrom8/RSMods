using System;
using System.Runtime.InteropServices;

namespace RSMods.Util
{
    public static class WinMsgUtil // Use WM_COPYDATA message as a means of interprocess communication between the GUI and RS 
    {
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

            public string AsAnsiString
            {
                get { return Marshal.PtrToStringAnsi(lpData, cbData); }
            }
            public string AsUnicodeString
            {
                get { return Marshal.PtrToStringUni(lpData); }
            }

            public static CopyData CreateForString(int dwData, string value, bool Unicode = false)
            {
                var result = new CopyData();
                result.dwData = (IntPtr)dwData;
                result.lpData = Unicode ? Marshal.StringToCoTaskMemUni(value) : Marshal.StringToCoTaskMemAnsi(value);
                result.cbData = value.Length + 1;
                return result;
            }

            public static UIntPtr Send(IntPtr targetHandle, int dwData, string value, uint timeoutMs = 1000, bool Unicode = false)
            {
                var cds = CopyData.CreateForString(dwData, value, Unicode);
                UIntPtr result;
                SendMessageTimeout(targetHandle, WM_COPYDATA, IntPtr.Zero, ref cds, SendMessageTimeoutFlags.SMTO_NORMAL, timeoutMs, out result);
                cds.Dispose();
                return result;
            }
        }

        public static void SendMsgToRS(string msg)
        {
            IntPtr rsHandle = FindWindow("Rocksmith 2014", null);

            CopyData.Send(rsHandle, 1, msg);
        }
    }
}
