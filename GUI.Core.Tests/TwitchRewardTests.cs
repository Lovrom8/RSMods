using RSMods.Twitch;

namespace RSMods.Core.Tests;

public sealed class TwitchRewardTests
{
    [Fact]
    public void Catalog_ReturnsFreshCompleteLists()
    {
        List<TwitchReward> first = TwitchRewardCatalog.CreateDefaults();
        List<TwitchReward> second = TwitchRewardCatalog.CreateDefaults();

        Assert.Equal(23, first.Count);
        Assert.Equal(23, second.Count);
        Assert.NotSame(first, second);
        Assert.NotSame(first[0], second[0]);
        Assert.Contains(first, reward => reward.InternalMsgEnable == "turbospeed");
    }

    [Fact]
    public async Task Repository_RoundTripsExistingPolymorphicSchema()
    {
        using var temporary = new TemporaryDirectory();
        var repository = new TwitchRewardRepository(temporary.File("TwitchEnabledEffects.xml"));
        var rewards = new List<TwitchReward>
        {
            new BitsReward
            {
                BitsID = 1,
                BitsAmount = 100,
                Enabled = true,
                Name = "Bits effect",
                InternalMsgEnable = "rainbowstrings",
                Length = 12
            },
            new ChannelPointsReward
            {
                PointsID = 2,
                PointsAmount = 500,
                Enabled = true,
                Name = "Points effect",
                InternalMsgEnable = "solidnotes",
                AdditionalMsg = "A0B1C2",
                Length = 7
            },
            new SubReward
            {
                SubID = 3,
                Enabled = false,
                Name = "Sub effect",
                InternalMsgEnable = "drunkmode",
                Length = 4
            }
        };

        await repository.SaveAsync(rewards);
        List<TwitchReward> loaded = await repository.LoadAsync();

        Assert.Collection(
            loaded,
            reward =>
            {
                BitsReward bits = Assert.IsType<BitsReward>(reward);
                Assert.Equal(100, bits.BitsAmount);
                Assert.Equal(12, bits.Length);
            },
            reward =>
            {
                ChannelPointsReward points = Assert.IsType<ChannelPointsReward>(reward);
                Assert.Equal(500, points.PointsAmount);
                Assert.Equal("A0B1C2", points.AdditionalMsg);
            },
            reward => Assert.IsType<SubReward>(reward));
    }

    [Fact]
    public void Matcher_PreservesExactAmountAndSubscriptionRules()
    {
        var enabledBits = new BitsReward { Enabled = true, BitsAmount = 100 };
        var otherBits = new BitsReward { Enabled = true, BitsAmount = 200 };
        var disabledBits = new BitsReward { Enabled = false, BitsAmount = 100 };
        var points = new ChannelPointsReward { Enabled = true, PointsAmount = 100 };
        var firstSub = new SubReward { Enabled = true };
        var secondSub = new SubReward { Enabled = true };
        TwitchReward[] rewards = [enabledBits, otherBits, disabledBits, points, firstSub, secondSub];

        List<TwitchReward> bitsMatches = TwitchRewardMatcher.FindMatches(
            rewards,
            new TwitchTriggerEvent { Kind = TwitchTriggerKind.Bits, Amount = 100 });
        List<TwitchReward> pointsMatches = TwitchRewardMatcher.FindMatches(
            rewards,
            new TwitchTriggerEvent { Kind = TwitchTriggerKind.ChannelPoints, Amount = 100 });
        List<TwitchReward> subMatches = TwitchRewardMatcher.FindMatches(
            rewards,
            new TwitchTriggerEvent { Kind = TwitchTriggerKind.Subscription });

        Assert.Equal([enabledBits], bitsMatches);
        Assert.Equal([points], pointsMatches);
        Assert.Equal([firstSub, secondSub], subMatches);
    }

    [Fact]
    public void RuleAdapter_PreservesLegacyIdsAndValidatesWireValues()
    {
        TwitchReward catalogEffect = TwitchRewardCatalog.CreateDefaults()
            .Single(reward => reward.InternalMsgEnable == "solidnotes");
        TwitchReward reward = TwitchRewardRules.Create(
            catalogEffect,
            TwitchTriggerKind.ChannelPoints,
            localId: 42,
            durationSeconds: 15,
            amount: 500,
            enabled: true,
            additionalMessage: "A0B1C2");

        Assert.True(TwitchRewardRules.TryValidate(reward, out string error), error);
        Assert.Equal(TwitchTriggerKind.ChannelPoints, TwitchRewardRules.GetTriggerKind(reward));
        Assert.Equal(42, TwitchRewardRules.GetLocalId(reward));
        Assert.Equal(500, TwitchRewardRules.GetAmount(reward));
        Assert.Equal("A0B1C2", reward.AdditionalMsg);

        reward.Length = -1;
        Assert.False(TwitchRewardRules.TryValidate(reward, out error));
        Assert.Contains("Duration", error);
    }

}
