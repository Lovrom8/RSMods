using System;
using System.Collections.Generic;

namespace RSMods.Twitch
{
    public static class TwitchRewardMatcher
    {
        public static List<TwitchReward> FindMatches(IEnumerable<TwitchReward> rewards, TwitchTriggerEvent trigger)
        {
            ArgumentNullException.ThrowIfNull(rewards);
            ArgumentNullException.ThrowIfNull(trigger);

            var matches = new List<TwitchReward>();
            foreach (TwitchReward reward in rewards)
            {
                if (reward?.Enabled != true)
                    continue;

                if (trigger.Kind == TwitchTriggerKind.Bits && reward is BitsReward bitsReward && bitsReward.BitsAmount == trigger.Amount)
                {
                    matches.Add(reward);
                }
                else if (trigger.Kind == TwitchTriggerKind.ChannelPoints && reward is ChannelPointsReward pointsReward && pointsReward.PointsAmount == trigger.Amount)
                {
                    matches.Add(reward);
                }
                else if (trigger.Kind == TwitchTriggerKind.Subscription && reward is SubReward)
                {
                    matches.Add(reward);
                }
            }

            return matches;
        }
    }
}
