using Newtonsoft.Json;
using System.Collections.Generic;

namespace RSMods.Twitch.EffectServer
{
    public sealed class RocksmithEffectResponse
    {
        [JsonProperty("id")]
        public int Id { get; set; }

        [JsonProperty("code")]
        public string Code { get; set; }

        [JsonProperty("status")]
        public int Status { get; set; }

        [JsonProperty("timeRemaining")]
        public long TimeRemaining { get; set; }

        [JsonProperty("type")]
        public int Type { get; set; }
    }

    public sealed class RocksmithEffectRequest
    {
        [JsonProperty("code")]
        public string Code { get; set; }

        [JsonProperty("id")]
        public int Id { get; set; }

        [JsonProperty("type")]
        public int Type { get; set; }

        [JsonProperty("viewer")]
        public string Viewer { get; set; }

        [JsonProperty("duration")]
        public long DurationMilliseconds { get; set; }

        [JsonProperty("parameters")]
        public List<object> Parameters { get; set; } = new List<object>();
    }
}
