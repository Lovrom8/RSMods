using RocksmithToolkitLib.DLCPackage;
using RSMods.Data;
using RSMods.Util;
using SevenZip;
using System;
using System.IO;

namespace RSMods.SetAndForget
{
    /// <summary>
    /// Owns the <c>cache.psarc</c> lifecycle (unpack, backup, repack, restore, cleanup) and the
    /// archive injection/extraction primitives. Every cache mod runs through <see cref="Modify"/>, so the
    /// "ensure unpacked → mutate files → repack" round-trip, backup creation, and mutation-failure handling
    /// live in exactly one place instead of being repeated (and quietly ignored) at each mod site.
    /// </summary>
    public sealed class CachePsarcService
    {
        /// <summary>
        /// Ensures the cache is unpacked, applies the caller's mutations, then repacks. Any failure inside
        /// <paramref name="mutate"/> (including an injection that did not take) aborts before the repack and
        /// propagates to the caller, so a partially-applied mod can never be reported as success.
        /// </summary>
        public void Modify(Action<CacheModification> mutate)
        {
            ArgumentNullException.ThrowIfNull(mutate);

            EnsureUnpacked();
            mutate(new CacheModification());
            Repack();
        }

        public void AddCustomTunings(TuningService tuning)
        {
            ArgumentNullException.ThrowIfNull(tuning);

            Modify(cache =>
            {
                tuning.AddLocalizationEntries(
                    Constants.LocalizationCSV_CustomPath,
                    Constants.TuningJSON_CustomPath);

                cache.Inject(Constants.TuningJSON_CustomPath, Constants.Cache7_7zPath, Constants.TuningsJSON_InternalPath);
                cache.Inject(Constants.LocalizationCSV_CustomPath, Constants.Cache4_7zPath, Constants.LocalizationCSV_InternalPath);
            });
        }

        public void AddExitGameMenuOption() =>
            Modify(cache => cache.Inject(
                Constants.MainMenuJson_CustomPath, Constants.Cache7_7zPath, Constants.MainMenuJson_InternalPath));

        public void AddDirectConnectModeOption() =>
            Modify(cache =>
            {
                cache.Inject(Constants.ExtendedMenuJson_CustomPath, Constants.Cache7_7zPath, Constants.ExtendedMenuJson_InternalPath);
                cache.Inject(Constants.DirectConnectStartupJson_CustomPath, Constants.Cache7_7zPath, Constants.DirectConnectStartupJson_InternalPath);
            });

        public void AddIncreasedVolumeWwiseBank() =>
            Modify(cache => cache.Inject(
                Constants.WwiseInitBnk_CustomPath, Constants.Cache3_7zPath, Constants.WwiseInitBnk_InternalPath));

        /// <summary>
        /// Restores the original cache archive from the backup created during the first unpack. Confirmation
        /// and result presentation belong to the frontend; returning false means no backup was available.
        /// </summary>
        public bool RestoreDefaults()
        {
            if (!File.Exists(Constants.CacheBackupPath))
                return false;

            File.Copy(Constants.CacheBackupPath, Constants.CachePsarcPath, true);
            GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, typeof(CachePsarcService).Assembly, "RSMods.Core.Resources", ["tuning.database.json"]);
            //TODO: extract the rest

            return true;
        }

        public void CleanUnpackedCache()
        {
            if (!Directory.Exists(Constants.CachePcPath))
                return;

            ZipUtilities.DeleteDirectory(Constants.CachePcPath, true);

            Unpack();
        }

        /// <summary>
        /// Extracts the current tuning and localization files out of the unpacked cache so the editor can
        /// import settings already applied to the game. Returns false if either extraction failed.
        /// </summary>
        public bool ImportExistingSettings()
        {
            if (!File.Exists(Constants.Cache4_7zPath) || !File.Exists(Constants.Cache7_7zPath))
                Unpack();

            bool tunings = ZipUtilities.ExtractSingleFile(
                Constants.CustomModsFolder, Constants.Cache7_7zPath, Constants.TuningsJSON_InternalPath);
            bool localization = ZipUtilities.ExtractSingleFile(
                Constants.CustomModsFolder, Constants.Cache4_7zPath, Constants.LocalizationCSV_InternalPath);

            return tunings && localization;
        }

        public void RemoveTempFolders()
        {
            ZipUtilities.DeleteDirectory(Constants.WorkFolder);
        }

        public void LoadDefaultFiles()
        {
            EnsureResource(Constants.TuningJSON_CustomPath, "tuning.database.json");
            EnsureResource(Constants.IntroGFX_MidPath, "introsequence_mid.gfx");
            EnsureResource(Constants.IntroGFX_MaxPath, "introsequence_max.gfx");
            EnsureResource(Constants.LocalizationCSV_CustomPath, "maingame.csv");
            EnsureResource(Constants.ExtendedMenuJson_CustomPath, "ui_menu_pillar_mission.database.json");
            EnsureResource(Constants.MainMenuJson_CustomPath, "ui_menu_pillar_main.database.json");
            EnsureResource(Constants.DirectConnectStartupJson_CustomPath, "ui_menu_pillar_startup.database.json");
            EnsureResource(Constants.WwiseInitBnk_CustomPath, "init.bnk");
        }

        private static void EnsureResource(string destinationPath, string resourceName)
        {
            if (!File.Exists(destinationPath))
                GenUtil.ExtractEmbeddedResource(Constants.CustomModsFolder, typeof(CachePsarcService).Assembly, "RSMods.Core.Resources", [resourceName]);
        }

        private static void EnsureUnpacked()
        {
            // Don't replace an existing unpacked cache, in case the user wants to stack several mods together.
            if (!Directory.Exists(Constants.CachePcPath) || GenUtil.IsDirectoryEmpty(Constants.CachePcPath))
                Unpack();
        }

        private static void Unpack()
        {
            if (!Directory.Exists(Constants.WorkFolder))
                Directory.CreateDirectory(Constants.WorkFolder);

            if (!File.Exists(Constants.CacheBackupPath))
                File.Copy(Constants.CachePsarcPath, Constants.CacheBackupPath);

            Packer.Unpack(Constants.CachePsarcPath, Constants.WorkFolder);
        }

        private static void Repack()
        {
            if (!Directory.Exists(Constants.CachePcPath))
                Unpack();

            //NOTE: when adding resources, change Build Action to Embedded Resource
            if (!File.Exists(Path.Combine(Constants.CachePcPath, "sltsv1_aggregategraph.nt")))
                GenUtil.ExtractEmbeddedResource(Constants.CachePcPath, typeof(CachePsarcService).Assembly, "RSMods.Core.Resources", ["sltsv1_aggregategraph.nt"]);

            Packer.Pack(Constants.CachePcPath, Constants.CachePsarcPath);
        }
    }

    /// <summary>
    /// The set of archive operations available inside a <see cref="CachePsarcService.Modify"/> call. Each
    /// operation throws when the underlying archive change did not take, turning a silently-ignored failure
    /// into an aborted (and surfaced) mod.
    /// </summary>
    public sealed class CacheModification
    {
        internal CacheModification()
        {
        }

        public void Inject(string sourceFilePath, string archivePath, string internalArchivePath)
        {
            if (!ZipUtilities.InjectFile(sourceFilePath, archivePath, internalArchivePath, OutArchiveFormat.SevenZip, CompressionMode.Append))
                throw new IOException($"Failed to inject '{internalArchivePath}' into '{Path.GetFileName(archivePath)}'.");
        }
    }
}
