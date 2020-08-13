using System;
using System.Collections.Generic;

namespace RSMods
{
    class GuitarSpeakFretboard
    {


        /*
            How to call:

            for (int i = 0; i < NoteToFretArray.Count(); i++)
            {
                if(NoteToFretArray[i].Item1 = currentNoteOctaveValue)
                {
                    // Note: If a value is -1 don't show it.
                    int lowBStringValue = NoteToFretArray[i].Item2;
                    int lowEStringValue = NoteToFretArray[i].Item3;
                    int AStringValue = NoteToFretArray[i].Item4;
                    int DStringValue = NoteToFretArray[i].Item5;
                    int GStringValue = NoteToFretArray[i].Item6;
                    int highBStringValue = NoteToFretArray[i].Item7;
                    int highEStringValue = NoteToFretArray[i].Item8;
                }
            }
         */

        public static List<Tuple<string, int, int, int, int, int, int, int>> SevenString_NoteToFretArray = new List<Tuple<string, int, int, int, int, int, int, int>>();

        public static void SevenString_FillNoteToFretArray() // -1 means that the note doesn't exist on this string
        {
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("B1", 0, -1, -1, -1, -1, -1, -1));

            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("C2", 1, -1, -1, -1, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("C#2", 2, -1, -1, -1, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("D2", 3, -1, -1, -1, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("Eb2", 4, -1, -1, -1, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("E2", 5, 0, -1, -1, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("F2", 6, 1, -1, -1, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("F#2", 7, 2, -1, -1, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("G2", 8, 3, -1, -1, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("Ab2", 9, 4, -1, -1, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("A2", 10, 5, 0, -1, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("Bb2", 11, 6, 1, -1, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("B2", 12, 7, 2, -1, -1, -1, -1));

            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("C3", 13, 8, 3, -1, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("C#3", 14, 9, 4, -1, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("D3", 15, 10, 5, 0, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("Eb3", 16, 11, 6, 1, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("E3", 17, 12, 7, 2, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("F3", 18, 13, 8, 3, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("F#3", 19, 14, 9, 4, -1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("G3", 20, 15, 10, 5, 0, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("Ab3", 21, 16, 11, 6, 1, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("A3", 22, 17, 12, 7, 2, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("Bb3", 23, 18, 13, 8, 3, -1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("B3", 24, 19, 14, 9, 4, 0, -1));

            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("C4", -1, 20, 15, 10, 5, 1, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("C#4", -1, 21, 16, 11, 6, 2, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("D4", -1, 22, 17, 12, 7, 3, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("Eb4", -1, 23, 18, 13, 8, 4, -1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("E4", -1, 24, 19, 14, 9, 5, 0));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("F4", -1, -1, 20, 15, 10, 6, 1));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("F#4", -1, -1, 21, 16, 11, 7, 2));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("G4", -1, -1, 22, 17, 12, 8, 3));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("Ab4", -1, -1, 23, 18, 13, 9, 4));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("A4", -1, -1, 24, 19, 14, 10, 5));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("Bb4", -1, -1, -1, 20, 15, 11, 6));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("B4", -1, -1, -1, 21, 16, 12, 7));

            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("C5", -1, -1, -1, 22, 17, 13, 8));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("C#5", -1, -1, -1, 23, 18, 14, 9));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("D5", -1, -1, -1, 24, 19, 15, 10));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("Eb5", -1, -1, -1, -1, 20, 16, 11));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("E5", -1, -1, -1, -1, 21, 17, 12));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("F5", -1, -1, -1, -1, 22, 18, 13));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("F#5", -1, -1, -1, -1, 23, 19, 14));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("G5", -1, -1, -1, -1, 24, 20, 15));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("Ab5", -1, -1, -1, -1, -1, 21, 16));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("A5", -1, -1, -1, -1, -1, 22, 17));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("Bb5", -1, -1, -1, -1, -1, 23, 18));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("B5", -1, -1, -1, -1, -1, 24, 19));

            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("C6", -1, -1, -1, -1, -1, -1, 20));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("C#6", -1, -1, -1, -1, -1, -1, 21));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("D6", -1, -1, -1, -1, -1, -1, 22));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("Eb6", -1, -1, -1, -1, -1, -1, 23));
            SevenString_NoteToFretArray.Add(new Tuple<string, int, int, int, int, int, int, int>("E6", -1, -1, -1, -1, -1, -1, 24));
        }

        public static Dictionary<int, int> FretToXValue = new Dictionary<int, int>()
        {
            {0, 0}, // Open String

            {1, 1},
            {2, 2},
            {3, 3},
            {4, 4},
            {5, 5},
            {6, 6},
            {7, 7},
            {8, 8},
            {9, 9},
            {10, 10},
            {11, 11},
            {12, 12}, // Octave

            {13, 13},
            {14, 14},
            {15, 15},
            {16, 16},
            {17, 17},
            {18, 18},
            {19, 19},
            {20, 20},
            {21, 21},
            {22, 22},
            {23, 23},
            {24, 24} // 2 Octaves
        };
        public static Dictionary<int, int> StringToYValue = new Dictionary<int, int>()
        {
            {0, 0},
            {1, 1},
            {2, 2},
            {3, 3},
            {4, 4},
            {5, 5},
            {6, 6}
        };
    }

    
}
