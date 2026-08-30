using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RSMods
{
    public class MidiDebug
    {
        public static void MidiInProc(IntPtr hMidiIn, Midi.Responses wMsg, IntPtr dwInstance, uint midiMessage, uint timeStamp)
        {
            switch (wMsg)
            {
                case Midi.Responses.MIM_DATA:
                    byte statusByte = (byte)(midiMessage & 0xFF);
                    byte data1 = (byte)((midiMessage >> 8) & 0xFF);
                    byte data2 = (byte)((midiMessage >> 16) & 0xFF);

                    // Split the status byte into the Command (upper 4 bits) and Channel (lower 4 bits)
                    Midi.Status command = (Midi.Status)(statusByte & 0xF0);
                    byte channel = (byte)(statusByte & 0x0F);

                    // 0xFE is Active Sensing (Keep-Alive). Ignore it early to avoid spam.
                    if (statusByte == 0xFE) return;

                    switch (command)
                    {
                        case Midi.Status.NoteOff:
                            Debug.WriteLine($"Note Off on ch {channel}. Key = {data1}, Vel = {data2}");
                            break;
                        case Midi.Status.NoteOn:
                            // Note: A NoteOn with velocity 0 is historically treated as a NoteOff
                            if (data2 == 0)
                                Debug.WriteLine($"Note Off (Vel 0) on ch {channel}. Key = {data1}");
                            else
                                Debug.WriteLine($"Note On on ch {channel}. Key = {data1}, Vel = {data2}");
                            break;
                        case Midi.Status.AfterTouch:
                            Debug.WriteLine($"Aftertouch on ch {channel}. Key = {data1}, Touch = {data2}");
                            break;
                        case Midi.Status.CC:
                            Debug.WriteLine($"CC on ch {channel}. Controller = {data1}, Value = {data2}");
                            break;
                        case Midi.Status.PC:
                            Debug.WriteLine($"PC on ch {channel}. Program = {data1}");
                            break;
                        case Midi.Status.Pressure:
                            Debug.WriteLine($"Channel Pressure on ch {channel}. Value = {data1}");
                            break;
                        case Midi.Status.PitchBend:
                            // Pitch bend is usually a 14-bit value combined from data1 and data2
                            int bendValue = (data2 << 7) | data1;
                            Debug.WriteLine($"Pitch Bend on ch {channel}. Value = {bendValue}");
                            break;
                        case Midi.Status.SystemEx:
                            Debug.WriteLine($"SystemEX/Realtime. Status = {statusByte:X2}");
                            break;
                        default:
                            Debug.WriteLine($"Unknown MIDI command {command} on ch {channel}. D1={data1}, D2={data2}");
                            break;
                    }
                    break;

                case Midi.Responses.MIM_OPEN:
                case Midi.Responses.MIM_CLOSE:
                case Midi.Responses.MIM_LONGDATA:
                case Midi.Responses.MIM_ERROR:
                case Midi.Responses.MIM_LONGERROR:
                case Midi.Responses.MIM_MOREDATA:
                    Debug.WriteLine($"wMsg = {wMsg}");
                    break;
                default:
                    Debug.WriteLine("wMsg = unknown");
                    break;
            }
        }
    }
}
