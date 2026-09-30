using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace RSMods
{
    /// <summary>
    /// <c>RSMods.exe --extract-beats &lt;rocksmithFolder&gt; &lt;songKey&gt; &lt;outputFile&gt;</c>. The DLL's metronome
    /// runs this headless when a song is picked, to get the song's beats written where it will read them.
    /// </summary>
    public static class ExtractBeatsCommand
    {
        public const string Name = "--extract-beats";

        public const int Success = 0;
        public const int SongNotFound = 1;
        public const int BadArguments = 2;
        public const int Failed = 3;

        private const int ArgumentCount = 4;

        public static bool TryRun(string[] args, out int exitCode)
        {
            exitCode = Success;
            if (args.Length == 0 || args[0] != Name)
                return false;

            exitCode = args.Length == ArgumentCount ? Run(args[1], args[2], args[3]) : BadArguments;
            return true;
        }

        private static int Run(string rocksmithFolder, string songKey, string outputFile)
        {
            try
            {
                IEnumerable<string> archives = PreviousSourceFirst(outputFile, SongArchives.Enumerate(rocksmithFolder));
                SongBeats song = new BeatMapExtractor().FindSongBeats(archives, songKey);
                if (song == null)
                    return SongNotFound;

                BeatMapFile.Write(outputFile, song);
                return Success;
            }
            catch (Exception)
            {
                return Failed;
            }
        }

        // A song is almost always still in the archive it was found in last time, which spares scanning the library.
        private static IEnumerable<string> PreviousSourceFirst(string outputFile, IReadOnlyList<string> archives)
        {
            string previousSource = BeatMapFile.ReadSourceArchive(outputFile);
            if (previousSource == null || !File.Exists(previousSource))
                return archives;

            return archives.Prepend(previousSource).Distinct(StringComparer.OrdinalIgnoreCase);
        }
    }
}
