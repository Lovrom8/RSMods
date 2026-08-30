using RSMods.Data;
using RSMods.SetAndForget;

namespace RSMods.Core.Tests;

public sealed class FastLoadServiceTests
{
    [Theory]
    [InlineData(DriveMediaType.Hdd, false, FastLoadDrivePrompt.ConfirmHddRisk)]
    [InlineData(DriveMediaType.Ssd, true, FastLoadDrivePrompt.ConfirmNvme)]
    [InlineData(DriveMediaType.Ssd, false, FastLoadDrivePrompt.None)]
    [InlineData(DriveMediaType.Scm, false, FastLoadDrivePrompt.None)]
    [InlineData(DriveMediaType.Unspecified, false, FastLoadDrivePrompt.ConfirmUnknown)]
    public void GetDrivePrompt_MapsDriveTypeToPrompt(
        DriveMediaType mediaType, bool isNvme, FastLoadDrivePrompt expected)
    {
        Constants.RSFolder = @"D:\Games\Rocksmith2014";
        var service = new FastLoadService(new CachePsarcService(), new FakeDriveInfoProvider(mediaType, isNvme));

        Assert.Equal(expected, service.GetDrivePrompt());
    }

    [Fact]
    public void GetDrivePrompt_QueriesTheInstallationDriveLetter()
    {
        Constants.RSFolder = @"e:\rocksmith";
        var drives = new FakeDriveInfoProvider(DriveMediaType.Ssd, isNvme: false);
        var service = new FastLoadService(new CachePsarcService(), drives);

        service.GetDrivePrompt();

        Assert.Equal('E', drives.LastQueriedDriveLetter);
    }

    private sealed class FakeDriveInfoProvider(DriveMediaType mediaType, bool isNvme) : IDriveInfoProvider
    {
        public char? LastQueriedDriveLetter { get; private set; }

        public (DriveMediaType Type, bool IsNVMe) GetDriveType(char driveLetter)
        {
            LastQueriedDriveLetter = driveLetter;
            return (mediaType, isNvme);
        }
    }
}
