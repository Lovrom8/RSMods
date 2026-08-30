using System.Collections.Generic;
using System.IO;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Text;

namespace RS2014_Mod_Installer.Payload
{
    // Mirror of the manifest produced by Build/New-ConfiguratorPayload.ps1. Serialized with the in-box
    // DataContractJsonSerializer so the installer stays a self-contained single exe.
    // Serializer reads members in contract order, so DataMember Order mirrors the exact key
    // order the PowerShell payload script emits.
    [DataContract]
    public sealed class PayloadManifest
    {
        [DataMember(Name = "payloadFormatVersion", Order = 0)]
        public int PayloadFormatVersion { get; set; }

        [DataMember(Name = "application", Order = 1)]
        public string Application { get; set; }

        [DataMember(Name = "applicationVersion", Order = 2)]
        public string ApplicationVersion { get; set; }

        // Relative path (forward slashes) of the configurator executable, e.g. "RSMods.exe".
        [DataMember(Name = "executable", Order = 3)]
        public string Executable { get; set; }

        // Directories owned by the payload, relative and forward-slashed (e.g. "ddc", "tools").
        [DataMember(Name = "directories", Order = 4)]
        public List<string> Directories { get; set; }

        [DataMember(Name = "files", Order = 5)]
        public List<PayloadFile> Files { get; set; }

        private static readonly DataContractJsonSerializer Serializer = new(typeof(PayloadManifest));

        public static PayloadManifest Parse(string json)
        {
            using var stream = new MemoryStream(Encoding.UTF8.GetBytes(json));

            return (PayloadManifest)Serializer.ReadObject(stream);
        }

        public string ToJson()
        {
            using var stream = new MemoryStream();
            Serializer.WriteObject(stream, this);

            return Encoding.UTF8.GetString(stream.ToArray());
        }
    }

    [DataContract]
    public sealed class PayloadFile
    {
        // Relative path with forward slashes; never rooted and never containing a ".." segment.
        [DataMember(Name = "path", Order = 0)]
        public string Path { get; set; }

        [DataMember(Name = "length", Order = 1)]
        public long Length { get; set; }

        // Lowercase hex SHA-256 of the file's bytes.
        [DataMember(Name = "sha256", Order = 2)]
        public string Sha256 { get; set; }
    }
}
