using System.IO.Compression;
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

    // A .rs_soundpack is a zip anyone can share; an entry named "..\x" mustn't land outside the audio folder.
    [WindowsTheory]
    [InlineData("../escaped.wem")]
    [InlineData("english(us)/../../escaped.wem")]
    public void ImportSoundFile_RefusesEntriesOutsideTheAudioFolder(string entryName)
    {
        using var temporary = new TemporaryDirectory();
        string pack = temporary.File("evil.rs_soundpack");
        using (ZipArchive zip = ZipFile.Open(pack, ZipArchiveMode.Create))
        {
            using (var writer = new StreamWriter(zip.CreateEntry("english(us)/2066953778.wem").Open()))
                writer.Write("voice");
            using (var writer = new StreamWriter(zip.CreateEntry(entryName).Open()))
                writer.Write("escaped");
        }
        string audio = Path.Combine(temporary.Path, "rsmods", "audio");
        Directory.CreateDirectory(audio);

        Assert.ThrowsAny<InvalidDataException>(() => Soundpacks.ImportSoundFile(pack, audio));

        Assert.False(File.Exists(Path.Combine(temporary.Path, "rsmods", "escaped.wem")));
        Assert.False(File.Exists(Path.Combine(temporary.Path, "escaped.wem")));
        Assert.Empty(Directory.GetFileSystemEntries(audio));
    }

    [WindowsFact]
    public void ImportSoundFile_ExtractsThePacksVoiceLines()
    {
        using var temporary = new TemporaryDirectory();
        string pack = temporary.File("good.rs_soundpack");
        using (ZipArchive zip = ZipFile.Open(pack, ZipArchiveMode.Create))
        using (var writer = new StreamWriter(zip.CreateEntry("english(us)/2066953778.wem").Open()))
            writer.Write("voice");
        string audio = Path.Combine(temporary.Path, "audio");
        Directory.CreateDirectory(audio);

        Soundpacks.ImportSoundFile(pack, audio);

        Assert.Equal("voice", File.ReadAllText(Path.Combine(audio, "english(us)", "2066953778.wem")));
    }
}
