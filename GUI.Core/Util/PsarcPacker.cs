using Microsoft.FSharp.Collections;
using Microsoft.FSharp.Core;
using Rocksmith2014.PSARC;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace RSMods.Util
{
    /// <summary>
    /// Unpacks and packs the game's psarc archives with DLC Builder's PSARC library. Unpacking keeps the folder
    /// layout the Rocksmith toolkit used (<c>&lt;root&gt;\&lt;name&gt;_psarc_RS2014_Pc</c>), which the cache and
    /// SoundPacks paths are built on.
    /// </summary>
    public static class PsarcPacker
    {
        public static string UnpackedFolder(string archivePath, string root) =>
            Path.Combine(root, Path.GetFileNameWithoutExtension(archivePath) + "_psarc_RS2014_Pc");

        /// <summary>Extracts every entry of <paramref name="archivePath"/> under <paramref name="root"/>, reporting 0–100.</summary>
        public static void Unpack(string archivePath, string root, IProgress<int> progress = null)
        {
            FSharpOption<FSharpFunc<double, Unit>> report = progress is null
                ? FSharpOption<FSharpFunc<double, Unit>>.None
                : FSharpOption<FSharpFunc<double, Unit>>.Some(FSharpFunc<double, Unit>.FromConverter(percent =>
                {
                    progress.Report((int)percent);
                    return null;
                }));

            using PSARC psarc = PSARC.OpenFile(archivePath);
            psarc.ExtractFiles(UnpackedFolder(archivePath, root), report).GetAwaiter().GetResult();
        }

        /// <summary>
        /// Packs every file under <paramref name="sourceFolder"/> into <paramref name="archivePath"/> with an encrypted
        /// table of contents, like the game's own archives, reporting 0–100. The archive is written beside the target and
        /// swapped in only when complete: a half-written cache.psarc stops the game from starting.
        /// </summary>
        public static void Pack(string sourceFolder, string archivePath, IProgress<int> progress = null)
        {
            string[] files = Directory.GetFiles(sourceFolder, "*", SearchOption.AllDirectories);
            var counter = new ReadCounter(files.Sum(file => new FileInfo(file).Length), progress);
            FSharpList<NamedEntry> entries = ListModule.OfSeq(files.Select(file => new NamedEntry(
                Path.GetRelativePath(sourceFolder, file).Replace('\\', '/'),
                new CountingReadStream(File.OpenRead(file), counter))));

            string temporaryPath = archivePath + ".tmp";
            try
            {
                PSARC.Create(temporaryPath, true, entries).GetAwaiter().GetResult();
                File.Move(temporaryPath, archivePath, overwrite: true);
            }
            finally
            {
                // The library disposes the entry streams as it writes them; these only matter when it failed partway.
                foreach (NamedEntry entry in entries)
                    entry.Data.Dispose();
                if (File.Exists(temporaryPath))
                    File.Delete(temporaryPath);
            }
        }

        public static bool ContainsEntry(string archivePath, string entryName)
        {
            using PSARC psarc = PSARC.OpenFile(archivePath);
            return psarc.Manifest.Contains(entryName);
        }

        public static void ExtractEntry(string archivePath, string entryName, string targetFile)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(targetFile));
            using PSARC psarc = PSARC.OpenFile(archivePath);
            psarc.InflateFile(entryName, targetFile).GetAwaiter().GetResult();
        }

        /// <summary>
        /// Replaces the named entries with the given files, in place in the entry order, and adds the ones the archive
        /// doesn't have at the end. Every other entry is carried over unchanged. Like <see cref="Pack"/>, the edit goes to
        /// a copy that is swapped in only when complete.
        /// </summary>
        public static void ReplaceEntries(string archivePath, IReadOnlyDictionary<string, string> filesByEntry)
        {
            string temporaryPath = archivePath + ".tmp";
            var opened = new List<Stream>();
            try
            {
                File.Copy(archivePath, temporaryPath, overwrite: true);
                using (PSARC psarc = PSARC.OpenFile(temporaryPath))
                {
                    NamedEntry Open(string name)
                    {
                        Stream data = File.OpenRead(filesByEntry[name]);
                        opened.Add(data);
                        return new NamedEntry(name, data);
                    }

                    var edit = FSharpFunc<FSharpList<NamedEntry>, FSharpList<NamedEntry>>.FromConverter(entries =>
                    {
                        List<NamedEntry> edited = entries.Select(entry => filesByEntry.ContainsKey(entry.Name) ? Open(entry.Name) : entry).ToList();
                        edited.AddRange(filesByEntry.Keys.Where(name => !entries.Any(entry => entry.Name == name)).Select(Open));
                        return ListModule.OfSeq(edited);
                    });
                    psarc.Edit(new EditOptions(EditMode.InMemory, true), edit).GetAwaiter().GetResult();
                }

                File.Move(temporaryPath, archivePath, overwrite: true);
            }
            finally
            {
                // The library disposes what it writes; these only matter when it failed partway.
                foreach (Stream stream in opened)
                    stream.Dispose();
                if (File.Exists(temporaryPath))
                    File.Delete(temporaryPath);
            }
        }

        // Packing reads each source file exactly once, so bytes read over total bytes is the packing progress.
        private sealed class ReadCounter(long total, IProgress<int> progress)
        {
            private long _read;
            private int _lastPercent = -1;

            public void Add(int bytes)
            {
                if (progress is null || bytes <= 0 || total == 0)
                    return;

                int percent = (int)(Interlocked.Add(ref _read, bytes) * 100 / total);
                if (Interlocked.Exchange(ref _lastPercent, percent) != percent)
                    progress.Report(percent);
            }
        }

        private sealed class CountingReadStream(Stream inner, ReadCounter counter) : Stream
        {
            public override bool CanRead => true;
            public override bool CanSeek => inner.CanSeek;
            public override bool CanWrite => false;
            public override long Length => inner.Length;
            public override long Position { get => inner.Position; set => inner.Position = value; }

            public override int Read(byte[] buffer, int offset, int count) => Counted(inner.Read(buffer, offset, count));
            public override int Read(Span<byte> buffer) => Counted(inner.Read(buffer));

            public override async Task<int> ReadAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken) =>
                Counted(await inner.ReadAsync(buffer.AsMemory(offset, count), cancellationToken).ConfigureAwait(false));

            public override async ValueTask<int> ReadAsync(Memory<byte> buffer, CancellationToken cancellationToken = default) =>
                Counted(await inner.ReadAsync(buffer, cancellationToken).ConfigureAwait(false));

            public override long Seek(long offset, SeekOrigin origin) => inner.Seek(offset, origin);
            public override void Flush() { }
            public override void SetLength(long value) => throw new NotSupportedException();
            public override void Write(byte[] buffer, int offset, int count) => throw new NotSupportedException();

            protected override void Dispose(bool disposing)
            {
                if (disposing)
                    inner.Dispose();
                base.Dispose(disposing);
            }

            private int Counted(int read)
            {
                counter.Add(read);
                return read;
            }
        }
    }
}
