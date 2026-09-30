using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;

namespace RSMods
{
    /// <summary>
    /// The beats file the DLL's metronome reads: "#" header lines, then one "&lt;seconds&gt; &lt;1 if the beat
    /// starts a measure, else 0&gt;" line per beat. The DLL parses this format too, so change both together.
    /// </summary>
    public static class BeatMapFile
    {
        private const string Header = "# RSMods metronome beats v1";
        private const string SourcePrefix = "# source: ";

        public static void Write(string path, SongBeats song)
        {
            var lines = new List<string> { Header, SourcePrefix + song.ArchivePath };
            lines.AddRange(song.Beats.Select(FormatBeat));

            Directory.CreateDirectory(Path.GetDirectoryName(path));
            string temporaryPath = path + ".tmp";
            File.WriteAllLines(temporaryPath, lines);

            // The game may read the file at any moment; replacing it in one rename never shows it half written.
            File.Move(temporaryPath, path, overwrite: true);
        }

        /// <summary>The archive the beats in the file came from, or null.</summary>
        public static string ReadSourceArchive(string path)
        {
            if (!File.Exists(path))
                return null;

            string sourceLine = File.ReadLines(path)
                .TakeWhile(line => line.StartsWith('#'))
                .FirstOrDefault(line => line.StartsWith(SourcePrefix));
            return sourceLine?[SourcePrefix.Length..];
        }

        private static string FormatBeat(Beat beat)
        {
            int measureFlag = beat.StartsMeasure ? 1 : 0;
            return string.Create(CultureInfo.InvariantCulture, $"{beat.Seconds:R} {measureFlag}");
        }
    }
}
