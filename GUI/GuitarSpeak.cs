using System;

namespace RSMods
{
    class GuitarSpeak
    {
        public static string GuitarSpeakNoteOctaveMath(string inputString)
        {
            if (inputString == "")
                return "";

            int inputInt = Int32.Parse(inputString);

            int octave = (inputInt / 12) - 1; // We support the -1st octave, so we need to minus 1 from our octave.

            return MidiToNoteName(inputInt) + octave.ToString();
        }

        public static string MidiToNoteName(int midiNoteToConvert) => noteArray[midiNoteToConvert % 12];

        public static string[] noteArray = new string[12] { "C", "C♯", "D", "E♭", "E", "F", "F♯", "G", "A♭", "A", "B♭", "B" };
    }
}
