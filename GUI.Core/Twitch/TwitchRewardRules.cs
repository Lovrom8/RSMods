using System;

namespace RSMods.Twitch
{
    public static class TwitchRewardRules
    {
        public static TwitchTriggerKind GetTriggerKind(TwitchReward reward)
        {
            if (reward is BitsReward)
                return TwitchTriggerKind.Bits;
            if (reward is ChannelPointsReward)
                return TwitchTriggerKind.ChannelPoints;
            if (reward is SubReward)
                return TwitchTriggerKind.Subscription;
            throw new ArgumentException("The reward does not define a Twitch trigger type.", nameof(reward));
        }

        public static int GetLocalId(TwitchReward reward)
        {
            if (reward is BitsReward bits)
                return bits.BitsID;
            if (reward is ChannelPointsReward points)
                return points.PointsID;
            if (reward is SubReward subscription)
                return subscription.SubID;
            throw new ArgumentException("The reward does not define a legacy local ID.", nameof(reward));
        }

        public static int GetAmount(TwitchReward reward)
        {
            if (reward is BitsReward bits)
                return bits.BitsAmount;
            if (reward is ChannelPointsReward points)
                return points.PointsAmount;
            return 0;
        }

        public static TwitchReward Create(
            TwitchReward catalogEffect,
            TwitchTriggerKind triggerKind,
            int localId,
            int durationSeconds,
            int amount,
            bool enabled,
            string additionalMessage = "")
        {
            if (catalogEffect == null)
                throw new ArgumentNullException(nameof(catalogEffect));

            TwitchReward reward;
            switch (triggerKind)
            {
                case TwitchTriggerKind.Bits:
                    reward = new BitsReward { BitsID = localId, BitsAmount = amount };
                    break;
                case TwitchTriggerKind.ChannelPoints:
                    reward = new ChannelPointsReward { PointsID = localId, PointsAmount = amount };
                    break;
                case TwitchTriggerKind.Subscription:
                    reward = new SubReward { SubID = localId };
                    break;
                default:
                    throw new ArgumentOutOfRangeException(nameof(triggerKind));
            }

            reward.Name = catalogEffect.Name;
            reward.Description = catalogEffect.Description;
            reward.InternalMsgEnable = catalogEffect.InternalMsgEnable;
            reward.Length = durationSeconds;
            reward.Enabled = enabled;
            reward.AdditionalMsg = additionalMessage ?? string.Empty;
            return reward;
        }

        public static bool TryValidate(TwitchReward reward, out string error)
        {
            if (reward == null)
            {
                error = "A reward rule is required.";
                return false;
            }
            if (string.IsNullOrWhiteSpace(reward.Name) || string.IsNullOrWhiteSpace(reward.InternalMsgEnable))
            {
                error = "The reward must reference a catalog effect.";
                return false;
            }
            if (reward.Length < 0)
            {
                error = "Duration must be a non-negative whole number of seconds.";
                return false;
            }
            if (!(reward is BitsReward) && !(reward is ChannelPointsReward) && !(reward is SubReward))
            {
                error = "The reward must use a Subscription, Bits, or Channel Points trigger.";
                return false;
            }

            // The native protocol stores milliseconds in a signed 64-bit integer.
            try
            {
                checked { _ = (long)reward.Length * 1000L; }
            }
            catch (OverflowException)
            {
                error = "Duration is too large for the Rocksmith effect protocol.";
                return false;
            }

            if (GetLocalId(reward) < 0)
            {
                error = "Local ID must be non-negative.";
                return false;
            }
            if ((reward is BitsReward || reward is ChannelPointsReward) && GetAmount(reward) < 0)
            {
                error = "Bits and Channel Point amounts must be non-negative whole numbers.";
                return false;
            }

            if (string.Equals(reward.InternalMsgEnable, "solidnotes", StringComparison.OrdinalIgnoreCase))
            {
                string color = reward.AdditionalMsg ?? string.Empty;
                if (!string.Equals(color, "Random", StringComparison.OrdinalIgnoreCase) && !IsHexColor(color))
                {
                    error = "Solid Notes color must be Random or exactly six hexadecimal digits.";
                    return false;
                }
            }

            error = string.Empty;
            return true;
        }

        private static bool IsHexColor(string value)
        {
            if (value.Length != 6)
                return false;
            for (int index = 0; index < value.Length; index++)
            {
                char character = value[index];
                if (!((character >= '0' && character <= '9') ||
                      (character >= 'a' && character <= 'f') ||
                      (character >= 'A' && character <= 'F')))
                    return false;
            }
            return true;
        }
    }
}
