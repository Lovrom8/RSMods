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

        public static Dictionary<string, string> DLCKeyToSongName()
        {
            Dictionary<string, string> dlcKeyToSongName = new Dictionary<string, string>();
            List<string> allDLCKeys = new List<string>(), allSongNames = new List<string>();
            List<string> allFiles = Directory.GetFiles(Path.Combine(GenUtil.GetRSDirectory(), "dlc"), "*.psarc", SearchOption.AllDirectories).ToList();
            allFiles.Add(Path.Combine(GenUtil.GetRSDirectory(), "songs.psarc"));


            foreach (string file in allFiles)
            {
                try
                {
                    using (PsarcFile psarc = new PsarcFile(file))
                    {
                        foreach(Rocksmith2014PsarcLib.Psarc.Models.Json.SongArrangement arrangement in psarc.ExtractArrangementManifests())
                        {
                            if (!allDLCKeys.Contains(arrangement.Attributes.SongKey) && arrangement.Attributes.ArrangementType != 0) // Prevent multiple arrangements from adding the same SongKey. ArrangementType of 0 report no song name / artist
                            {
                                allDLCKeys.Add(arrangement.Attributes.SongKey);
                                allSongNames.Add($"{arrangement.Attributes.ArtistName} - {arrangement.Attributes.SongName}");
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
            }

            dlcKeyToSongName = allDLCKeys.Zip(allSongNames, (k, v) => new { Key = k, Value = v }).ToDictionary(x => x.Key, x => x.Value);

            return dlcKeyToSongName;
        }
    }
}
