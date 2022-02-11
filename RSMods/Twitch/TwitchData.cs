using System.Collections.Generic;

namespace RSMods.Twitch
{
    public class TwitchAuthResponse
    {
        public string access_token { get; set; }
        public string refresh_token { get; set; }
        public List<string> scope { get; set; }
    }


    public class TwitchUserResponseData
    {
        public string id { get; set; }
        public string login { get; set; }
        public string display_name { get; set; }
        public string type { get; set; }
        public string broadcaster_type { get; set; }
        public string description { get; set; }
        public string profile_image_url { get; set; }
        public string offline_image_url { get; set; }
        public string view_count { get; set; }
    }

    public class TwitchUserResponse
    {
        public List<TwitchUserResponseData> data { get; set; }
    }
}
