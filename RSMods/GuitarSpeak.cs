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

            return IntToNote(inputInt) + octave.ToString();
        }

        public static string IntToNote(int intToConvert) => noteArray[intToConvert % 12];

        public static string[] noteArray = new string[12] { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    }
}
