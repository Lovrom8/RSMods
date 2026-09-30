namespace RSMods.Core.Tests;

public sealed class ExtractBeatsCommandTests : IDisposable
{
    private readonly TemporaryDirectory _rocksmithFolder = new();

    public void Dispose() => _rocksmithFolder.Dispose();

    [Fact]
    public void OtherArgumentsAreLeftToTheApp()
    {
        Assert.False(ExtractBeatsCommand.TryRun(["--minimized"], out _));
    }

    [Fact]
    public void NoArgumentsAreLeftToTheApp()
    {
        Assert.False(ExtractBeatsCommand.TryRun([], out _));
    }

    [Fact]
    public void AMissingArgumentIsRejected()
    {
        ExtractBeatsCommand.TryRun([ExtractBeatsCommand.Name, _rocksmithFolder.Path, "2minutes"], out int exitCode);

        Assert.Equal(ExtractBeatsCommand.BadArguments, exitCode);
    }

    [Fact]
    public void ASongInNoArchiveIsReportedAsNotFound()
    {
        string output = _rocksmithFolder.File("2minutes.beats");

        ExtractBeatsCommand.TryRun([ExtractBeatsCommand.Name, _rocksmithFolder.Path, "2minutes", output], out int exitCode);

        Assert.Equal(ExtractBeatsCommand.SongNotFound, exitCode);
    }

    [Fact]
    public void ASongInNoArchiveWritesNoFile()
    {
        string output = _rocksmithFolder.File("2minutes.beats");

        ExtractBeatsCommand.TryRun([ExtractBeatsCommand.Name, _rocksmithFolder.Path, "2minutes", output], out _);

        Assert.False(File.Exists(output));
    }
}
