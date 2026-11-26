using RSMods.Data;
using System;
using System.Collections.Generic;
using System.IO;

namespace RSMods.SetAndForget
{
    /// <summary>
    /// Replaces files inside <c>static.psarc</c>, for game files that cache.psarc can't override. The untouched
    /// archive is backed up before the first edit so Restore Defaults can put it back. The backup has its own name
    /// because other tools already leave a <c>static.bak</c> in the game folder.
    /// </summary>
    public sealed class StaticPsarcService
    {
        private readonly ICacheArchive _archive;

        internal StaticPsarcService(ICacheArchive archive)
        {
            _archive = archive ?? throw new ArgumentNullException(nameof(archive));
        }

        /// <param name="internalArchivePath">The file's path inside the archive, e.g. gfxassets\views\x.gfx.</param>
        public void Replace(string sourceFilePath, string internalArchivePath)
        {
            if (!File.Exists(Constants.StaticBackupPath))
                File.Copy(Constants.StaticPsarcPath, Constants.StaticBackupPath);

            string entryName = internalArchivePath.Replace('\\', '/');
            _archive.Replace(Constants.StaticPsarcPath, new Dictionary<string, string> { [entryName] = sourceFilePath });
        }

        /// <summary>Puts the backed-up static.psarc back. False when RSMods never changed it.</summary>
        public bool RestoreDefaults()
        {
            if (!File.Exists(Constants.StaticBackupPath))
                return false;

            File.Copy(Constants.StaticBackupPath, Constants.StaticPsarcPath, true);
            return true;
        }
    }
}
