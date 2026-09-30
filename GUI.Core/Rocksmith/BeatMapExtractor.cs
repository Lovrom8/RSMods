using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Rocksmith2014PsarcLib.Psarc;
using Rocksmith2014PsarcLib.Psarc.Asset;
using Rocksmith2014PsarcLib.Psarc.Models.Sng;

namespace RSMods
{
    public sealed record Beat(float Seconds, bool StartsMeasure);

    public sealed record SongBeats(string ArchivePath, IReadOnlyList<Beat> Beats);

    /// <summary>Reads a song's beat grid (the SNG "ebeats") out of the Rocksmith archives.</summary>
    public sealed class BeatMapExtractor
    {
        private const string SngFolder = "songs/bin/generic/";
        private const string SngExtension = ".sng";

        /// <summary>The beats from the first archive that holds the song, or null when none does.</summary>
        public SongBeats FindSongBeats(IEnumerable<string> archivePaths, string songKey)
        {
            foreach (string archivePath in archivePaths)
            {
                IReadOnlyList<Beat> beats = TryReadSongBeats(archivePath, songKey);
                if (beats.Count > 0)
                    return new SongBeats(archivePath, beats);
            }

            return null;
        }

        /// <summary>Empty when the archive doesn't hold the song.</summary>
        public IReadOnlyList<Beat> ReadSongBeats(string archivePath, string songKey)
        {
            using var psarc = new PsarcFile(archivePath);
            IReadOnlyList<Beat>[] arrangementBeats = psarc.TOC.Entries
                .Where(entry => IsArrangementOf(entry.Path, songKey))
                .Select(entry => ToBeats(psarc.InflateEntry<SngAsset>(entry).BPMs))
                .ToArray();

            return Longest(arrangementBeats);
        }

        /// <summary>
        /// Arrangements of one song share a beat grid, apart from a beat or two more at the end in some of them,
        /// so the longest one covers the whole song. Vocals have no beats and never win.
        /// </summary>
        public static IReadOnlyList<Beat> Longest(IEnumerable<IReadOnlyList<Beat>> arrangementBeats) =>
            arrangementBeats.MaxBy(beats => beats.Count) ?? [];

        /// <summary>
        /// SNGs are named "&lt;songKey&gt;_&lt;arrangement&gt;.sng" and the arrangement part has no underscore,
        /// which keeps "song_live_lead" from matching the key "song".
        /// </summary>
        public static bool IsArrangementOf(string entryPath, string songKey)
        {
            bool isSng = entryPath != null
                && entryPath.StartsWith(SngFolder, StringComparison.OrdinalIgnoreCase)
                && entryPath.EndsWith(SngExtension, StringComparison.OrdinalIgnoreCase);
            if (!isSng)
                return false;

            string name = Path.GetFileNameWithoutExtension(entryPath);
            int arrangementSeparator = name.LastIndexOf('_');
            return arrangementSeparator > 0
                && string.Equals(name[..arrangementSeparator], songKey, StringComparison.OrdinalIgnoreCase);
        }

        // Beat counts from 0 within each measure.
        private static IReadOnlyList<Beat> ToBeats(Bpm[] bpms) =>
            bpms.Select(bpm => new Beat(bpm.Time, StartsMeasure: bpm.Beat == 0)).ToArray();

        // An archive that can't be read (damaged, or a format the library doesn't know) must not end the search:
        // it is almost never the song being looked for.
        private IReadOnlyList<Beat> TryReadSongBeats(string archivePath, string songKey)
        {
            try
            {
                return ReadSongBeats(archivePath, songKey);
            }
            catch (Exception)
            {
                return [];
            }
        }
    }
}
