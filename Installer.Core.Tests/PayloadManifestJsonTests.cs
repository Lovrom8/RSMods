using System.Linq;
using RS2014_Mod_Installer.Payload;
using Xunit;

namespace RS2014_Mod_Installer.Tests;

public sealed class PayloadManifestJsonTests
{
    // A manifest exactly as Build/New-ConfiguratorPayload.ps1 emits it: camelCase keys in that script's
    // property order, indented, with a git-suffixed version. DataContractJsonSerializer reads members in
    // contract order, so this guards that the DataMember ordering still matches the script's output.
    private const string PowerShellStyleJson = @"{
  ""payloadFormatVersion"": 1,
  ""application"": ""RSMods"",
  ""applicationVersion"": ""1.0.0+ac392a87750c2ce043323bb40684d23a90cc38ba"",
  ""executable"": ""RSMods.exe"",
  ""directories"": [
    ""ddc"",
    ""tools""
  ],
  ""files"": [
    {
      ""path"": ""RSMods.exe"",
      ""length"": 12345,
      ""sha256"": ""aabbccddeeff00112233445566778899aabbccddeeff00112233445566778899""
    },
    {
      ""path"": ""ddc/ddc.exe"",
      ""length"": 6789,
      ""sha256"": ""00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff""
    }
  ]
}";

    [Fact]
    public void Parse_ReadsPowerShellGeneratedManifest()
    {
        PayloadManifest manifest = PayloadManifest.Parse(PowerShellStyleJson);

        Assert.Equal(1, manifest.PayloadFormatVersion);
        Assert.Equal("RSMods", manifest.Application);
        Assert.Equal("1.0.0+ac392a87750c2ce043323bb40684d23a90cc38ba", manifest.ApplicationVersion);
        Assert.Equal("RSMods.exe", manifest.Executable);
        Assert.Equal(new[] { "ddc", "tools" }, manifest.Directories.ToArray());

        Assert.Equal(2, manifest.Files.Count);
        PayloadFile exe = manifest.Files.Single(f => f.Path == "RSMods.exe");
        Assert.Equal(12345, exe.Length);
        Assert.Equal("aabbccddeeff00112233445566778899aabbccddeeff00112233445566778899", exe.Sha256);
        PayloadFile ddc = manifest.Files.Single(f => f.Path == "ddc/ddc.exe");
        Assert.Equal(6789, ddc.Length);
    }

    [Fact]
    public void ToJson_RoundTripsThroughParse()
    {
        var original = new PayloadManifest
        {
            PayloadFormatVersion = 1,
            Application = "RSMods",
            ApplicationVersion = "2.5.1",
            Executable = "RSMods.exe",
            Directories = new System.Collections.Generic.List<string> { "ddc", "tools" },
            Files = new System.Collections.Generic.List<PayloadFile>
            {
                new PayloadFile { Path = "RSMods.exe", Length = 10, Sha256 = "deadbeef" },
                new PayloadFile { Path = "tools/ww2ogg.exe", Length = 20, Sha256 = "cafef00d" }
            }
        };

        PayloadManifest round = PayloadManifest.Parse(original.ToJson());

        Assert.Equal(original.ApplicationVersion, round.ApplicationVersion);
        Assert.Equal(original.Executable, round.Executable);
        Assert.Equal(original.Files.Count, round.Files.Count);
        Assert.Equal("tools/ww2ogg.exe", round.Files[1].Path);
        Assert.Equal(20, round.Files[1].Length);
    }
}
