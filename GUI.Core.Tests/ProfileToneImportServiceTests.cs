using Newtonsoft.Json.Linq;

namespace RSMods.Core.Tests;

public sealed class ProfileToneImportServiceTests
{
    // As the toolkit writes it: every slot is present, the empty ones nil.
    private const string ToolkitTone = """
        <?xml version="1.0" encoding="utf-8"?>
        <Tone2014 xmlns:i="http://www.w3.org/2001/XMLSchema-instance" xmlns="http://schemas.datacontract.org/2004/07/RocksmithToolkitLib.DLCPackage.Manifest2014.Tone">
          <GearList>
            <Amp>
              <Category>Amp</Category>
              <KnobValues xmlns:d4p1="http://schemas.microsoft.com/2003/10/Serialization/Arrays">
                <d4p1:KeyValueOfstringfloat><d4p1:Key>Amp_OrangeAD50_Gain</d4p1:Key><d4p1:Value>88</d4p1:Value></d4p1:KeyValueOfstringfloat>
              </KnobValues>
              <PedalKey>Amp_OrangeAD50</PedalKey>
              <Skin i:nil="true" />
              <SkinIndex i:nil="true" />
              <Type>Amps</Type>
            </Amp>
            <Cabinet>
              <Category i:nil="true" />
              <KnobValues xmlns:d4p1="http://schemas.microsoft.com/2003/10/Serialization/Arrays" />
              <PedalKey>Cab_OrangeJimmyBean_57_Cone</PedalKey>
              <Skin i:nil="true" />
              <SkinIndex i:nil="true" />
              <Type>Cabinets</Type>
            </Cabinet>
            <PostPedal1 i:nil="true" />
            <PostPedal2 i:nil="true" />
            <PostPedal3 i:nil="true" />
            <PostPedal4 i:nil="true" />
            <PrePedal1>
              <Category i:nil="true" />
              <KnobValues xmlns:d4p1="http://schemas.microsoft.com/2003/10/Serialization/Arrays">
                <d4p1:KeyValueOfstringfloat><d4p1:Key>Pedal_Chorus20_Mix</d4p1:Key><d4p1:Value>8</d4p1:Value></d4p1:KeyValueOfstringfloat>
              </KnobValues>
              <PedalKey>Pedal_Chorus20</PedalKey>
              <Type>Pedals</Type>
            </PrePedal1>
            <PrePedal2 i:nil="true" />
            <PrePedal3 i:nil="true" />
            <PrePedal4 i:nil="true" />
            <Rack1 i:nil="true" />
            <Rack2 i:nil="true" />
            <Rack3 i:nil="true" />
            <Rack4 i:nil="true" />
          </GearList>
          <IsCustom>true</IsCustom>
          <Key>Thrashy</Key>
          <Name>Thrashy</Name>
          <NameSeparator> - </NameSeparator>
          <SortOrder>0</SortOrder>
          <ToneDescriptors xmlns:d2p1="http://schemas.microsoft.com/2003/10/Serialization/Arrays">
            <d2p1:string>$[35716]DISTORTION</d2p1:string>
          </ToneDescriptors>
          <Volume>-14</Volume>
        </Tone2014>
        """;

    [Fact]
    public void ToolkitXmlToneBecomesTheGamesToneJson()
    {
        using var dir = new TemporaryDirectory();
        string file = dir.File("Thrashy.tone2014.xml");
        File.WriteAllText(file, ToolkitTone);

        JObject tone = ProfileToneImportService.LoadXmlTone(file);

        Assert.Equal("Thrashy", tone["Name"]?.ToString());
        Assert.Equal("Thrashy", tone["Key"]?.ToString());
        Assert.Equal(true, tone["IsCustom"]?.Value<bool>());
        Assert.Equal("-14.000", tone["Volume"]?.ToString());
        Assert.Equal(new[] { "$[35716]DISTORTION" }, tone["ToneDescriptors"]?.ToObject<string[]>());

        JObject gear = Assert.IsType<JObject>(tone["GearList"]);
        Assert.Equal("Amp_OrangeAD50", gear.SelectToken("Amp.Key")?.ToString());
        Assert.Equal(88f, gear.SelectToken("Amp.KnobValues.Amp_OrangeAD50_Gain")?.Value<float>());
        Assert.Equal("Cab_OrangeJimmyBean_57_Cone", gear.SelectToken("Cabinet.Key")?.ToString());
        Assert.Equal("Pedal_Chorus20", gear.SelectToken("PrePedal1.Key")?.ToString());
        // Empty slots are left out, not written as null.
        Assert.Null(gear.Property("PrePedal2"));
        Assert.Null(gear.Property("Rack1"));
    }
}
