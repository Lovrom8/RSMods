using System.IO;
using RS2014_Mod_Installer.Core;
using Xunit;

namespace RS2014_Mod_Installer.Tests;

public sealed class RocksmithCompatibilityTests
{
    [Fact]
    public void CheckExecutable_MissingExe_IsInvalid()
    {
        using var temp = new TemporaryDirectory();
        Assert.Equal(ExecutableCheckStatus.Invalid, RocksmithCompatibility.CheckExecutable(temp.Path));
    }

    [Fact]
    public void CheckExecutable_WrongContent_IsInvalid()
    {
        using var temp = new TemporaryDirectory();
        File.WriteAllText(Path.Combine(temp.Path, "Rocksmith2014.exe"), "not the real game");
        Assert.Equal(ExecutableCheckStatus.Invalid, RocksmithCompatibility.CheckExecutable(temp.Path));
    }

    [Fact]
    public void CheckExecutable_LockedByRunningGame_IsUnreadable()
    {
        using var temp = new TemporaryDirectory();
        string exe = Path.Combine(temp.Path, "Rocksmith2014.exe");
        File.WriteAllText(exe, "locked");

        using (new FileStream(exe, FileMode.Open, FileAccess.Read, FileShare.None))
        {
            Assert.Equal(ExecutableCheckStatus.Unreadable, RocksmithCompatibility.CheckExecutable(temp.Path));
        }
    }

    [Fact]
    public void Evaluate_UnsupportedExecutable_IsIncompatible_WithReason()
    {
        using var temp = new TemporaryDirectory();
        File.WriteAllText(Path.Combine(temp.Path, "Rocksmith2014.exe"), "pirated");

        CompatibilityResult result = RocksmithCompatibility.Evaluate(temp.Path);

        Assert.False(result.IsCompatible);
        Assert.False(result.ExecutableUnreadable);
        Assert.Contains(result.Reasons, r => r.Contains("executable"));
        Assert.Contains(result.Reasons, r => r.Contains("steam_api.dll")); // no valid cert present either
    }

    [Fact]
    public void Evaluate_CrackIndicatorPresent_IsReported()
    {
        using var temp = new TemporaryDirectory();
        File.WriteAllText(Path.Combine(temp.Path, "Codex.ini"), "crack");

        CompatibilityResult result = RocksmithCompatibility.Evaluate(temp.Path);

        Assert.False(result.IsCompatible);
        Assert.Contains(result.Reasons, r => r.Contains("crack"));
    }

    [Fact]
    public void Evaluate_RunningGame_ReportsExecutableUnreadable()
    {
        using var temp = new TemporaryDirectory();
        string exe = Path.Combine(temp.Path, "Rocksmith2014.exe");
        File.WriteAllText(exe, "locked");

        using (new FileStream(exe, FileMode.Open, FileAccess.Read, FileShare.None))
        {
            CompatibilityResult result = RocksmithCompatibility.Evaluate(temp.Path);
            Assert.True(result.ExecutableUnreadable);
            Assert.False(result.IsCompatible);
        }
    }
}
