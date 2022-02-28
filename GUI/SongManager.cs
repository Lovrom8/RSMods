using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Rocksmith2014PsarcLib.Psarc;
using Rocksmith2014PsarcLib.Psarc.Models.Json;
using RSMods.Util;
using System.Windows.Forms;

namespace RSMods
{
    public class SongManager
    {
        public static List<SongData> Songs = new List<SongData>();
        public static List<SongData> ExtractSongData(ProgressBar progressBar = null)
        {
            Songs.Clear();
            bool progressBarAvailable = progressBar != null;
            List<string> allFiles = Directory.GetFiles(Path.Combine(GenUtil.GetRSDirectory(), "dlc"), "*_p.psarc", SearchOption.AllDirectories).ToList();
            allFiles.Add(Path.Combine(GenUtil.GetRSDirectory(), "songs.psarc"));

            if (progressBarAvailable)
            {
                progressBar.Visible = true;
                progressBar.Minimum = 1;
                progressBar.Maximum = allFiles.Count;
                progressBar.Value = 1;
                progressBar.Step = 1;
            }

            ParallelLoopResult loopResult = Parallel.ForEach<string>(allFiles, (file) =>
            {
                try
                {
                    using (PsarcFile psarc = new PsarcFile(file))
                    {

                        List<int> arrangementTypes = new List<int>();
                        List<SongArrangement.ArrangementAttributes.ArrangementTuning> tunings = new List<SongArrangement.ArrangementAttributes.ArrangementTuning>();

                        List<SongArrangement> ExtractedArrangementManifests = psarc.ExtractArrangementManifests();

                        foreach (SongArrangement arrangement in ExtractedArrangementManifests)
                        {
                            SongData song = new SongData()
                            {
                                arrangement = arrangement,
                                DLCKey = arrangement.Attributes.SongKey,
                                Artist = arrangement.Attributes.ArtistName,
                                Title = arrangement.Attributes.SongName,
                                Shipping = arrangement.Attributes.Shipping,
                                SKU = arrangement.Attributes.SKU,
                                CommonName = $"{arrangement.Attributes.ArtistName} - {arrangement.Attributes.SongName}"
                            };

                            if (song.CommonName == String.Empty || song.CommonName == " - ") // Some songs have a glitched arrangment, so we skip it.
                                continue;
                            
                            arrangementTypes.Add(arrangement.Attributes.ArrangementType);
                            tunings.Add(arrangement.Attributes.Tuning);

                            // Load all RS1 DLC and their ID so we can determine if the user owns it, and if we should display it accordingly.
                            if (psarc.ExtractToolkitInfo().PackageAuthor == "Ubisoft")
                            {
                                song.ODLC = true;
                                if (song.arrangement.Attributes.SKU == "RS1" && song.arrangement.Attributes.DLCRS1Key != null) 
                                    song.RS1AppID = song.arrangement.Attributes.DLCRS1Key[0].WIN32;
                            }

                            Songs.RemoveAll(songData => songData == null); // If this isn't run then we can end up with some blank songs getting passed in and crashing the app.

                            // If Song Name Exists -> Add To Current Values -> Return Updated Song
                            if (Songs.Exists(songData => songData != null && Equals(songData.DLCKey, song.DLCKey)))
                            {
                                int index = Songs.IndexOf(new SongData { DLCKey = song.DLCKey });

                                if (index != -1)
                                {
                                    SongData previousIteration = Songs[index];
                                    previousIteration.arrangements.Add(arrangement);
                                    previousIteration.ArrangementTypes.AddRange(song.ArrangementTypes);
                                    previousIteration.Tunings.AddRange(song.Tunings);
                                    Songs[index] = previousIteration;
                                }
                                else
                                {
                                    song.ArrangementTypes = arrangementTypes;
                                    song.Tunings = tunings;
                                    song.arrangements = new List<SongArrangement>();
                                    song.arrangements.Add(song.arrangement);
                                    Songs.Add(song);
                                }
                            }

                            // Song Doesn't Exist In Our List
                            else
                            {
                                song.ArrangementTypes = arrangementTypes;
                                song.Tunings = tunings;
                                song.arrangements = new List<SongArrangement>();
                                song.arrangements.Add(song.arrangement);
                                Songs.Add(song);
                            }
                        }
                    }
                }
                catch (Exception e)
                {
                }
                    
            });

            // Make sure we remove duplicate instances of the songs.
            // We need to make our own Comparer class for it, since Equals() and GetHashCode() won't work for our custom type.
            Songs = Songs.Distinct(new SongDataComparer()).ToList();

            Songs.Sort(CompareSongs);

            if (progressBarAvailable)
            {
                progressBar.Visible = false;
                progressBar.Value = progressBar.Minimum;
            }

            return Songs;
        }

        private static int CompareSongs(SongData x, SongData y)
        {
            if (x == null)
            {
                if (y == null)
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                if (y == null)
                {
                    return 1;
                }
                else
                {
                    return x.CommonName.CompareTo(y.CommonName); //
                }
            }
        }
    }

    public class SongData
    {
        public SongArrangement arrangement { get; set; } // Raw arrangment, just in-case we need to mess with it.
        public List<SongArrangement> arrangements { get; set; } // List of arrangements
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

    public class SongDataComparer : IEqualityComparer<SongData>
    {
        public bool Equals(SongData x, SongData y)
        {
            if (x == null && y == null)
                return true;
            if (x == null || y == null)
                return false;

            if (x.DLCKey == y.DLCKey)
                return true;
            else
                return false;
        }

        public int GetHashCode(SongData obj)
        {
            return obj.DLCKey.GetHashCode();
        }
    }

    
}
