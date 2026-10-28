using RSMods.Data;
using RSMods.Util;
using SevenZip;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

namespace RSMods.SetAndForget
{
    /// <summary>The cache.psarc operations <see cref="CachePsarcService"/> needs. A seam for tests.</summary>
    internal interface ICacheArchive
    {
        bool Contains(string archivePath, string entryName);
        void Extract(string archivePath, string entryName, string targetFile);
        void Replace(string archivePath, IReadOnlyDictionary<string, string> filesByEntry);
    }

    /// <summary>
    /// Owns the <c>cache.psarc</c> lifecycle (backup, edit, restore, cleanup) and the archive injection/extraction
    /// primitives. Every cache mod runs through <see cref="Modify"/>, so the "extract → mutate files → write back →
    /// clean up" round-trip, backup creation, and mutation-failure handling live in exactly one place instead of being
    /// repeated (and quietly ignored) at each mod site.
    /// </summary>
    public sealed class CachePsarcService
    {
        private const string AggregateGraphEntry = "sltsv1_aggregategraph.nt";

        private readonly ICacheArchive _archive;

        public CachePsarcService()
            : this(new PsarcCacheArchive())
        {
        }

        internal CachePsarcService(ICacheArchive archive)
        {
            _archive = archive ?? throw new ArgumentNullException(nameof(archive));
        }

        /// <summary>
        /// Applies the caller's mutations to the cache, writes back the entries they changed, and deletes the extracted
        /// copies. Only the entries a mutation touches are extracted (cache7.7z and the like), and only the changed ones
        /// are replaced; the rest of cache.psarc is carried over as it is. Any failure inside <paramref name="mutate"/>
        /// (including an injection that did not take) aborts before anything is written and propagates to the caller,
        /// so a partially-applied mod can never be reported as success.
        /// <para>
        /// Every call starts from the cache.psarc on disk. Mods still stack, since each one is written into it, but
        /// old extracted files are never reused: they would re-apply mods the user has since removed with Restore
        /// Defaults or a Steam verify.
        /// </para>
        /// </summary>
        public void Modify(Action<CacheModification> mutate)
        {
            ArgumentNullException.ThrowIfNull(mutate);

            try
            {
                CacheModification cache = StartEdit();
                mutate(cache);
                WriteBack(cache.Changed);
            }
            finally
            {
                RemoveTempFolders();
            }
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

        /// <summary>
        /// Extracts the current tuning and localization files out of cache.psarc so the editor can import
        /// settings already applied to the game. Returns false if either extraction failed.
        /// </summary>
        public bool ImportExistingSettings()
        {
            try
            {
                CacheModification cache = StartEdit();

                bool tunings = cache.ExtractFile(Constants.Cache7_7zPath, Constants.TuningsJSON_InternalPath, Constants.CustomModsFolder);
                bool localization = cache.ExtractFile(Constants.Cache4_7zPath, Constants.LocalizationCSV_InternalPath, Constants.CustomModsFolder);

                return tunings && localization;
            }
            finally
            {
                RemoveTempFolders();
            }
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

        private CacheModification StartEdit()
        {
            RemoveTempFolders();
            Directory.CreateDirectory(Constants.CachePcPath);

            if (!File.Exists(Constants.CacheBackupPath))
                File.Copy(Constants.CachePsarcPath, Constants.CacheBackupPath);

            return new CacheModification(_archive);
        }

        private void WriteBack(IReadOnlyCollection<string> changedFiles)
        {
            if (changedFiles.Count == 0)
                return;

            Dictionary<string, string> filesByEntry = changedFiles.ToDictionary(CacheModification.EntryName);

            //NOTE: when adding resources, change Build Action to Embedded Resource
            if (!_archive.Contains(Constants.CachePsarcPath, AggregateGraphEntry))
            {
                GenUtil.ExtractEmbeddedResource(Constants.CachePcPath, typeof(CachePsarcService).Assembly, "RSMods.Core.Resources", [AggregateGraphEntry]);
                filesByEntry[AggregateGraphEntry] = Path.Combine(Constants.CachePcPath, AggregateGraphEntry);
            }

            _archive.Replace(Constants.CachePsarcPath, filesByEntry);
        }

        private sealed class PsarcCacheArchive : ICacheArchive
        {
            public bool Contains(string archivePath, string entryName) => PsarcPacker.ContainsEntry(archivePath, entryName);
            public void Extract(string archivePath, string entryName, string targetFile) => PsarcPacker.ExtractEntry(archivePath, entryName, targetFile);
            public void Replace(string archivePath, IReadOnlyDictionary<string, string> filesByEntry) => PsarcPacker.ReplaceEntries(archivePath, filesByEntry);
        }
    }

    /// <summary>
    /// The set of archive operations available inside a <see cref="CachePsarcService.Modify"/> call. Each
    /// operation throws when the underlying archive change did not take, turning a silently-ignored failure
    /// into an aborted (and surfaced) mod.
    /// </summary>
    public sealed class CacheModification
    {
        private readonly ICacheArchive _archive;
        private readonly HashSet<string> _extracted = new(StringComparer.OrdinalIgnoreCase);
        private readonly HashSet<string> _changed = new(StringComparer.OrdinalIgnoreCase);

        internal CacheModification(ICacheArchive archive)
        {
            _archive = archive;
        }

        /// <summary>The extracted files that were written to, which go back into cache.psarc.</summary>
        internal IReadOnlyCollection<string> Changed => _changed;

        /// <param name="archivePath">The extracted cache archive, e.g. <see cref="Constants.Cache7_7zPath"/>.</param>
        public void Inject(string sourceFilePath, string archivePath, string internalArchivePath)
        {
            Open(archivePath, forWriting: true);
            if (!ZipUtilities.InjectFile(sourceFilePath, archivePath, internalArchivePath, OutArchiveFormat.SevenZip, CompressionMode.Append))
                throw new IOException($"Failed to inject '{internalArchivePath}' into '{Path.GetFileName(archivePath)}'.");
        }

        /// <summary>Copies one file out of a cache archive into <paramref name="targetDirectory"/>. False when it isn't there.</summary>
        public bool ExtractFile(string archivePath, string internalArchivePath, string targetDirectory)
        {
            Open(archivePath, forWriting: false);
            return ZipUtilities.ExtractSingleFile(targetDirectory, archivePath, internalArchivePath);
        }

        /// <summary>Extracts a cache.psarc entry under <see cref="Constants.CachePcPath"/> the first time it's used.</summary>
        internal string Open(string extractedPath, bool forWriting)
        {
            if (_extracted.Add(extractedPath))
                _archive.Extract(Constants.CachePsarcPath, EntryName(extractedPath), extractedPath);
            if (forWriting)
                _changed.Add(extractedPath);

            return extractedPath;
        }

        internal static string EntryName(string extractedPath) =>
            Path.GetRelativePath(Constants.CachePcPath, extractedPath).Replace('\\', '/');
    }
}
