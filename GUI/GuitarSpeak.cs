namespace RSMods
{
    static class GuitarSpeak
    {
        public static string GuitarSpeakNoteOctaveMath(string inputString)
        {
            if (inputString.Length == 0)
                return "";

            int inputInt = int.Parse(inputString);

            int octave = (inputInt / 12) - 1; // We support the -1st octave, so we need to minus 1 from our octave.

            return MidiToNoteName(inputInt) + octave.ToString();
        }

        public static string MidiToNoteName(int midiNoteToConvert) => noteArray[midiNoteToConvert % 12];

        public static readonly string[] noteArray = ["C", "C♯", "D", "E♭", "E", "F", "F♯", "G", "A♭", "A", "B♭", "B"];
    }
}
