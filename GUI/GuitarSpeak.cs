namespace RSMods
{
    static class GuitarSpeak
    {
        public static string GuitarSpeakNoteOctaveMath(string inputString)
        {
            if (!int.TryParse(inputString, out int inputInt))
                return "";

            int octave = (inputInt / 12) - 1; // We support the -1st octave, so we need to minus 1 from our octave.

            return MidiToNoteName(inputInt) + octave.ToString();
        }

        // Floored modulo so negative note numbers don't index out of bounds (C# % can be negative).
        public static string MidiToNoteName(int midiNoteToConvert) => noteArray[((midiNoteToConvert % 12) + 12) % 12];

        public static readonly string[] noteArray = ["C", "C♯", "D", "E♭", "E", "F", "F♯", "G", "A♭", "A", "B♭", "B"];
    }
}
