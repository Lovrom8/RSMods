using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Rocksmith2014PsarcLib.Psarc;
using Rocksmith2014PsarcLib.Psarc.Models.Json;
using RSMods.Util;

namespace RSMods
{
    public sealed class SongCatalogResult(IReadOnlyList<SongData> songs, IReadOnlyList<SongCatalogWarning> warnings)
    {
        public IReadOnlyList<SongData> Songs { get; } = songs;
        public IReadOnlyList<SongCatalogWarning> Warnings { get; } = warnings;
    }

    public sealed record SongCatalogWarning(string ArchivePath, string Message);

    /// <summary>
    /// Builds and caches the Rocksmith song catalog. Archive parsing is synchronous in the bundled PSARC
    /// library, so scans run on a worker thread with bounded archive parallelism.
    /// </summary>
    public sealed class SongCatalogService
    {
        private const int MaxConcurrentArchiveReads = 2;

        private readonly SemaphoreSlim _scanGate = new(1, 1);
        private readonly object _cacheLock = new();
        private string _cachedRocksmithFolder = string.Empty;
        private SongCatalogResult _cachedCatalog;

        public async Task<SongCatalogResult> GetSongsAsync(bool forceRefresh = false, IProgress<int> progress = null, CancellationToken cancellationToken = default)
        {
            string rocksmithFolder = ResolveRocksmithFolder();
            SongCatalogResult cached = GetCachedCatalog(rocksmithFolder, forceRefresh);
            if (cached != null)
            {
                progress?.Report(100);
                return cached;
            }

            await _scanGate.WaitAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                cached = GetCachedCatalog(rocksmithFolder, forceRefresh);
                if (cached != null)
                {
                    progress?.Report(100);
                    return cached;
                }

                SongCatalogResult catalog = await Task.Run(() => ScanSongs(rocksmithFolder, progress, cancellationToken), cancellationToken).ConfigureAwait(false);

                lock (_cacheLock)
                {
                    _cachedRocksmithFolder = rocksmithFolder;
                    _cachedCatalog = catalog;
                }

                return catalog;
            }
            finally
            {
                _scanGate.Release();
            }
        }

        private SongCatalogResult GetCachedCatalog(string rocksmithFolder, bool forceRefresh)
        {
            if (forceRefresh)
                return null;

            lock (_cacheLock)
            {
                return string.Equals(_cachedRocksmithFolder, rocksmithFolder, StringComparison.OrdinalIgnoreCase)
                    ? _cachedCatalog
                    : null;
            }
        }

        private static string ResolveRocksmithFolder()
        {
            string folder = GenUtil.GetRSDirectory();
            if (string.IsNullOrWhiteSpace(folder) || !Directory.Exists(folder))
                throw new DirectoryNotFoundException("The Rocksmith 2014 installation folder could not be found.");

            return Path.GetFullPath(folder);
        }

        private static SongCatalogResult ScanSongs(string rocksmithFolder, IProgress<int> progress, CancellationToken cancellationToken)
        {
            progress?.Report(0);
            IReadOnlyList<string> archivePaths = EnumerateArchivePaths(rocksmithFolder);
            if (archivePaths.Count == 0)
            {
                progress?.Report(100);
                return new SongCatalogResult([], []);
            }

            var archiveContents = new ConcurrentBag<ArchiveContents>();
            var warnings = new ConcurrentBag<SongCatalogWarning>();
            int processedArchives = 0;

            var parallelOptions = new ParallelOptions
            {
                CancellationToken = cancellationToken,
                MaxDegreeOfParallelism = MaxConcurrentArchiveReads
            };

            Parallel.ForEach(archivePaths, parallelOptions, archivePath =>
            {
                try
                {
                    archiveContents.Add(ReadArchive(archivePath, cancellationToken));
                }
                catch (OperationCanceledException)
                {
                    throw;
                }
                catch (Exception exception)
                {
                    warnings.Add(new SongCatalogWarning(archivePath, exception.Message));
                }
                finally
                {
                    int completed = Interlocked.Increment(ref processedArchives);
                    progress?.Report(completed * 100 / archivePaths.Count);
                }
            });

            IReadOnlyList<SongData> songs = BuildSongCatalog(archiveContents);
            SongCatalogWarning[] orderedWarnings = warnings
                .OrderBy(warning => warning.ArchivePath, StringComparer.OrdinalIgnoreCase)
                .ToArray();

            return new SongCatalogResult(songs, orderedWarnings);
        }

        private static IReadOnlyList<string> EnumerateArchivePaths(string rocksmithFolder)
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

        private static ArchiveContents ReadArchive(string archivePath, CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();

            using var psarc = new PsarcFile(archivePath);
            bool isOfficialDlc = string.Equals(psarc.ExtractToolkitInfo().PackageAuthor, "Ubisoft", StringComparison.Ordinal);

            cancellationToken.ThrowIfCancellationRequested();
            SongArrangement[] arrangements = psarc.ExtractArrangementManifests().ToArray();
            return new ArchiveContents(isOfficialDlc, arrangements);
        }

        private static IReadOnlyList<SongData> BuildSongCatalog(IEnumerable<ArchiveContents> archives)
        {
            var arrangements = new List<ArrangementSource>();
            foreach (ArchiveContents archive in archives)
            {
                foreach (SongArrangement arrangement in archive.Arrangements)
                {
                    if (HasSongIdentity(arrangement))
                        arrangements.Add(new ArrangementSource(arrangement, archive.IsOfficialDlc));
                }
            }

            return arrangements
                .GroupBy(source => source.Arrangement.Attributes.SongKey)
                .Select(BuildSongData)
                .OrderBy(song => song.Artist, StringComparer.OrdinalIgnoreCase)
                .ThenBy(song => song.Title, StringComparer.OrdinalIgnoreCase)
                .ToArray();
        }

        private static bool HasSongIdentity(SongArrangement arrangement) =>
            !string.IsNullOrEmpty(arrangement?.Attributes?.ArtistName) ||
            !string.IsNullOrEmpty(arrangement?.Attributes?.SongName);

        private static SongData BuildSongData(IGrouping<string, ArrangementSource> group)
        {
            ArrangementSource[] sources = group.ToArray();
            ArrangementSource first = sources[0];
            SongArrangement.ArrangementAttributes attributes = first.Arrangement.Attributes;

            var song = new SongData
            {
                Arrangements = sources.Select(source => source.Arrangement).ToList(),
                DLCKey = attributes.SongKey,
                Artist = attributes.ArtistName,
                Title = attributes.SongName,
                Shipping = attributes.Shipping,
                ODLC = first.IsOfficialDlc
            };

            if (song.ODLC && attributes.SKU == "RS1" && attributes.DLCRS1Key?.Count > 0)
                song.RS1AppID = attributes.DLCRS1Key[0].WIN32;

            return song;
        }

        private sealed record ArchiveContents(bool IsOfficialDlc, IReadOnlyList<SongArrangement> Arrangements);
        private readonly record struct ArrangementSource(SongArrangement Arrangement, bool IsOfficialDlc);
    }

    public sealed class SongData
    {
        public List<SongArrangement> Arrangements { get; set; }
        public string DLCKey { get; set; }
        public string Artist { get; set; }
        public string Title { get; set; }
        public bool Shipping { get; set; }
        public bool ODLC { get; set; }
        public int RS1AppID { get; set; }
    }
}
