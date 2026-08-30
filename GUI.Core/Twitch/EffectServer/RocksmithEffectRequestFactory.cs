using System;
using System.Collections.Generic;
using System.Globalization;

namespace RSMods.Twitch.EffectServer
{
    public static class RocksmithEffectRequestFactory
    {
        public static RocksmithEffectRequest Create(TwitchReward reward, int requestId, string viewer)
        {
            if (reward == null)
                throw new ArgumentNullException(nameof(reward));
            if (string.IsNullOrWhiteSpace(reward.InternalMsgEnable))
                throw new ArgumentException("The reward does not define an effect code.", nameof(reward));
            if (reward.Length < 0)
                throw new ArgumentOutOfRangeException(nameof(reward), "Reward duration cannot be negative.");

            var request = new RocksmithEffectRequest
            {
                Id = requestId,
                Code = NormalizeEffectCode(reward.InternalMsgEnable),
                Type = 1,
                Viewer = string.IsNullOrWhiteSpace(viewer) ? "rsmods" : viewer,
                DurationMilliseconds = checked((long)reward.Length * 1000L),
                Parameters = []
            };

            if (request.Code == "solidnotes" && !string.IsNullOrWhiteSpace(reward.AdditionalMsg))
            {
                if (string.Equals(reward.AdditionalMsg, "Random", StringComparison.OrdinalIgnoreCase))
                {
                    request.Code = "solidrandom";
                }
                else if (reward.AdditionalMsg.Length == 6 && IsHexColor(reward.AdditionalMsg))
                {
                    request.Code = "solidcustomrgb";
                    request.Parameters.Add(int.Parse(reward.AdditionalMsg.Substring(0, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture));
                    request.Parameters.Add(int.Parse(reward.AdditionalMsg.Substring(2, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture));
                    request.Parameters.Add(int.Parse(reward.AdditionalMsg.Substring(4, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture));
                }
            }

            return request;
        }

        private static string NormalizeEffectCode(string code)
        {
            string normalized = code.Trim().ToLowerInvariant();
            if (normalized.StartsWith("enable ", StringComparison.Ordinal))
                normalized = normalized.Substring("enable ".Length).Trim();

            return normalized;
        }

        private static bool IsHexColor(string value)
        {
            for (int i = 0; i < value.Length; i++)
            {
                char character = value[i];
                bool isHex = (character >= '0' && character <= '9') ||
                             (character >= 'a' && character <= 'f') ||
                             (character >= 'A' && character <= 'F');

                if (!isHex)
                    return false;
            }

            return true;
        }
    }
}
