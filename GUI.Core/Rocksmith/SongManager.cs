using System;
using System.IO;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Rocksmith2014PsarcLib.Psarc;
using Rocksmith2014PsarcLib.Psarc.Models.Json;
using RSMods.Util;

namespace RSMods
{
    public static class SongManager
    {
        /// <summary>
        /// Reads Rocksmith song archives away from the calling UI thread. Progress is reported as a
        /// percentage from 0 through 100 so either frontend can present it with its native controls.
        /// </summary>
        public static Task<List<SongData>> ExtractSongDataAsync(
            IProgress<int> progress = null,
            CancellationToken cancellationToken = default)
        {
            return Task.Run(() => ExtractSongData(progress, cancellationToken), cancellationToken);
        }

        private static List<SongData> ExtractSongData(
            IProgress<int> progress,
            CancellationToken cancellationToken)
        {
            progress?.Report(0);

            List<string> allFiles = Directory
                .GetFiles(Path.Combine(GenUtil.GetRSDirectory(), "dlc"), "*_p.psarc", SearchOption.AllDirectories)
                .Append(Path.Combine(GenUtil.GetRSDirectory(), "songs.psarc"))
                .ToList();

            var rawArrangements = new ConcurrentBag<(SongArrangement Arrangement, bool IsODLC)>();
            var parallelOptions = new ParallelOptions { CancellationToken = cancellationToken };
            object progressLock = new object();
            int processedFiles = 0;

            Parallel.ForEach(allFiles, parallelOptions, file =>
            {
                try
                {
                    using PsarcFile psarc = new(file);
                    bool isODLC = psarc.ExtractToolkitInfo().PackageAuthor == "Ubisoft";

                    foreach (SongArrangement arrangement in psarc.ExtractArrangementManifests())
                        rawArrangements.Add((arrangement, isODLC));
                }
                catch { }

                lock (progressLock)
                {
                    processedFiles++;
                    progress?.Report(processedFiles * 100 / allFiles.Count);
                }
            });

            return rawArrangements
                .Where(x =>
                {
                    string name = $"{x.Arrangement.Attributes.ArtistName} - {x.Arrangement.Attributes.SongName}";
                    return name.Length > 0 && name != " - "; // Some songs have a glitched arrangement
                })
                .GroupBy(x => x.Arrangement.Attributes.SongKey)
                .Select(g =>
                {
                    var (first, isODLC) = g.First();
                    var song = new SongData
                    {
                        Arrangement = first,
                        Arrangements = g.Select(x => x.Arrangement).ToList(),
                        DLCKey = first.Attributes.SongKey,
                        Artist = first.Attributes.ArtistName,
                        Title = first.Attributes.SongName,
                        CommonName = $"{first.Attributes.ArtistName} - {first.Attributes.SongName}",
                        Shipping = first.Attributes.Shipping,
                        SKU = first.Attributes.SKU,
                        ODLC = isODLC,
                        ArrangementTypes = g.Select(x => x.Arrangement.Attributes.ArrangementType).ToList(),
                        Tunings = g.Select(x => x.Arrangement.Attributes.Tuning).ToList(),
                    };

                    if (isODLC && first.Attributes.SKU == "RS1" && first.Attributes.DLCRS1Key != null)
                        song.RS1AppID = first.Attributes.DLCRS1Key[0].WIN32;

                    return song;
                })
                .OrderBy(s => s.CommonName)
                .ToList();
        }
    }

    public class SongData
    {
        public SongArrangement Arrangement { get; set; } // Raw arrangement, just in-case we need to mess with it.
        public List<SongArrangement> Arrangements { get; set; } // List of arrangements
        public string DLCKey { get; set; }
        public string Artist { get; set; }
        public string Title { get; set; }
        public string CommonName { get; set; } // Artist - Title
        public bool Shipping { get; set; } // Should the song show up in game
        public bool ODLC { get; set; } // Is the file made by Ubisoft
        public string SKU { get; set; } // What game was this initially made for ("RS1", "RS2")
        public int RS1AppID { get; set; } // AppID from RS1CompatDLC
        public List<int> ArrangementTypes { get; set; } // Lead = 0, Rhythm = 1, Combo = 2, Bass = 3
        public List<SongArrangement.ArrangementAttributes.ArrangementTuning> Tunings { get; set; }
    }
}
