using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace RSMods
{
    /// <summary>The archives Rocksmith loads songs from: every DLC package and the on-disc songs.</summary>
    public static class SongArchives
    {
        public static IReadOnlyList<string> Enumerate(string rocksmithFolder)
        {
            var paths = new List<string>();

            string dlcFolder = Path.Combine(rocksmithFolder, "dlc");
            if (Directory.Exists(dlcFolder))
            {
                paths.AddRange(Directory.EnumerateFiles(dlcFolder, "*_p.psarc", SearchOption.AllDirectories));
            }

            string builtInSongs = Path.Combine(rocksmithFolder, "songs.psarc");
            if (File.Exists(builtInSongs))
            {
                paths.Add(builtInSongs);
            }

            return paths
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .OrderBy(path => path, StringComparer.OrdinalIgnoreCase)
                .ToArray();
        }
    }
}
