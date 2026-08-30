using RSMods.Rocksmith;

namespace RSMods.Core.Tests;

public sealed class SoundpacksTests
{
    [Fact]
    public void ConvertSoundAndReplace_WemPreservesSourceAndReplacesDestination()
    {
        using var temporary = new TemporaryDirectory();
        string source = temporary.File("source.wem");
        string destination = temporary.File("voice.wem");
        byte[] expected = [1, 2, 3, 4];
        File.WriteAllBytes(source, expected);
        File.WriteAllBytes(destination, [9, 9]);

        Soundpacks.ConvertSoundAndReplace("voice.wem", source, temporary.Path);

        Assert.Equal(expected, File.ReadAllBytes(source));
        Assert.Equal(expected, File.ReadAllBytes(destination));
    }
}
