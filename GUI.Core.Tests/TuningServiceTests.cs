using RSMods.SetAndForget;

namespace RSMods.Core.Tests;

public sealed class TuningServiceTests
{
    [Fact]
    public void Load_DeserializesBundledFrameworkNewtonsoftJsonOnNet8()
    {
        using var temporary = new TemporaryDirectory();
        string tuningPath = temporary.File("tuning.database.json");
        File.WriteAllText(tuningPath, """
            {
              "Static": {
                "TuningDefinitions": {
                  "Standard": {
                    "UIName": "$[36970]E Standard",
                    "Strings": {
                      "string0": 0,
                      "string1": 0,
                      "string2": 0,
                      "string3": 0,
                      "string4": 0,
                      "string5": 0
                    }
                  }
                }
              }
            }
            """);

        var service = new TuningService();

        service.Load(tuningPath);

        Assert.Equal("$[36970]E Standard", service.Tunings["Standard"].UIName);
        Assert.Equal(0, service.Tunings["Standard"].Strings["string5"]);
    }
}
