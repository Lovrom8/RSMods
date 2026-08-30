using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace RSMods
{
    public class Midi
    {
        public enum Status : byte
        {
            NoteOff = 0x80,
            NoteOn = 0x90,
            AfterTouch = 0xA0,
            CC = 0xB0,
            PC = 0xC0,
            Pressure = 0xD0,
            PitchBend = 0xE0,
            SystemEx = 0xF0
        }

        public const uint CALLBACK_FUNCTION = 0x30000;
        public static IntPtr MidiInHandle = IntPtr.Zero;
        public static uint? SelectedMidiInDeviceId = null;
        public static MidiInProc MidiInProcessing = null;

        public enum Responses : uint
        {
            MIM_OPEN = 0x3C1,
            MIM_CLOSE = 0x3C2,
            MIM_DATA = 0x3C3,
            MIM_LONGDATA = 0x3C4,
            MIM_ERROR = 0x3C5,
            MIM_LONGERROR = 0x3C6,
            MIM_MOREDATA = 0x3CC
        }
        public enum MMRESULT : uint
        {
            MMSYSERR_NOERROR,
            MMSYSERR_ERROR,
            MMSYSERR_BADDEVICEID,
            MMSYSERR_NOTENABLED,
            MMSYSERR_ALLOCATED,
            MMSYSERR_INVALHANDLE,
            MMSYSERR_NODRIVER,
            MMSYSERR_NOMEM,
            MMSYSERR_NOTSUPPORTED,
            MMSYSERR_BADERRNUM,
            MMSYSERR_INVALFLAG,
            MMSYSERR_INVALPARAM,
            MMSYSERR_HANDLEBUSY,
            MMSYSERR_INVALIDALIAS,
            MMSYSERR_BADDB,
            MMSYSERR_KEYNOTFOUND,
            MMSYSERR_READERROR,
            MMSYSERR_WRITEERROR,
            MMSYSERR_DELETEERROR,
            MMSYSERR_VALNOTFOUND,
            MMSYSERR_NODRIVERCB,
            WAVERR_BADFORMAT = 32,
            WAVERR_STILLPLAYING = 33,
            WAVERR_UNPREPARED = 34
        }

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void MidiInProc(IntPtr hMidiIn, Responses wMsg, IntPtr dwInstance, uint dwParam1, uint dwParam2);

        [StructLayout(LayoutKind.Sequential)]
        public struct MIDIOUTCAPS
        {
            public ushort wMid;
            public ushort wPid;
            public uint vDriverVersion;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string szPname;
            public ushort wTechnology;
            public ushort wVoices;
            public ushort wNotes;
            public ushort wChannelMask;
            public uint dwSupport;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct MIDIINCAPS
        {
            public ushort wMid;
            public ushort wPid;
            public uint vDriverVersion;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string szPname;
            public uint dwSupport;
        }

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern MMRESULT midiOutGetDevCaps(uint uDeviceID, ref MIDIOUTCAPS lpMidiOutCaps, uint cbMidiOutCaps);

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern uint midiOutGetNumDevs();

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern MMRESULT midiInGetDevCaps(uint uDeviceID, ref MIDIINCAPS pmic, uint cbmic);

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern uint midiInGetNumDevs();

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern uint midiInOpen(ref IntPtr hmi, uint uDeviceID, [MarshalAs(UnmanagedType.FunctionPtr)] MidiInProc dwCallback, IntPtr dwInstance, uint fdwOpen);

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern uint midiInStart(IntPtr hmi);

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern uint midiInStop(IntPtr hmi);

        [DllImport("winmm.dll", SetLastError = true)]
        public static extern uint midiInClose(IntPtr hmi);

        public static IEnumerable<string> GetOutputDeviceNames()
        {
            uint count = midiOutGetNumDevs();
            for (uint i = 0; i < count; i++)
            {
                MIDIOUTCAPS caps = new();
                midiOutGetDevCaps(i, ref caps, (uint)Marshal.SizeOf(typeof(MIDIOUTCAPS)));
                yield return caps.szPname;
            }
        }

        public static IEnumerable<string> GetInputDeviceNames()
        {
            uint count = midiInGetNumDevs();
            for (uint i = 0; i < count; i++)
            {
                MIDIINCAPS caps = new();
                midiInGetDevCaps(i, ref caps, (uint)Marshal.SizeOf(typeof(MIDIINCAPS)));
                yield return caps.szPname;
            }
        }
    }
}
