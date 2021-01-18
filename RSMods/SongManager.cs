using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Rocksmith2014PsarcLib.Psarc;
using RSMods.Util;
using System.Windows.Forms;
using System.Collections.Concurrent;
using System.Drawing;

namespace RSMods
{
    public class SongManager
    {
        public static List<SongData> Songs = new List<SongData>();
        public static List<SongData> ExtractSongData(ProgressBar progressBar = null)
        {
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
                        List<Rocksmith2014PsarcLib.Psarc.Models.Json.SongArrangement.ArrangementAttributes.ArrangementTuning> tunings = new List<Rocksmith2014PsarcLib.Psarc.Models.Json.SongArrangement.ArrangementAttributes.ArrangementTuning>();

                        List<Rocksmith2014PsarcLib.Psarc.Models.Json.SongArrangement> ExtractedArrangementManifests = psarc.ExtractArrangementManifests();

                        foreach (Rocksmith2014PsarcLib.Psarc.Models.Json.SongArrangement arrangement in ExtractedArrangementManifests)
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

                            if (song.CommonName == String.Empty || song.CommonName == " - ")
                                continue;

                            Console.WriteLine(song.CommonName);
                            
                            
                            arrangementTypes.Add(arrangement.Attributes.ArrangementType);
                            tunings.Add(arrangement.Attributes.Tuning);

                            if (psarc.ExtractToolkitInfo().PackageAuthor == "Ubisoft")
                            {
                                song.ODLC = true;
                                if (song.arrangement.Attributes.SKU == "RS1" && song.arrangement.Attributes.DLCRS1Key != null) // Load all RS1 DLC and their ID so we can determine if the user owns it, and if we should display it accordingly.
                                    song.RS1AppID = song.arrangement.Attributes.DLCRS1Key[0].WIN32;
                            }   

                            // If Song Name Exists -> Add To Current Values -> Return Updated Song
                            if (Songs.Exists(songData => songData.DLCKey == song.DLCKey))
                            {
                                int index = Songs.IndexOf(new SongData { DLCKey = song.DLCKey });

                                if (index > -1)
                                {
                                    SongData previousIteration = Songs[index];
                                    previousIteration.ArrangementTypes.AddRange(song.ArrangementTypes);
                                    previousIteration.Tunings.AddRange(song.Tunings);

                                    Songs[index] = previousIteration;
                                }
                            }
                            else
                            {
                                song.ArrangementTypes = arrangementTypes;
                                song.Tunings = tunings;
                                Songs.Add(song);
                            }
                        }
                    }
                }
                catch (Exception e)
                {
                    MessageBox.Show($"Unable to read {file}\n{e.Message}\n{e.StackTrace}");
                }
                    
            });

            if (progressBarAvailable)
            {
                progressBar.Visible = false;
                progressBar.Value = progressBar.Minimum;
            }

            return Songs;
        }
    }

    public class SongData
    {
        public Rocksmith2014PsarcLib.Psarc.Models.Json.SongArrangement arrangement { get; set; } // Raw arrangment, just in-case we need to mess with it.
        public string DLCKey { get; set; }
        public string Artist { get; set; }
        public string Title { get; set; }
        public string CommonName { get; set; } // Artist - Title
        public bool Shipping { get; set; } // Should the song show up in game
        public bool ODLC { get; set; } // Is the file made by Ubisoft
        public string SKU { get; set; } // What game was this initially made for ("RS1", "RS2")
        public int RS1AppID { get; set; } // AppID from RS1CompatDLC
        public List<int> ArrangementTypes { get; set; }
        public List<Rocksmith2014PsarcLib.Psarc.Models.Json.SongArrangement.ArrangementAttributes.ArrangementTuning> Tunings { get; set; }
    }
}
