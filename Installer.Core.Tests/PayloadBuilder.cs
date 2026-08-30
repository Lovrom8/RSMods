using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using RS2014_Mod_Installer.Payload;

namespace RS2014_Mod_Installer.Tests;

// Builds an in-memory configurator payload (zip + matching manifest) for tests, mirroring what
// Build/New-ConfiguratorPayload.ps1 produces.
internal sealed class PayloadBuilder
{
    private readonly Dictionary<string, byte[]> _files = new();

    public PayloadBuilder Add(string relativePath, byte[] content)
    {
        _files[relativePath.Replace('\\', '/')] = content;
        return this;
    }

    public PayloadBuilder AddText(string relativePath, string content)
        => Add(relativePath, Encoding.UTF8.GetBytes(content));

    // A representative payload: the executable, a managed dll, and files under ddc/ and tools/.
    public static PayloadBuilder Default()
        => new PayloadBuilder()
            .AddText("RSMods.exe", "fake configurator executable v-new")
            .AddText("RocksmithToolkitLib.dll", "toolkit assembly")
            .AddText("ddc/ddc.exe", "ddc tool")
            .AddText("tools/ww2ogg.exe", "ww2ogg tool");

    public MemoryStream BuildZip()
    {
        var stream = new MemoryStream();
        using (var archive = new ZipArchive(stream, ZipArchiveMode.Create, leaveOpen: true))
        {
            foreach (KeyValuePair<string, byte[]> file in _files)
            {
                ZipArchiveEntry entry = archive.CreateEntry(file.Key, CompressionLevel.Optimal);
                using Stream entryStream = entry.Open();
                entryStream.Write(file.Value, 0, file.Value.Length);
            }
        }
        stream.Position = 0;
        return stream;
    }

    // A zip containing a single zip-slip entry that tries to escape the extraction root.
    public static MemoryStream BuildZipSlipZip(string escapingEntryName = "../evil.txt")
    {
        var stream = new MemoryStream();
        using (var archive = new ZipArchive(stream, ZipArchiveMode.Create, leaveOpen: true))
        {
            ZipArchiveEntry entry = archive.CreateEntry(escapingEntryName, CompressionLevel.Optimal);
            using Stream entryStream = entry.Open();
            byte[] bytes = Encoding.UTF8.GetBytes("pwned");
            entryStream.Write(bytes, 0, bytes.Length);
        }
        stream.Position = 0;
        return stream;
    }

    public PayloadManifest BuildManifest(string applicationVersion = "1.0.0")
    {
        var manifest = new PayloadManifest
        {
            PayloadFormatVersion = 1,
            Application = "RSMods",
            ApplicationVersion = applicationVersion,
            Executable = "RSMods.exe",
            Directories = _files.Keys
                .Where(p => p.Contains('/'))
                .Select(p => p.Substring(0, p.LastIndexOf('/')))
                .Distinct()
                .OrderBy(p => p)
                .ToList(),
            Files = _files
                .OrderBy(f => f.Key)
                .Select(f => new PayloadFile
                {
                    Path = f.Key,
                    Length = f.Value.Length,
                    Sha256 = Sha256Hex(f.Value)
                })
                .ToList()
        };
        return manifest;
    }

    public byte[] Content(string relativePath) => _files[relativePath.Replace('\\', '/')];

    private static string Sha256Hex(byte[] bytes)
    {
        using var sha = SHA256.Create();
        byte[] hash = sha.ComputeHash(bytes);
        var builder = new StringBuilder(hash.Length * 2);
        foreach (byte b in hash)
            builder.Append(b.ToString("x2"));
        return builder.ToString();
    }
}
