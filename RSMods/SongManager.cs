using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Rocksmith2014PsarcLib.Psarc;
using RSMods.Util;
using System.Windows.Forms;

namespace RSMods
{
    public class SongManager
    {

        public static Dictionary<string, string> ODLCArray = new Dictionary<string, string>();
        public static Dictionary<string, int> RS1DLCArray = new Dictionary<string, int>();

        public static Dictionary<string, string> DLCKeyToSongName(ProgressBar progressBar = null)
        {
            bool progressBarAvailable = progressBar != null;
            Dictionary<string, string> dlcKeyToSongName = new Dictionary<string, string>();
            List<string> allDLCKeys = new List<string>(), allSongNames = new List<string>();

            List<string> allFiles = Directory.GetFiles(Path.Combine(GenUtil.GetRSDirectory(), "dlc"), "*.psarc", SearchOption.AllDirectories).ToList();
            allFiles.Add(Path.Combine(GenUtil.GetRSDirectory(), "songs.psarc"));

            if (progressBarAvailable)
            {
                progressBar.Visible = true;
                progressBar.Minimum = 1;
                progressBar.Maximum = allFiles.Count;
                progressBar.Value = 1;
                progressBar.Step = 1;
            }

            foreach (string file in allFiles)
            {
                try
                {
                    using (PsarcFile psarc = new PsarcFile(file))
                    {
                        foreach(Rocksmith2014PsarcLib.Psarc.Models.Json.SongArrangement arrangement in psarc.ExtractArrangementManifests())
                        {

                            if (!allDLCKeys.Contains(arrangement.Attributes.SongKey) && arrangement.Attributes.ArrangementType != 0 && arrangement.Attributes.Shipping) // Prevent multiple arrangements from adding the same SongKey. ArrangementType of 0 report no song name / artist. Shipping removes hidden demo songs.
                            {
                                allDLCKeys.Add(arrangement.Attributes.SongKey);
                                allSongNames.Add($"{arrangement.Attributes.ArtistName} - {arrangement.Attributes.SongName}");
                                
                                if (psarc.ExtractToolkitInfo().PackageAuthor == "Ubisoft")
                                {
                                    ODLCArray.Add(arrangement.Attributes.SongKey, $"{arrangement.Attributes.ArtistName} - {arrangement.Attributes.SongName}");
                                    if (arrangement.Attributes.SKU == "RS1" && arrangement.Attributes.DLCRS1Key != null) // Load all RS1 DLC and their ID so we can determine if the user owns it, and if we should display it accordingly.
                                        RS1DLCArray.Add(arrangement.Attributes.SongKey, arrangement.Attributes.DLCRS1Key[0].WIN32);
                                } 
                            }
                        } 
                    }
                }
                catch (Exception e)
                {
                    Console.WriteLine($"Unable to read {file}");
                    Console.WriteLine(e.Message);
                    Console.WriteLine(e.StackTrace);
                }

                if (progressBarAvailable)
                    progressBar.PerformStep();
            }

            if (progressBarAvailable)
            {
                progressBar.Visible = false;
                progressBar.Value = progressBar.Minimum;
            }
                
                

            dlcKeyToSongName = allDLCKeys.Zip(allSongNames, (k, v) => new { Key = k, Value = v }).ToDictionary(x => x.Key, x => x.Value);

            return dlcKeyToSongName;
        }
    }
}
