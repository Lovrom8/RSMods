using System.IO;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Rocksmith2014PsarcLib.Psarc;
using Rocksmith2014PsarcLib.Psarc.Models.Json;
using RSMods.Util;
using System.Windows.Forms;

namespace RSMods
{
    public static class SongManager
    {
        private static List<SongData> Songs = [];

        public static List<SongData> ExtractSongData(ProgressBar progressBar = null)
        {
            Songs.Clear();

            List<string> allFiles = Directory
                .GetFiles(Path.Combine(GenUtil.GetRSDirectory(), "dlc"), "*_p.psarc", SearchOption.AllDirectories)
                .Append(Path.Combine(GenUtil.GetRSDirectory(), "songs.psarc"))
                .ToList();

            if (progressBar != null)
            {
                progressBar.Visible = true;
                progressBar.Minimum = 1;
                progressBar.Maximum = allFiles.Count;
                progressBar.Value = 1;
                progressBar.Step = 1;
            }

            var rawArrangements = new ConcurrentBag<(SongArrangement Arrangement, bool IsODLC)>();

            Parallel.ForEach(allFiles, file =>
            {
                try
                {
                    using PsarcFile psarc = new(file);
                    bool isODLC = psarc.ExtractToolkitInfo().PackageAuthor == "Ubisoft";

                    foreach (SongArrangement arrangement in psarc.ExtractArrangementManifests())
                        rawArrangements.Add((arrangement, isODLC));
                }
                catch { }

                progressBar?.Invoke(() => progressBar.PerformStep());
            });

            Songs = rawArrangements
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

            if (progressBar != null)
            {
                progressBar.Visible = false;
                progressBar.Value = progressBar.Minimum;
            }

            return Songs;
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
