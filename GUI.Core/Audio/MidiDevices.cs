using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace RSMods.Audio
{
    /// <summary>
    /// Enumeration of MIDI output and input device names via the Windows multimedia (winmm) API.
    /// Detection is best-effort and returns an empty list on failure.
    /// Mirrors the shared ASIO and capture-device enumerations.
    /// </summary>
    /// <remarks>
    /// This intentionally covers only device enumeration for the auto-tune/tuning-pedal settings screen.
    /// The former live MIDI-in listener was a debug-only diagnostic and is intentionally retired.
    /// </remarks>
    public static class MidiDevices
    {
        /// <summary>The names of the installed MIDI output devices, in enumeration order.</summary>
        public static List<string> OutputDeviceNames()
        {
            var names = new List<string>();

            try
            {
                uint count = midiOutGetNumDevs();
                for (uint i = 0; i < count; i++)
                {
                    var caps = new MIDIOUTCAPS();
                    if (midiOutGetDevCaps(i, ref caps, (uint)Marshal.SizeOf(typeof(MIDIOUTCAPS))) == MMRESULT.MMSYSERR_NOERROR)
                        names.Add(caps.szPname);
                }
            }
            catch
            {
                // Best-effort detection; leave the list empty on any winmm failure.
            }

            return names;
        }

        /// <summary>The names of the installed MIDI input devices, in enumeration order.</summary>
        public static List<string> InputDeviceNames()
        {
            var names = new List<string>();

            try
            {
                uint count = midiInGetNumDevs();
                for (uint i = 0; i < count; i++)
                {
                    var caps = new MIDIINCAPS();
                    if (midiInGetDevCaps(i, ref caps, (uint)Marshal.SizeOf(typeof(MIDIINCAPS))) == MMRESULT.MMSYSERR_NOERROR)
                        names.Add(caps.szPname);
                }
            }
            catch
            {
                // Best-effort detection; leave the list empty on any winmm failure.
            }

            return names;
        }

        private enum MMRESULT : uint
        {
            MMSYSERR_NOERROR = 0,
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct MIDIOUTCAPS
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
        private struct MIDIINCAPS
        {
            public ushort wMid;
            public ushort wPid;
            public uint vDriverVersion;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
            public string szPname;
            public uint dwSupport;
        }

        [DllImport("winmm.dll")]
        private static extern uint midiOutGetNumDevs();

        [DllImport("winmm.dll")]
        private static extern MMRESULT midiOutGetDevCaps(uint uDeviceID, ref MIDIOUTCAPS lpMidiOutCaps, uint cbMidiOutCaps);

        [DllImport("winmm.dll")]
        private static extern uint midiInGetNumDevs();

        [DllImport("winmm.dll")]
        private static extern MMRESULT midiInGetDevCaps(uint uDeviceID, ref MIDIINCAPS lpMidiInCaps, uint cbMidiInCaps);
    }
}
