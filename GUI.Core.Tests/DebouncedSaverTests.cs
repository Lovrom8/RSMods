#nullable enable
using RSMods.Util;

namespace RSMods.Core.Tests;

public sealed class DebouncedSaverTests
{
    private static readonly TimeSpan Quiet = TimeSpan.FromMilliseconds(60);
    private static readonly TimeSpan Settle = TimeSpan.FromMilliseconds(400);

    [Fact]
    public async Task ABurstOfChangesIsSavedOnce()
    {
        int saves = 0;
        var saver = new DebouncedSaver(() => { Interlocked.Increment(ref saves); return Task.CompletedTask; }, Quiet);

        for (int i = 0; i < 5; i++)
            saver.Request();
        await Task.Delay(Settle);

        Assert.Equal(1, saves);
        Assert.False(saver.HasUnsavedChanges);
    }

    [Fact]
    public async Task NothingIsSavedBeforeTheQuietPeriodEnds()
    {
        int saves = 0;
        var saver = new DebouncedSaver(() => { Interlocked.Increment(ref saves); return Task.CompletedTask; }, TimeSpan.FromSeconds(5));

        saver.Request();
        await Task.Delay(100);

        Assert.Equal(0, saves);
        Assert.True(saver.HasUnsavedChanges);
    }

    [Fact]
    public async Task FlushSavesAPendingChangeImmediately()
    {
        int saves = 0;
        var saver = new DebouncedSaver(() => { Interlocked.Increment(ref saves); return Task.CompletedTask; }, TimeSpan.FromSeconds(5));

        saver.Request();
        await saver.FlushAsync();

        Assert.Equal(1, saves);
        Assert.False(saver.HasUnsavedChanges);
    }

    [Fact]
    public async Task FlushWithNothingPendingDoesNothing()
    {
        int saves = 0;
        var saver = new DebouncedSaver(() => { Interlocked.Increment(ref saves); return Task.CompletedTask; }, Quiet);

        await saver.FlushAsync();

        Assert.Equal(0, saves);
    }

    [Fact]
    public async Task AChangeDuringASaveIsSavedByOneMoreRun()
    {
        var firstSaveStarted = new TaskCompletionSource();
        var releaseFirstSave = new TaskCompletionSource();
        int saves = 0;
        int running = 0;
        int maxConcurrent = 0;

        var saver = new DebouncedSaver(async () =>
        {
            int now = Interlocked.Increment(ref running);
            maxConcurrent = Math.Max(maxConcurrent, now);
            if (Interlocked.Increment(ref saves) == 1)
            {
                firstSaveStarted.SetResult();
                await releaseFirstSave.Task;
            }
            Interlocked.Decrement(ref running);
        }, Quiet);

        saver.Request();
        await firstSaveStarted.Task;
        saver.Request(); // Arrives mid-save.
        await Task.Delay(Settle);
        releaseFirstSave.SetResult();
        await saver.FlushAsync();

        Assert.Equal(2, saves);
        Assert.Equal(1, maxConcurrent);
        Assert.False(saver.HasUnsavedChanges);
    }

    [Fact]
    public async Task AFailedSaveIsReportedAndLaterChangesStillSave()
    {
        int attempts = 0;
        Exception? reported = null;
        var saver = new DebouncedSaver(() =>
        {
            if (Interlocked.Increment(ref attempts) == 1)
                throw new IOException("disk full");
            return Task.CompletedTask;
        }, Quiet, ex => reported = ex);

        saver.Request();
        await saver.FlushAsync();
        saver.Request();
        await saver.FlushAsync();

        Assert.IsType<IOException>(reported);
        Assert.Equal(2, attempts);
        Assert.False(saver.HasUnsavedChanges);
    }
}
